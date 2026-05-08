// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   signal.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Concrete implementations of HydroCouple::ISignal<Args...> and
 *         HydroCouple::ISlot<Args...> using std::shared_mutex and std::weak_ptr.
 *
 * Design notes
 * ============
 * - Slots are stored as std::weak_ptr so the signal never prevents slot owners
 *   from being destroyed. Expired weak_ptrs are purged lazily on each emit().
 * - emit() takes a snapshot copy of the slot list under a shared lock, then
 *   fires outside the lock to allow re-entrant connect()/disconnect() calls.
 * - blockSignals(true) prevents emit() from firing; existing connections are
 *   preserved and resume when blockSignals(false) is called.
 * - FunctionSlot<Args...> wraps any callable as an ISlot for convenience.
 */

#ifndef HYDROCOUPLESDK_SIGNAL_H
#define HYDROCOUPLESDK_SIGNAL_H

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "hydrocouple.h"  // ISignal, ISlot

namespace HydroCouple {

// ── Signal<Args...> ───────────────────────────────────────────────────────

/*!
 * \brief Thread-safe concrete implementation of ISignal<Args...>.
 *
 * Stores connected slots as std::weak_ptr; calls them all on emit().
 * Safe to use from multiple threads: reads use a shared lock, mutations
 * (connect/disconnect/blockSignals) use an exclusive lock.
 *
 * \tparam Args  Parameter types forwarded to each connected slot.
 */
template<typename... Args>
class Signal : public virtual ISignal<Args...>
{
public:
    Signal() = default;
    ~Signal() override = default;

    // Non-copyable, non-movable — signals own their connections.
    Signal(const Signal &) = delete;
    Signal &operator=(const Signal &) = delete;

    // ── ISignal interface ─────────────────────────────────────────────────

    void connect(const std::shared_ptr<ISlot<Args...>> &slot) override
    {
        if (!slot) return;
        std::unique_lock lock(m_mutex);
        // Avoid duplicate connections from the same shared_ptr
        for (const auto &w : m_slots)
            if (auto s = w.lock(); s == slot) return;
        m_slots.emplace_back(slot);
    }

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

    void blockSignals(bool block) override
    {
        std::unique_lock lock(m_mutex);
        m_blocked = block;
    }

    // ── Public emit (not in interface — call through derived classes) ─────

    /*!
     * \brief Fire the signal, calling all live connected slots.
     *
     * Takes a snapshot under a shared lock, then fires outside the lock
     * so slots may safely call connect()/disconnect() re-entrantly.
     * Expired weak_ptrs are pruned after the snapshot is taken.
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

    [[nodiscard]] bool isBlocked() const noexcept
    {
        std::shared_lock lock(m_mutex);
        return m_blocked;
    }

    [[nodiscard]] std::size_t connectionCount() const noexcept
    {
        std::shared_lock lock(m_mutex);
        return m_slots.size();
    }

private:
    mutable std::shared_mutex                         m_mutex;
    std::vector<std::weak_ptr<ISlot<Args...>>>        m_slots;
    bool                                              m_blocked = false;
};


// ── FunctionSlot<Args...> ─────────────────────────────────────────────────

/*!
 * \brief Wraps a std::function as an ISlot<Args...>.
 *
 * Allows connecting any lambda, free function, or bound member function
 * to a Signal without writing a full ISlot subclass:
 *
 * \code
 *     auto slot = std::make_shared<HydroCouple::FunctionSlot<std::string>>(
 *         [](const HydroCouple::ISignal<std::string> &, std::string name) {
 *             std::cout << "property changed: " << name << '\n';
 *         });
 *     myObject.propertyChanged().connect(slot);
 * \endcode
 */
template<typename... Args>
class FunctionSlot : public ISlot<Args...>
{
public:
    using Fn = std::function<void(const ISignal<Args...> &, Args...)>;

    explicit FunctionSlot(Fn fn) : m_fn(std::move(fn)) {}

    void operator()(const ISignal<Args...> &sender, Args... args) override
    {
        if (m_fn) m_fn(sender, std::forward<Args>(args)...);
    }

private:
    Fn m_fn;
};

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_SIGNAL_H
