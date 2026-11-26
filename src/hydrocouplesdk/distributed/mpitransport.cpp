// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/distributed/mpitransport.h"
#include "hydrocouplesdk/distributed/valuepacker.h"

#include <mpi.h>

#include <algorithm>
#include <chrono>
#include <thread>
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
 * Bounded wait by polling test(): MPI has no timed wait. A non-positive
 * timeout tests once.
 */
bool pollForCompletion(HydroCouple::Distributed::IExchangeRequest &request,
                       double timeoutSeconds)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::duration<double>(std::max(0.0, timeoutSeconds));
    for (;;)
    {
        if (request.test())
            return true;
        if (std::chrono::steady_clock::now() >= deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

/*!
 * Already-completed request carrying an optional failure.
 */
class CompletedRequest final
    : public HydroCouple::Distributed::IExchangeRequest
{
public:
    //! \p failureMessage is the failure to report, or empty for success.
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
 * Nonblocking send: owns the packed wire block until MPI_Isend completes.
 */
class MpiSendRequest final
    : public HydroCouple::Distributed::IExchangeRequest
{
public:
    //! Posts MPI_Isend of \p wire (moved in) to \p target on \p tag; a
    //! failing post completes the request as failed.
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

    [[nodiscard]] bool waitFor(double timeoutSeconds) override
    {
        return pollForCompletion(*this, timeoutSeconds);
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
    std::vector<std::byte> m_wire;     //!< Packed block; must outlive the send.
    MPI_Request            m_request = MPI_REQUEST_NULL; //!< The MPI_Isend handle.
    bool                   m_complete = false; //!< MPI reported completion (or failure).
    std::string            m_failure;  //!< Empty on success.
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
    //! Records what to match; posts nothing until test() or wait().
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

    [[nodiscard]] bool waitFor(double timeoutSeconds) override
    {
        return pollForCompletion(*this, timeoutSeconds);
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

    //! Unpacks the staged block into the destination and completes.
    void finish()
    {
        std::string unpackMessage;
        if (!ValuePacker::unpack(m_staging, m_into, unpackMessage))
            m_failure = unpackMessage;
        m_complete = true;
    }

    //! Completes the request with \p message as its failure.
    void fail(std::string message)
    {
        m_failure  = std::move(message);
        m_complete = true;
    }

    MPI_Comm               m_comm;     //!< The transport's duplicated communicator.
    int                    m_source;   //!< MPI source rank (MPI_ANY_SOURCE allowed).
    int                    m_tag;      //!< MPI tag to match.
    BufferDescriptor       m_into;     //!< Caller's destination buffer.
    std::vector<std::byte> m_staging;  //!< Received wire block, sized by the probe.
    MPI_Request            m_request = MPI_REQUEST_NULL; //!< The MPI_Imrecv handle.
    bool                   m_started  = false; //!< A message was matched and MPI_Imrecv posted.
    bool                   m_complete = false; //!< Completed (successfully or not).
    std::string            m_failure;  //!< Empty on success.
};

} // namespace

// ── MpiEnvironment ────────────────────────────────────────────────────────

MpiEnvironment::MpiEnvironment(int &argc, char **&argv)
{
    int initialized = 0;
    MPI_Initialized(&initialized);
    if (!initialized)
    {
        // SERIALIZED, not MULTIPLE. The SDK needs only FUNNELED — nothing
        // in the distributed layer starts a thread and the IO thread never
        // calls MPI — and asking for MULTIPLE excludes implementations that
        // do not offer it (Microsoft MPI among them) while providing no
        // benefit here. The answer is kept rather than discarded so a host
        // can query what it actually got.
        MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &m_provided);
        m_ownsInit = true;
    }
    else
    {
        MPI_Query_thread(&m_provided);
    }
}

bool MpiEnvironment::threadLevelSufficient() const noexcept
{
    return m_provided >= MPI_THREAD_FUNNELED;
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

/*!
 * \brief The MPI state behind an MpiTransport.
 */
struct MpiTransport::Impl
{
    MPI_Comm comm = MPI_COMM_NULL; //!< Duplicate of MPI_COMM_WORLD owned by the transport.
    int      rank = -1;            //!< This process's rank in \c comm.
    int      size = 0;             //!< Number of processes in \c comm.
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
