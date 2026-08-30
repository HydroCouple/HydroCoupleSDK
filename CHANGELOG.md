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

### Fixed

- **The test suite no longer carries a second copy of netCDF and HDF5.** Four
  `NetCDFUGRIDWriter` tests had been failing for some time: `nc_open` returned
  `NC_EHDFERR` on a file the writer had just produced and the SDK's own reader
  opened without complaint. The file was never the problem. vcpkg supplies
  netCDF and HDF5 as static archives, and the test executable linked them as
  well as the copies already inside `libHydroCoupleSDK`, so one process held
  two sets of HDF5's globals and a file written through one could not be
  reopened through the other. The macOS linker had been saying so all along —
  "ignoring duplicate libraries: libhdf5.a".

  The test target now takes those headers **without** their libraries and
  resolves `nc_*` and `H5*` from the shared library, which is one instance and
  the same one that wrote the file. The tests still check written files with
  the library rather than with our own reader, which is the point of them.
  The suite is green at 260/260 for the first time; the four failures were
  never a defect in the writer and are no longer a baseline to be explained
  away.
- **The suite can be run in parallel again.** `ctest -jN` had been reporting a
  different failing set on every run — seven, then nine, with different
  members — so its verdict was only meaningful at `-j 1`. ctest registers each
  gtest case as a test of its own and runs them as concurrent processes, and
  several cases asked for the same artifact file name: `io_ugrid.nc`,
  `io_ugrid.h5`, `io_results.gpkg`, and a whole `sidecars/` directory that
  every `SidecarTest` wiped in `SetUp()`. Each case writes its file and then
  reads it back, so in parallel they truncated and deleted each other's work
  mid-read.

  Artifact paths are now namespaced by the running case, and the sidecar
  fixture gets a directory of its own. Files still land under
  `tests/artifacts/` where they can be opened afterwards, and now say which
  case produced them. Five consecutive `-j 8` runs: 260/260.
- **`NetCDFUGRIDWriter` now names every node coordinate variable it writes.**
  `node_coordinates` listed only `_node_x` and `_node_y`, so an elevation
  written as `_node_z` was present in the file but undiscoverable: a
  conventions-driven reader looks for coordinates through that attribute and
  would never find it. Elevations round-trip now.

### Added

- **A meshing run can be watched, and stopped.** `Tools::MeshProgress` is an
  optional callback taken by new overloads of `Triangulator::triangulate()`
  and `QuadMesher::triToQuadDominant()`: it is told the phase under way and
  answers whether to carry on. Returning false abandons the run, which then
  reports itself cancelled and **leaves its output empty** — a cancelled run
  hands back nothing rather than half a mesh, including when the caller
  passed in a variable that already held one. Overloads rather than default
  arguments, so every existing exported symbol is untouched.

  What the callback promises is deliberately modest, and the header says so:
  the fraction is how far through the *phases* a run is and not a measure of
  work remaining, and **the CDT insertion phases cannot be interrupted** —
  they are one call into a library that offers no hook, so a cancellation
  asked for during one takes effect when it returns. Cancellation is offered
  where this library's own loops run: densification and extraction in the
  triangulator, and every phase of the quad merge, which is entirely ours
  and so stops promptly. A caller must not offer a cancel that lands sooner
  than it does.
- **CF vertical coordinates, read and written.** `readVerticalCoordinate()`
  joins the UGRID reader: a layered file is a 2-D topology with a sigma
  coordinate over it, and this returns the interfaces, the per-face bed depth
  and the water surface at a chosen time. Found by convention rather than by
  any writer's names — the coordinate is the variable whose `standard_name`
  is an `ocean_sigma_coordinate`, and its `formula_terms` names what it is
  built from. Interfaces come from a CF `bounds` attribute when present;
  failing that from a variable one longer than the layers whose values run 0
  to -1, which is what an interface sigma *is*; failing that they are
  midpointed from the centres and **flagged as derived**, since midpointing
  is exact only for a uniform distribution.
  `NetCDFUGRIDWriter::setVerticalCoordinate()` is the other half — without it
  the reader could read files the SDK itself could not produce, the same
  asymmetry the `node_coordinates` fix closed.
- **Spatial interface adapters** (`spatial/geometryadapters.h`,
  `spatial/meshadapters.h`) — the SDK's spatial value types served through the
  standard's `IGeometry` family. Before these, `EnvelopeAdapter` was the only
  implementation of any spatial interface in the SDK, so a component wanting
  to publish a geometry data item had to implement `IGeometry` itself —
  **thirty-two methods** — plus `IPoint`/`IPolygon`/`INetwork` as applicable,
  and each would have done it differently. `PointAdapter`,
  `LineStringAdapter`, `PolygonAdapter`, `VertexAdapter`, `EdgeAdapter`,
  `NetworkAdapter` and `PolyhedralSurfaceAdapter` follow the
  `EnvelopeAdapter` precedent: the value types stay values, and the adapter
  owns a copy and serves the interface over it. `MeshViewAdapter` exposes a
  `MeshDefinition` as an `IMeshView` — the two were written to the same
  vocabulary, so it hands out spans rather than copying.
  The relational predicates and constructive operations (`intersects`,
  `buffer`, `convexHull`, …) need a geometry engine the SDK does not link and
  report `false`/`nullptr` rather than guessing; the WKB the adapters do
  produce is what a caller with GEOS or GDAL uses instead.
- **UGRID mesh reader** (`io/ugridreader.h`) — the counterpart the writers
  lacked. The SDK could produce UGRID files that nothing in the ecosystem
  could read back, so every consumer had to write its own NetCDF walker.
  Driven by the conventions rather than by our own variable names: the
  topology variable is found by its `cf_role` and the coordinate and
  connectivity variables by the attributes it names, so a mesh written by
  another tool reads as readily as one of ours. Honours `start_index` (a
  1-based file is converted once, here) and drops `_FillValue` padding from
  rectangular connectivity. `ugridReadSupported()` answers at runtime, so a
  consumer linking a prebuilt SDK need not know how it was configured.
- **Microsoft MPI on Windows** — the MPI job is a matrix over
  ubuntu-latest and windows-latest. Launcher differences stay out of the
  build files: rank count comes from `HYDROCOUPLESDK_MPI_TEST_RANKS` and
  launcher flags from FindMPI's `MPIEXEC_PREFLAGS`, so OpenMPI gets
  `--oversubscribe` while MS-MPI — which rejects unknown flags and
  oversubscribes by default — gets none.
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

- **macOS carried two HDF5 instances in one process**, so the four
  `NetCDFUGRIDWriterTest` cases failed with `NC_EHDFERR` on files the SDK
  had just written — while the SDK's own reader opened those same files.
  The default `arm64-osx` triplet builds hdf5/netcdf as static archives,
  which `libHydroCoupleSDK.dylib` absorbs *and re-exports* (a prebuilt C
  archive is untouched by `-fvisibility=hidden`); the test executable
  links its own copies, and its netcdf then binds its `H5*` calls to the
  dylib's instance while the direct HDF5 tests use the executable's. macOS
  builds now use the `*-osx-dynamic` triplet, leaving one instance per
  process, as on Windows.
- **The Windows package-consumption check never found the package it had
  just installed.** `CMAKE_PREFIX_PATH` is `;`-separated, and MSYS reads
  that separator as proof the value is already in Windows form, so Git
  Bash's `/d/a/…` reached CMake verbatim (a lone path argument, such as
  the `cmake --install` prefix, *is* converted — which is why the install
  itself looked fine). The workflow now converts each entry with
  `cygpath -m`.
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
- **`MpiEnvironment` requested `MPI_THREAD_MULTIPLE` and discarded the
  answer.** `MPI_Init_thread` reports what it actually granted through its
  out-parameter; that value was never read, so the SDK proceeded as though
  full multithreading had been guaranteed no matter what the implementation
  said. Nothing in the distributed layer starts a thread and the IO thread
  never calls MPI, so the real requirement is `MPI_THREAD_FUNNELED`. The
  environment now requests `MPI_THREAD_SERIALIZED` (leaving room for a host
  to drive components from a worker thread), queries the level when MPI was
  initialized elsewhere, and exposes `providedThreadLevel()` and
  `threadLevelSufficient()` so a host can check instead of assume. Asking
  for `MULTIPLE` also excluded Microsoft MPI, which does not offer it.
- **`AbstractAdaptedOutputFactoryComponentInfo`'s constructor was declared
  but never defined.** Every sibling `*ComponentInfo` has a `.cpp` carrying
  its out-of-line constructor; this one's was never written, so the symbol
  was absent from the built library entirely (`nm` found zero occurrences).
  GCC and Clang emit only what is used and the class is abstract, so nothing
  ever referenced it — but `__declspec(dllexport)` requires a definition for
  every declared member, so MSVC failed to link the DLL with LNK2019. Same
  root cause as the move-only fix below: MSVC's dllexport instantiates the
  whole class, and two defects hid behind toolchains that do not.
- **Exported classes owning move-only members did not compile under MSVC.**
  `__declspec(dllexport)` instantiates *every* member of a class, including
  the implicit copy assignment; against a `std::vector<std::unique_ptr<…>>`
  member that is ill-formed. GCC and Clang instantiate it only if something
  calls it, so `ModelInitializer` compiled everywhere except MSVC. It is now
  explicitly move-only, and the seven other exported classes owning
  `unique_ptr`/`mutex`/`atomic` state (`InProcHub`, `InProcTransport`,
  `ProxyModelComponent`, `ResultsModelComponent`, `AbstractModelComponent`,
  `AbstractWorkflowComponent`, `AbstractAdaptedOutputFactory`,
  `TimeSliceOutput`) declare their copy operations deleted. All eight already
  declared destructors, so none were movable and none were copyable in
  practice — the declarations change no behaviour, they only stop MSVC
  generating a member that cannot exist.
- The standalone consume step could not resolve `nlohmann_json`. Unlike the
  IO backends, this one is a genuine usage requirement — `nlohmann/json.hpp`
  is included by the SDK's public headers — so the fix is to give the
  example build access to the vcpkg tree the SDK was built against, as a
  real consumer would have.
- **The installed package demanded dependencies consumers do not need.**
  `HydroCoupleSDKConfig.cmake` called `find_dependency()` for HDF5, NetCDF,
  SQLite, yaml-cpp and Kokkos, so an external project consuming an
  HDF5-enabled SDK failed at `find_package(HydroCoupleSDK)` unless it could
  also find HDF5 — a dependency it never asked for and that is entirely
  encapsulated in the shared library. The comment justifying those calls
  claimed CMake exports a target's PRIVATE dependencies as
  `$<LINK_ONLY:...>`; that is true of **static** libraries only. For the
  SHARED `HydroCoupleSDK` nothing is exported, so the calls were pure
  overhead. Removed; the `HYDROCOUPLESDK_WITH_*` variables still report how
  the SDK was configured, for consumers that want a backend on their own
  terms.
- **Windows CI compiled with MinGW instead of MSVC.** The job runs Ninja,
  which needs `cl.exe` on `PATH`; without the MSVC environment CMake picked
  up the runner's `g++`, which was then handed `/utf-8` by the MSVC-built
  vcpkg packages and reported it as a missing linker input. Beyond the flag,
  MinGW objects and MSVC-built `x64-windows` packages would not have linked.
  The job now sets up the MSVC environment before vcpkg, matching the other
  HydroCouple projects.
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
