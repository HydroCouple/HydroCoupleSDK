// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/io/iothread.h"

#include <algorithm>

namespace HydroCouple::SDK::IO {

IOThread::IOThread(size_t capacity)
    : m_capacity(std::max<size_t>(1, capacity))
{
}

IOThread::~IOThread()
{
    finish();
}

void IOThread::addWriter(std::shared_ptr<IOutputWriter> writer)
{
    if (writer)
        m_writers.push_back({std::move(writer), true});
}

bool IOThread::start()
{
    if (m_running.load())
        return true;

    bool allInitialized = true;
    for (WriterState &state : m_writers)
    {
        const int code = state.writer->initialize();
        if (code != 0)
        {
            state.enabled = false;
            allInitialized = false;
            latchError(state.writer->name(), code,
                       state.writer->errorMessage());
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopRequested = false;
    }
    m_running.store(true);
    m_thread = std::thread(&IOThread::workerLoop, this);
    return allInitialized;
}

bool IOThread::enqueue(Snapshot snapshot)
{
    if (!m_running.load())
        return false;

    std::unique_lock<std::mutex> lock(m_mutex);
    m_notFull.wait(lock, [this] {
        return m_queue.size() < m_capacity || m_stopRequested;
    });
    if (m_stopRequested)
        return false;

    m_queue.push_back(std::move(snapshot));
    lock.unlock();
    m_notEmpty.notify_one();
    return true;
}

void IOThread::finish()
{
    if (!m_running.exchange(false))
        return;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopRequested = true;
    }
    m_notEmpty.notify_all();
    m_notFull.notify_all();

    if (m_thread.joinable())
        m_thread.join();

    for (WriterState &state : m_writers)
    {
        const int code = state.writer->finalize();
        if (code != 0 && state.enabled)
            latchError(state.writer->name(), code,
                       state.writer->errorMessage());
    }
}

bool IOThread::running() const noexcept
{
    return m_running.load();
}

int IOThread::errorCode() const noexcept
{
    return m_errorCode.load();
}

std::string IOThread::errorMessage() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_errorMessage;
}

size_t IOThread::deliveredCount() const noexcept
{
    return m_delivered.load();
}

void IOThread::workerLoop()
{
    for (;;)
    {
        Snapshot snapshot(0.0);
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_notEmpty.wait(lock, [this] {
                return !m_queue.empty() || m_stopRequested;
            });
            if (m_queue.empty())
                break; // stop requested and fully drained
            snapshot = std::move(m_queue.front());
            m_queue.pop_front();
        }
        m_notFull.notify_one();

        for (WriterState &state : m_writers)
        {
            if (!state.enabled)
                continue;
            const int code = state.writer->write(snapshot);
            if (code != 0)
            {
                state.enabled = false;
                latchError(state.writer->name(), code,
                           state.writer->errorMessage());
            }
        }
        m_delivered.fetch_add(1);
    }
}

void IOThread::latchError(const std::string &writerName, int code,
                          const std::string &message)
{
    int expected = 0;
    if (m_errorCode.compare_exchange_strong(expected, code))
    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_errorMessage = writerName + ": " + message;
    }
}

} // namespace HydroCouple::SDK::IO
