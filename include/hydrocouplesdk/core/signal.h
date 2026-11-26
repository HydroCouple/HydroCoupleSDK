// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   signal.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete implementations of HydroCouple::ISignal and
 *         HydroCouple::ISlot using std::shared_mutex and std::weak_ptr.
 *
 * Design notes
 * ============
 * - Slots are stored as std::weak_ptr so the signal never prevents slot owners
 *   from being destroyed. Expired weak_ptrs are purged lazily on each emit()
 *   and disconnect().
 * - emit() takes a snapshot copy of the live slots under the lock, then
 *   fires outside the lock to allow re-entrant connect()/disconnect() calls.
 * - blockSignals(true) prevents emit() from firing; existing connections are
 *   preserved and resume when blockSignals(false) is called. Emissions made
 *   while blocked are dropped, not queued.
 * - FunctionSlot wraps any callable as an ISlot for convenience.
 */

#ifndef HYDROCOUPLESDK_HYDROCOUPLE_SIGNAL_H
#define HYDROCOUPLESDK_HYDROCOUPLE_SIGNAL_H

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "hydrocouple.h"  // ISignal, ISlot

namespace HydroCouple::SDK {

// ── Signal<Args...> ───────────────────────────────────────────────────────

/*!
 * \brief Thread-safe concrete implementation of the HydroCouple::ISignal template.
 *
 * Stores connected slots as std::weak_ptr; calls them all on emit().
 * Safe to use from multiple threads: isBlocked() and connectionCount()
 * take a shared lock; connect(), disconnect(), blockSignals() and the
 * snapshot step of emit() take an exclusive lock.
 *
 * Slots run synchronously on the thread that calls emit(), in connection
 * order. The \c sender argument each slot receives is this Signal object.
 * A slot that throws propagates the exception out of emit() and the
 * remaining slots of that emission are not called.
 *
 * Description, AbstractModelComponent and AbstractWorkflowComponent each
 * own Signal members and forward the ISignal methods to them.
 *
 * \tparam Args  Parameter types forwarded to each connected slot.
 */
template<typename... Args>
class Signal : public virtual ISignal<Args...>
{
public:
    //! Creates a signal with no connections, not blocked.
    Signal() = default;
    //! Destroys the signal; connected slots are released, not notified.
    ~Signal() override = default;

    //! Not copyable: a signal owns its connection list.
    Signal(const Signal &) = delete;
    //! Not copy-assignable: a signal owns its connection list.
    Signal &operator=(const Signal &) = delete;

    // ── ISignal interface ─────────────────────────────────────────────────

    /*!
     * \brief Connects a slot.
     * \details The signal stores a weak reference: the caller owns the slot
     *          and keeps it alive for as long as it should be called. A slot
     *          already connected through the same \c shared_ptr target is not
     *          added twice.
     * \param[in] slot  Slot to connect; a null pointer is ignored.
     */
    void connect(const std::shared_ptr<ISlot<Args...>> &slot) override
    {
        if (!slot) return;
        std::unique_lock lock(m_mutex);
        // Avoid duplicate connections from the same shared_ptr
        for (const auto &w : m_slots)
            if (auto s = w.lock(); s == slot) return;
        m_slots.emplace_back(slot);
    }

    /*!
     * \brief Disconnects a slot.
     * \details Removes \p slot and, in the same pass, every slot whose owner
     *          has already released it. Disconnecting a slot that is not
     *          connected only removes the expired entries.
     * \param[in] slot  Slot to disconnect; a null pointer is ignored.
     */
    void disconnect(const std::shared_ptr<ISlot<Args...>> &slot) override
    {
        if (!slot) return;
        std::unique_lock lock(m_mutex);
        m_slots.erase(
            std::remove_if(m_slots.begin(), m_slots.end(),
                [&slot](const std::weak_ptr<ISlot<Args...>> &w) {
                    auto s = w.lock();
                    return !s || s == slot;
                }),
            m_slots.end());
    }

    /*!
     * \brief Blocks or unblocks emission.
     * \details While blocked, emit() returns at once: no slot is called and
     *          nothing is queued. Connections are kept.
     * \param[in] block  \c true to block; \c false to unblock.
     */
    void blockSignals(bool block) override
    {
        std::unique_lock lock(m_mutex);
        m_blocked = block;
    }

    // ── Public emit (not in interface — call through derived classes) ─────

    /*!
     * \brief Fire the signal, calling all live connected slots.
     *
     * Takes a snapshot of the live slots under the lock and, in the same
     * step, drops expired weak references; then calls each slot outside the
     * lock so slots may safely call connect()/disconnect() re-entrantly. A
     * slot connected during the emission is first called by the next
     * emission; a slot disconnected during it may still be called once by
     * this one. Does nothing while the signal is blocked.
     *
     * \param[in] args  Values passed to every slot, after the sender.
     */
    void emit(Args... args) override
    {
        std::vector<std::shared_ptr<ISlot<Args...>>> snapshot;
        {
            std::unique_lock lock(m_mutex);
            if (m_blocked) return;

            // Snapshot live slots; drop expired ones in-place.
            std::vector<std::weak_ptr<ISlot<Args...>>> live;
            live.reserve(m_slots.size());
            for (auto &w : m_slots) {
                if (auto s = w.lock()) {
                    snapshot.push_back(s);
                    live.push_back(w);
                }
            }
            m_slots = std::move(live);
        }
        for (auto &slot : snapshot)
            (*slot)(*this, args...);
    }

    /*!
     * \brief Whether the signal is blocked.
     * \return \c true after blockSignals(true) until blockSignals(false).
     */
    [[nodiscard]] bool isBlocked() const noexcept
    {
        std::shared_lock lock(m_mutex);
        return m_blocked;
    }

    /*!
     * \brief Number of stored connections.
     * \return The number of weak references held, which may include slots
     *         whose owners have released them since the last emit() or
     *         disconnect() pruned the list.
     */
    [[nodiscard]] std::size_t connectionCount() const noexcept
    {
        std::shared_lock lock(m_mutex);
        return m_slots.size();
    }

private:
    //! Guards m_slots and m_blocked.
    mutable std::shared_mutex                         m_mutex;
    //! Connected slots, held weakly, in connection order.
    std::vector<std::weak_ptr<ISlot<Args...>>>        m_slots;
    //! True while emission is blocked.
    bool                                              m_blocked = false;
};


// ── FunctionSlot<Args...> ─────────────────────────────────────────────────

/*!
 * \brief Wraps a std::function as a HydroCouple::ISlot.
 *
 * Allows connecting any lambda, free function, or bound member function
 * to a Signal without writing a full ISlot subclass:
 *
 * \code
 *     auto slot = std::make_shared<HydroCouple::SDK::FunctionSlot<std::string>>(
 *         [](const HydroCouple::ISignal<std::string> &, std::string name) {
 *             std::cout << "property changed: " << name << '\n';
 *         });
 *     myObject.connect(slot);
 * \endcode
 *
 * Keep the \c shared_ptr alive: signals hold slots weakly, so a temporary
 * slot is released and never called.
 *
 * \tparam Args  Parameter types of the signal the slot connects to.
 */
template<typename... Args>
class FunctionSlot : public ISlot<Args...>
{
public:
    //! The callable type: receives the emitting signal and the emitted values.
    using Fn = std::function<void(const ISignal<Args...> &, Args...)>;

    /*!
     * \brief Wraps a callable.
     * \param[in] fn  Callable invoked on each emission; moved in. An empty
     *                function makes the slot a no-op.
     */
    explicit FunctionSlot(Fn fn) : m_fn(std::move(fn)) {}

    /*!
     * \brief Calls the wrapped callable, if any.
     * \param[in] sender  The signal that emitted.
     * \param[in] args    The emitted values, forwarded to the callable.
     */
    void operator()(const ISignal<Args...> &sender, Args... args) override
    {
        if (m_fn) m_fn(sender, std::forward<Args>(args)...);
    }

private:
    //! The wrapped callable; may be empty.
    Fn m_fn;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_HYDROCOUPLE_SIGNAL_H
