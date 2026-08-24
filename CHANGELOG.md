# Changelog

All notable changes to HydroCoupleSDK are recorded here. Entries list the
critical commits for each release, per project convention.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
and stay in lockstep with the HydroCouple interface definitions — the SDK's
MAJOR.MINOR must match the interface headers it is built against
(enforced at configure time).

## [Unreleased] — 2.1.0

Composition IO moves into the SDK, and completed runs become reopenable.

### Added

- **Composition Specification v1** — the composition document becomes an
  SDK-owned, versioned specification replacing the Composer's XML:
  `schema_version`, `metadata`, per-component `info` (instantiation hint)
  and `execution` blocks, connections with ordered adapted-output chains
  and `IMultiInput` provider roles, `workflow` strategy configuration,
  `writers`, and the `run` manifest path. Formal JSON Schemas (draft
  2020-12) for compositions and run manifests ship in
  `share/hydrocouplesdk/schema/`, so editors, CI, and external tools
  validate without any C++; the runtime validator remains authoritative
  for anything requiring live components.
- **Run manifests** (`RunManifest`, `RunRecorder`) — a completed run is
  described explicitly: identity, timing, status, SDK/interface versions,
  composition reference (path, optional hash, optional embedded copy),
  per-component final status and diagnostics, and a results catalog
  mapping each recorded item to its artifact, format, variable, kind,
  shape, dimension names, units, mesh attachment, and time axis.
- **`IOutputWriter::catalog()`** — defaulted to empty (so third-party
  writers keep compiling) and implemented by all four shipped writers.
  Required because `FieldSlice` carries only id/kind/shape, while units,
  mesh, location, and the in-artifact variable name are writer-side.
- **`ResultsModelComponent`** — a previously executed model reopened from
  its manifest, presented as an ordinary `IModelComponent` whose recorded
  items serve values straight out of the artifacts. **No model binary is
  involved**, which is what makes analysis and visualization
  model-agnostic. It reaches `Finished` during `initialize()` because a
  recorded run has already computed its values; items appear in both
  `results()` and `outputs()`, so a stored run can also drive a new
  composition.
- **Result readers** (`IResultReader` + registry) — CSV always; HDF5,
  NetCDF, and GeoPackage behind the same gates as their writers. Reads are
  guided entirely by the catalog entry; nothing is inferred from the file.
- **`execution.mode`** — `run` (default, never silently reuses results),
  `open` (load recorded results and finish; the SDK substitutes a
  `ResultsModelComponent` when the host has no live component, which is
  the norm for an analysis tool), and `resume` (reserved, diagnosed).
  `compositionForRun()` turns a manifest back into an analysis
  composition.
- **`TimeSliceOutput`** — presents one time level of a time-major item as
  a lower-rank output, bridging recorded history (`{time, entities…}`) to
  a live consumer's present (`{entities…}`). The level is selected
  explicitly; the SDK has no global clock and does not guess.
- **`analysis_reopen` example** — reopens a completed run and summarizes
  it with no model components linked.

### Changed

- `ComponentStatus::Finished` documentation clarifies that components
  release *computational* resources; data items of results-backed
  components remain readable (interface repo, doc-only).

### Fixed

- The GeoPackage result reader queried a non-existent column; it now
  matches the writer's `variables.item_id` schema (found by running the
  analysis example end to end).
- The serial example attached an HDF5 writer without declaring its
  fields, so `results.h5` carried a mesh but no data.
- Every shipped result reader is now covered by a write → catalog → read
  → compare round trip. Previously only CSV and HDF5 were, which is how
  the GeoPackage column bug above escaped into a commit; the new tests
  fail if it is reintroduced.
- **The vcpkg manifest named features that do not exist**, which failed
  dependency resolution before a single file compiled — on every job in
  the matrix. `netcdf-c`'s HDF5 backing is the `netcdf-4` feature (there
  is no `hdf5` feature); `hdf5[threadsafe]` conflicts with the `hl` and
  `cpp` features the port needs and would have force-enabled
  `HDF5_ALLOW_UNSUPPORTED` (the SDK's concurrency guarantee is the
  single-writer IO thread, not a thread-safe HDF5); and the `tools`
  feature pulled GDAL though `HydroCoupleTools` links only CDT.
- The MPI test hardcoded three ranks, so it failed on any runner with
  fewer cores ("not enough slots"). Rank count is now
  `HYDROCOUPLESDK_MPI_TEST_RANKS`, and launcher flags come from FindMPI's
  `MPIEXEC_PREFLAGS` — OpenMPI needs `--oversubscribe` to pack ranks onto
  fewer cores and MPICH rejects that flag outright, so it belongs to the
  environment rather than the build file.
- The configure-time version-alignment check silently passed whenever the
  interface was consumed from a source checkout, because it only read a
  generated `version.h`; it now falls back to the interface's
  `project(HydroCouple VERSION …)` declaration.
- `version.h` was generated *into the source tree* and tracked, so every
  configure dirtied the working tree and commits carried whichever branch
  and commit happened to be checked out when someone last configured —
  wrong provenance for every other checkout. It is now generated into the
  build tree, installed from there, and ignored in git; only
  `version.h.in` is tracked.
- Windows test and example executables now have their dependent DLLs
  copied beside them (`$<TARGET_RUNTIME_DLLS:…>`), which is what the
  runner needs to launch them at all.
- The 10⁷-element sidecar benchmark asserted a 10 s wall clock, which
  measures the runner rather than the SDK and flaked under load. The
  structural assertion (a flat document under 4 KB) is what actually
  proves the bulk path was taken — a DOM-shaped serialization cannot pass
  it at any speed — so the timing bound is now a pathology guard only.
- CDT is licensed MPL-2.0 AND BSD-3-Clause; three places said MIT.
- **`HydroCoupleSDK` linked `HDF5::HDF5` by hardcoded name**, so CMake
  generation failed on every machine where HDF5 arrives in config mode —
  which is every CI runner, since vcpkg exports `hdf5::hdf5-static`. Only
  module-mode `FindHDF5` defines `HDF5::HDF5`, which is why local builds
  configured fine. The detection block already resolved the right target
  into `HYDROCOUPLESDK_HDF5_TARGET` and the library already linked it a few
  lines earlier; the hardcoded line was a second, redundant link. Removed,
  along with the `netcdf-cxx4` half of the same block — the SDK uses the
  NetCDF C API (`netcdf.h`) and never declared a C++ dependency. Both stay
  PRIVATE: no installed public header includes `hdf5.h` or `netcdf.h`.

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
  quality metrics), constrained Delaunay triangulation over CDT (MPL-2.0/BSD-3-Clause),
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
