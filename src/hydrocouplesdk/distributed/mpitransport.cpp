// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/distributed/mpitransport.h"
#include "hydrocouplesdk/distributed/valuepacker.h"

#include <mpi.h>

#include <vector>

namespace HydroCouple::SDK::Distributed {

namespace {

/*!
 * Formats an MPI error code through MPI_Error_string.
 */
std::string mpiErrorText(int code)
{
    char text[MPI_MAX_ERROR_STRING];
    int  length = 0;
    if (MPI_Error_string(code, text, &length) == MPI_SUCCESS)
        return std::string(text, static_cast<size_t>(length));
    return "MPI error " + std::to_string(code);
}

/*!
 * Already-completed request carrying an optional failure.
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
    bool failed(std::string &message) const override
    {
        message = m_failure;
        return !m_failure.empty();
    }

private:
    std::string m_failure;
};

/*!
 * Nonblocking send: owns the packed wire block until MPI_Isend completes.
 */
class MpiSendRequest final
    : public HydroCouple::Distributed::IExchangeRequest
{
public:
    MpiSendRequest(std::vector<std::byte> wire, MPI_Comm comm, int target,
                   int tag)
        : m_wire(std::move(wire))
    {
        const int code = MPI_Isend(m_wire.data(),
                                   static_cast<int>(m_wire.size()), MPI_BYTE,
                                   target, tag, comm, &m_request);
        if (code != MPI_SUCCESS)
        {
            m_failure  = "MpiTransport::sendAsync: " + mpiErrorText(code);
            m_complete = true;
        }
    }

    ~MpiSendRequest() override
    {
        if (!m_complete)
            wait(); // The wire block must outlive the operation.
    }

    bool test() override
    {
        if (m_complete)
            return true;
        int       flag = 0;
        const int code = MPI_Test(&m_request, &flag, MPI_STATUS_IGNORE);
        if (code != MPI_SUCCESS)
        {
            m_failure  = "MpiTransport::sendAsync: " + mpiErrorText(code);
            m_complete = true;
            return true;
        }
        m_complete = flag != 0;
        return m_complete;
    }

    void wait() override
    {
        if (m_complete)
            return;
        const int code = MPI_Wait(&m_request, MPI_STATUS_IGNORE);
        if (code != MPI_SUCCESS)
            m_failure = "MpiTransport::sendAsync: " + mpiErrorText(code);
        m_complete = true;
    }

    bool failed(std::string &message) const override
    {
        message = m_failure;
        return !m_failure.empty();
    }

private:
    std::vector<std::byte> m_wire;
    MPI_Request            m_request = MPI_REQUEST_NULL;
    bool                   m_complete = false;
    std::string            m_failure;
};

/*!
 * Nonblocking receive: matched probe discovers the message and its size,
 * MPI_Imrecv drains it into staging, completion unpacks into the caller's
 * descriptor with kind/shape validation.
 */
class MpiReceiveRequest final
    : public HydroCouple::Distributed::IExchangeRequest
{
public:
    MpiReceiveRequest(MPI_Comm comm, int source, int tag,
                      BufferDescriptor into)
        : m_comm(comm), m_source(source), m_tag(tag), m_into(into)
    {
    }

    ~MpiReceiveRequest() override
    {
        if (m_started && !m_complete)
            (void)MPI_Wait(&m_request, MPI_STATUS_IGNORE);
    }

    bool test() override
    {
        if (m_complete)
            return true;
        if (!m_started && !tryStart(/*blocking=*/false))
            return m_complete; // true only when tryStart failed hard
        if (!m_started)
            return false;
        int       flag = 0;
        const int code = MPI_Test(&m_request, &flag, MPI_STATUS_IGNORE);
        if (code != MPI_SUCCESS)
        {
            fail("MpiTransport::receiveAsync: " + mpiErrorText(code));
            return true;
        }
        if (flag)
            finish();
        return m_complete;
    }

    void wait() override
    {
        if (m_complete)
            return;
        if (!m_started && !tryStart(/*blocking=*/true))
            return; // failed hard
        const int code = MPI_Wait(&m_request, MPI_STATUS_IGNORE);
        if (code != MPI_SUCCESS)
        {
            fail("MpiTransport::receiveAsync: " + mpiErrorText(code));
            return;
        }
        finish();
    }

    bool failed(std::string &message) const override
    {
        message = m_failure;
        return !m_failure.empty();
    }

private:
    /*!
     * Probes for the message (matched, so no other receive can steal it)
     * and posts the MPI_Imrecv. Returns false without failure when a
     * nonblocking probe found nothing yet.
     */
    bool tryStart(bool blocking)
    {
        MPI_Message matched;
        MPI_Status  status;
        if (blocking)
        {
            const int code =
                MPI_Mprobe(m_source, m_tag, m_comm, &matched, &status);
            if (code != MPI_SUCCESS)
            {
                fail("MpiTransport::receiveAsync: " + mpiErrorText(code));
                return false;
            }
        }
        else
        {
            int       flag = 0;
            const int code = MPI_Improbe(m_source, m_tag, m_comm, &flag,
                                         &matched, &status);
            if (code != MPI_SUCCESS)
            {
                fail("MpiTransport::receiveAsync: " + mpiErrorText(code));
                return false;
            }
            if (!flag)
                return false;
        }

        int byteCount = 0;
        MPI_Get_count(&status, MPI_BYTE, &byteCount);
        m_staging.resize(static_cast<size_t>(byteCount));
        const int code = MPI_Imrecv(m_staging.data(), byteCount, MPI_BYTE,
                                    &matched, &m_request);
        if (code != MPI_SUCCESS)
        {
            fail("MpiTransport::receiveAsync: " + mpiErrorText(code));
            return false;
        }
        m_started = true;
        return true;
    }

    void finish()
    {
        std::string unpackMessage;
        if (!ValuePacker::unpack(m_staging, m_into, unpackMessage))
            m_failure = unpackMessage;
        m_complete = true;
    }

    void fail(std::string message)
    {
        m_failure  = std::move(message);
        m_complete = true;
    }

    MPI_Comm               m_comm;
    int                    m_source;
    int                    m_tag;
    BufferDescriptor       m_into;
    std::vector<std::byte> m_staging;
    MPI_Request            m_request = MPI_REQUEST_NULL;
    bool                   m_started  = false;
    bool                   m_complete = false;
    std::string            m_failure;
};

} // namespace

// ── MpiEnvironment ────────────────────────────────────────────────────────

MpiEnvironment::MpiEnvironment(int &argc, char **&argv)
{
    int initialized = 0;
    MPI_Initialized(&initialized);
    if (!initialized)
    {
        int provided = 0;
        MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
        m_ownsInit = true;
    }
}

MpiEnvironment::~MpiEnvironment()
{
    if (m_ownsInit)
    {
        int finalized = 0;
        MPI_Finalized(&finalized);
        if (!finalized)
            MPI_Finalize();
    }
}

// ── MpiTransport ──────────────────────────────────────────────────────────

struct MpiTransport::Impl
{
    MPI_Comm comm = MPI_COMM_NULL;
    int      rank = -1;
    int      size = 0;
};

MpiTransport::MpiTransport()
    : Identity("mpi-transport"), m_impl(std::make_unique<Impl>())
{
    MPI_Comm_dup(MPI_COMM_WORLD, &m_impl->comm);
    MPI_Comm_rank(m_impl->comm, &m_impl->rank);
    MPI_Comm_size(m_impl->comm, &m_impl->size);
    setCaption("MPI transport (rank " + std::to_string(m_impl->rank) + " of " +
               std::to_string(m_impl->size) + ")");
}

MpiTransport::~MpiTransport()
{
    if (m_impl && m_impl->comm != MPI_COMM_NULL)
    {
        int finalized = 0;
        MPI_Finalized(&finalized);
        if (!finalized)
            MPI_Comm_free(&m_impl->comm);
    }
}

bool MpiTransport::available()
{
    int initialized = 0;
    MPI_Initialized(&initialized);
    if (!initialized)
        return false;
    int finalized = 0;
    MPI_Finalized(&finalized);
    return !finalized;
}

MpiTransport::Endpoint MpiTransport::localEndpoint() const
{
    Endpoint endpoint;
    endpoint.address = "mpi";
    endpoint.rank    = m_impl->rank;
    return endpoint;
}

int32_t MpiTransport::participantCount() const
{
    return m_impl->size;
}

bool MpiTransport::send(const Endpoint &to, int32_t tag,
                        const BufferDescriptor &payload, std::string *message)
{
    if (to.rank < 0 || to.rank >= m_impl->size)
    {
        if (message)
            *message = "MpiTransport::send: invalid target rank " +
                       std::to_string(to.rank);
        return false;
    }
    std::vector<std::byte> wire;
    std::string            packMessage;
    if (!ValuePacker::pack(payload, wire, packMessage))
    {
        if (message)
            *message = packMessage;
        return false;
    }
    const int code = MPI_Send(wire.data(), static_cast<int>(wire.size()),
                              MPI_BYTE, to.rank, tag, m_impl->comm);
    if (code != MPI_SUCCESS)
    {
        if (message)
            *message = "MpiTransport::send: " + mpiErrorText(code);
        return false;
    }
    if (message)
        message->clear();
    return true;
}

bool MpiTransport::receive(const Endpoint &from, int32_t tag,
                           const BufferDescriptor &into, std::string *message)
{
    if (from.rank >= m_impl->size)
    {
        if (message)
            *message = "MpiTransport::receive: invalid source rank " +
                       std::to_string(from.rank);
        return false;
    }
    const int source = from.rank < 0 ? MPI_ANY_SOURCE : from.rank;

    MPI_Message matched;
    MPI_Status  status;
    int code = MPI_Mprobe(source, tag, m_impl->comm, &matched, &status);
    if (code != MPI_SUCCESS)
    {
        if (message)
            *message = "MpiTransport::receive: " + mpiErrorText(code);
        return false;
    }
    int byteCount = 0;
    MPI_Get_count(&status, MPI_BYTE, &byteCount);
    std::vector<std::byte> staging(static_cast<size_t>(byteCount));
    code = MPI_Mrecv(staging.data(), byteCount, MPI_BYTE, &matched,
                     MPI_STATUS_IGNORE);
    if (code != MPI_SUCCESS)
    {
        if (message)
            *message = "MpiTransport::receive: " + mpiErrorText(code);
        return false;
    }
    std::string unpackMessage;
    if (!ValuePacker::unpack(staging, into, unpackMessage))
    {
        if (message)
            *message = unpackMessage;
        return false;
    }
    if (message)
        message->clear();
    return true;
}

std::unique_ptr<HydroCouple::Distributed::IExchangeRequest>
MpiTransport::sendAsync(const Endpoint &to, int32_t tag,
                        const BufferDescriptor &payload)
{
    if (to.rank < 0 || to.rank >= m_impl->size)
        return std::make_unique<CompletedRequest>(
            "MpiTransport::sendAsync: invalid target rank " +
            std::to_string(to.rank));
    std::vector<std::byte> wire;
    std::string            packMessage;
    if (!ValuePacker::pack(payload, wire, packMessage))
        return std::make_unique<CompletedRequest>(packMessage);
    return std::make_unique<MpiSendRequest>(std::move(wire), m_impl->comm,
                                            to.rank, tag);
}

std::unique_ptr<HydroCouple::Distributed::IExchangeRequest>
MpiTransport::receiveAsync(const Endpoint &from, int32_t tag,
                           const BufferDescriptor &into)
{
    if (from.rank >= m_impl->size)
        return std::make_unique<CompletedRequest>(
            "MpiTransport::receiveAsync: invalid source rank " +
            std::to_string(from.rank));
    const int source = from.rank < 0 ? MPI_ANY_SOURCE : from.rank;
    return std::make_unique<MpiReceiveRequest>(m_impl->comm, source, tag,
                                               into);
}

} // namespace HydroCouple::SDK::Distributed
