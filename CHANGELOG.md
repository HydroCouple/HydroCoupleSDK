# Changelog

All notable changes to HydroCoupleSDK are recorded here. Entries list the
critical commits for each release, per project convention.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
and stay in lockstep with the HydroCouple interface definitions — the SDK's
MAJOR.MINOR must match the interface headers it is built against
(enforced at configure time).

## [2.0.0-alpha.1] — 2026-08-23

First release of the modernized SDK: a complete rewrite of the data plane,
build system, and execution layers against HydroCouple v2.0.0-alpha.1.

### Added

- **Typed hyperslab data plane** (`94d690c`) — `shape()` / `dataKind()` /
  `getValuesInto()` / `setValuesFrom()` over `BufferDescriptor`, replacing
  the variant convention entirely. Storage mixins (`ComponentDataItem1D/2D`,
  id-based, time-series) supply the slab engine to every item type.
- **IO abstraction** (`b2b3cda`, `af9225d`, `43e9f66`, `e56d271`,
  `a85da70`) — `IOutputWriter` + `IOThread` (bounded queue, back-pressure,
  first-error latch, drain-then-join) with owned `Snapshot` value
  semantics; CSV, HDF5/UGRID and NetCDF/UGRID (CF-1.11 UGRID-1.0,
  chunk+deflate, extend-and-write), and OGC GeoPackage writers;
  `ModelInitializer` for YAML/JSON composition documents with symmetric
  serialization; sidecar `@ref` binary references keeping large payloads
  out of the document DOM (10⁷ elements / 153 MB in ~3.3 s).
- **Distributed execution** (`625bc88`, `b79f091`, `4415152`, `31068da`) —
  32-byte protocol `Envelope`, `ValuePacker` typed wire blocks,
  `InProcTransport` and `MpiTransport` behind the standard's `ITransport`
  seam; `ComponentWorker` + `ProxyModelComponent` (heartbeats, request
  timeouts, peer-death diagnostics); `HaloExchanger` +
  `PartitionedDataItem<T>` implementing the local virtual node
  representation with epoch-stamped begin/overlap/end exchanges;
  `FluxDerivativePayload` for first-order implicit boundary coupling.
- **Workflow orchestration** (`ac0b5a4`) — `AbstractWorkflowComponent`
  with `PullDrivenWorkflow` (the OpenMI request-reply default as a
  declared strategy with an explicit trigger) and `TimeSteppedWorkflow`
  (execution schedule derived from the connection graph; Tarjan SCCs
  collapse feedback loops into diagnosed iteration groups).
- **GPU abstraction** (`2ee1efb`) — vendor-neutral `DeviceBackend` seam
  with `HostBackend` and `KokkosBackend`, `DeviceBufferRegistry`, and
  space-aware transfer negotiation deciding the cheapest legal copy path
  once at `prepare()`.
- **Spatiotemporal and id-based data items** (`c79bc88`) — time-geometry,
  time-network, time-polyhedral-surface/TIN, and time-grid items
  (rank-3/rank-4 hyperslabs), plus id-based inputs/outputs and the
  time-and-identifier item.
- **Meshing tools** (`c4f4940`) — `HydroCoupleTools`: terrain-following
  sigma grids, boundary-fitted curvilinear grids (transfinite + Winslow,
  quality metrics), constrained Delaunay triangulation over CDT,
  quad-dominant meshing, and IDW terrain sampling.
- **Examples** — a YAML/JSON-initialized two-component serial coupling
  writing CSV/GeoPackage/HDF5 through the IO thread, and the same domain
  decomposed under `mpirun` with halo exchange. Both also build
  standalone against an *installed* SDK.
- **Build and packaging** — CMake + vcpkg manifest with per-feature
  options (`USE_HDF5`, `USE_GEOPACKAGE`, `USE_NETCDF`, `USE_MPI`,
  `USE_KOKKOS`, `USE_GDAL`, `BUILD_TOOLS`, `BUILD_EXAMPLES`), configure-time
  version alignment with the interface headers, Doxygen documentation, and
  a GoogleTest suite (157 tests at release, plus an MPI-launched suite).

### Changed

- **Qt removed entirely** (`28996e3`) — the SDK depends only on the C++20
  standard library plus opt-in native libraries. Signals/slots, containers,
  and file IO are standard-library based.
- **Reorganized by domain** (`94d690c`) — headers live under
  `include/hydrocouplesdk/{core,component,data,temporal,spatial,
  spatiotemporal,io,distributed,device,tools}/` in namespace
  `HydroCouple::SDK` (with `::Temporal`, `::Spatial`, `::SpatioTemporal`,
  `::IO`, `::Distributed`, `::Device`, `::Tools`).
- **Licensed MIT**, with authorship normalized across the tree.
- `threadsafenetcdf` retired — its ~100 `#pragma omp critical` wrappers are
  subsumed by the single-writer IO thread.

### Fixed

- `TimeSeries::loadFromCSV` parsed whitespace instead of commas.
- `julianDayToGregorian` dropped a term in the Richards conversion,
  skewing dates.
- Install export defects found by the standalone example builds: the CMake
  package is installed to `lib/cmake/HydroCoupleSDK/` (was `lib/cmake/`,
  where `find_package` could not see it); `HydroCoupleSDKConfig.cmake`
  now resolves its own path variables and dependencies
  (`find_dependency(MPI/GDAL/HydroCouple/nlohmann_json)`); MPI is a PUBLIC
  usage requirement because the installed `abstractmodelcomponent.h`
  includes `<mpi.h>`; and the interface headers and the nlohmann single
  header are installed alongside the SDK when no package supplies them, so
  an installed SDK is self-contained.

### Known limitations

- Interface-conformant adapters for the SDK's spatial value classes
  (`TIN`/`Network`/`RegularGrid` → `ITIN`/`INetwork`/`IRegularGrid`) are
  not yet implemented; spatiotemporal items take borrowed interface
  pointers supplied by the owning component.
- GDAL-gated raster items (including the time-raster item) remain
  unported.
- `nlohmann::json` appears in a public header
  (`abstractcomponentdataitem.h`); hiding it behind a pimpl is deferred to
  avoid an ABI-visible change during alpha.
- Device fast paths for `ValuePacker` and a direct GPU-aware-MPI path are
  deferred; device-resident fields exchange through staged copies today.

[2.0.0-alpha.1]: https://github.com/cbuahin/HydroCoupleSDK/releases/tag/v2.0.0-alpha.1
