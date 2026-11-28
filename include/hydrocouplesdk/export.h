// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   export.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Portable symbol-visibility macros for HydroCoupleSDK.
 *
 * Every exported SDK class and free function is marked with
 * HYDROCOUPLESDK_EXPORT; HydroCoupleTools uses HYDROCOUPLETOOLS_EXPORT. The
 * libraries' own builds define HYDROCOUPLESDK_LIBRARY and
 * HYDROCOUPLETOOLS_LIBRARY (privately, in CMakeLists.txt), so on Windows
 * the library exports the symbols and every consumer imports them. A
 * consumer defines neither. The include guard is HYDROCOUPLESDK_H, a name
 * kept from when this file was hydrocouplesdk.h.
 */

#ifndef HYDROCOUPLESDK_H
#define HYDROCOUPLESDK_H

// ── Symbol visibility ──────────────────────────────────────────────────────
// Windows: __declspec(dllexport / dllimport)
// GCC / Clang: __attribute__((visibility("default")))
// Other compilers: no-op (static lib or compiler without visibility support)
#if defined(_WIN32) || defined(_WIN64)
#  ifdef HYDROCOUPLESDK_LIBRARY
#    define HYDROCOUPLESDK_EXPORT __declspec(dllexport)
#  else
#    define HYDROCOUPLESDK_EXPORT __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define HYDROCOUPLESDK_EXPORT __attribute__((visibility("default")))
#else
#  define HYDROCOUPLESDK_EXPORT
#endif

// ── Tools library visibility ───────────────────────────────────────────────
#if defined(_WIN32) || defined(_WIN64)
#  ifdef HYDROCOUPLETOOLS_LIBRARY
#    define HYDROCOUPLETOOLS_EXPORT __declspec(dllexport)
#  else
#    define HYDROCOUPLETOOLS_EXPORT __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define HYDROCOUPLETOOLS_EXPORT __attribute__((visibility("default")))
#else
#  define HYDROCOUPLETOOLS_EXPORT
#endif

/*!
 * \def HYDROCOUPLESDK_EXPORT
 * \brief Marks a declaration as part of the HydroCoupleSDK shared library's interface.
 * \details On Windows, \c __declspec(dllexport) while the SDK itself is built
 *          (HYDROCOUPLESDK_LIBRARY defined) and \c __declspec(dllimport) in
 *          every consumer; default visibility on GCC and Clang; empty on other
 *          compilers.
 */

/*!
 * \def HYDROCOUPLETOOLS_EXPORT
 * \brief Marks a declaration as part of the HydroCoupleTools shared library's interface.
 * \details The HydroCoupleTools counterpart of HYDROCOUPLESDK_EXPORT, keyed on
 *          HYDROCOUPLETOOLS_LIBRARY.
 */

#endif // HYDROCOUPLESDK_H
