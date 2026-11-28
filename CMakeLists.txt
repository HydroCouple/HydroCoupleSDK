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
set(HYDROCOUPLESDK_VERSION_SUFFIX "-beta.1")
set(HYDROCOUPLESDK_FULL_VERSION "${PROJECT_VERSION}${HYDROCOUPLESDK_VERSION_SUFFIX}")

option(HYDROCOUPLE_DOCS_ONLY "Configure documentation without model or GUI dependencies" OFF)
if(HYDROCOUPLE_DOCS_ONLY)
    find_package(Doxygen REQUIRED OPTIONAL_COMPONENTS dot)
    add_subdirectory(docs)
    return()
endif()

# ── Language standard ─────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Custom CMake modules (includes FindOpenMP.cmake for macOS/Homebrew) ──
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Standard install layout variables — needed by install() rules throughout.
include(GNUInstallDirs)

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
option(HYDROCOUPLESDK_WITH_TORCH "Build the optional LibTorch differentiation layer (HydroCoupleSDKTorch)" OFF)
option(BUILD_TESTS  "Build unit tests"                      ON)
option(BUILD_EXAMPLES "Build runnable examples"             OFF)
option(BUILD_DOCS   "Build Doxygen documentation"           OFF)

# ── HydroCouple interface headers ─────────────────────────────────────────
# Preferred: the interface project's installed CMake package (exports the
# HydroCouple::HydroCouple INTERFACE target, so consumers of an installed
# SDK inherit the interface include directory automatically).
# Fallback: a loose header directory (sibling checkout, CI sandboxes).
find_package(HydroCouple CONFIG QUIET)
if(TARGET HydroCouple::HydroCouple)
    get_target_property(_hc_interface_includes HydroCouple::HydroCouple
                        INTERFACE_INCLUDE_DIRECTORIES)
    list(GET _hc_interface_includes 0 HYDROCOUPLE_INCLUDE_DIR)
    set(HYDROCOUPLESDK_HAVE_INTERFACE_PACKAGE ON)
    message(STATUS "HydroCouple interface package found: ${HYDROCOUPLE_INCLUDE_DIR}")
else()
    set(HYDROCOUPLESDK_HAVE_INTERFACE_PACKAGE OFF)
endif()
set(HYDROCOUPLE_INCLUDE_DIR "${HYDROCOUPLE_INCLUDE_DIR}"
    CACHE PATH "Directory containing the HydroCouple interface headers")
if(NOT HYDROCOUPLE_INCLUDE_DIR)
    set(HYDROCOUPLE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../HydroCouple/include"
        CACHE PATH "Directory containing the HydroCouple interface headers" FORCE)
endif()
if(NOT EXISTS "${HYDROCOUPLE_INCLUDE_DIR}/hydrocouple.h")
    message(FATAL_ERROR
        "HydroCouple interface headers not found at ${HYDROCOUPLE_INCLUDE_DIR}. "
        "Set -DHYDROCOUPLE_INCLUDE_DIR=<path>.")
endif()

# ── Version alignment with the interface definitions ──────────────────────
# The SDK versions in lockstep with the HydroCouple interface: MAJOR.MINOR
# must match the interface headers this build compiles against.
#
# The interface's generated include/hydrocoupleversion.h is the source of
# truth when a built or installed tree is used. A plain source checkout (what
# CI does) only carries hydrocoupleversion.h.in, so fall back to the version
# declared in the interface's own CMakeLists.txt — otherwise the check
# silently skips exactly where it matters most.
set(HYDROCOUPLE_INTERFACE_VERSION "")
if(EXISTS "${HYDROCOUPLE_INCLUDE_DIR}/hydrocoupleversion.h")
    file(STRINGS "${HYDROCOUPLE_INCLUDE_DIR}/hydrocoupleversion.h" _hc_version_line
         REGEX "#define HYDROCOUPLE_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+")
    if(_hc_version_line MATCHES "\"([0-9]+)\\.([0-9]+)\\.([0-9]+)")
        set(HYDROCOUPLE_INTERFACE_VERSION
            "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
        set(_hc_version_source "${HYDROCOUPLE_INCLUDE_DIR}/hydrocoupleversion.h")
    endif()
endif()

if(NOT HYDROCOUPLE_INTERFACE_VERSION)
    # include/ is a sibling of the interface project root in every layout we
    # support (source checkout or install prefix).
    get_filename_component(_hc_project_root "${HYDROCOUPLE_INCLUDE_DIR}" DIRECTORY)
    if(EXISTS "${_hc_project_root}/CMakeLists.txt")
        file(STRINGS "${_hc_project_root}/CMakeLists.txt" _hc_project_lines)
        string(JOIN " " _hc_project_text ${_hc_project_lines})
        if(_hc_project_text MATCHES
           "project[ \t]*\\([ \t]*HydroCouple[ \t]+VERSION[ \t]+([0-9]+)\\.([0-9]+)\\.([0-9]+)")
            set(HYDROCOUPLE_INTERFACE_VERSION
                "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
            set(_hc_version_source "${_hc_project_root}/CMakeLists.txt")
        endif()
    endif()
endif()

if(HYDROCOUPLE_INTERFACE_VERSION)
    string(REPLACE "." ";" _hc_version_parts "${HYDROCOUPLE_INTERFACE_VERSION}")
    list(GET _hc_version_parts 0 _hc_major)
    list(GET _hc_version_parts 1 _hc_minor)
    if(NOT (_hc_major STREQUAL PROJECT_VERSION_MAJOR AND
            _hc_minor STREQUAL PROJECT_VERSION_MINOR))
        message(FATAL_ERROR
            "Version misalignment: HydroCoupleSDK ${HYDROCOUPLESDK_FULL_VERSION} "
            "must share MAJOR.MINOR with the HydroCouple interface "
            "(found ${HYDROCOUPLE_INTERFACE_VERSION} via ${_hc_version_source}). "
            "Bump the SDK version or point HYDROCOUPLE_INCLUDE_DIR at a matching "
            "interface checkout.")
    endif()
    message(STATUS
        "HydroCoupleSDK ${HYDROCOUPLESDK_FULL_VERSION} against HydroCouple "
        "interface ${HYDROCOUPLE_INTERFACE_VERSION} — versions aligned")
else()
    message(WARNING
        "Could not determine the HydroCouple interface version from "
        "${HYDROCOUPLE_INCLUDE_DIR} (no hydrocoupleversion.h, and no project(HydroCouple "
        "VERSION ...) nearby); skipping the version-alignment check.")
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

# version.h is generated into the build tree, not the source tree. It stamps
# the git commit, branch, and configure date, so writing it back into
# include/ left every working tree permanently dirty and committed whichever
# branch happened to be checked out when someone last configured — provenance
# that is wrong for every other checkout. Only version.h.in is tracked.
set(HYDROCOUPLESDK_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hydrocouplesdk/version.h.in
    ${HYDROCOUPLESDK_GENERATED_INCLUDE_DIR}/hydrocouplesdk/version.h
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
        # Generated first, so a stale version.h left in the source tree by an
        # older checkout cannot shadow the current one.
        $<BUILD_INTERFACE:${HYDROCOUPLESDK_GENERATED_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${HYDROCOUPLE_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# The SDK's public headers include hydrocouple.h, so the interface headers
# are a PUBLIC usage requirement. With the interface package present the
# exported target carries them; without it, the headers are installed
# alongside the SDK's own so an installed SDK is self-contained.
if(HYDROCOUPLESDK_HAVE_INTERFACE_PACKAGE)
    target_link_libraries(HydroCoupleSDK PUBLIC HydroCouple::HydroCouple)
else()
    install(
        DIRECTORY ${HYDROCOUPLE_INCLUDE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "hydrocouple*.h"
    )
endif()

target_link_libraries(HydroCoupleSDK
    PUBLIC
        $<$<BOOL:${USE_GDAL}>:GDAL::GDAL>
        # USE_MPI is a PUBLIC definition and the installed
        # abstractmodelcomponent.h includes <mpi.h>, so consumers of an
        # MPI-enabled SDK need the MPI usage requirements too.
        $<$<BOOL:${USE_MPI}>:MPI::MPI_CXX>
    PRIVATE
        $<$<BOOL:${USE_OPENMP}>:OpenMP::OpenMP_CXX>
        $<$<BOOL:${USE_KOKKOS}>:Kokkos::kokkos>
)

# The IO backends below are PRIVATE and stay that way. HydroCoupleSDK is a
# SHARED library, and CMake does not put a shared library's PRIVATE
# dependencies into its exported link interface, so none of these reach a
# consumer. (For a STATIC library CMake would export them as
# $<LINK_ONLY:...>, and the installed package config would then have to
# resolve them -- see cmake/HydroCoupleSDKConfig.cmake.in.)
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

# nlohmann/json.hpp is included by the public abstractcomponentdataitem.h,
# so it is a PUBLIC usage requirement of an installed SDK.
if(nlohmann_json_FOUND)
    # A real package: consumers resolve it through find_dependency().
    target_link_libraries(HydroCoupleSDK PUBLIC nlohmann_json::nlohmann_json)
else()
    # The fallback header-only target exists only in the build tree, so the
    # single header ships with the SDK and is reached through the SDK's own
    # installed include directory.
    target_link_libraries(HydroCoupleSDK PUBLIC
        $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>)
    install(
        FILES "${_NLOHMANN_DIR}/nlohmann/json.hpp"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nlohmann
    )
endif()

# NOTE: HDF5, NetCDF and SQLite are linked once, above, through the
# HYDROCOUPLESDK_*_TARGET variables the detection blocks resolve. Nothing
# links them a second time by hardcoded name here: config-mode packages
# export hdf5::hdf5-static / hdf5::hdf5-shared, and only module-mode
# FindHDF5 defines HDF5::HDF5, so a hardcoded name configures on whichever
# machine happens to use module mode and fails everywhere else. They are
# PRIVATE deliberately — no installed public header includes netcdf.h or
# hdf5.h, so consumers need neither the headers nor the libraries.

# ── Precompiled header (CXX only — stdafx.h has C++ headers) ─────────────
target_precompile_headers(HydroCoupleSDK PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.h>"
)

# ── Portable optimization flags ───────────────────────────────────────────
check_cxx_compiler_flag("-ftree-vectorize" HAS_TREE_VECTORIZE)
target_compile_options(HydroCoupleSDK PRIVATE
    $<$<CONFIG:Release>:$<IF:$<CXX_COMPILER_ID:MSVC>,/O2,-O3>>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${HAS_TREE_VECTORIZE}>>:-ftree-vectorize>
)

# ── Tools library (optional): mesh generation (plan §7b) ──────────────────
if(BUILD_TOOLS)
    # MeshKit (Apache-2.0) provides the constrained Delaunay engine: the same
    # library the OpenSWMM GUI meshes with, fetched at a release tag (see
    # cmake/FetchMeshKit.cmake for the local-checkout overrides).
    include(FetchMeshKit)

    add_library(HydroCoupleTools SHARED
        src/hydrocoupletools/sigmagrid.cpp
        src/hydrocoupletools/curvilineargrid.cpp
        src/hydrocoupletools/triangulator.cpp
        src/hydrocoupletools/quadmesher.cpp
        src/hydrocoupletools/terrainsampler.cpp
    )

    target_compile_definitions(HydroCoupleTools PRIVATE HYDROCOUPLETOOLS_LIBRARY)
    target_compile_features(HydroCoupleTools PUBLIC cxx_std_20)

    target_include_directories(HydroCoupleTools
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(HydroCoupleTools PUBLIC HydroCoupleSDK)
    # Static and position-independent: absorbed into the shared tools library,
    # so it is not part of the exported interface.
    target_link_libraries(HydroCoupleTools PRIVATE meshkit::core)
endif()


# ── Torch differentiation layer (optional, Phase G2.3) ────────────────────
# A separate library, so the core SDK never includes or links LibTorch.
# LibTorch comes from find_package(Torch) -- by default the one inside the
# Python `torch` wheel, so the SDK and Python torch share one LibTorch in a
# process (plan §5.1). HYDROCOUPLESDK_TORCH_ROOT (a directory holding
# include/ and lib/ of a LibTorch, e.g. site-packages/torch) bypasses
# TorchConfig for installs whose config cannot load -- a CUDA wheel on a
# machine without the CUDA toolkit, for one -- and links the CPU libraries
# directly. Build-tree only for now: not in the install/export set.
if(HYDROCOUPLESDK_WITH_TORCH)
    set(HYDROCOUPLESDK_TORCH_ROOT "" CACHE PATH
        "LibTorch root (include/, lib/) to use without TorchConfig.cmake")
    if(HYDROCOUPLESDK_TORCH_ROOT)
        find_library(HYDROCOUPLESDK_TORCH_CPU torch_cpu
            PATHS "${HYDROCOUPLESDK_TORCH_ROOT}/lib" NO_DEFAULT_PATH REQUIRED)
        find_library(HYDROCOUPLESDK_C10 c10
            PATHS "${HYDROCOUPLESDK_TORCH_ROOT}/lib" NO_DEFAULT_PATH REQUIRED)
        add_library(hydrocouplesdk_torch INTERFACE)
        target_include_directories(hydrocouplesdk_torch SYSTEM INTERFACE
            "${HYDROCOUPLESDK_TORCH_ROOT}/include"
            "${HYDROCOUPLESDK_TORCH_ROOT}/include/torch/csrc/api/include")
        target_link_libraries(hydrocouplesdk_torch INTERFACE
            ${HYDROCOUPLESDK_TORCH_CPU} ${HYDROCOUPLESDK_C10})
        set(HYDROCOUPLESDK_TORCH_TARGET hydrocouplesdk_torch)
        set(HYDROCOUPLESDK_TORCH_RPATH "${HYDROCOUPLESDK_TORCH_ROOT}/lib")
    else()
        if(NOT Torch_DIR)
            find_package(Python3 COMPONENTS Interpreter QUIET)
            if(Python3_Interpreter_FOUND)
                execute_process(
                    COMMAND ${Python3_EXECUTABLE} -c
                            "import torch.utils; print(torch.utils.cmake_prefix_path)"
                    OUTPUT_VARIABLE _torch_prefix
                    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
                if(_torch_prefix)
                    list(APPEND CMAKE_PREFIX_PATH "${_torch_prefix}")
                endif()
            endif()
        endif()
        find_package(Torch REQUIRED)
        set(HYDROCOUPLESDK_TORCH_TARGET ${TORCH_LIBRARIES})
        if(TARGET torch)
            get_target_property(_torch_loc torch IMPORTED_LOCATION)
            if(_torch_loc)
                get_filename_component(HYDROCOUPLESDK_TORCH_RPATH "${_torch_loc}" DIRECTORY)
            endif()
        endif()
    endif()
    message(STATUS "LibTorch: ${HYDROCOUPLESDK_TORCH_TARGET}")

    add_library(HydroCoupleSDKTorch SHARED
        src/hydrocouplesdk/differentiation/torch/atenmodelcomponent.cpp
        src/hydrocouplesdk/differentiation/torch/torchcomposition.cpp
    )
    target_compile_definitions(HydroCoupleSDKTorch PRIVATE HYDROCOUPLESDKTORCH_LIBRARY)
    target_compile_features(HydroCoupleSDKTorch PUBLIC cxx_std_20)
    target_link_libraries(HydroCoupleSDKTorch PUBLIC HydroCoupleSDK ${HYDROCOUPLESDK_TORCH_TARGET})
    if(HYDROCOUPLESDK_TORCH_RPATH)
        set_target_properties(HydroCoupleSDKTorch PROPERTIES
            BUILD_RPATH "${HYDROCOUPLESDK_TORCH_RPATH}")
    endif()
endif()

# ── Windows runtime co-location ───────────────────────────────────────────
# Windows has no rpath: an executable finds a sibling shared library only if
# the DLL sits next to it (or on PATH). Tests and examples live in their own
# build subdirectories, so copy the DLLs they link against alongside them.
# vcpkg app-local deployment handles its own ports; HydroCoupleSDK is ours.
# The copy runs before the link: vcpkg's deployment runs after it and reaches
# the ports HydroCoupleSDK.dll needs (z.dll for hdf5.dll) only through a
# HydroCoupleSDK.dll already beside the executable. Copied after the link,
# the result depended on whether a sibling target had copied it first.
function(hydrocouplesdk_colocate_runtime_dlls target)
    if(WIN32 AND NOT CMAKE_VERSION VERSION_LESS "3.21")
        add_custom_command(TARGET ${target} PRE_LINK
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endfunction()

# ── Tests ─────────────────────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()

# ── Examples ──────────────────────────────────────────────────────────────
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
    add_subdirectory(docs/examples)
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
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HydroCoupleSDK
)

configure_package_config_file(
    cmake/HydroCoupleSDKConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/HydroCoupleSDKConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HydroCoupleSDK
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
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HydroCoupleSDK
)

# The source tree's headers, minus version.h — that one is generated, and a
# stale copy in an older checkout must not be the one that ships.
install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "hydrocouplesdk/version.h" EXCLUDE
        # The Torch layer is build-tree only (see HYDROCOUPLESDK_WITH_TORCH).
        PATTERN "torch" EXCLUDE
        PATTERN "*.h"
)

install(
    FILES ${HYDROCOUPLESDK_GENERATED_INCLUDE_DIR}/hydrocouplesdk/version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hydrocouplesdk
)

install(FILES License Readme.md CHANGELOG.md DESTINATION .)

# Formal JSON Schemas for composition documents and run manifests. Shipped
# so editors, CI, and external tools can validate without any C++.
install(
    DIRECTORY share/schema/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/hydrocouplesdk/schema
    FILES_MATCHING PATTERN "*.json"
)

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
