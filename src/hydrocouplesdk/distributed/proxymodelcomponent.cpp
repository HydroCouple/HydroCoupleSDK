// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/distributed/proxymodelcomponent.h"
#include "hydrocouplehelpers.h"

#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

namespace HydroCouple::SDK::Distributed {

namespace {

/*! Poll period while waiting for a reply. */
constexpr auto kPollInterval = std::chrono::microseconds(200);

BufferDescriptor byteBuffer(void *data, const int64_t &shapeStorage)
{
    BufferDescriptor descriptor;
    descriptor.data  = data;
    descriptor.kind  = DataKind::UInt8;
    descriptor.rank  = 1;
    descriptor.shape = &shapeStorage;
    descriptor.space = MemorySpace::Host;
    return descriptor;
}

} // namespace

ProxyModelComponent::ProxyModelComponent(
    std::string_view id, HydroCouple::Distributed::ITransport *transport,
    int32_t remoteRank, std::string remoteId)
    : AbstractModelComponent(id),
      m_transport(transport),
      m_remoteRank(remoteRank),
      m_remoteId(std::move(remoteId))
{
}

// ── IDistributedModelComponent ────────────────────────────────────────────

HydroCouple::Distributed::ITransport *ProxyModelComponent::transport() const
{
    return m_transport;
}

int32_t ProxyModelComponent::partitionCount() const
{
    return m_transport->participantCount();
}

int32_t ProxyModelComponent::partitionRank() const
{
    return m_transport->localEndpoint().rank;
}

// ── IProxyModelComponent ──────────────────────────────────────────────────

ProxyModelComponent::Endpoint ProxyModelComponent::remoteEndpoint() const
{
    Endpoint endpoint;
    endpoint.address = m_transport->localEndpoint().address;
    endpoint.rank    = m_remoteRank;
    return endpoint;
}

std::string ProxyModelComponent::remoteId() const
{
    return m_remoteId;
}

bool ProxyModelComponent::connectToPeer(std::string &message)
{
    if (m_connected)
    {
        message.clear();
        return true;
    }
    if (!roundTrip(OpCode::Hello, nullptr, nullptr, message))
        return false;
    m_connected = true;
    return true;
}

void ProxyModelComponent::disconnectFromPeer()
{
    m_connected = false;
}

bool ProxyModelComponent::isConnected() const
{
    return m_connected;
}

bool ProxyModelComponent::ping(double timeoutSeconds)
{
    std::string message;
    return roundTrip(OpCode::Heartbeat, nullptr, nullptr, message,
                     timeoutSeconds);
}

double ProxyModelComponent::requestTimeout() const
{
    return m_requestTimeout;
}

void ProxyModelComponent::setRequestTimeout(double seconds)
{
    m_requestTimeout = seconds > 0.0 ? seconds : m_requestTimeout;
}

// ── Lifecycle (remoted) ───────────────────────────────────────────────────

void ProxyModelComponent::initialize()
{
    using Status = HydroCouple::IModelComponent::ComponentStatus;
    setStatus(Status::Initializing);
    std::string message;
    if (roundTrip(OpCode::Initialize, nullptr, nullptr, message))
        setStatus(Status::Initialized);
    else
        peerFailure("remote initialize failed: " + message);
}

std::vector<std::string> ProxyModelComponent::validate()
{
    using Status = HydroCouple::IModelComponent::ComponentStatus;
    setStatus(Status::Validating);
    std::string message;
    bool rejected = false;
    if (roundTrip(OpCode::Validate, nullptr, nullptr, message, -1.0, &rejected))
    {
        setStatus(Status::Valid);
        return {};
    }
    if (rejected)
    {
        // The peer is alive and found problems: the composition is Invalid,
        // not the proxy Failed. The messages are queued per the error-channel
        // convention so a reader of errors() alone misses nothing.
        reportError(ErrorEntry::Severity::Error, 0, message);
        setStatus(Status::Invalid, message);
        return {message};
    }
    peerFailure("remote validate failed: " + message);
    return {message};
}

void ProxyModelComponent::prepare()
{
    using Status = HydroCouple::IModelComponent::ComponentStatus;
    setStatus(Status::Preparing);
    std::string message;
    if (roundTrip(OpCode::Prepare, nullptr, nullptr, message))
        setStatus(Status::Updated, "remote prepared");
    else
        peerFailure("remote prepare failed: " + message);
}

void ProxyModelComponent::update(
    const std::vector<HydroCouple::IOutput *> &)
{
    using Status = HydroCouple::IModelComponent::ComponentStatus;
    setStatus(Status::Updating);
    std::string message;
    if (!roundTrip(OpCode::Update, nullptr, nullptr, message))
    {
        peerFailure("remote update failed: " + message);
        return;
    }

    // Mirror what the peer reached: Done after its final step, or
    // WaitingForData. Without this a proxy reports Updated forever and an
    // orchestrator waiting for Done never stops.
    const Status remote = remoteStatusOr(Status::Updated);
    switch (remote)
    {
        case Status::Done:
            setStatus(Status::Done, "remote done");
            break;
        case Status::WaitingForData:
            setStatus(Status::WaitingForData, "remote waiting for data");
            break;
        default:
            setStatus(Status::Updated, "remote updated");
            break;
    }
}

void ProxyModelComponent::finish()
{
    using Status = HydroCouple::IModelComponent::ComponentStatus;
    setStatus(Status::Finishing);
    std::string message;
    if (!roundTrip(OpCode::Finish, nullptr, nullptr, message))
    {
        peerFailure("remote finish failed: " + message);
        return;
    }
    // A peer that can be re-initialized returns to Created; mirror it.
    setStatus(remoteStatusOr(Status::Finished) == Status::Created
                  ? Status::Created
                  : Status::Finished);
}

HydroCouple::IModelComponent::ComponentStatus
ProxyModelComponent::remoteStatusOr(
    HydroCouple::IModelComponent::ComponentStatus fallback) const
{
    const int remote = m_remoteStatus.load();
    return remote < 0 ? fallback
                      : static_cast<HydroCouple::IModelComponent::ComponentStatus>(remote);
}

// ── Remote data access ────────────────────────────────────────────────────

bool ProxyModelComponent::getRemoteValues(const std::string &itemId,
                                          const BufferDescriptor &destination,
                                          std::string &message)
{
    std::vector<std::byte> idBytes(itemId.size());
    std::memcpy(idBytes.data(), itemId.data(), itemId.size());
    const int64_t idShape = static_cast<int64_t>(itemId.size());
    const BufferDescriptor idDescriptor =
        byteBuffer(idBytes.data(), idShape);
    return roundTrip(OpCode::GetValues, &idDescriptor, &destination, message);
}

bool ProxyModelComponent::setRemoteValues(const std::string &itemId,
                                          const BufferDescriptor &source,
                                          std::string &message)
{
    std::vector<std::byte> idBytes(itemId.size());
    std::memcpy(idBytes.data(), itemId.data(), itemId.size());
    const int64_t idShape = static_cast<int64_t>(itemId.size());
    const BufferDescriptor idDescriptor =
        byteBuffer(idBytes.data(), idShape);

    // Request: envelope + id payload + value block (three-phase, one tag).
    const uint32_t tag = m_nextTag.fetch_add(1);
    Envelope request = makeEnvelope(
        OpCode::SetValues, m_transport->localEndpoint().rank, m_remoteRank,
        tag, static_cast<uint64_t>(itemId.size()));
    const int64_t envelopeShape = static_cast<int64_t>(sizeof(Envelope));
    if (!m_transport->send({"", m_remoteRank}, 0,
                           byteBuffer(&request, envelopeShape), &message) ||
        !m_transport->send({"", m_remoteRank}, static_cast<int32_t>(tag),
                           idDescriptor, &message) ||
        !m_transport->send({"", m_remoteRank}, static_cast<int32_t>(tag),
                           source, &message))
        return false;

    // Await Ack/Error on the request tag.
    Envelope reply;
    auto pending = m_transport->receiveAsync(
        {"", m_remoteRank}, static_cast<int32_t>(tag),
        byteBuffer(&reply, envelopeShape));
    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>(m_requestTimeout));
    while (!pending->test())
    {
        if (std::chrono::steady_clock::now() > deadline)
        {
            message = "request timed out after " +
                      std::to_string(m_requestTimeout) + " s (op SetValues)";
            return false;
        }
        std::this_thread::sleep_for(kPollInterval);
    }
    if (reply.op() == OpCode::Ack)
        return true;
    // Error text follows.
    std::string text(reply.payloadBytes, '\0');
    if (reply.payloadBytes > 0)
    {
        const int64_t textShape = static_cast<int64_t>(reply.payloadBytes);
        (void)m_transport->receive({"", m_remoteRank},
                                   static_cast<int32_t>(tag),
                                   byteBuffer(text.data(), textShape),
                                   &message);
    }
    message = text.empty() ? "remote error" : text;
    return false;
}

// ── Round trip core ───────────────────────────────────────────────────────

bool ProxyModelComponent::roundTrip(OpCode op,
                                    const BufferDescriptor *requestPayload,
                                    const BufferDescriptor *replyInto,
                                    std::string &message,
                                    double timeoutSeconds,
                                    bool *rejected)
{
    if (rejected)
        *rejected = false;
    const double timeout =
        timeoutSeconds > 0.0 ? timeoutSeconds : m_requestTimeout;
    const uint32_t tag = m_nextTag.fetch_add(1);

    uint64_t payloadBytes = 0;
    if (requestPayload)
        payloadBytes = static_cast<uint64_t>(
            HydroCouple::Helpers::elementCount(*requestPayload) *
            HydroCouple::Helpers::dataKindSize(requestPayload->kind));

    Envelope request = makeEnvelope(op, m_transport->localEndpoint().rank,
                                    m_remoteRank, tag, payloadBytes);
    const int64_t envelopeShape = static_cast<int64_t>(sizeof(Envelope));
    if (!m_transport->send({"", m_remoteRank}, /*control tag*/ 0,
                           byteBuffer(&request, envelopeShape), &message))
        return false;
    if (requestPayload &&
        !m_transport->send({"", m_remoteRank}, static_cast<int32_t>(tag),
                           *requestPayload, &message))
        return false;

    // Await the reply envelope on the request tag, bounded by the timeout.
    Envelope reply;
    auto pending = m_transport->receiveAsync(
        {"", m_remoteRank}, static_cast<int32_t>(tag),
        byteBuffer(&reply, envelopeShape));
    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>(timeout));
    while (!pending->test())
    {
        if (std::chrono::steady_clock::now() > deadline)
        {
            message = "request timed out after " + std::to_string(timeout) +
                      " s (op " +
                      std::to_string(static_cast<int>(op)) + ")";
            return false;
        }
        std::this_thread::sleep_for(kPollInterval);
    }
    std::string requestFailure;
    if (pending->failed(requestFailure))
    {
        message = requestFailure;
        return false;
    }
    if (!reply.valid())
    {
        message = "malformed reply envelope";
        return false;
    }

    if (reply.flags & kFlagStatusPresent)
        m_remoteStatus = static_cast<int>(reply.flags & 0xFF);

    if (reply.op() == OpCode::Ack)
    {
        if (reply.payloadBytes > 0 && replyInto &&
            !m_transport->receive({"", m_remoteRank},
                                  static_cast<int32_t>(tag), *replyInto,
                                  &message))
            return false;
        message.clear();
        return true;
    }

    // Error: the payload carries the text, and the peer is alive.
    if (rejected)
        *rejected = true;
    std::string text(reply.payloadBytes, '\0');
    if (reply.payloadBytes > 0)
    {
        const int64_t textShape = static_cast<int64_t>(reply.payloadBytes);
        (void)m_transport->receive({"", m_remoteRank},
                                   static_cast<int32_t>(tag),
                                   byteBuffer(text.data(), textShape),
                                   &message);
    }
    message = text.empty() ? "remote error" : text;
    return false;
}

void ProxyModelComponent::peerFailure(const std::string &message)
{
    setStatus(HydroCouple::IModelComponent::ComponentStatus::Failed, message);
    m_connected = false;
}

} // namespace HydroCouple::SDK::Distributed
