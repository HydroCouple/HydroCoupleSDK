// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   stdafx.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Precompiled-header seed for HydroCoupleSDK.
 *
 * Lists the stable, frequently-included C++20 standard library headers
 * and the project's own base headers.  Every translation unit includes
 * this file first (via CMake's target_precompile_headers).
 */

#ifndef HYDROCOUPLESDK_STDAFX_H
#define HYDROCOUPLESDK_STDAFX_H

// ── C++20 standard library ────────────────────────────────────────────────
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <mutex>
#include <numbers>
#include <numeric>
#include <optional>
#include <ranges>
#include <regex>
#include <set>
#include <shared_mutex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

// ── SDK base headers ──────────────────────────────────────────────────────
// Note: HydroCouple interface headers (hydrocouple.h etc.) are NOT included
// here — they must be included by each translation unit individually to
// control inclusion order (hydrocouplespatiotemporal.h includes
// hydrocoupletemporal.h, so ordering matters).
#include "hydrocouplesdk.h"

#endif // HYDROCOUPLESDK_STDAFX_H
