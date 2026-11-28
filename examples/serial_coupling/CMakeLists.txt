# SPDX-License-Identifier: MIT
# Standalone build of the serial coupling example against an INSTALLED SDK.
#
#   cmake -S . -B build -DCMAKE_PREFIX_PATH=<sdk-install-prefix>
#   cmake --build build
#   ./build/serial_coupling composition.yaml output/
#
# Linking only the installed package is the acceptance test for the export.

cmake_minimum_required(VERSION 3.20)
project(serial_coupling LANGUAGES CXX)

find_package(HydroCoupleSDK REQUIRED)

add_executable(serial_coupling main.cpp)
target_compile_features(serial_coupling PRIVATE cxx_std_20)
target_link_libraries(serial_coupling PRIVATE HydroCoupleSDK::HydroCoupleSDK)
