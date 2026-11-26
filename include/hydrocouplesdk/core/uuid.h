// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   uuid.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Portable UUID v4 generator using only C++20 standard headers.
 *
 * No platform UUID APIs (uuid_generate, CoCreateGuid) are used.
 * Suitable for use on Windows, macOS, Linux, and any C++20-compliant platform.
 */

#ifndef HYDROCOUPLESDK_UUID_H
#define HYDROCOUPLESDK_UUID_H

#include <array>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace HydroCouple::SDK {

/*!
 * \brief Generate a random UUID v4 string in RFC 4122 format.
 *
 * Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
 * where x is a random hex digit and y is one of 8, 9, a, or b.
 *
 * Thread-safe: each thread owns a \c std::mt19937_64 generator, seeded once
 * from \c std::random_device on the thread's first call. The digits are
 * lowercase hexadecimal. The generator is not cryptographically secure; the
 * ids are meant to be unique, not unguessable. Identity uses this function
 * when no id is supplied.
 *
 * \return A new 36-character UUID string, for example
 *         "550e8400-e29b-41d4-a716-446655440000".
 */
[[nodiscard]] inline std::string generateUUID()
{
    // Use thread_local so each thread gets its own seeded PRNG
    thread_local std::mt19937_64 rng{std::random_device{}()};
    thread_local std::uniform_int_distribution<uint32_t> dist(0, 15);
    thread_local std::uniform_int_distribution<uint32_t> dist8(8, 11);

    std::ostringstream ss;
    ss << std::hex;

    for (int i = 0; i < 8;  ++i) ss << dist(rng);
    ss << '-';
    for (int i = 0; i < 4;  ++i) ss << dist(rng);
    ss << "-4";                                      // version 4
    for (int i = 0; i < 3;  ++i) ss << dist(rng);
    ss << '-';
    ss << dist8(rng);                                // variant bits: 8-b
    for (int i = 0; i < 3;  ++i) ss << dist(rng);
    ss << '-';
    for (int i = 0; i < 12; ++i) ss << dist(rng);

    return ss.str();
}

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_UUID_H
