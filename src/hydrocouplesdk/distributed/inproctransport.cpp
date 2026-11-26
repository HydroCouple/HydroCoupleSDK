// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/distributed/inproctransport.h"
#include "hydrocouplesdk/distributed/valuepacker.h"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

namespace HydroCouple::SDK::Distributed {

// ── Hub internals ─────────────────────────────────────────────────────────

/*!
 * \brief The hub's state: one mailbox per rank.
 *
 * Every mailbox has its own lock, so ranks that exchange with different
 * peers do not contend. Messages are matched oldest first.
 */
struct InProcHub::Impl
{
    /*!
     * \brief One queued message.
     */
    struct Message
    {
        int32_t                sourceRank = -1; //!< Rank of the transport that sent it.
        int32_t                tag        = 0;  //!< Tag it was sent under.
        std::vector<std::byte> wire; //!< ValuePacker block.
    };

    /*!
     * \brief The queue of messages addressed to one rank.
     */
    struct Mailbox
    {
        std::mutex              mutex;    //!< Guards \c messages.
        std::condition_variable arrived;  //!< Notified (all waiters) on every delivery.
        std::deque<Message>     messages; //!< Undelivered messages, oldest first.
    };

    int32_t                                participantCount; //!< Ranks the hub connects.
    std::vector<std::unique_ptr<Mailbox>>  mailboxes;        //!< One per rank, indexed by rank.

    /*!
     * \brief Creates \p count empty mailboxes.
     * \param[in] count Number of ranks (at least 1).
     */
    explicit Impl(int32_t count) : participantCount(count)
    {
        mailboxes.reserve(static_cast<size_t>(count));
        for (int32_t rank = 0; rank < count; ++rank)
            mailboxes.push_back(std::make_unique<Mailbox>());
    }

    /*!
     * \brief Checks a rank against the hub's size.
     * \param[in] rank Rank to check.
     * \return True when 0 ≤ \p rank < participantCount.
     */
    [[nodiscard]] bool validRank(int32_t rank) const noexcept
    {
        return rank >= 0 && rank < participantCount;
    }

    /*!
     * \brief Non-blocking match: removes and returns the first message in
     *        \p rank's mailbox matching (sourceRank or any, tag).
     * \param[in]  rank       Mailbox to search (must be valid).
     * \param[in]  sourceRank Sender to match; negative matches any sender.
     * \param[in]  tag        Tag to match.
     * \param[out] out        The removed message when the return is true.
     * \return True when a message matched and was removed.
     */
    bool tryMatch(int32_t rank, int32_t sourceRank, int32_t tag,
                  Message &out)
    {
        Mailbox &mailbox = *mailboxes[static_cast<size_t>(rank)];
        std::lock_guard<std::mutex> lock(mailbox.mutex);
        for (auto it = mailbox.messages.begin(); it != mailbox.messages.end();
             ++it)
        {
            if (it->tag == tag &&
                (sourceRank < 0 || it->sourceRank == sourceRank))
            {
                out = std::move(*it);
                mailbox.messages.erase(it);
                return true;
            }
        }
        return false;
    }

    /*!
     * \brief Blocking match with the same criteria as tryMatch().
     *
     * Waits on the mailbox's condition variable with no timeout.
     *
     * \param[in] rank       Mailbox to search (must be valid).
     * \param[in] sourceRank Sender to match; negative matches any sender.
     * \param[in] tag        Tag to match.
     * \return The removed message.
     */
    Message waitMatch(int32_t rank, int32_t sourceRank, int32_t tag)
    {
        Mailbox &mailbox = *mailboxes[static_cast<size_t>(rank)];
        std::unique_lock<std::mutex> lock(mailbox.mutex);
        for (;;)
        {
            for (auto it = mailbox.messages.begin();
                 it != mailbox.messages.end(); ++it)
            {
                if (it->tag == tag &&
                    (sourceRank < 0 || it->sourceRank == sourceRank))
                {
                    Message out = std::move(*it);
                    mailbox.messages.erase(it);
                    return out;
                }
            }
            mailbox.arrived.wait(lock);
        }
    }

    /*!
     * \brief Blocking match bounded by \p timeoutSeconds; false when it
     *        elapses before a matching message arrives.
     * \param[in]  rank           Mailbox to search (must be valid).
     * \param[in]  sourceRank     Sender to match; negative matches any sender.
     * \param[in]  tag            Tag to match.
     * \param[in]  timeoutSeconds Longest wait, in seconds.
     * \param[out] out            The removed message when the return is true.
     * \return True when a message matched within the timeout.
     */
    bool waitMatchFor(int32_t rank, int32_t sourceRank, int32_t tag,
                      double timeoutSeconds, Message &out)
    {
        Mailbox &mailbox = *mailboxes[static_cast<size_t>(rank)];
        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::duration<double>(timeoutSeconds);
        std::unique_lock<std::mutex> lock(mailbox.mutex);
        for (;;)
        {
            for (auto it = mailbox.messages.begin();
                 it != mailbox.messages.end(); ++it)
            {
                if (it->tag == tag &&
                    (sourceRank < 0 || it->sourceRank == sourceRank))
                {
                    out = std::move(*it);
                    mailbox.messages.erase(it);
                    return true;
                }
            }
            if (std::chrono::steady_clock::now() >= deadline)
                return false;
            mailbox.arrived.wait_until(lock, deadline);
        }
    }

    /*!
     * \brief Appends \p message to \p rank's mailbox and wakes its waiters.
     * \param[in] rank    Destination mailbox (must be valid).
     * \param[in] message The packed message (moved in).
     */
    void deliver(int32_t rank, Message message)
    {
        Mailbox &mailbox = *mailboxes[static_cast<size_t>(rank)];
        {
            std::lock_guard<std::mutex> lock(mailbox.mutex);
            mailbox.messages.push_back(std::move(message));
        }
        mailbox.arrived.notify_all();
    }
};

std::shared_ptr<InProcHub> InProcHub::create(int32_t participantCount)
{
    return std::shared_ptr<InProcHub>(
        new InProcHub(std::max<int32_t>(1, participantCount)));
}

InProcHub::InProcHub(int32_t participantCount)
    : m_impl(std::make_unique<Impl>(participantCount))
{
}

InProcHub::~InProcHub() = default;

int32_t InProcHub::participantCount() const noexcept
{
    return m_impl->participantCount;
}

std::unique_ptr<InProcTransport> InProcHub::createTransport(int32_t rank)
{
    if (!m_impl->validRank(rank))
        return nullptr;
    return std::unique_ptr<InProcTransport>(
        new InProcTransport(shared_from_this(), rank));
}

// ── Requests ──────────────────────────────────────────────────────────────

namespace {

/*!
 * Already-completed request (used for sendAsync, which delivers eagerly).
 * test(), wait() and waitFor() return at once; failed() reports the
 * message it was made with (empty: success).
 */
class CompletedRequest final
    : public HydroCouple::Distributed::IExchangeRequest
{
public:
    //! \p failureMessage is the send failure, or empty for success.
    explicit CompletedRequest(std::string failureMessage = {})
        : m_failure(std::move(failureMessage))
    {
    }
    bool test() override { return true; }
    void wait() override {}
    [[nodiscard]] bool waitFor(double) override { return true; }
    // Cancellation is not supported by this request: report whether the
    // operation has already completed (true) or is still pending (false).
    [[nodiscard]] bool cancel() override { return test(); }
    bool failed(std::string &message) const override
    {
        message = m_failure;
        return !m_failure.empty();
    }

private:
    std::string m_failure; //!< Empty on success.
};

/*!
 * Nonblocking receive request: matches lazily against the hub mailbox.
 * Nothing is removed from the mailbox until test(), wait() or waitFor()
 * finds a match; the destructor does not wait.
 */
class InProcReceiveRequest final
    : public HydroCouple::Distributed::IExchangeRequest
{
public:
    //! Records what to match and where to unpack; matches nothing yet.
    InProcReceiveRequest(std::shared_ptr<InProcHub> hub,
                         InProcHub::Impl &impl, int32_t rank,
                         int32_t sourceRank, int32_t tag,
                         BufferDescriptor into)
        : m_hub(std::move(hub)),
          m_impl(impl),
          m_rank(rank),
          m_sourceRank(sourceRank),
          m_tag(tag),
          m_into(into)
    {
    }

    bool test() override
    {
        if (m_complete)
            return true;
        InProcHub::Impl::Message message;
        if (!m_impl.tryMatch(m_rank, m_sourceRank, m_tag, message))
            return false;
        finish(std::move(message));
        return true;
    }

    void wait() override
    {
        if (m_complete)
            return;
        finish(m_impl.waitMatch(m_rank, m_sourceRank, m_tag));
    }

    [[nodiscard]] bool waitFor(double timeoutSeconds) override
    {
        if (m_complete || timeoutSeconds <= 0.0)
            return test();
        InProcHub::Impl::Message message;
        if (!m_impl.waitMatchFor(m_rank, m_sourceRank, m_tag, timeoutSeconds,
                                 message))
            return false;
        finish(std::move(message));
        return true;
    }

    // Cancellation is not supported by this request: report whether the
    // operation has already completed (true) or is still pending (false).
    [[nodiscard]] bool cancel() override { return test(); }
    bool failed(std::string &message) const override
    {
        message = m_failure;
        return !m_failure.empty();
    }

private:
    //! Unpacks the matched message into the destination, records an unpack
    //! failure, and marks the request complete.
    void finish(InProcHub::Impl::Message message)
    {
        std::string unpackMessage;
        if (!ValuePacker::unpack(message.wire, m_into, unpackMessage))
            m_failure = unpackMessage;
        m_complete = true;
    }

    std::shared_ptr<InProcHub> m_hub; //!< Keeps the hub alive.
    InProcHub::Impl           &m_impl;       //!< The hub's mailboxes.
    int32_t                    m_rank;       //!< The receiving rank's mailbox.
    int32_t                    m_sourceRank; //!< Sender to match (negative: any).
    int32_t                    m_tag;        //!< Tag to match.
    BufferDescriptor           m_into;       //!< Caller's destination buffer.
    bool                       m_complete = false; //!< A message was matched and unpacked (or failed to).
    std::string                m_failure;    //!< Unpack failure; empty on success.
};

} // namespace

// ── Transport ─────────────────────────────────────────────────────────────

InProcTransport::InProcTransport(std::shared_ptr<InProcHub> hub, int32_t rank)
    : Identity("inproc-transport-" + std::to_string(rank)),
      m_hub(std::move(hub)),
      m_rank(rank)
{
}

InProcTransport::~InProcTransport() = default;

InProcTransport::Endpoint InProcTransport::localEndpoint() const
{
    Endpoint endpoint;
    endpoint.address = "inproc";
    endpoint.rank    = m_rank;
    return endpoint;
}

int32_t InProcTransport::participantCount() const
{
    return m_hub->participantCount();
}

bool InProcTransport::send(const Endpoint &to, int32_t tag,
                           const BufferDescriptor &payload,
                           std::string *message)
{
    InProcHub::Impl &impl = *m_hub->m_impl;
    if (!impl.validRank(to.rank))
    {
        if (message)
            *message = "InProcTransport::send: invalid target rank " +
                       std::to_string(to.rank);
        return false;
    }
    InProcHub::Impl::Message wireMessage;
    wireMessage.sourceRank = m_rank;
    wireMessage.tag        = tag;
    std::string packMessage;
    if (!ValuePacker::pack(payload, wireMessage.wire, packMessage))
    {
        if (message)
            *message = packMessage;
        return false;
    }
    impl.deliver(to.rank, std::move(wireMessage));
    if (message)
        message->clear();
    return true;
}

bool InProcTransport::receive(const Endpoint &from, int32_t tag,
                              const BufferDescriptor &into,
                              std::string *message)
{
    InProcHub::Impl &impl = *m_hub->m_impl;
    if (from.rank >= 0 && !impl.validRank(from.rank))
    {
        if (message)
            *message = "InProcTransport::receive: invalid source rank " +
                       std::to_string(from.rank);
        return false;
    }
    InProcHub::Impl::Message wireMessage =
        impl.waitMatch(m_rank, from.rank, tag);
    std::string unpackMessage;
    if (!ValuePacker::unpack(wireMessage.wire, into, unpackMessage))
    {
        if (message)
            *message = unpackMessage;
        return false;
    }
    if (message)
        message->clear();
    return true;
}

std::unique_ptr<InProcTransport::IExchangeRequest>
InProcTransport::sendAsync(const Endpoint &to, int32_t tag,
                           const BufferDescriptor &payload)
{
    std::string message;
    if (!send(to, tag, payload, &message))
        return std::make_unique<CompletedRequest>(message);
    return std::make_unique<CompletedRequest>();
}

std::unique_ptr<InProcTransport::IExchangeRequest>
InProcTransport::receiveAsync(const Endpoint &from, int32_t tag,
                              const BufferDescriptor &into)
{
    return std::make_unique<InProcReceiveRequest>(
        m_hub, *m_hub->m_impl, m_rank, from.rank, tag, into);
}

} // namespace HydroCouple::SDK::Distributed
