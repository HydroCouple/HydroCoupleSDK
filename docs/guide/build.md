@page sdk_guide_build 2. Install, build and link

[TOC]

This chapter builds the SDK from source, installs it, and links a component or host against it.
@ref sdk_ref_cmake lists every option, cache variable and target in one place.

## Requirements

| Requirement | Notes |
| --- | --- |
| CMake 3.18 or newer | 3.21 or newer on Windows, where the build copies DLLs beside test and example programs |
| A C++20 compiler | The CI builds with the default compilers of GitHub's latest Linux (GCC), macOS (Apple Clang) and Windows (MSVC) runners |
| HydroCouple interface headers 2.0 | Found as the interface project's installed CMake package, or as a directory of headers (below) |
| nlohmann/json | Found as a CMake package; if none is found, the configure step downloads the single header (v3.11.3) into the build tree |
| yaml-cpp (optional) | Found if present; without it composition documents must be JSON |
| GoogleTest | Only for the tests (`BUILD_TESTS`, on by default) |
| Optional backends | GDAL, HDF5, NetCDF, SQLite, OpenMP, MPI, Kokkos, LibTorch, each behind its own option |

The repository's `vcpkg.json` declares every dependency, so the simplest way to get them is
[vcpkg](https://github.com/microsoft/vcpkg) in manifest mode: set `VCPKG_ROOT` to a vcpkg checkout
and use one of the presets below, or pass vcpkg's toolchain file yourself.

## The interface headers

The SDK compiles against the HydroCouple interface headers (`hydrocouple.h` and its companions). It
looks for them in this order:

1. The interface project's installed CMake package (`find_package(HydroCouple CONFIG)`), which
   exports the `HydroCouple::HydroCouple` target. An SDK built this way links that target
   publicly, so a consumer of the installed SDK inherits the interface include directory.
2. `HYDROCOUPLE_INCLUDE_DIR`, a cache variable naming the directory that contains
   `hydrocouple.h`. Its default is a sibling checkout, `../HydroCouple/include`. An SDK built this
   way installs the interface headers beside its own, so the installed SDK is self-contained.

If neither yields `hydrocouple.h`, the configure step stops:

```text
HydroCouple interface headers not found at <path>. Set -DHYDROCOUPLE_INCLUDE_DIR=<path>.
```

The configure step then checks that the interfaces have the SDK's major and minor version (2.0),
reading `hydrocoupleversion.h` or, in a plain source checkout, the version in the interface
project's `CMakeLists.txt`. A mismatch stops the configure with a "Version misalignment" error
naming both versions; if no version can be found, it warns and skips the check.

## Configure and build

### With a preset

`CMakePresets.json` defines presets that use vcpkg's toolchain (`$VCPKG_ROOT`) and build into
`build/`:

| Preset | Generator | What it adds |
| --- | --- | --- |
| `default` | Ninja | vcpkg features `tests` |
| `linux` | Unix Makefiles | as `default` |
| `windows` | Visual Studio 17 2022 | as `default` |
| `macos` | Unix Makefiles | the `arm64-osx-dynamic` vcpkg triplet |
| `macos-full` | Unix Makefiles | as `macos`, plus vcpkg features `netcdf;hdf5;gdal;tools;geopackage` and the options `USE_NETCDF`, `USE_HDF5`, `USE_GDAL`, `USE_GEOPACKAGE`, `BUILD_TOOLS` |

```shell
cmake --preset macos
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Each preset has a build preset of the same name.

### By hand

Every optional feature is two switches that must agree: a CMake option that compiles the code
(`-DUSE_NETCDF=ON`) and a vcpkg manifest feature that provides the library (`netcdf`). The
manifest features are `tests`, `gdal`, `netcdf`, `hdf5`, `geopackage`, `mpi`, `gpu` (Kokkos) and
`tools`. This configures what the SDK's CI builds on Linux, macOS and Windows:

```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  "-DVCPKG_MANIFEST_FEATURES=tests;hdf5;geopackage;netcdf;tools" \
  -DHYDROCOUPLE_INCLUDE_DIR=../HydroCouple/include \
  -DUSE_GDAL=OFF -DUSE_NETCDF=ON -DUSE_HDF5=ON -DUSE_GEOPACKAGE=ON -DUSE_OPENMP=OFF \
  -DBUILD_TOOLS=ON -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON -DMESHKIT_REQUIRE_PINNED=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The options that select what is built:

| Option | Default | Effect |
| --- | --- | --- |
| `USE_GDAL` | OFF | Raster data items, and GDAL-based input in mesh import and terrain sampling |
| `USE_NETCDF` | OFF | The NetCDF UGRID writer and reader, and mesh packages (@ref sdk_guide_mesh_packages) |
| `USE_HDF5` | OFF | The HDF5 UGRID writer and reader |
| `USE_GEOPACKAGE` | OFF | The OGC GeoPackage writer (SQLite) |
| `USE_OPENMP` | OFF | OpenMP parallel loops inside the SDK |
| `USE_MPI` | OFF | The MPI transport and the MPI members of components (@ref sdk_guide_distributed) |
| `USE_KOKKOS` | OFF | The Kokkos device backend (@ref sdk_guide_devices) |
| `BUILD_TOOLS` | OFF | The `HydroCoupleTools` meshing library (@ref sdk_guide_tools) |
| `HYDROCOUPLESDK_WITH_TORCH` | OFF | The `HydroCoupleSDKTorch` library (@ref sdk_guide_differentiation) |
| `BUILD_TESTS` | ON | The GoogleTest suite (needs GoogleTest) |
| `BUILD_EXAMPLES` | OFF | `examples/` and the developer guide's compiled examples in `docs/examples/` |
| `BUILD_DOCS` | OFF | The Doxygen manual, as the `docs` target |

`BUILD_TOOLS` needs MeshKit, which the configure step fetches from GitHub at a pinned release tag,
or takes from a sibling `../MeshKit` checkout when there is one (@ref sdk_ref_cmake explains how
to choose). MeshKit is linked into `HydroCoupleTools` privately, so consumers never need it.

### What the build defines

Building with an optional backend adds compile definitions to the `HydroCoupleSDK` target. The ones
marked public reach every program that links the SDK, so a component can test for a feature in its
own code, as the serial-coupling example does with `HYDROCOUPLESDK_HAVE_HDF5`:

| Option | Public definitions |
| --- | --- |
| `USE_NETCDF` | `USE_NETCDF`, `HYDROCOUPLESDK_HAVE_NETCDF` |
| `USE_HDF5` | `USE_HDF5`, `HYDROCOUPLESDK_HAVE_HDF5` |
| `USE_GEOPACKAGE` | `HYDROCOUPLESDK_HAVE_GEOPACKAGE` |
| `USE_GDAL` | `USE_GDAL` |
| `USE_MPI` | `USE_MPI` |
| `USE_KOKKOS` | `USE_KOKKOS` |

YAML support is not a public definition; code asks `IO::ModelInitializer::yamlSupported()` at run
time.

## Install

```shell
cmake --install build --prefix /opt/hydrocouplesdk
```

Give `--prefix` as an absolute path. The installed tree contains:

| Path | Contents |
| --- | --- |
| `include/hydrocouplesdk/` | The SDK's public headers and the generated `version.h` (the Torch headers are not installed) |
| `include/hydrocouple*.h` | The interface headers, when the SDK was built from a header directory rather than the interface package |
| `include/nlohmann/json.hpp` | Only when the SDK downloaded the header instead of finding a package |
| `lib/` (`bin/` for Windows DLLs) | `HydroCoupleSDK` and, with `BUILD_TOOLS`, `HydroCoupleTools` |
| `lib/cmake/HydroCoupleSDK/` | The CMake package: `HydroCoupleSDKConfig.cmake`, its version file and the exported targets |
| `share/hydrocouplesdk/schema/` | The JSON schemas for composition documents (1.0 and 1.1) and run manifests (1.0) |
| `License`, `Readme.md`, `CHANGELOG.md` | At the prefix root |

The libraries are shared libraries. `HydroCoupleSDKTorch` is built in the build tree only.

## Link a component or host

A project that uses an installed SDK finds its package and links its target:

```cmake
cmake_minimum_required(VERSION 3.18)
project(my_component LANGUAGES CXX)

find_package(HydroCoupleSDK 2.0 REQUIRED)

add_library(my_component MODULE my_component.cpp)
target_compile_features(my_component PRIVATE cxx_std_20)
target_link_libraries(my_component PRIVATE HydroCoupleSDK::HydroCoupleSDK)
```

Configure it with the SDK's prefix on `CMAKE_PREFIX_PATH`:

```shell
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/hydrocouplesdk
```

`find_package(HydroCoupleSDK 2.0)` accepts any installed 2.x SDK. The package provides:

- the target `HydroCoupleSDK::HydroCoupleSDK` and, if the SDK was built with `BUILD_TOOLS`,
  `HydroCoupleSDK::HydroCoupleTools`;
- the usage requirements those targets need: the interface headers (through the interface package
  or the installed copies), nlohmann/json, and, for an SDK built with them, MPI and GDAL;
- variables that report how the SDK was built, `HYDROCOUPLESDK_WITH_MPI`, `_WITH_KOKKOS`,
  `_WITH_GDAL`, `_WITH_HDF5`, `_WITH_GEOPACKAGE`, `_WITH_NETCDF`, `_WITH_TOOLS`, `_WITH_YAML_CPP`,
  so a consumer can branch on them.

HDF5, NetCDF, SQLite, yaml-cpp, Kokkos and OpenMP are linked into the shared SDK privately and
appear in no installed header, so a consumer of an SDK built with them does not need to find them.
MPI and GDAL are public: an MPI-enabled SDK's `abstractmodelcomponent.h` includes `mpi.h`, and a
GDAL-enabled SDK links GDAL publicly, so the package finds both for its consumers.

The SDK's `examples/serial_coupling/` and `examples/analysis_reopen/` each have a standalone
`CMakeLists.txt` that consumes only the installed package; building them that way is the check
that an installation is complete.

### Building the SDK inside your project

A project can instead add an SDK source checkout to its own build, as FVQual does when no installed
SDK is found:

```cmake
add_subdirectory(${MY_SDK_SOURCE_DIR} ${CMAKE_BINARY_DIR}/_hydrocouplesdk)
if(NOT TARGET HydroCoupleSDK::HydroCoupleSDK)
    add_library(HydroCoupleSDK::HydroCoupleSDK ALIAS HydroCoupleSDK)
endif()
```

Inside a build the targets are named `HydroCoupleSDK` and `HydroCoupleTools` without the
namespace, so define the aliases to keep the same `target_link_libraries` line in both cases. Set
the SDK's options (for example `BUILD_TOOLS`, `BUILD_TESTS=OFF`) before `add_subdirectory`.

### Windows

Windows has no runtime search path, so a program finds the SDK's DLLs only in its own directory or
on `PATH`. Copy them next to each executable after it is built:

```cmake
add_custom_command(TARGET my_host POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:my_host> $<TARGET_FILE_DIR:my_host>
    COMMAND_EXPAND_LISTS)
```

The SDK does this for its own tests and examples (`hydrocouplesdk_colocate_runtime_dlls`).
A component library loaded by a host also needs the SDK's DLLs where the host can find them,
usually beside the host's executable.

## Build the manual

The manual you are reading and the API reference are one Doxygen build. Without building the
library or finding any dependency:

```shell
cmake -S . -B build/manual -DHYDROCOUPLE_DOCS_ONLY=ON
cmake --build build/manual --target docs
```

The site is in `build/manual/docs/doxygen/html/`, and `guide/index.html` there redirects to this
guide. A normal build makes the same `docs` target with `-DBUILD_DOCS=ON`. The CI publishes the
manual with Doxygen 1.13.2. Formulas render with MathJax in the browser, so no LaTeX is needed.

## Common configure problems

| Message | Cause and fix |
| --- | --- |
| `HydroCouple interface headers not found at <path>` | Neither the interface package nor `HYDROCOUPLE_INCLUDE_DIR` provides `hydrocouple.h`; pass `-DHYDROCOUPLE_INCLUDE_DIR=<checkout>/include` |
| `Version misalignment: HydroCoupleSDK ... must share MAJOR.MINOR with the HydroCouple interface` | The interface headers are from another 2.x minor version (or another major); check out the interfaces' matching branch |
| `Failed to download nlohmann/json.hpp` | No nlohmann_json package and no network; install the package (vcpkg `nlohmann-json`) or set `NLOHMANN_JSON_INCLUDE_DIR` to a directory containing `nlohmann/json.hpp` |
| `Could NOT find GTest` | `BUILD_TESTS` is on by default; add the vcpkg feature `tests` or pass `-DBUILD_TESTS=OFF` |
| A `find_package` failure for GDAL, HDF5, netCDF, SQLite3, MPI or Kokkos | The option is on but the library is missing; add the matching vcpkg feature or install it |
| `MeshKit: MESHKIT_REQUIRE_PINNED is ON, so MeshKit must come from ...` | A local MeshKit checkout was given while release builds must fetch the pinned tag; unset `MESHKIT_SOURCE_DIR` |
