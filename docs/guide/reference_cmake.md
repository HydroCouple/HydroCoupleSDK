@page sdk_ref_cmake 21. CMake options and targets

[TOC]

Every option, cache variable, target, package variable and compile definition of the SDK's CMake
build, taken from the top-level `CMakeLists.txt`, `cmake/FetchMeshKit.cmake` and
`cmake/HydroCoupleSDKConfig.cmake.in`. @ref sdk_guide_build explains how to use them.

## Options

| Option | Default | Effect |
| --- | --- | --- |
| `USE_GDAL` | OFF | Find GDAL (required) and build raster data items and GDAL-based input; GDAL becomes a public dependency |
| `USE_NETCDF` | OFF | Find netCDF (config package `netCDF::netcdf`, else a header and library search) and build the NetCDF UGRID writer and reader and mesh packages |
| `USE_HDF5` | OFF | Find HDF5 (config package, else CMake's FindHDF5; zlib is chained for static archives) and build the HDF5 UGRID writer and reader |
| `USE_GEOPACKAGE` | OFF | Find SQLite (`unofficial::sqlite3::sqlite3`, else FindSQLite3) and build the GeoPackage writer |
| `USE_OPENMP` | OFF | Find OpenMP (required) and compile the SDK's parallel loops |
| `USE_MPI` | OFF | Find MPI (required), build the MPI transport and the MPI distributed example; MPI becomes a public dependency |
| `USE_KOKKOS` | OFF | Find Kokkos (config package, required) and build the Kokkos device backend |
| `BUILD_TOOLS` | OFF | Fetch MeshKit and build the `HydroCoupleTools` library |
| `HYDROCOUPLESDK_WITH_TORCH` | OFF | Find LibTorch and build the `HydroCoupleSDKTorch` library and its tests |
| `BUILD_TESTS` | ON | Find GoogleTest (required) and build the unit tests |
| `BUILD_EXAMPLES` | OFF | Build `examples/` and the guide's compiled examples in `docs/examples/`; with `BUILD_TESTS`, register them as tests |
| `BUILD_DOCS` | OFF | Find Doxygen and add the `docs` target |
| `HYDROCOUPLE_DOCS_ONLY` | OFF | Configure only the documentation: find Doxygen, add the `docs` target and stop; no compiler dependencies are searched for |
| `MESHKIT_REQUIRE_PINNED` | OFF | Release and CI builds: ignore local MeshKit checkouts and always fetch `MESHKIT_GIT_TAG`; an explicit `MESHKIT_SOURCE_DIR` is then an error |

Each optional dependency has a matching feature in `vcpkg.json`: `gdal`, `netcdf` (netcdf-c),
`hdf5`, `geopackage` (sqlite3), `mpi`, `gpu` (kokkos), `tools` (no extra port) and `tests`
(GoogleTest 1.15.2). yaml-cpp and nlohmann-json are base dependencies.

## Cache variables

| Variable | Default | Meaning |
| --- | --- | --- |
| `HYDROCOUPLE_INCLUDE_DIR` | the interface package's include directory, else `../HydroCouple/include` | Directory containing `hydrocouple.h` |
| `NLOHMANN_JSON_INCLUDE_DIR` | empty | Directory containing `nlohmann/json.hpp`, used when no nlohmann_json package is found, instead of downloading the header |
| `MESHKIT_SOURCE_DIR` | empty | A local MeshKit checkout to build `HydroCoupleTools` against |
| `MESHKIT_GIT_REPOSITORY` | `https://github.com/HydroCouple/Meshkit.git` | Where MeshKit is fetched from |
| `MESHKIT_GIT_TAG` | `v0.2.0` | The MeshKit release tag (or branch or commit) to fetch |
| `HYDROCOUPLESDK_TORCH_ROOT` | empty | A LibTorch directory holding `include/` and `lib/`, linked directly when its `TorchConfig.cmake` cannot be loaded (for example a CUDA wheel on a machine without the CUDA toolkit) |
| `Torch_DIR` | found from Python | The directory of `TorchConfig.cmake`; when unset, the configure step asks the Python interpreter for `torch.utils.cmake_prefix_path` |

The usual vcpkg variables apply as well: `CMAKE_TOOLCHAIN_FILE`, `VCPKG_MANIFEST_FEATURES`,
`VCPKG_TARGET_TRIPLET` and `VCPKG_INSTALLED_DIR`.

**MeshKit resolution.** With `BUILD_TOOLS=ON`, MeshKit comes from, in order: `MESHKIT_SOURCE_DIR`;
a sibling checkout `../MeshKit` (or `../Meshkit`); a fetch of `MESHKIT_GIT_TAG`. With
`MESHKIT_REQUIRE_PINNED=ON` only the fetch is allowed. The version MeshKit's sources declare is
compared with the tag: a fetched tag whose sources declare another version stops the configure,
and a local checkout of another version is reported. MeshKit is linked into `HydroCoupleTools`
privately and its LICENSE and NOTICE are installed beside the SDK's.

**LibTorch resolution.** With `HYDROCOUPLESDK_WITH_TORCH=ON`: `HYDROCOUPLESDK_TORCH_ROOT` if set
(the `torch_cpu` and `c10` libraries are linked directly); otherwise `find_package(Torch)`, with
the Python `torch` package's CMake prefix added when `Torch_DIR` is unset, so the SDK and Python
share one LibTorch in a process.

## Targets

| Target | Kind | Built when | Notes |
| --- | --- | --- | --- |
| `HydroCoupleSDK` | shared library | always | Exported as `HydroCoupleSDK::HydroCoupleSDK` |
| `HydroCoupleTools` | shared library | `BUILD_TOOLS` | Links `HydroCoupleSDK` publicly and MeshKit privately; exported as `HydroCoupleSDK::HydroCoupleTools` |
| `HydroCoupleSDKTorch` | shared library | `HYDROCOUPLESDK_WITH_TORCH` | Build tree only; links LibTorch publicly |
| `HydroCoupleSDKTests` | executable | `BUILD_TESTS` | The unit tests; each GoogleTest case is a ctest test |
| `HydroCoupleSDKMpiTests` | executable | `BUILD_TESTS` and `USE_MPI` | Run under `mpiexec` as the ctest test `MpiTransportSuite` |
| `HydroCoupleSDKTorchTests` | executable | `BUILD_TESTS` and Torch | The Torch layer's tests |
| `serial_coupling`, `analysis_reopen` | executables | `BUILD_EXAMPLES` | @ref sdk_guide_example; ctest tests `examples.serial_coupling`, `examples.analysis_reopen` and, with yaml-cpp, `examples.serial_coupling_yaml` |
| `distributed_coupling` | executable | `BUILD_EXAMPLES` and `USE_MPI` | @ref sdk_guide_distributed |
| `docs_example_<directory>` | executables | `BUILD_EXAMPLES` | One per directory of `docs/examples/`; ctest test `docs.example.<directory>` |
| `docs` | custom | `BUILD_DOCS` or `HYDROCOUPLE_DOCS_ONLY` | Runs Doxygen and writes the `guide/` bookmark pages |

The tests of the examples and the guide's examples carry the ctest label `docs-example`
(`ctest -L docs-example` runs them all) and work in `build/docs-examples/<name>/`, which keeps their
output for review.

### The guide's compiled examples

`docs/examples/CMakeLists.txt` turns each directory with a `main.cpp` into a program and a test:

- every `*.cpp` in the directory is compiled into `docs_example_<directory>`, linked to
  `HydroCoupleSDK`;
- an optional file `REQUIRES` lists CMake options, one per line; if any is off, the example is
  skipped and the configure step says so;
- an optional file `LIBRARIES` lists extra link targets, one per line (for example
  `HydroCoupleTools`);
- the compile definition `DOCS_EXAMPLE_SOURCE_DIR` holds the directory's path, for reading inputs;
- the test runs the program in `build/docs-examples/<directory>/` with a 300-second timeout.

## Package variables

`find_package(HydroCoupleSDK)` (version compatibility: same major version) sets:

| Variable | Meaning |
| --- | --- |
| `HYDROCOUPLESDK_FOUND` | 1 |
| `HYDROCOUPLESDK_INCLUDE_DIR`, `HYDROCOUPLESDK_INCLUDE_DIRS` | The installed include directory |
| `HYDROCOUPLESDK_LIBRARY_DIR` | The installed library directory |
| `HYDROCOUPLESDK_LIBRARIES` | `HydroCoupleSDK::HydroCoupleSDK`, plus `HydroCoupleSDK::HydroCoupleTools` when present |
| `HYDROCOUPLESDK_WITH_MPI`, `_WITH_KOKKOS`, `_WITH_GDAL`, `_WITH_HDF5`, `_WITH_GEOPACKAGE`, `_WITH_NETCDF`, `_WITH_TOOLS` | The values of the corresponding options when the SDK was built |
| `HYDROCOUPLESDK_WITH_YAML_CPP` | Whether the SDK found yaml-cpp |
| `HYDROCOUPLESDK_WITH_INTERFACE_PACKAGE` | Whether the SDK was built against the interface package (then the package finds `HydroCouple` for you) |
| `HYDROCOUPLESDK_WITH_NLOHMANN_PACKAGE` | Whether the SDK was built against an nlohmann_json package (then the package finds it for you) |

The package resolves the dependencies its targets need: `HydroCouple` (when built against that
package), `nlohmann_json` (when built against a package), `MPI` and `GDAL` (when enabled). It does
not look for HDF5, NetCDF, SQLite, yaml-cpp, Kokkos or OpenMP, which are private to the shared
library.

## Compile definitions

| Definition | Scope | Set when |
| --- | --- | --- |
| `HYDROCOUPLESDK_LIBRARY` | private | always (on Windows, makes `HYDROCOUPLESDK_EXPORT` in `export.h` export instead of import) |
| `USE_NETCDF`, `HYDROCOUPLESDK_HAVE_NETCDF` | public | `USE_NETCDF` |
| `USE_HDF5`, `HYDROCOUPLESDK_HAVE_HDF5` | public | `USE_HDF5` |
| `HYDROCOUPLESDK_HAVE_GEOPACKAGE` | public | `USE_GEOPACKAGE` |
| `USE_GDAL` | public | `USE_GDAL` |
| `USE_MPI` | public | `USE_MPI` |
| `USE_KOKKOS` | public | `USE_KOKKOS` |
| `HYDROCOUPLESDK_HAVE_YAML_CPP` | private | yaml-cpp found |
| `HYDROCOUPLETOOLS_LIBRARY` | private to `HydroCoupleTools` | `BUILD_TOOLS` |
| `HYDROCOUPLESDKTORCH_LIBRARY` | private to `HydroCoupleSDKTorch` | Torch |

## Generated version header

The configure step writes `hydrocouplesdk/version.h` into the build tree
(`build/generated/hydrocouplesdk/version.h`), never into the source tree, and installs it with the
headers. It defines:

| Macro | Value |
| --- | --- |
| `HYDROCOUPLESDK_NAME` | `"HydroCoupleSDK"` |
| `HYDROCOUPLESDK_VERSION_MAJOR`, `_MINOR`, `_PATCH` | 2, 0, 0 |
| `HYDROCOUPLESDK_VERSION` | `"2.0.0-beta.1"` |
| `HYDROCOUPLESDK_BUILD_ID` | A fixed UUID for the SDK |
| `HYDROCOUPLESDK_BUILD_COMMIT`, `HYDROCOUPLESDK_BUILD_BRANCH` | The git commit and branch at configure time (`unknown` outside a git checkout) |
| `HYDROCOUPLESDK_BUILD_DATE` | The configure time, UTC |

## Compilation settings

- C++20, without compiler extensions.
- Symbols are hidden by default where the compiler supports it (`-fvisibility=hidden`,
  `-fvisibility-inlines-hidden`); the SDK's export macros make its public classes visible.
- Release builds of the library add `-O3` (`/O2` with MSVC) and, where supported,
  `-ftree-vectorize`.
- The library uses a precompiled header (`src/stdafx.h`) for its C++ sources.
- With GCC older than 9, `stdc++fs` is linked for `std::filesystem`.

## Packaging

`cpack` builds a ZIP and a NuGet package on Windows and a gzipped tarball and a NuGet package
elsewhere, for `HydroCoupleSDK` version 2.0.0-beta.1. The NuGet package id is `HydroCoupleSDK` and
its name is `HydroCoupleSDK.<architecture>.<system>`, for example `HydroCoupleSDK.arm64.darwin`.
