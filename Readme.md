# HydroCoupleSDK

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](License)

HydroCoupleSDK is a software development kit that simplifies model component development for the [HydroCouple](https://github.com/HydroCouple/HydroCouple) component-based modeling interface definitions (v2.0.0-alpha.1). It provides concrete base implementations of the core interfaces — the typed hyperslab data plane, component lifecycle machinery, JSON argument IO, and temporal and spatial data structures — so component authors implement only their model-specific logic.

The SDK is dependency-lean C++20 with no Qt dependence. Implementation classes live in `namespace HydroCouple::SDK`; headers are organized by domain under `include/hydrocouplesdk/` (`core/`, `component/`, `data/`, `temporal/`, `spatial/`, `spatiotemporal/`, `io/`, `distributed/`, `device/`, `tools/`). Dependencies are managed with vcpkg and the build with CMake, following the same conventions as [openswmm.engine](https://github.com/HydroCouple/openswmm.engine).

## Highlights

The v2 data plane replaces the former variant-based accessors: every data item advertises a `shape()` and `DataKind` and moves values through `getValuesInto()`/`setValuesFrom()` hyperslab copies described by a `BufferDescriptor` (DLPack/NumPy buffer-protocol style, strided destinations supported, `std::copy` fast path on contiguous runs). Concrete typed items are provided as class templates — `Argument1D<T>`, `Argument2D<T>`, `IdBasedArgument<T>`, `Input1D<T>`/`Output1D<T>`, `Input2D<T>`/`Output2D<T>`, and `Temporal::TimeSeriesInput<T>`/`TimeSeriesOutput<T>` — with `Int`/`Double`/`String` aliases exported from the library. Arguments serialize to JSON (string, file, or URL sources) and support the v2 `serialize()` write-side contract. `AbstractModelComponent` implements the v2 lifecycle including the `errors()` diagnostic queue and `capabilities()` advertisement.

Beyond the data plane, the SDK ships the layers a coupled simulation needs:

- **IO** (`HydroCouple::SDK::IO`) — a single writer thread behind a bounded queue carries owned snapshots to pluggable format writers (CSV, HDF5/UGRID, NetCDF/UGRID, OGC GeoPackage), plus `ModelInitializer` for YAML/JSON composition documents with symmetric serialization and sidecar binary references for large payloads.
- **Workflows** (`HydroCouple::SDK`) — `PullDrivenWorkflow` expresses OpenMI's request-reply chain as a declared strategy with an explicit trigger; `TimeSteppedWorkflow` derives its execution schedule from the exchange-connection graph, collapsing feedback loops into diagnosed iteration groups. Both support cooperative pause/stop at synchronization points.
- **Distributed execution** (`HydroCouple::SDK::Distributed`) — a transport-neutral protocol (`InProcTransport` for tests, `MpiTransport` for clusters), remote component proxies, and `PartitionedDataItem<T>` with epoch-stamped halo exchange that hides communication behind computation.
- **Device memory** (`HydroCouple::SDK::Device`) — a vendor-neutral backend seam (`HostBackend`, `KokkosBackend`) with a buffer registry and copy-path negotiation decided once at `prepare()`.
- **Meshing tools** (`HydroCouple::SDK::Tools`, `BUILD_TOOLS=ON`) — constrained Delaunay triangulation, quad-dominant meshing, boundary-fitted curvilinear grids, terrain-following sigma grids, and IDW terrain sampling, all emitting UGRID-congruent meshes the IO writers consume directly.

## Compositions and stored runs

Compositions are JSON or YAML documents governed by **Composition
Specification v1** and validated two ways: against the JSON Schemas
installed at `share/hydrocouplesdk/schema/` (editors, CI, external tools)
and by `ModelInitializer`, which additionally checks the document against
the live components before applying anything. A document lists components
with their argument payloads, the connections between them (including
adapted-output chains and multi-input roles), the workflow strategy, the
output writers, and where to write the run manifest.

A finished run is described by that **run manifest** — `RunRecorder`
writes it at the end of a run — which records what ran, how it ended, and
a catalog mapping every recorded data item to the artifact holding its
values. That catalog is what makes analysis model-agnostic:

```cpp
// No model components, no model libraries — just the manifest.
auto results = ResultsModelComponent::open("out/run.json", "channel", message);
results->initialize();                       // → Finished: values already exist
for (IComponentDataItem *item : results->results())
    item->getValuesInto(destination, start, count, &message);
```

Reuse is always declared, never inferred from files on disk: a component
block carries `execution: { mode: run | open | resume }`, and `open`
substitutes a results-backed component automatically. See the
`analysis_reopen` example.

## Examples

Two runnable examples live under `examples/` and build with `-DBUILD_EXAMPLES=ON`. Each also has a standalone `CMakeLists.txt` that consumes only the **installed** package — the acceptance test for the install export:

```bash
# In-tree
cmake -S . -B build -DBUILD_EXAMPLES=ON -DUSE_MPI=ON
cmake --build build --parallel
./build/examples/serial_coupling examples/serial_coupling/composition.yaml out/
./build/examples/analysis_reopen out/run.json
mpirun -n 4 ./build/examples/distributed_coupling

# Against an installed SDK
cmake --install build --prefix /opt/hydrocouplesdk
cmake -S examples/serial_coupling -B build-standalone -DCMAKE_PREFIX_PATH=/opt/hydrocouplesdk
cmake --build build-standalone
```

`serial_coupling` initializes a two-component composition from YAML (or the identical JSON), orchestrates it with a workflow, persists results through the IO thread, and records a run manifest. `distributed_coupling` decomposes the same domain across MPI ranks with halo exchange. `analysis_reopen` reads that manifest back and summarizes the run with **no model components linked at all** — the analysis and visualization case.

## Building

Requires CMake ≥ 3.18, a C++20 compiler, and [vcpkg](https://github.com/microsoft/vcpkg) (`VCPKG_ROOT` set). The HydroCouple interface headers are located via `HYDROCOUPLE_INCLUDE_DIR` (defaults to a sibling `../HydroCouple/include` checkout).

```bash
cmake --preset macos          # or linux / windows
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Core builds need only nlohmann-json (supplied by the vcpkg manifest, with a header-download fallback). Optional features are gated both in CMake and as vcpkg manifest features: `-DUSE_GDAL=ON` (feature `gdal`, raster data items), `-DUSE_NETCDF=ON` (feature `netcdf`), `-DUSE_HDF5=ON` (feature `hdf5`), `-DUSE_GEOPACKAGE=ON` (feature `geopackage`), `-DUSE_MPI=ON` (feature `mpi`), `-DUSE_KOKKOS=ON` (feature `gpu`), `-DBUILD_TOOLS=ON` (feature `tools`, meshing library), `-DBUILD_EXAMPLES=ON`, and tests via `-DBUILD_TESTS=ON` (feature `tests`, Google Test).

Consuming an installed SDK is a normal `find_package`:

```cmake
find_package(HydroCoupleSDK REQUIRED)
target_link_libraries(my_component PRIVATE HydroCoupleSDK::HydroCoupleSDK)
```

The package config records how the SDK was built (`HYDROCOUPLESDK_WITH_MPI`, `..._WITH_KOKKOS`, `..._WITH_TOOLS`, …) and resolves the matching dependencies, so an MPI-enabled SDK brings MPI's usage requirements with it.

## Documentation

| Site | URL |
| --- | --- |
| **SDK API (this repository)** | <https://www.hydrocouple.org/HydroCoupleSDK/> |
| HydroCouple interface standard | <https://www.hydrocouple.org/HydroCouple/> |
| HydroCouple Python bindings | <https://www.hydrocouple.org/HydroCouple/python/> |

API documentation is generated with Doxygen (`-DBUILD_DOCS=ON`, target `docs`) and published to GitHub Pages by `.github/workflows/build_docs.yml`, which deploys from `master` and `dev` only. The generated pages carry a navbar link back to the interface standard, and the interface docs link here in return.

## Changelog

Release history and known limitations are in [CHANGELOG.md](CHANGELOG.md).

## References
-------------------------------------------------------------------------------------------------------------------
* Buahin, C. and J. Horsburgh, 2016. From OpenMI to HydroCouple: Advancing OpenMI to Support Experimental Simulations and Standard Geospatial Datasets. International Congress on Environmental Modelling and Software. Toulouse, France. http://scholarsarchive.byu.edu/iemssconference/2016/Stream-A/11
* Buahin, C.A. and J.S. Horsburgh, 2018. Advancing the Open Modeling Interface (OpenMI) for Integrated Water Resources Modeling. Environmental Modelling & Software, 108: 133–153. https://doi.org/10.1016/j.envsoft.2018.07.015
