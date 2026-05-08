# SPDX-License-Identifier: MIT
# Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>
#
# CMakeLists.txt — HydroCoupleSDK top-level build configuration
#
# Usage:
#   cmake --preset macos   (or windows / linux)
#   cmake --build build --parallel
#   ctest --test-dir build --output-on-failure

cmake_minimum_required(VERSION 3.18)

project(
    HydroCoupleSDK
    VERSION 1.0.0
    LANGUAGES C CXX
    HOMEPAGE_URL "https://hydrocouple.org"
    DESCRIPTION  "HydroCouple C++ Modeling Framework SDK"
)

# ── Language standard ─────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Custom CMake modules (includes FindOpenMP.cmake for macOS/Homebrew) ──
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── Symbol visibility (GCC/Clang; probed portably) ────────────────────────
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fvisibility=hidden"         COMPILER_HAS_VISIBILITY)
check_cxx_compiler_flag("-fvisibility-inlines-hidden" COMPILER_HAS_VISIBILITY_INLINES)
if(COMPILER_HAS_VISIBILITY)
    add_compile_options(-fvisibility=hidden)
endif()
if(COMPILER_HAS_VISIBILITY_INLINES)
    add_compile_options(-fvisibility-inlines-hidden)
endif()

# std::filesystem needs explicit link on GCC < 9
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
    link_libraries(stdc++fs)
endif()

# ── Feature options ───────────────────────────────────────────────────────
option(USE_NETCDF   "Build with NetCDF support"             ON)
option(USE_HDF5     "Build with HDF5 support"               ON)
option(USE_OPENMP   "Build with OpenMP support"             ON)
option(USE_MPI      "Build with MPI support"                OFF)
option(BUILD_TOOLS  "Build HydroCouple Tools library"       ON)
option(BUILD_TESTS  "Build unit tests"                      ON)
option(BUILD_DOCS   "Build Doxygen documentation"           OFF)

# ── Dependencies (all resolved by vcpkg toolchain) ────────────────────────
find_package(GDAL          REQUIRED)
find_package(nlohmann_json REQUIRED)

if(USE_NETCDF)
    find_package(netCDF QUIET)
    if(NOT netCDF_FOUND)
        # Fallback: some vcpkg builds expose it as netcdf-cxx4
        find_package(netcdf-cxx4 REQUIRED)
    endif()
endif()

if(USE_HDF5)
    find_package(HDF5 REQUIRED COMPONENTS C CXX)
endif()

if(USE_OPENMP)
    find_package(OpenMP REQUIRED)
endif()

if(USE_MPI)
    find_package(MPI REQUIRED)
endif()

# ── Version metadata (mirrors HydroCouple interface project) ─────────────
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC")
set(BUILD_DATE "${BUILD_TIMESTAMP}")

execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
endif()
if(NOT GIT_BRANCH_NAME)
    set(GIT_BRANCH_NAME "unknown")
endif()

string(
    UUID BUILD_ID
    NAMESPACE "00000000-0000-0000-0000-000000000000"
    NAME "https://www.hydrocouple.org/sdk"
    TYPE MD5
)
set(BUILD_COMMIT "${GIT_COMMIT_HASH}")
set(BUILD_BRANCH "${GIT_BRANCH_NAME}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h
    @ONLY
)

# ── Main library target ───────────────────────────────────────────────────
add_library(HydroCoupleSDK SHARED)

add_subdirectory(src)

target_compile_definitions(HydroCoupleSDK PRIVATE HYDROCOUPLESDK_LIBRARY)
if(USE_NETCDF)
    target_compile_definitions(HydroCoupleSDK PUBLIC USE_NETCDF)
endif()
if(USE_HDF5)
    target_compile_definitions(HydroCoupleSDK PUBLIC USE_HDF5)
endif()
if(USE_MPI)
    target_compile_definitions(HydroCoupleSDK PUBLIC USE_MPI)
endif()

target_include_directories(HydroCoupleSDK
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../HydroCouple/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(HydroCoupleSDK
    PUBLIC
        GDAL::GDAL
        nlohmann_json::nlohmann_json
    PRIVATE
        $<$<BOOL:${USE_OPENMP}>:OpenMP::OpenMP_CXX>
        $<$<BOOL:${USE_MPI}>:MPI::MPI_CXX>
)

if(USE_NETCDF)
    if(TARGET netCDF::netcdf-cxx4)
        target_link_libraries(HydroCoupleSDK PUBLIC netCDF::netcdf-cxx4)
    elseif(TARGET netcdf-cxx4)
        target_link_libraries(HydroCoupleSDK PUBLIC netcdf-cxx4)
    endif()
    if(TARGET netCDF::netcdf)
        target_link_libraries(HydroCoupleSDK PUBLIC netCDF::netcdf)
    endif()
endif()

if(USE_HDF5)
    target_link_libraries(HydroCoupleSDK PUBLIC HDF5::HDF5)
endif()

# ── Precompiled header (CXX only — stdafx.h has C++ headers) ─────────────
target_precompile_headers(HydroCoupleSDK PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/include/stdafx.h>"
)

# ── Portable optimisation flags ───────────────────────────────────────────
check_cxx_compiler_flag("-ftree-vectorize" HAS_TREE_VECTORIZE)
target_compile_options(HydroCoupleSDK PRIVATE
    $<$<CONFIG:Release>:$<IF:$<CXX_COMPILER_ID:MSVC>,/O2,-O3>>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${HAS_TREE_VECTORIZE}>>:-ftree-vectorize>
)

# ── Tools library (optional) ──────────────────────────────────────────────
if(BUILD_TOOLS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/tools/domain/terraindomain.cpp")
    add_library(HydroCoupleTools SHARED)
    add_subdirectory(src/tools)

    target_compile_definitions(HydroCoupleTools PRIVATE HYDROCOUPLETOOLS_LIBRARY)
    target_compile_features(HydroCoupleTools PUBLIC cxx_std_20)

    target_include_directories(HydroCoupleTools
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(HydroCoupleTools PUBLIC HydroCoupleSDK GDAL::GDAL)

    # Triangle library for Delaunay generation (CDT is the MIT replacement; Triangle kept for non-commercial)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/triangle/triangle.c)
        add_library(triangle_lib STATIC
            src/vendor/triangle/triangle.c)
        target_compile_definitions(triangle_lib PRIVATE
            TRILIBRARY ANSI_DECLARATORS NO_TIMER)
        set_property(TARGET triangle_lib PROPERTY C_STANDARD 99)
        target_link_libraries(HydroCoupleTools PRIVATE triangle_lib)
    endif()
endif()

# ── Tests ─────────────────────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()

# ── Documentation ─────────────────────────────────────────────────────────
if(BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    add_subdirectory(docs)
endif()

# ── Install rules (mirrors HydroCouple interface project) ─────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    TARGETS HydroCoupleSDK
    EXPORT  HydroCoupleSDKTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(BUILD_TOOLS AND TARGET HydroCoupleTools)
    install(
        TARGETS HydroCoupleTools
        EXPORT  HydroCoupleSDKTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(
    EXPORT HydroCoupleSDKTargets
    FILE   HydroCoupleSDKTargets.cmake
    NAMESPACE HydroCoupleSDK::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)

configure_package_config_file(
    cmake/HydroCoupleSDKConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/HydroCoupleSDKConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/HydroCoupleSDKConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/HydroCoupleSDKConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/HydroCoupleSDKConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(FILES LICENSE README.md DESTINATION .)

# ── CPack packaging (mirrors HydroCouple interface project) ───────────────
string(TOLOWER ${CMAKE_SYSTEM_NAME} OS_NAME)
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} ARCH_NAME)

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NuGet")
else()
    set(CPACK_GENERATOR "TGZ;NuGet")
endif()

set(CPACK_PACKAGE_NAME             HydroCoupleSDK)
set(CPACK_PACKAGE_VERSION          ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR           "hydrocouple.org")
set(CPACK_PACKAGE_CONTACT          "Caleb Buahin <caleb.buahin@gmail.com>")
set(CPACK_RESOURCE_FILE_LICENSE    "${CMAKE_CURRENT_SOURCE_DIR}/License")
set(CPACK_NUGET_PACKAGE_ID         HydroCoupleSDK)
set(CPACK_NUGET_PACKAGE_NAME       "HydroCoupleSDK.${ARCH_NAME}.${OS_NAME}")
set(CPACK_NUGET_PACKAGE_LICENSE_URL "https://opensource.org/licenses/MIT")
set(CPACK_NUGET_PACKAGE_PROJECT_URL "https://hydrocouple.org")
include(CPack)
