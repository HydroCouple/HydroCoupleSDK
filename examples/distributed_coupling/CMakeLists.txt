# SPDX-License-Identifier: MIT
# Standalone build of the distributed coupling example against an INSTALLED
# SDK that was built with USE_MPI=ON.
#
#   cmake -S . -B build -DCMAKE_PREFIX_PATH=<sdk-install-prefix>
#   cmake --build build
#   mpirun -n 4 ./build/distributed_coupling

cmake_minimum_required(VERSION 3.20)
project(distributed_coupling LANGUAGES CXX)

find_package(HydroCoupleSDK REQUIRED)
find_package(MPI REQUIRED)

add_executable(distributed_coupling main.cpp)
target_compile_features(distributed_coupling PRIVATE cxx_std_20)
target_link_libraries(distributed_coupling
    PRIVATE HydroCoupleSDK::HydroCoupleSDK MPI::MPI_CXX)
