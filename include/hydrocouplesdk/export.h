// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   hydrocouplesdk.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Portable symbol-visibility macros for HydroCoupleSDK.
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

#endif // HYDROCOUPLESDK_H
