@page sdk_guide_overview 1. What the SDK is

[TOC]

The HydroCouple SDK (HydroCoupleSDK) is a C++20 library that implements the
[HydroCouple interfaces](https://www.hydrocouple.org/HydroCouple/), the standard that lets
independently written models exchange data while they run. The interfaces are pure abstract
classes: they say what a component, an argument, an input, an output, a time series or a mesh must
provide, but contain no code. The SDK supplies working implementations of all of them, so a
component author writes only what is specific to their model, and a host writer gets the
scheduling, composition, recording and reopening machinery ready-made.

This chapter describes what the SDK contains, how it relates to the interfaces and to the
programs built on it, and how the rest of this guide is organized.

## Interfaces, SDK, components and hosts

<pre class="mermaid">
flowchart TB
    iface["HydroCouple interfaces<br/>(header-only: hydrocouple.h and companions)"]
    sdk["HydroCouple SDK<br/>HydroCoupleSDK, HydroCoupleTools, HydroCoupleSDKTorch"]
    comps["Components<br/>FVQual, HydroCoupleComponents, your model"]
    hosts["Hosts<br/>HydroCouple Composer, the fvqual runner, your program"]
    engine["Components written directly<br/>against the interfaces<br/>(the OpenSWMM Engine)"]
    iface --> sdk
    sdk --> comps
    sdk --> hosts
    iface --> engine
    comps -. "loaded and run by" .-> hosts
    engine -. "loaded and run by" .-> hosts
</pre>

- **The interfaces** (the HydroCouple repository, version 2.0) define the contracts and a few
  header-only helpers (`hydrocouplehelpers.h`, `hydrocouplecomponentabi.h`). Every component and
  every host includes them.
- **The SDK** implements the contracts. It is optional: a component may implement the interfaces
  itself, as the OpenSWMM Engine does, and still be loaded and coupled by any host. Most
  components use the SDK because the interfaces are large and the SDK's implementations already
  follow every rule of the standard (status transitions, the error queue, value-access semantics,
  JSON argument formats).
- **Components** wrap models. FVQual and the HydroCoupleComponents library are built on the SDK.
- **Hosts** load components, connect them and run them. HydroCouple Composer is the desktop host;
  the SDK's `ModelInitializer` and workflows make a command-line host a few dozen lines, as the
  `fvqual` runner and @ref sdk_guide_example show.

## What the SDK contains

The SDK builds up to three libraries:

| Library | Built when | Contents |
| --- | --- | --- |
| `HydroCoupleSDK` | always | Everything below except the meshing tools and the Torch layer |
| `HydroCoupleTools` | `BUILD_TOOLS=ON` | Mesh generation: constrained Delaunay triangulation, quad-dominant meshing, boundary-fitted curvilinear grids, terrain-following sigma grids, terrain sampling (@ref sdk_guide_tools) |
| `HydroCoupleSDKTorch` | `HYDROCOUPLESDK_WITH_TORCH=ON` | Differentiation through LibTorch (@ref sdk_guide_differentiation); build tree only, not installed |

The public headers live under `include/hydrocouplesdk/`, one directory per area. Each area has
its own chapter:

| Directory | Namespace | What it provides | Chapter |
| --- | --- | --- | --- |
| `core/` | `HydroCouple::SDK` | Identities and descriptions, units, unit dimensions, quantities and quality value definitions, dimensions, signals and slots, diagnostic codes | @ref sdk_guide_quantities |
| `component/` | `HydroCouple::SDK` | `AbstractModelComponent` and component info, the time-stepped and pull-driven workflows, connection validation, the results-backed component | @ref sdk_guide_component, @ref sdk_guide_workflow |
| `data/` | `HydroCouple::SDK` | Arguments, inputs and outputs in one and two dimensions and by id; multi-inputs; adapted outputs and their factories; the typed storage behind every data item | @ref sdk_guide_ports, @ref sdk_guide_adapted_outputs |
| `temporal/` | `HydroCouple::SDK::Temporal` | Time stamps and spans, time series, time-series arguments, inputs and outputs, temporal interpolation | @ref sdk_guide_time |
| `spatial/` | `HydroCouple::SDK::Spatial` | Points, line strings, polygons, polyhedral surfaces, TINs, envelopes, networks, rasters, regular grids, layered meshes, and adapters that expose existing data as these types | @ref sdk_guide_space |
| `spatiotemporal/` | `HydroCouple::SDK::SpatioTemporal` | Data items that vary in both space and time | @ref sdk_guide_space |
| `io/` | `HydroCouple::SDK::IO` | Composition documents and `ModelInitializer`, URI resolution, the IO thread and the CSV, GeoPackage, HDF5 and NetCDF writers, run manifests and result readers, mesh packages | @ref sdk_guide_compositions, @ref sdk_guide_record, @ref sdk_guide_mesh_packages |
| `distributed/` | `HydroCouple::SDK::Distributed` | Running components in other processes: transports (in-process and MPI), proxies, workers, partitioned items and halo exchange | @ref sdk_guide_distributed |
| `device/` | `HydroCouple::SDK::Device` | Device memory backends (host, Kokkos) and the device buffer registry | @ref sdk_guide_devices |
| `differentiation/` | `HydroCouple::SDK::Differentiation` | Gradients and seeds, differentiable workflows, the Torch layer | @ref sdk_guide_differentiation |
| `tools/` | `HydroCouple::SDK::Tools` | The `HydroCoupleTools` meshing library | @ref sdk_guide_tools |

`componentabi.h` includes the interfaces' component-loading convention
(`hydrocouplecomponentabi.h`), and `export.h` defines the SDK's symbol-export macros.

## How the SDK implements the standard

A few design decisions run through the whole SDK; knowing them makes the rest of this guide easier
to follow.

- **One data plane.** Every data item (argument, input, output, recorded result) reports its
  `shape()`, one extent per dimension, and its `dataKind()`, the element type. Values move only
  through `getValuesInto()` and `setValuesFrom()`, which copy a box of the item's index space (a
  hyperslab) to or from a buffer the caller describes with a `BufferDescriptor`. There is no
  variant type and no per-value virtual call; a contiguous selection is copied in one pass
  (@ref sdk_guide_ports).
- **Typed items as templates.** `Argument1D<T>`, `Input1D<T>`, `Output1D<T>`, their 2-D and
  id-based forms and the time-series items are class templates; `int`, `double` and `std::string`
  versions are compiled into the library and named by aliases such as `Argument1DDouble`.
- **A checked lifecycle.** `AbstractModelComponent` runs initialization in a fixed order, records
  every status change, checks it against the standard's transition table and announces it
  (@ref sdk_guide_component).
- **The standard's error channel.** Interface methods report failure by returning `false` (or a
  null pointer) with a message, and by queuing an entry on the component's error queue, which a host
  drains with `errors()`; a component that fails sets its status to `Failed` with the reason.
- **Declared, not inferred.** A composition document states every argument, connection, adapted
  output, writer and execution mode; reusing a finished run is a declared execution mode, never a
  guess from files on disk (@ref sdk_guide_compositions).
- **Recorded runs are self-describing.** A run manifest catalogs every recorded data item and the
  artifact that holds it, so a program can reopen a run with no model code
  (@ref sdk_guide_record).
- **No Qt, few dependencies.** The core library needs only the standard library, the interface
  headers and nlohmann/json (and yaml-cpp for YAML compositions, if present). Every optional
  backend (GDAL, HDF5, NetCDF, SQLite, MPI, Kokkos, OpenMP, LibTorch) is a build option
  (@ref sdk_guide_build).
- **No component loader.** The SDK does not load shared libraries. Hosts load component libraries
  themselves using the interfaces' loading convention (@ref sdk_guide_first_component, step 7) and
  hand `ModelInitializer` a function that returns component instances.

## Versions

This guide describes HydroCoupleSDK 2.0.0-beta.1, which implements HydroCouple interfaces 2.0.
The SDK's major and minor version always match the interfaces it is built against: configuring the
SDK against interface headers of another major or minor version stops with an error (if CMake
cannot determine the headers' version, it warns and skips the check). Component
libraries carry an ABI stamp (@ref sdk_guide_first_component) so that a host can refuse a library
built against another interface ABI or with an incompatible toolchain before calling into it.

The CMake package checks major versions only: `find_package(HydroCoupleSDK 2.0)` accepts any 2.x
SDK.

## How this guide is organized

- **Getting started.** @ref sdk_guide_build installs and links the SDK.
  @ref sdk_guide_first_component writes a component; @ref sdk_guide_example couples two and
  records the run.
- **Concepts.** The component lifecycle, quantities and units, data items, adapted outputs, time
  and space: what a component author needs to model their data correctly.
- **Running models.** Workflows, composition documents, recording and reopening results, mesh
  packages: what a host writer needs.
- **Advanced topics.** Distributed coupling, device memory, differentiation and the meshing tools.
- **Reference.** Every field of composition documents and run manifests, every CMake option and
  target, and the error messages with their causes.

Readers who only want to wrap a model can read chapters 2 to 7 and then the chapters for the data
they exchange (time, space). Readers writing a host should add chapters 11 to 13. The
[API reference](../../annotated.html) documents every class and function.
