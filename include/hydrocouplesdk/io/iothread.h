// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   iothread.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  IOThread — the single writer thread of the HydroCouple::SDK::IO
 *         layer: bounded queue, back-pressure, drain-then-join shutdown.
 *
 * Design (ported from the proven openswmm.engine implementation): one
 * `std::mutex` and two condition variables guard a bounded Snapshot queue.
 * Producers block when the queue is full (back-pressure) so a slow disk
 * throttles the simulation instead of exhausting memory. All registered
 * IOutputWriter instances are driven from the single worker thread, so
 * writers never see concurrent calls. finish() drains the queue, calls
 * finalize() on every writer, and joins the thread.
 *
 * Errors are latched: the first nonzero writer return code is stored
 * (code + message + writer name) and that writer receives no further
 * snapshots; remaining writers continue.
 */

#ifndef HYDROCOUPLESDK_IO_IOTHREAD_H
#define HYDROCOUPLESDK_IO_IOTHREAD_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/io/snapshot.h"
#include "hydrocouplesdk/io/outputwriter.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace HydroCouple::SDK::IO {

/*!
 * \brief Single-writer IO pump with a bounded snapshot queue.
 *
 * Usage:
 * \code
 *   IOThread io(64);
 *   io.addWriter(std::make_shared<CSVWriter>("out.csv"));
 *   io.start();                 // initialize() on every writer
 *   ...
 *   io.enqueue(std::move(snap)); // blocks when the queue is full
 *   ...
 *   io.finish();                // drain, finalize(), join
 * \endcode
 *
 * \threadsafety enqueue() may be called from one or more producer threads;
 * start()/finish() are owner-thread operations. Writers are only ever
 * touched from the internal worker thread (and finalize() during finish()).
 */
class HYDROCOUPLESDK_EXPORT IOThread
{
public:
    /*!
     * \brief Construct with a queue capacity.
     * \param[in] capacity Maximum queued snapshots before enqueue() blocks
     *                     (≥ 1; the back-pressure bound).
     */
    explicit IOThread(size_t capacity = 64);

    /*!
     * \brief Calls finish() if the thread is still running.
     */
    ~IOThread();

    IOThread(const IOThread &)            = delete;
    IOThread &operator=(const IOThread &) = delete;

    /*!
     * \brief Registers a writer. Must be called before start().
     */
    void addWriter(std::shared_ptr<IOutputWriter> writer);

    /*!
     * \brief Calls initialize() on every writer and starts the worker.
     * \return true when all writers initialized (writers that fail are
     *         disabled and latched as the first error; the thread still
     *         starts for the remaining writers).
     */
    bool start();

    /*!
     * \brief Enqueues a snapshot for delivery, blocking while the queue is
     *        full (back-pressure).
     * \return false when the thread is not running (snapshot dropped).
     */
    bool enqueue(Snapshot snapshot);

    /*!
     * \brief Drains the queue, finalizes every writer, joins the thread.
     *        Idempotent.
     */
    void finish();

    /*!
     * \brief True between successful start() and finish().
     */
    [[nodiscard]] bool running() const noexcept;

    /*!
     * \brief First latched writer error code (0 when none).
     */
    [[nodiscard]] int errorCode() const noexcept;

    /*!
     * \brief Description of the first latched error ("" when none):
     *        "<writer>: <message>".
     */
    [[nodiscard]] std::string errorMessage() const;

    /*!
     * \brief Snapshots delivered to writers so far (diagnostics/tests).
     */
    [[nodiscard]] size_t deliveredCount() const noexcept;

private:
    void workerLoop();
    void latchError(const std::string &writerName, int code,
                    const std::string &message);

    struct WriterState
    {
        std::shared_ptr<IOutputWriter> writer;
        bool enabled = true; //!< Cleared on first error from this writer.
    };

    size_t                    m_capacity;
    std::vector<WriterState>  m_writers;

    mutable std::mutex        m_mutex;
    std::condition_variable   m_notFull;  //!< Signals producers.
    std::condition_variable   m_notEmpty; //!< Signals the worker.
    std::deque<Snapshot>      m_queue;
    bool                      m_stopRequested = false;

    std::thread               m_thread;
    std::atomic<bool>         m_running{false};
    std::atomic<size_t>       m_delivered{0};

    std::atomic<int>          m_errorCode{0};
    mutable std::mutex        m_errorMutex;
    std::string               m_errorMessage;
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_IOTHREAD_H
