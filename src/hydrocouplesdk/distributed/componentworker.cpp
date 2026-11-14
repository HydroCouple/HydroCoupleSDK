// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/distributed/componentworker.h"
#include "hydrocouplehelpers.h"

#include <cstring>
#include <vector>

namespace HydroCouple::SDK::Distributed {

namespace {

using Endpoint = HydroCouple::Distributed::ITransport::Endpoint;

/*! Descriptor over an Envelope's raw bytes. */
BufferDescriptor envelopeBuffer(Envelope &envelope,
                                const int64_t &shapeStorage)
{
    BufferDescriptor descriptor;
    descriptor.data  = &envelope;
    descriptor.kind  = DataKind::UInt8;
    descriptor.rank  = 1;
    descriptor.shape = &shapeStorage;
    descriptor.space = MemorySpace::Host;
    return descriptor;
}

/*! Descriptor over a byte vector. */
BufferDescriptor bytesBuffer(std::vector<std::byte> &bytes,
                             const int64_t &shapeStorage)
{
    BufferDescriptor descriptor;
    descriptor.data  = bytes.data();
    descriptor.kind  = DataKind::UInt8;
    descriptor.rank  = 1;
    descriptor.shape = &shapeStorage;
    descriptor.space = MemorySpace::Host;
    return descriptor;
}

/*! Finds a data item by id among a component's inputs and outputs. */
HydroCouple::IComponentDataItem *findItem(
    HydroCouple::IModelComponent &component, const std::string &itemId)
{
    for (HydroCouple::IOutput *output : component.outputs())
        if (output && output->id() == itemId)
            return output;
    for (HydroCouple::IInput *input : component.inputs())
        if (input && input->id() == itemId)
            return input;
    return nullptr;
}

} // namespace

ComponentWorker::ComponentWorker(
    HydroCouple::Distributed::ITransport *transport,
    HydroCouple::IModelComponent *component)
    : m_transport(transport),
      m_component(component)
{
}

int ComponentWorker::run()
{
    int served = 0;
    while (serveOne())
        ++served;
    return served + 1; // count the Shutdown itself
}

bool ComponentWorker::serveOne()
{
    using Status = HydroCouple::IModelComponent::ComponentStatus;

    // Phase 1: the fixed-size envelope, from any sender.
    Envelope envelope;
    const int64_t envelopeShape = static_cast<int64_t>(sizeof(Envelope));
    std::string message;
    if (!m_transport->receive({"", -1}, /*tag=*/0,
                              envelopeBuffer(envelope, envelopeShape),
                              &message) ||
        !envelope.valid())
    {
        return true; // malformed traffic; keep serving
    }

    const int32_t  requester = envelope.sourceRank;
    const uint32_t tag       = envelope.tag;

    // Phase 2: the payload, when the envelope announces one.
    std::vector<std::byte> payload(
        static_cast<size_t>(envelope.payloadBytes));
    if (envelope.payloadBytes > 0)
    {
        const int64_t payloadShape =
            static_cast<int64_t>(envelope.payloadBytes);
        if (!m_transport->receive({"", requester},
                                  static_cast<int32_t>(tag),
                                  bytesBuffer(payload, payloadShape),
                                  &message))
        {
            reply(requester, tag, OpCode::Error,
                  "worker failed to receive the request payload: " + message);
            return true;
        }
    }

    switch (envelope.op())
    {
    case OpCode::Hello:
    case OpCode::Heartbeat:
        reply(requester, tag, OpCode::Ack);
        return true;

    case OpCode::Initialize:
        m_component->initialize();
        if (m_component->status() == Status::Initialized)
            reply(requester, tag, OpCode::Ack);
        else
            reply(requester, tag, OpCode::Error,
                  "remote initialize() did not reach Initialized");
        return true;

    case OpCode::Validate:
    {
        const std::vector<std::string> problems = m_component->validate();
        if (problems.empty())
            reply(requester, tag, OpCode::Ack);
        else
        {
            std::string joined;
            for (const std::string &problem : problems)
                joined += (joined.empty() ? "" : "; ") + problem;
            reply(requester, tag, OpCode::Error, joined);
        }
        return true;
    }

    case OpCode::Prepare:
        m_component->prepare();
        reply(requester, tag,
              m_component->status() == Status::Failed ? OpCode::Error
                                                      : OpCode::Ack,
              m_component->status() == Status::Failed
                  ? "remote prepare() failed"
                  : "");
        return true;

    case OpCode::Update:
        m_component->update();
        reply(requester, tag,
              m_component->status() == Status::Failed ? OpCode::Error
                                                      : OpCode::Ack,
              m_component->status() == Status::Failed
                  ? "remote update() failed"
                  : "");
        return true;

    case OpCode::Finish:
        m_component->finish();
        reply(requester, tag, OpCode::Ack);
        return true;

    case OpCode::GetValues:
    {
        const std::string itemId(reinterpret_cast<const char *>(payload.data()),
                                 payload.size());
        HydroCouple::IComponentDataItem *item =
            findItem(*m_component, itemId);
        if (!item)
        {
            reply(requester, tag, OpCode::Error,
                  "remote component has no data item '" + itemId + "'");
            return true;
        }
        replyValues(requester, tag, *item);
        return true;
    }

    case OpCode::SetValues:
    {
        // Payload = item id; a third message on the same tag carries the
        // typed value block, received straight into a staging buffer of
        // the item's own kind and shape (the transport validates both).
        const std::string itemId(reinterpret_cast<const char *>(payload.data()),
                                 payload.size());
        HydroCouple::IComponentDataItem *item =
            findItem(*m_component, itemId);
        if (!item)
        {
            reply(requester, tag, OpCode::Error,
                  "remote component has no data item '" + itemId + "'");
            return true;
        }

        const std::vector<int64_t> shape = item->shape();
        const int64_t elementSize =
            HydroCouple::Helpers::dataKindSize(item->dataKind());
        int64_t count = 1;
        for (int64_t extent : shape)
            count *= extent;
        std::vector<std::byte> staging(
            static_cast<size_t>(count * elementSize));
        BufferDescriptor block;
        block.data  = staging.data();
        block.kind  = item->dataKind();
        block.rank  = static_cast<int32_t>(shape.size());
        block.shape = shape.data();
        block.space = MemorySpace::Host;
        if (!m_transport->receive({"", requester},
                                  static_cast<int32_t>(tag), block,
                                  &message))
        {
            reply(requester, tag, OpCode::Error,
                  "SetValues value block rejected: " + message);
            return true;
        }
        const std::vector<int64_t> start(shape.size(), 0);
        if (!item->setValuesFrom(block, start, shape, &message))
        {
            reply(requester, tag, OpCode::Error, message);
            return true;
        }
        reply(requester, tag, OpCode::Ack);
        return true;
    }

    case OpCode::Shutdown:
        reply(requester, tag, OpCode::Ack);
        return false;

    default:
        reply(requester, tag, OpCode::Error,
              "unsupported op code " + std::to_string(envelope.opCode));
        return true;
    }
}

// ── replies ───────────────────────────────────────────────────────────────

void ComponentWorker::reply(int32_t toRank, uint32_t tag, OpCode op,
                            const std::string &text)
{
    Envelope envelope = makeEnvelope(
        op, m_transport->localEndpoint().rank, toRank, tag,
        static_cast<uint64_t>(text.size()));
    if (m_component)
        envelope.flags = static_cast<uint16_t>(
            kFlagStatusPresent |
            static_cast<uint16_t>(static_cast<uint8_t>(m_component->status())));
    const int64_t envelopeShape = static_cast<int64_t>(sizeof(Envelope));
    std::string message;
    (void)m_transport->send({"", toRank}, static_cast<int32_t>(tag),
                            envelopeBuffer(envelope, envelopeShape),
                            &message);
    if (!text.empty())
    {
        std::vector<std::byte> bytes(text.size());
        std::memcpy(bytes.data(), text.data(), text.size());
        const int64_t textShape = static_cast<int64_t>(text.size());
        (void)m_transport->send({"", toRank}, static_cast<int32_t>(tag),
                                bytesBuffer(bytes, textShape), &message);
    }
}

void ComponentWorker::replyValues(int32_t toRank, uint32_t tag,
                                  const HydroCouple::IComponentDataItem &item)
{
    // Stage the item's full extent, then send it as one typed block.
    const std::vector<int64_t> shape = item.shape();
    const int64_t elementSize =
        HydroCouple::Helpers::dataKindSize(item.dataKind());
    if (elementSize <= 0)
    {
        reply(toRank, tag, OpCode::Error,
              "item '" + item.id() + "' has a non-fixed-width DataKind");
        return;
    }
    int64_t count = 1;
    for (int64_t extent : shape)
        count *= extent;

    std::vector<std::byte> staging(static_cast<size_t>(count * elementSize));
    BufferDescriptor block;
    block.data  = staging.data();
    block.kind  = item.dataKind();
    block.rank  = static_cast<int32_t>(shape.size());
    block.shape = shape.data();
    block.space = MemorySpace::Host;
    const std::vector<int64_t> start(shape.size(), 0);
    std::string message;
    if (!item.getValuesInto(block, start, shape, &message))
    {
        reply(toRank, tag, OpCode::Error, message);
        return;
    }

    Envelope envelope = makeEnvelope(
        OpCode::Ack, m_transport->localEndpoint().rank, toRank, tag,
        static_cast<uint64_t>(staging.size()));
    const int64_t envelopeShape = static_cast<int64_t>(sizeof(Envelope));
    (void)m_transport->send({"", toRank}, static_cast<int32_t>(tag),
                            envelopeBuffer(envelope, envelopeShape),
                            &message);
    (void)m_transport->send({"", toRank}, static_cast<int32_t>(tag), block,
                            &message);
}

} // namespace HydroCouple::SDK::Distributed
