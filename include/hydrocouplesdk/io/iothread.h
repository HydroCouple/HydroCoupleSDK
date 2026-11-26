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
 *
 * A snapshot is delivered to every enabled writer before the next one is
 * taken from the queue, so writers see the same sequence of snapshots.
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
     *                     (≥ 1; the back-pressure bound). 0 is stored as 1.
     *                     The snapshot the worker is writing is not counted.
     */
    explicit IOThread(size_t capacity = 64);

    /*!
     * \brief Calls finish() if the thread is still running.
     *
     * The writers are released afterwards; a writer shared with a
     * RunRecorder stays alive through the recorder's reference.
     */
    ~IOThread();

    //! Deleted: the worker thread refers to this object.
    IOThread(const IOThread &)            = delete;
    //! Deleted: the worker thread refers to this object.
    IOThread &operator=(const IOThread &) = delete;

    /*!
     * \brief Registers a writer. Must be called before start().
     *
     * Not synchronized with the worker thread: adding a writer while the
     * thread runs is a data race.
     *
     * \param[in] writer Shared ownership of the writer; nullptr is ignored.
     *                   The same writer may also be registered with a
     *                   RunRecorder.
     */
    void addWriter(std::shared_ptr<IOutputWriter> writer);

    /*!
     * \brief Calls initialize() on every writer and starts the worker.
     *
     * initialize() runs on the calling thread, in registration order. A
     * second call while running does nothing and returns true.
     *
     * \return true when all writers initialized (writers that fail are
     *         disabled and latched as the first error; the thread still
     *         starts for the remaining writers).
     */
    bool start();

    /*!
     * \brief Enqueues a snapshot for delivery, blocking while the queue is
     *        full (back-pressure).
     *
     * A writer error does not make enqueue() fail; check errorCode().
     *
     * \param[in] snapshot The snapshot, taken by move.
     * \return false when the thread is not running, or when finish() was
     *         called while this call waited for room (snapshot dropped).
     */
    bool enqueue(Snapshot snapshot);

    /*!
     * \brief Drains the queue, finalizes every writer, joins the thread.
     *        Idempotent.
     *
     * Every snapshot already queued is written; producers blocked in
     * enqueue() are released and their snapshots dropped. Then finalize()
     * is called on every writer, disabled ones included, on the calling
     * thread; a finalize() failure is latched only for a writer that had not
     * failed before. Does nothing when start() was never called, so
     * finalize() is then never called either.
     */
    void finish();

    /*!
     * \brief True between start() and finish().
     *
     * \return Whether the worker thread is running (start() starts it even
     *         when a writer failed to initialize).
     */
    [[nodiscard]] bool running() const noexcept;

    /*!
     * \brief First latched writer error code (0 when none).
     *
     * Safe to call from any thread.
     *
     * \return The first nonzero code any writer returned from
     *         initialize(), write() or finalize().
     */
    [[nodiscard]] int errorCode() const noexcept;

    /*!
     * \brief Description of the first latched error ("" when none):
     *        "\<writer\>: \<message\>".
     *
     * Safe to call from any thread.
     *
     * \return The failing writer's name() and errorMessage().
     */
    [[nodiscard]] std::string errorMessage() const;

    /*!
     * \brief Snapshots delivered to writers so far (diagnostics/tests).
     *
     * \return Snapshots taken off the queue and offered to every enabled
     *         writer, whether or not a writer failed on them.
     */
    [[nodiscard]] size_t deliveredCount() const noexcept;

private:
    /*!
     * \brief Worker thread body: takes snapshots in order and writes each to
     *        every enabled writer until stop is requested and the queue is
     *        empty.
     */
    void workerLoop();
    /*!
     * \brief Stores the first error only; later calls change nothing.
     *
     * \param[in] writerName Failing writer's name().
     * \param[in] code       Its nonzero return code.
     * \param[in] message    Its errorMessage().
     */
    void latchError(const std::string &writerName, int code,
                    const std::string &message);

    /*!
     * \brief One registered writer and whether it still receives snapshots.
     */
    struct WriterState
    {
        std::shared_ptr<IOutputWriter> writer; //!< The writer (shared ownership).
        bool enabled = true; //!< Cleared on first error from this writer.
    };

    size_t                    m_capacity; //!< Queue bound (at least 1).
    std::vector<WriterState>  m_writers;  //!< Registered writers, in registration order.

    mutable std::mutex        m_mutex;    //!< Guards m_queue and m_stopRequested.
    std::condition_variable   m_notFull;  //!< Signals producers.
    std::condition_variable   m_notEmpty; //!< Signals the worker.
    std::deque<Snapshot>      m_queue;    //!< Snapshots waiting for the worker.
    bool                      m_stopRequested = false; //!< Set by finish().

    std::thread               m_thread;   //!< The worker thread.
    std::atomic<bool>         m_running{false}; //!< True between start() and finish().
    std::atomic<size_t>       m_delivered{0};   //!< Snapshots taken off the queue.

    std::atomic<int>          m_errorCode{0};   //!< First latched error code.
    mutable std::mutex        m_errorMutex;     //!< Guards m_errorMessage.
    std::string               m_errorMessage;   //!< First latched error message.
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_IOTHREAD_H
