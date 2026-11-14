// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
/*!
 * \file   torchexport.h
 * \brief  Symbol visibility for the optional HydroCoupleSDKTorch library.
 */
#ifndef HYDROCOUPLESDK_TORCHEXPORT_H
#define HYDROCOUPLESDK_TORCHEXPORT_H

#if defined(_WIN32) || defined(_WIN64)
#  ifdef HYDROCOUPLESDKTORCH_LIBRARY
#    define HYDROCOUPLESDKTORCH_EXPORT __declspec(dllexport)
#  else
#    define HYDROCOUPLESDKTORCH_EXPORT __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define HYDROCOUPLESDKTORCH_EXPORT __attribute__((visibility("default")))
#else
#  define HYDROCOUPLESDKTORCH_EXPORT
#endif

#endif // HYDROCOUPLESDK_TORCHEXPORT_H
