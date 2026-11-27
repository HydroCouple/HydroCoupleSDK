// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   iolibrarylock.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  ioLibraryMutex() — the one lock around the HDF5 and NetCDF C
 *         libraries in a process.
 *
 * Neither library, as the SDK builds it, may be called from two threads at
 * once. HDF5's own thread-safe mode is incompatible with the high-level
 * library netcdf-c needs, and netcdf-c keeps unguarded process-wide state.
 * Working on separate files does not help: the race is on that shared state
 * (open-file and identifier tables, the error stack), not on a file.
 *
 * The single-writer IOThread keeps one pump's writers off each other, but a
 * process can run several pumps and readers at once: a host drawing results
 * on its UI thread while a run records, or two components each recording
 * their own files. Every SDK function that calls either library therefore
 * holds this lock for the duration of that one call.
 *
 * Defined in the SDK library rather than inline, so every component that
 * links the SDK shares the one mutex, just as it shares the one copy of each
 * library the SDK carries. Recursive, so an SDK call that makes another (a
 * result reader reading its mesh) cannot deadlock on itself.
 *
 * Code outside the SDK that calls nc_* or H5* through the SDK's copy of the
 * libraries must hold this lock for each call too. A component that links a
 * copy of its own must keep that copy private to its binary, and must never
 * open a file that another copy has open.
 */

#ifndef HYDROCOUPLESDK_IO_IOLIBRARYLOCK_H
#define HYDROCOUPLESDK_IO_IOLIBRARYLOCK_H

#include "hydrocouplesdk/export.h"

#include <mutex>

namespace HydroCouple::SDK::IO {

/*!
 * \brief The process-wide lock around every HDF5 and NetCDF C call.
 *
 * Hold it for one operation (an open, a read, a snapshot write, a close),
 * never for a file's lifetime, so a long recording does not block reads of
 * other files.
 *
 * \return The mutex; the caller acquires and releases it.
 * \threadsafety Safe to call from any thread at any time.
 */
HYDROCOUPLESDK_EXPORT std::recursive_mutex &ioLibraryMutex();

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_IOLIBRARYLOCK_H
