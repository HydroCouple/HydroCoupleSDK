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

struct InProcHub::Impl
{
    struct Message
    {
        int32_t                sourceRank = -1;
        int32_t                tag        = 0;
        std::vector<std::byte> wire; //!< ValuePacker block.
    };

    struct Mailbox
    {
        std::mutex              mutex;
        std::condition_variable arrived;
        std::deque<Message>     messages;
    };

    int32_t                                participantCount;
    std::vector<std::unique_ptr<Mailbox>>  mailboxes;

    explicit Impl(int32_t count) : participantCount(count)
    {
        mailboxes.reserve(static_cast<size_t>(count));
        for (int32_t rank = 0; rank < count; ++rank)
            mailboxes.push_back(std::make_unique<Mailbox>());
    }

    [[nodiscard]] bool validRank(int32_t rank) const noexcept
    {
        return rank >= 0 && rank < participantCount;
    }

    /*!
     * Non-blocking match: removes and returns the first message in \p rank's
     * mailbox matching (sourceRank or any, tag).
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
     * Blocking match with the same criteria.
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
     * Blocking match bounded by \p timeoutSeconds; false when it elapses
     * before a matching message arrives.
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
 */
class CompletedRequest final
    : public HydroCouple::Distributed::IExchangeRequest
{
public:
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
    std::string m_failure;
};

/*!
 * Nonblocking receive request: matches lazily against the hub mailbox.
 */
class InProcReceiveRequest final
    : public HydroCouple::Distributed::IExchangeRequest
{
public:
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
    void finish(InProcHub::Impl::Message message)
    {
        std::string unpackMessage;
        if (!ValuePacker::unpack(message.wire, m_into, unpackMessage))
            m_failure = unpackMessage;
        m_complete = true;
    }

    std::shared_ptr<InProcHub> m_hub; //!< Keeps the hub alive.
    InProcHub::Impl           &m_impl;
    int32_t                    m_rank;
    int32_t                    m_sourceRank;
    int32_t                    m_tag;
    BufferDescriptor           m_into;
    bool                       m_complete = false;
    std::string                m_failure;
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
