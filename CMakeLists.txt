# SPDX-License-Identifier: MIT
# Copyright (c) 2014-2026 Caleb Buahin
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
    VERSION 2.0.0
    LANGUAGES C CXX
    HOMEPAGE_URL "https://hydrocouple.org"
    DESCRIPTION  "HydroCouple C++ Modeling Framework SDK"
)

# Pre-release suffix — appended to PROJECT_VERSION wherever the full version
# string is used (packaging, docs). Mirrors the HydroCouple interface repo.
set(HYDROCOUPLESDK_VERSION_SUFFIX "-alpha.1")
set(HYDROCOUPLESDK_FULL_VERSION "${PROJECT_VERSION}${HYDROCOUPLESDK_VERSION_SUFFIX}")

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
# Defaults track vcpkg.json default-features (lean core; opt in per feature:
# configure with -DUSE_<X>=ON and install the matching vcpkg --x-feature).
option(USE_GDAL     "Build with GDAL support (raster IO)"   OFF)
option(USE_NETCDF   "Build with NetCDF support"             OFF)
option(USE_HDF5     "Build with HDF5 support"               OFF)
option(USE_GEOPACKAGE "Build with GeoPackage (SQLite) output" OFF)
option(USE_OPENMP   "Build with OpenMP support"             OFF)
option(USE_MPI      "Build with MPI support"                OFF)
option(USE_KOKKOS   "Build the Kokkos device backend"       OFF)
option(BUILD_TOOLS  "Build HydroCouple Tools library"       OFF)
option(BUILD_TESTS  "Build unit tests"                      ON)
option(BUILD_DOCS   "Build Doxygen documentation"           OFF)

# ── HydroCouple interface headers ─────────────────────────────────────────
# Configurable so builds are not tied to a sibling checkout layout.
set(HYDROCOUPLE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../HydroCouple/include"
    CACHE PATH "Directory containing the HydroCouple interface headers")
if(NOT EXISTS "${HYDROCOUPLE_INCLUDE_DIR}/hydrocouple.h")
    message(FATAL_ERROR
        "HydroCouple interface headers not found at ${HYDROCOUPLE_INCLUDE_DIR}. "
        "Set -DHYDROCOUPLE_INCLUDE_DIR=<path>.")
endif()

# ── Version alignment with the interface definitions ──────────────────────
# The SDK versions in lockstep with the HydroCouple interface: MAJOR.MINOR
# must match the interface headers this build compiles against. The
# interface's include/version.h is the source of truth.
if(EXISTS "${HYDROCOUPLE_INCLUDE_DIR}/version.h")
    file(STRINGS "${HYDROCOUPLE_INCLUDE_DIR}/version.h" _hc_version_line
         REGEX "#define PROJECT_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\"")
    if(_hc_version_line MATCHES "\"([0-9]+)\\.([0-9]+)\\.([0-9]+)\"")
        set(HYDROCOUPLE_INTERFACE_VERSION
            "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
        if(NOT (CMAKE_MATCH_1 STREQUAL PROJECT_VERSION_MAJOR AND
                CMAKE_MATCH_2 STREQUAL PROJECT_VERSION_MINOR))
            message(FATAL_ERROR
                "Version misalignment: HydroCoupleSDK ${HYDROCOUPLESDK_FULL_VERSION} "
                "must share MAJOR.MINOR with the HydroCouple interface headers "
                "(found ${HYDROCOUPLE_INTERFACE_VERSION} at ${HYDROCOUPLE_INCLUDE_DIR}). "
                "Bump the SDK version or point HYDROCOUPLE_INCLUDE_DIR at a matching "
                "interface checkout.")
        endif()
        message(STATUS
            "HydroCoupleSDK ${HYDROCOUPLESDK_FULL_VERSION} against HydroCouple "
            "interface ${HYDROCOUPLE_INTERFACE_VERSION} — versions aligned")
    endif()
else()
    message(WARNING
        "Interface version.h not found at ${HYDROCOUPLE_INCLUDE_DIR}; "
        "skipping version-alignment check.")
endif()

# ── Dependencies (resolved by vcpkg toolchain when present) ───────────────
if(USE_GDAL)
    find_package(GDAL REQUIRED)
endif()

# yaml-cpp: optional — YAML composition documents convert to JSON at the
# boundary; without it the ModelInitializer accepts JSON only.
find_package(yaml-cpp CONFIG QUIET)
if(TARGET yaml-cpp::yaml-cpp OR TARGET yaml-cpp)
    set(HYDROCOUPLESDK_HAVE_YAML_CPP ON)
else()
    set(HYDROCOUPLESDK_HAVE_YAML_CPP OFF)
endif()

# nlohmann_json: config package preferred; fall back to downloading the
# MIT-licensed single header so bare environments (CI docs jobs, sandboxes)
# can still build.
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    set(NLOHMANN_JSON_INCLUDE_DIR "" CACHE PATH
        "Optional pre-provided directory containing nlohmann/json.hpp")
    if(NLOHMANN_JSON_INCLUDE_DIR AND EXISTS "${NLOHMANN_JSON_INCLUDE_DIR}/nlohmann/json.hpp")
        set(_NLOHMANN_DIR "${NLOHMANN_JSON_INCLUDE_DIR}")
    else()
        set(_NLOHMANN_DIR "${CMAKE_BINARY_DIR}/_deps/nlohmann/include")
    endif()
    if(NOT EXISTS "${_NLOHMANN_DIR}/nlohmann/json.hpp")
        message(STATUS "nlohmann_json not found — downloading single header")
        file(DOWNLOAD
            https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
            "${_NLOHMANN_DIR}/nlohmann/json.hpp"
            STATUS _NLOHMANN_STATUS)
        list(GET _NLOHMANN_STATUS 0 _NLOHMANN_CODE)
        if(NOT _NLOHMANN_CODE EQUAL 0)
            message(FATAL_ERROR "Failed to download nlohmann/json.hpp")
        endif()
    endif()
    add_library(nlohmann_json INTERFACE)
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
    target_include_directories(nlohmann_json INTERFACE "${_NLOHMANN_DIR}")
endif()

if(USE_NETCDF)
    # Config-mode first (netCDFConfig.cmake exports netCDF::netcdf), then a
    # manual header/library probe — the openswmm.engine fallback chain.
    find_package(netCDF CONFIG QUIET)
    if(TARGET netCDF::netcdf)
        set(HYDROCOUPLESDK_NETCDF_TARGET netCDF::netcdf)
    else()
        find_path(NETCDF_INCLUDE_DIR netcdf.h REQUIRED)
        find_library(NETCDF_LIBRARY NAMES netcdf REQUIRED)
        add_library(hydrocouplesdk_netcdf INTERFACE)
        target_include_directories(hydrocouplesdk_netcdf INTERFACE ${NETCDF_INCLUDE_DIR})
        target_link_libraries(hydrocouplesdk_netcdf INTERFACE ${NETCDF_LIBRARY})
        set(HYDROCOUPLESDK_NETCDF_TARGET hydrocouplesdk_netcdf)
    endif()
    message(STATUS "NetCDF target: ${HYDROCOUPLESDK_NETCDF_TARGET}")
endif()

if(USE_HDF5)
    # Config-mode first (vcpkg/self-built exports hdf5-targets), then
    # module-mode FindHDF5 — the openswmm.engine fallback chain.
    find_package(hdf5 CONFIG QUIET NAMES hdf5 HDF5)
    if(TARGET hdf5::hdf5-shared)
        set(HYDROCOUPLESDK_HDF5_TARGET hdf5::hdf5-shared)
    elseif(TARGET hdf5::hdf5-static)
        set(HYDROCOUPLESDK_HDF5_TARGET hdf5::hdf5-static)
    elseif(TARGET hdf5::hdf5)
        set(HYDROCOUPLESDK_HDF5_TARGET hdf5::hdf5)
    else()
        find_package(HDF5 REQUIRED COMPONENTS C)
        set(HYDROCOUPLESDK_HDF5_TARGET HDF5::HDF5)
    endif()
    # Static HDF5 archives need zlib chained explicitly.
    find_package(ZLIB QUIET)
    if(TARGET ZLIB::ZLIB)
        set(HYDROCOUPLESDK_HDF5_TARGET ${HYDROCOUPLESDK_HDF5_TARGET} ZLIB::ZLIB)
    endif()
    message(STATUS "HDF5 target: ${HYDROCOUPLESDK_HDF5_TARGET}")
endif()

if(USE_GEOPACKAGE)
    # Config-mode first (vcpkg exports unofficial-sqlite3), then the CMake
    # FindSQLite3 module — the openswmm.engine fallback chain.
    find_package(unofficial-sqlite3 CONFIG QUIET)
    if(TARGET unofficial::sqlite3::sqlite3)
        set(HYDROCOUPLESDK_SQLITE_TARGET unofficial::sqlite3::sqlite3)
    else()
        find_package(SQLite3 REQUIRED)
        set(HYDROCOUPLESDK_SQLITE_TARGET SQLite::SQLite3)
    endif()
    message(STATUS "SQLite target: ${HYDROCOUPLESDK_SQLITE_TARGET}")
endif()

if(USE_OPENMP)
    find_package(OpenMP REQUIRED)
endif()

if(USE_MPI)
    find_package(MPI REQUIRED)
endif()

if(USE_KOKKOS)
    find_package(Kokkos CONFIG REQUIRED)
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hydrocouplesdk/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hydrocouplesdk/version.h
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
if(USE_KOKKOS)
    target_compile_definitions(HydroCoupleSDK PUBLIC USE_KOKKOS)
endif()
if(USE_GDAL)
    target_compile_definitions(HydroCoupleSDK PUBLIC USE_GDAL)
endif()

target_include_directories(HydroCoupleSDK
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${HYDROCOUPLE_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(HydroCoupleSDK
    PUBLIC
        $<$<BOOL:${USE_GDAL}>:GDAL::GDAL>
    PRIVATE
        $<$<BOOL:${USE_OPENMP}>:OpenMP::OpenMP_CXX>
        $<$<BOOL:${USE_MPI}>:MPI::MPI_CXX>
        $<$<BOOL:${USE_KOKKOS}>:Kokkos::kokkos>
)

if(USE_HDF5)
    target_compile_definitions(HydroCoupleSDK PUBLIC HYDROCOUPLESDK_HAVE_HDF5)
    target_link_libraries(HydroCoupleSDK PRIVATE ${HYDROCOUPLESDK_HDF5_TARGET})
endif()

if(USE_GEOPACKAGE)
    target_compile_definitions(HydroCoupleSDK PUBLIC HYDROCOUPLESDK_HAVE_GEOPACKAGE)
    target_link_libraries(HydroCoupleSDK PRIVATE ${HYDROCOUPLESDK_SQLITE_TARGET})
endif()

if(USE_NETCDF)
    target_compile_definitions(HydroCoupleSDK PUBLIC HYDROCOUPLESDK_HAVE_NETCDF)
    target_link_libraries(HydroCoupleSDK PRIVATE ${HYDROCOUPLESDK_NETCDF_TARGET})
endif()

if(HYDROCOUPLESDK_HAVE_YAML_CPP)
    target_compile_definitions(HydroCoupleSDK PRIVATE HYDROCOUPLESDK_HAVE_YAML_CPP)
    if(TARGET yaml-cpp::yaml-cpp)
        target_link_libraries(HydroCoupleSDK PRIVATE yaml-cpp::yaml-cpp)
    else()
        target_link_libraries(HydroCoupleSDK PRIVATE yaml-cpp)
    endif()
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(HydroCoupleSDK PUBLIC nlohmann_json::nlohmann_json)
else()
    # The fallback header-only target exists only in the build tree; keep it
    # out of the install export set (installed consumers provide their own
    # nlohmann_json via vcpkg/find_package).
    target_link_libraries(HydroCoupleSDK PUBLIC
        $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>)
endif()

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
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.h>"
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

install(FILES License Readme.md DESTINATION .)

# ── CPack packaging (mirrors HydroCouple interface project) ───────────────
string(TOLOWER ${CMAKE_SYSTEM_NAME} OS_NAME)
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} ARCH_NAME)

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NuGet")
else()
    set(CPACK_GENERATOR "TGZ;NuGet")
endif()

set(CPACK_PACKAGE_NAME             HydroCoupleSDK)
set(CPACK_PACKAGE_VERSION          ${HYDROCOUPLESDK_FULL_VERSION})
set(CPACK_PACKAGE_VENDOR           "hydrocouple.org")
set(CPACK_PACKAGE_CONTACT          "Caleb Buahin")
set(CPACK_RESOURCE_FILE_LICENSE    "${CMAKE_CURRENT_SOURCE_DIR}/License")
set(CPACK_NUGET_PACKAGE_ID         HydroCoupleSDK)
set(CPACK_NUGET_PACKAGE_NAME       "HydroCoupleSDK.${ARCH_NAME}.${OS_NAME}")
set(CPACK_NUGET_PACKAGE_LICENSE_URL "https://opensource.org/licenses/MIT")
set(CPACK_NUGET_PACKAGE_PROJECT_URL "https://hydrocouple.org")
include(CPack)
