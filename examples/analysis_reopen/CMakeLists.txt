# SPDX-License-Identifier: MIT
# Standalone build of the analysis example against an INSTALLED SDK.
#
#   cmake -S . -B build -DCMAKE_PREFIX_PATH=<sdk-install-prefix>
#   cmake --build build
#   ./build/analysis_reopen <run.json>
#
# The example links only the SDK — no model components — which is the
# point: a completed run is readable without the code that produced it.

cmake_minimum_required(VERSION 3.20)
project(analysis_reopen LANGUAGES CXX)

find_package(HydroCoupleSDK REQUIRED)

add_executable(analysis_reopen main.cpp)
target_compile_features(analysis_reopen PRIVATE cxx_std_20)
target_link_libraries(analysis_reopen PRIVATE HydroCoupleSDK::HydroCoupleSDK)
