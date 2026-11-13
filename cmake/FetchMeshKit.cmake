# cmake/FetchMeshKit.cmake
#
# Provides MeshKit's Qt-free targets (meshkit::core, meshkit::io,
# meshkit::runtime) for the tools library.
#
# Resolution order (first match wins), as for the GUI's QPropertyModel:
#   1. Explicit `-DMESHKIT_SOURCE_DIR=/path/to/checkout`   (user override)
#   2. Sibling checkout at `${CMAKE_SOURCE_DIR}/../MeshKit` (or `../Meshkit`) (development)
#   3. GitHub fetch via FetchContent at a release tag
#
# Cache variables (override at configure time with -D…):
#   MESHKIT_SOURCE_DIR     local checkout path (empty = use auto/fetch)
#   MESHKIT_GIT_REPOSITORY upstream URL
#   MESHKIT_GIT_TAG        release tag (or branch / commit SHA)
#
# MeshKit is linked privately into the shared tools library, so consumers of
# the installed SDK never need it. Its own install rules add its LICENSE and
# NOTICE beside the SDK's, which Apache-2.0 requires of a binary that carries
# its code.

include_guard(GLOBAL)
include(FetchContent)

set(MESHKIT_SOURCE_DIR "" CACHE PATH
    "Optional path to a local MeshKit checkout (overrides the GitHub fetch).")
set(MESHKIT_GIT_REPOSITORY "https://github.com/HydroCouple/Meshkit.git" CACHE STRING
    "Git URL to fetch MeshKit from.")
set(MESHKIT_GIT_TAG "v0.2.0" CACHE STRING
    "Release tag (or branch / commit SHA) of MeshKit to use.")

# A clone of HydroCouple/Meshkit lands in `Meshkit/`; accept either spelling
# (they differ on case-sensitive filesystems).
foreach(_meshkit_sibling MeshKit Meshkit)
    if(NOT MESHKIT_SOURCE_DIR
            AND EXISTS "${CMAKE_SOURCE_DIR}/../${_meshkit_sibling}/CMakeLists.txt")
        set(MESHKIT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../${_meshkit_sibling}")
    endif()
endforeach()

# The SDK is Qt-free: only the standard C++ targets, and none of MeshKit's
# tests, tools or Python bindings.
set(MESHKIT_BUILD_QT OFF)
set(MESHKIT_BUILD_TESTS OFF)
set(MESHKIT_BUILD_CLI OFF)
set(MESHKIT_BUILD_PYTHON OFF)
set(MESHKIT_BUILD_BENCHMARKS OFF)

if(MESHKIT_SOURCE_DIR AND EXISTS "${MESHKIT_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "MeshKit: using local checkout at ${MESHKIT_SOURCE_DIR}")
    FetchContent_Declare(MeshKit
        SOURCE_DIR "${MESHKIT_SOURCE_DIR}"
    )
else()
    message(STATUS "MeshKit: fetching ${MESHKIT_GIT_REPOSITORY} @ ${MESHKIT_GIT_TAG}")
    FetchContent_Declare(MeshKit
        GIT_REPOSITORY "${MESHKIT_GIT_REPOSITORY}"
        GIT_TAG        "${MESHKIT_GIT_TAG}"
        GIT_SHALLOW    TRUE
    )
endif()

FetchContent_MakeAvailable(MeshKit)

if(NOT TARGET meshkit::core)
    message(FATAL_ERROR "MeshKit did not provide meshkit::core")
endif()
