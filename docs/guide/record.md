@page sdk_guide_record 13. Recording and reopening results

[TOC]

A coupled run produces values that someone wants to look at afterwards. The HydroCouple SDK
records them in two parts: *writers* store the arrays in files (the *artifacts*), and a *run
manifest* (`run.json`) says what ran, how it ended, and which artifact holds each recorded item,
with its shape, dimension names, units, mesh attachment and time axis. Because the manifest states
all of that explicitly, a run can be reopened by a program that has none of the model code that
produced it: @ref HydroCouple::SDK::ResultsModelComponent presents the recorded items as the
outputs of an ordinary component.

This chapter covers capturing values (@ref HydroCouple::SDK::IO::Snapshot), carrying them to the
writers off the simulation thread (@ref HydroCouple::SDK::IO::IOThread), the four writers, the
run manifest and @ref HydroCouple::SDK::IO::RunRecorder, reading values back, and moving a run.
@ref sdk_ref_run_manifest lists every field of `run.json`. Everything here is in the namespace
`HydroCouple::SDK::IO` (headers under `include/hydrocouplesdk/io/`) except
`ResultsModelComponent`, which is in `HydroCouple::SDK`.

## The recording pipeline

<pre class="mermaid">
flowchart LR
    subgraph sim["simulation thread"]
        out["component outputs"] --> snap["Snapshot::capture()"]
    end
    snap -- "enqueue (blocks when full)" --> queue["IOThread queue"]
    subgraph io["IO thread"]
        queue --> csv["CSVWriter"]
        queue --> h5["HDF5UGRIDWriter"]
        queue --> nc["NetCDFUGRIDWriter"]
        queue --> gpkg["GeoPackageWriter"]
    end
    csv --> files["artifacts"]
    h5 --> files
    nc --> files
    gpkg --> files
    csv -. "catalog()" .-> rec["RunRecorder"]
    h5 -. "catalog()" .-> rec
    nc -. "catalog()" .-> rec
    gpkg -. "catalog()" .-> rec
    rec --> manifest["run.json"]
    manifest --> reopen["ResultsModelComponent<br/>IResultReader"]
    files --> reopen
</pre>

| Class | Role |
| --- | --- |
| @ref HydroCouple::SDK::IO::Snapshot | The values of several items at one time level, deep-copied so the model can move on. |
| @ref HydroCouple::SDK::IO::FieldSlice | One item inside a snapshot: id, element kind, shape, owned bytes. |
| @ref HydroCouple::SDK::IO::IOThread | One worker thread and a bounded queue that deliver snapshots to every writer in order. |
| @ref HydroCouple::SDK::IO::IOutputWriter | The interface a writer implements: initialize, write, finalize, catalog. |
| @ref HydroCouple::SDK::IO::CSVWriter | Long-format CSV. Always built. |
| @ref HydroCouple::SDK::IO::HDF5UGRIDWriter | CF/UGRID mesh fields in HDF5. Built with USE_HDF5. |
| @ref HydroCouple::SDK::IO::NetCDFUGRIDWriter | CF/UGRID mesh fields in NetCDF-4, optionally layered. Built with USE_NETCDF. |
| @ref HydroCouple::SDK::IO::GeoPackageWriter | Long-format SQLite tables plus a node point layer. Built with USE_GEOPACKAGE. |
| @ref HydroCouple::SDK::IO::MeshDefinition, @ref HydroCouple::SDK::IO::MeshLocation | The mesh the UGRID and GeoPackage writers store, and where a field's values live on it. |
| @ref HydroCouple::SDK::IO::ResultEntry, @ref HydroCouple::SDK::IO::ResultTimeAxis | One catalog row: where an item's values are and how to read them. |
| @ref HydroCouple::SDK::IO::RunRecorder | Collects component outcomes and writer catalogs into a manifest. |
| @ref HydroCouple::SDK::IO::RunManifest, @ref HydroCouple::SDK::IO::ComponentOutcome, @ref HydroCouple::SDK::IO::RunStatus | The manifest in memory. |
| @ref HydroCouple::SDK::IO::IResultReader, `createResultReader()` | Read one artifact's values, times and mesh. |
| @ref HydroCouple::SDK::ResultsModelComponent | A recorded run presented as a finished component. |
| `compositionForRun()` | The composition that reopens a run through `ModelInitializer`. |

The order of the steps matters in one place: **finish the writers before finalizing the
manifest**. A writer's catalog describes what it wrote and is complete only after `finalize()`,
which `IOThread::finish()` calls. A manifest written earlier describes partial artifacts.

## Recording a small run with CSV

The example `ch13_record_csv` has a component `gauge` with two outputs: `flow` (three cells, in
cubic feet per second) and `level` (one value, in meters). It runs six hourly steps, records them
with the CSV writer, writes a manifest, and reopens the run.

@snippet ch13_record_csv/main.cpp record

The writer is shared between the IO thread, which drives it, and the recorder, which reads its
catalog at the end; both hold a `std::shared_ptr`. The recorder also gets the component, so it can
record its final status and read its outputs' dimensions and units.

### Capturing values: Snapshot

A @ref HydroCouple::SDK::IO::Snapshot is the unit of work: the values of one or more items at one
time level. `Snapshot(time)` stamps it; `capture(item, message)` reads the whole item through the
typed data plane (`getValuesInto()` over a contiguous host buffer) into a new
@ref HydroCouple::SDK::IO::FieldSlice. The copy is the only one on the write path, and it is
complete when `capture()` returns, so the model may change its values immediately. A snapshot is
move-only and is handed to `IOThread::enqueue()` by move.

- The *time* is a number the writers store as given. The SDK's convention is a Julian day, and the
  readers and `ResultsModelComponent` assume it (see the time-units rules
  @ref sdk_guide_record_times "below"). The example uses `2461370.5 + step / 24.0`, hourly.
- Only fixed-width kinds can be captured: Float64, Float32, Int64, UInt64, Int32, UInt32, Int16,
  UInt16, Int8, UInt8 and Boolean. A String or Opaque item fails with
  "Snapshot::capture: item 'ID' has a non-fixed-width DataKind (String/Opaque) that cannot be
  captured as raw bytes".
- The slice's `itemId` is the item's `id()`, and writers match fields by it. Two items with the
  same id in one snapshot (outputs of two components with the same output id) are both appended;
  the UGRID writers then use the first and the CSV and GeoPackage writers write both under one
  name. Give recorded items distinct ids.
- `addField(slice)` appends a slice built by hand, without checks.
- `captureGeometry(item, message)` is for layered items (`HydroCouple::Spatial::ILayering`): it
  appends the vertical coordinate's interface elevations, once per vertical coordinate shared by
  several items, under a reserved id (`verticalFieldId()`, beginning "__hc_interfaces_").
  `RunRecorder::finishToFile()` folds that slice into the layered items' catalog entries. Hosts
  recording layered items call it in the same snapshot as `capture()`.

### The IO thread

@ref HydroCouple::SDK::IO::IOThread runs every writer on one worker thread, so writers need no
locking against each other and never block the simulation, except by design: the queue holds at
most `capacity` snapshots (default 64, at least 1), and `enqueue()` blocks while it is full. A slow
disk therefore throttles the model instead of exhausting memory.

<pre class="mermaid">
sequenceDiagram
    participant Host as Host thread
    participant IO as IOThread
    participant W as Worker thread
    participant Writer
    Host->>IO: addWriter(writer)
    Host->>IO: start()
    IO->>Writer: initialize() (on the host thread)
    IO->>W: start the worker
    loop every recorded step
        Host->>IO: enqueue(snapshot) (waits while the queue is full)
        W->>Writer: write(snapshot), in enqueue order
    end
    Host->>IO: finish()
    W->>Writer: write() the snapshots still queued
    IO->>W: join
    IO->>Writer: finalize() (on the host thread)
</pre>

- `addWriter()` must be called before `start()`; adding a writer to a running thread is a data race.
- `start()` calls every writer's `initialize()` on the calling thread, then starts the worker. It
  returns false when a writer failed; that writer is disabled and its error latched, and the worker
  still starts for the others.
- `enqueue()` returns false only when the thread is not running, or when `finish()` is called
  while it waits for room; the snapshot is then dropped. A failing writer does not make it return
  false.
- The worker writes each snapshot to every enabled writer before it takes the next. A writer that
  returns nonzero from `write()` is disabled; the others continue.
- `finish()` writes everything still queued, joins the worker, and calls `finalize()` on every
  writer, the disabled ones included, on the calling thread. It is idempotent, and the destructor
  calls it. It does nothing if `start()` was never called.
- Errors are *latched*: the first nonzero code from any writer's `initialize()`, `write()` or
  `finalize()` is kept, with "NAME: MESSAGE" (`errorCode()`, `errorMessage()`); later errors are
  not recorded. `deliveredCount()` counts the snapshots taken off the queue.

Check `errorCode()` after `finish()`, as the example does: a writer error during the run is not
reported anywhere else.

### Several pumps in one process

One IOThread keeps its own writers off each other, but a process often runs more than one: a host
reads results on its UI thread while a run records, or two components each record their own
files. HDF5 and NetCDF are built without thread-safe modes, and both keep process-wide state, so
two threads calling them at once race even on different files.

Every SDK function that calls either library therefore holds
@ref HydroCouple::SDK::IO::ioLibraryMutex() for that one call: the HDF5 and NetCDF writers'
`initialize()`, `write()` and `finalize()`, the UGRID reader's functions, and the HDF5 and NetCDF
result readers. The mutex lives in the SDK library, so every component that links the SDK shares
it, and it is recursive, so an SDK call that makes another cannot deadlock. It is held per call,
never for a file's lifetime, so a long recording does not block reads of other files.

- Code that calls `nc_*` or `H5*` through the SDK's copy of the libraries must hold the same mutex
  for each call.
- A component that links its own copy must keep it private to its binary and must never open a
  file that another copy has open.
- Give each file one writer. To show a file while it is being written, read the snapshots in
  memory rather than reopening the file.

## The writers

All four writers implement @ref HydroCouple::SDK::IO::IOutputWriter and are driven the same way.
They differ in what they store and in what they need before `initialize()`.

| | CSV | HDF5 UGRID | NetCDF UGRID | GeoPackage |
| --- | --- | --- | --- | --- |
| Class | `CSVWriter` | `HDF5UGRIDWriter` | `NetCDFUGRIDWriter` | `GeoPackageWriter` |
| Build option | always | USE_HDF5 | USE_NETCDF | USE_GEOPACKAGE |
| Format token in the manifest | "csv" | "hdf5_ugrid" | "netcdf_ugrid" | "geopackage" |
| `name()` in error messages | "csv" | "hdf5-ugrid" | "netcdf-ugrid" | "geopackage" |
| Fields | every field of every snapshot | registered with `addField()` before `initialize()` | registered with `addField()` before `initialize()` | every field; `addField()` adds units and a description |
| Field shape | any | one value per node, edge or face | one value per node, edge or face | any (flattened) |
| Stored kind | as captured (written as text) | Float64 | Float64 | REAL (Float64) |
| Mesh | none | nodes, faces, edges, elevations | nodes, faces, edges, elevations, optional sigma layers | node points only (`setMesh()`) |
| Time | a column of every row | `/time` dataset | `time` variable | `elapsed_time` column |
| Existing file | truncated | truncated | overwritten | deleted first |
| Error codes | 1 to 3 | 10 to 24 | 50 to 65 | 30 to 45 |

The UGRID and GeoPackage writers use @ref HydroCouple::SDK::IO::MeshDefinition, which holds node coordinates
(`nodeX`, `nodeY`, optional `nodeZ`), faces in compressed rows (`faceNodeOffsets`, `faceNodes`),
optional `edgeNodes`, and `meshName`, which prefixes every UGRID variable name. `initialize()`
refuses a mesh that fails `MeshDefinition::validate()`. @ref HydroCouple::SDK::IO::MeshLocation
(Node, Edge, Face) says where a field's values live; a UGRID field must have exactly as many values
as its location has entities. @ref sdk_guide_mesh_packages describes meshes in more detail.

The composition document's `writers` block (@ref sdk_guide_compositions) names writers, but the
SDK does not build them: a host maps each block to a writer itself. The natural mapping is `type`
to the class above, `path` to the constructor's path, `simulation` to the GeoPackage simulation
name, and each `fields` entry to an `addField()` call; a UGRID writer also needs the host to supply
the mesh and each field's location.

### CSVWriter

@ref HydroCouple::SDK::IO::CSVWriter writes one row per value, in long ("tidy") form:

```text
time,item,index,value
2461370.5,flow,0,-2500.3333333333335
2461370.5,flow,1,-2500.2083333333335
2461370.5,flow,2,-2500.0833333333335
2461370.5,level,0,100
```

`index` is the element's row-major position within its field. Values and times are written with 17
significant digits, so the CSV reader gives back the exact doubles that were written; the example
compares them byte for byte. Booleans are written 0 and 1, and every value passes through a double,
so 64-bit integers above 2^53 lose precision. Item ids are written unquoted: an id with a comma or
a line break makes the file unreadable by the CSV reader. `initialize()` creates missing parent
directories and truncates an existing file.

`setTimeUnits(units)` declares the units of the time column for the catalog (default
"julian_day"); the file itself does not say. The catalog gives each item the shape
`{ levels, n }` with dimensions `{ "time", "element" }` for a field of `n` values, or
`{ "time", "dim0", ... }` for a field of higher rank. Codes: 1 the file cannot be opened, 2 a
stream failure (also a `write()` without a successful `initialize()`), 3 an unsupported kind.

### HDF5UGRIDWriter

@ref HydroCouple::SDK::IO::HDF5UGRIDWriter writes a file that follows the CF-1.11 and UGRID-1.0
conventions on top of plain HDF5, so UGRID-aware tools (xarray with xugrid, QGIS through MDAL) can
load it. With `MESH` for the mesh name:

| Dataset | Shape and type | Attributes |
| --- | --- | --- |
| `/` | | `Conventions` "CF-1.11 UGRID-1.0", `title`, `source` |
| `/MESH` | scalar int | `cf_role` "mesh_topology", `topology_dimension` (2 with faces, else 1), `node_coordinates`, `face_node_connectivity`, `edge_node_connectivity` |
| `/MESH_node_x`, `_y`, `_z` | [nodes] double | `standard_name` |
| `/MESH_face_nodes` | [faces, widest face] int64 | `cf_role`, `start_index` 0, `_FillValue` -1 (pads shorter faces) |
| `/MESH_edge_nodes` | [edges, 2] int64 | `cf_role`, `start_index` 0 |
| `/time` | [unlimited] double, chunked, deflate 4 | `units` "days since -4713-01-01", `calendar` "julian" |
| `/FIELD` | [unlimited, location count] double, chunked 256 levels, deflate 4 | `mesh`, `location`, `coordinates` "time", optional `units` and `standard_name` |

`addField(itemId, location, units, standardName)` registers a field before `initialize()`; its
dataset is named after the item id. Every snapshot must then carry every registered field; fields
that were not registered are ignored. Each `write()` extends `/time` and every field by one row.
The `node_coordinates` attribute names only the x and y datasets, even when `_node_z` is written,
so a conventions-driven reader (the SDK's own UGRID reader included) does not find the elevations;
the NetCDF writer lists all three. Codes: 10 invalid mesh, 11 file creation, 12 to 15 topology,
coordinate, connectivity and time datasets, 16 a field on a location the mesh has none of, 17 a
field dataset; 20 time append, 21 a registered field missing from a snapshot, 22 a wrong element
count, 23 an unsupported kind, 24 a field append. `finalize()` always returns 0.

### NetCDFUGRIDWriter

@ref HydroCouple::SDK::IO::NetCDFUGRIDWriter writes the same layout through the NetCDF-4 API, with
named dimensions (`time` unlimited, `MESH_nNodes`, `MESH_nFaces`, `MESH_maxFaceNodes`,
`MESH_nEdges`, `Two`), all three node coordinates listed in `node_coordinates`, and fields shuffled
and deflated at level 4. Fields are registered and matched exactly as for HDF5. It supersedes the
legacy `threadsafenetcdf` module, whose critical sections the single-writer IO thread and the
process-wide library lock now provide.

`setVerticalCoordinate(interfaceSigma, depth, surface)`, called before `initialize()`, turns the
file into a layered one: the topology stays two-dimensional and a CF `ocean_sigma_coordinate`
describes the water column over each face (@ref sdk_guide_mesh_packages shows it). Codes: 50
invalid mesh, 51 file creation, 52 to 59 definitions and static values, 60 time write, 61 a missing
field, 62 a wrong element count, 63 an unsupported kind, 64 a field write, 65 closing. The NetCDF
library's own message is in `errorMessage()`.

### GeoPackageWriter

@ref HydroCouple::SDK::IO::GeoPackageWriter writes an OGC GeoPackage 1.3 (an SQLite file): the
`gpkg_spatial_ref_sys`, `gpkg_contents` and `gpkg_geometry_columns` tables with the three mandatory
reference systems, and the result tables

| Table | Columns |
| --- | --- |
| `simulations` | `id`, `name` (the constructor's simulation name), `started_at` |
| `variables` | `id`, `item_id` (unique), `units`, `description` |
| `result_timeseries` | `simulation_id`, `variable_id`, `object_index`, `elapsed_time`, `value` |
| `result_summary` | `simulation_id`, `variable_id`, `count`, `min`, `max`, `mean` (computed by `finalize()`) |
| `mesh_nodes` (with `setMesh()`) | `fid`, `geom` (GeoPackageBinary point, srs_id -1), `node_index` |

Every field of every snapshot is written; a field's `variables` row is created when it first
appears, with the units and description of `addField()` when one was given. `object_index` is the
element's position within its field. `elapsed_time` holds the snapshot time as given (a Julian day
by convention), not seconds. Rows are inserted with a prepared statement inside transactions of
`flushRowCount` rows (default 5000) under `PRAGMA synchronous = NORMAL`. Only the node x and y of
an attached mesh are stored. Codes: 30 invalid mesh, 31 to 39 creating the database and its tables,
40 a variables row, 41 an unsupported kind, 42 a value row, 43 a commit, 44 the last commit, 45 the
summary.

### Writing your own writer

A writer implements the five pure functions of @ref HydroCouple::SDK::IO::IOutputWriter, and
`catalog()` if its artifacts are to be reopened. The IO thread calls them in a fixed order from one
thread at a time, so no locking is needed: `initialize()` once, `write()` once per snapshot until
the first nonzero return, and `finalize()` once, also after a failed `initialize()`. Return 0 for
success and keep the reason of a failure for `errorMessage()`. A writer whose `catalog()` returns
nothing (the default) still writes its files; it just contributes nothing to the manifest. Pick
error codes outside the ranges of the SDK writers so a latched code identifies its writer.

## The run manifest

### RunRecorder

@ref HydroCouple::SDK::IO::RunRecorder assembles the manifest. Construct it before the run: the
constructor stamps the start time, starts a wall-clock timer, takes a run id (or generates a
UUID), and records the SDK version as both `versions.sdk` and `versions.interface`. Then:

- `setCaption(text)` names the run.
- `setComposition(path, document, embed)` records the composition's path, and with
  `embed = true` copies the document into the manifest. No hash is computed.
- `addWriter(writer)` registers a writer whose catalog is collected (shared ownership).
- `addComponent(component)` registers a component (non-owning; it must outlive the recorder's
  finish).

At the end, after `IOThread::finish()`:

- `finish(status)` returns the manifest: end time, wall seconds, the status, one
  @ref HydroCouple::SDK::IO::ComponentOutcome per component (id, caption, status spelling such as
  "Finished", and the messages of its `errors()`, which are read, not cleared), and every catalog
  entry of every writer. An entry without a component id is attributed to the first registered
  component that has an output or a result with the entry's item id.
- `finishToFile(path, message, status)` does the same and then, for every entry whose item is an
  output of a registered component, adds what a reader needs: the output's caption as
  `description`; `dimensions` as "time" followed by the output's dimension ids (the rank must match
  the shape); `units` and a `quantity` block with the unit's SI scale, offset, dimension powers and
  value kind, when the output's value definition is a quantity with a unit and the entry's units
  are empty or equal the unit's caption; for a time axis in "elapsed_s" or "seconds_since_start",
  an `epoch_julian_day` taken from the simulation period of the component, when it is a
  `HydroCouple::Temporal::ITimeModelComponent` that has one; and,
  for a mesh-located item (`IPolyhedralSurfaceComponentDataItem`), a recorded mesh file beside the
  manifest and a `geometry` block naming it. It then writes the manifest.

Call one of them once: each call appends the outcomes and catalogs again. Give `finishToFile()` a
path with a directory part. A bare file name such as "run.json" fails with
"recording result metadata: filesystem error: in create_directories: No such file or directory
[\"\"]", because the recorder tries to create an empty parent directory; use "./run.json".

`RunStatus` is the host's statement of how the run ended: Completed ("completed"), Failed
("failed": a component or writer failed; the results remain readable) or Stopped ("stopped",
ended early on request). The recorder does not infer it.

### What the manifest says

The example reads its manifest back and prints it:

@snippet ch13_record_csv/main.cpp manifest

The `run` part of the file:

```json
{
  "components": [
    { "caption": "Gauge", "final_status": "Finished", "id": "gauge" }
  ],
  "run": {
    "caption": "Chapter 13 recording example",
    "finished": "2026-11-26T19:36:52Z",
    "id": "chapter-13-run",
    "started": "2026-11-26T19:36:52Z",
    "status": "completed",
    "wall_seconds": 0.00124975
  },
  "schema_version": "1.0",
  "versions": { "interface": "2.0.0-beta.1", "sdk": "2.0.0-beta.1" }
}
```

and one of its two `results` entries, printed by the program:

```text
the flow entry as written in run.json:
{
  "artifact": "results.csv",
  "component": "gauge",
  "dimensions": [
    "time",
    "cell"
  ],
  "format": "csv",
  "item": "flow",
  "kind": "Float64",
  "quantity": {
    "offset": 0.0,
    "powers": [
      3.0,
      0.0,
      -1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0
    ],
    "scale": 0.0283168,
    "value_kind": 0
  },
  "shape": [
    6,
    3
  ],
  "time": {
    "count": 6,
    "end": 2461370.7083333335,
    "start": 2461370.5,
    "units": "julian_day"
  },
  "units": "ft³/s",
  "variable": "flow"
}
```

The catalog entry (@ref HydroCouple::SDK::IO::ResultEntry) is everything a reader needs, and
nothing is inferred at read time: `artifact` is relative to the manifest, `format` selects the
reader, `variable` is the name inside the artifact, `kind` the stored element type, `shape` the full
extent with time first, `dimensions` the name of each axis. `quantity.scale` 0.0283168 converts
cubic feet per second to cubic meters per second, and `powers` are the unit's exponents of length,
mass, time and the other fundamental dimensions, in that order, so a reader converts units without
parsing "ft³/s". `RunManifest` reads and writes the file:

- `RunManifest::read(path, manifest, message)` parses and validates it and sets `directory` to the
  file's parent; `write(path, message)` writes it (creating directories). `fromJson()` and
  `toJson()` work on JSON in memory.
- `entry(componentId, itemId)` finds one entry, `entriesFor(componentId)` all entries of a
  component, and `artifactPath(entry)` resolves an entry's artifact against `directory`.

## Reading results back

### Result readers

A *result reader* (@ref HydroCouple::SDK::IO::IResultReader) serves one artifact.
`createResultReader(format, message)` returns the reader for an entry's `format`, or nullptr with a
message naming the missing build feature; `resultFormatSupported(format)` asks in advance. `open()`
the artifact at `RunManifest::artifactPath(entry)`, then:

- `read(entry, start, count, into, message)` copies a hyperslab. `start` and `count` have one value
  per dimension of the entry's shape, and the selection must lie inside it. The destination must be
  host memory of the entry's kind holding exactly the selected number of elements; values are
  written contiguously in row-major order. The SDK's readers serve Float64, Float32, Int32 and
  Int64 destinations.
- `readTimes(entry, times, message)` returns the time values: the HDF5 and NetCDF readers read the
  stored time coordinate, the CSV reader the distinct times of the item's rows, and the GeoPackage
  reader reconstructs a uniform axis from the catalog's `time` block,
  \f$t_i = t_0 + i\,(t_1 - t_0)/(n - 1)\f$ for \f$i = 0, \ldots, n - 1\f$.
- `readMesh(entry, mesh, message)` returns the mesh the values are attached to, when the artifact
  stores one. The HDF5 and NetCDF readers read the UGRID mesh named by the entry's `mesh` (so both
  need NetCDF support) and refuse an entry that names none rather than guessing; the GeoPackage
  reader returns the `mesh_nodes` points as a node-only mesh; the CSV reader answers false with the
  reason "result format \"csv\" records values without geometry", which is not an error.

| Reader | `open()` | Values | Times | Mesh |
| --- | --- | --- | --- | --- |
| "csv" | reads and indexes the whole file | from the index (rows of an item in row-major order) | distinct row times | none |
| "hdf5_ugrid" | read-only, on demand | HDF5 hyperslab of `/VARIABLE` | `/time` | UGRID, via NetCDF |
| "netcdf_ugrid" | read-only, on demand | NetCDF hyperslab | `time` | UGRID |
| "geopackage" | read-only, on demand | whole variable loaded once, ordered by time and object | uniform from the catalog | node points |

The example `ch13_writers` records two mesh fields with all four writers through one IO thread and
reads every artifact back. It needs an SDK built with USE_HDF5, USE_NETCDF and USE_GEOPACKAGE, which
its `REQUIRES` file states; other builds skip it.

@snippet ch13_writers/main.cpp writers

@snippet ch13_writers/main.cpp catalog

@snippet ch13_writers/main.cpp readers

@snippet ch13_writers/main.cpp ugrid

Copied from the ctest run of `docs.example.ch13_writers`:

```text
csv depth: results.csv, Float64 {4, 2} (time, element)
csv head: results.csv, Float64 {4, 4} (time, element)
hdf5_ugrid depth: results.h5, Float64 {4, 2} (time, face), mesh square at face, units m
hdf5_ugrid head: results.h5, Float64 {4, 4} (time, node), mesh square at node, units m
netcdf_ugrid depth: results.nc, Float64 {4, 2} (time, face), mesh square at face, units m
netcdf_ugrid head: results.nc, Float64 {4, 4} (time, node), mesh square at node, units m
geopackage depth: results.gpkg, Float64 {4, 2} (time, object), mesh square, units m
geopackage head: results.gpkg, Float64 {4, 4} (time, object), mesh square, units m
csv depth reads back identical values, 4 times, no mesh: result format "csv" records values without geometry
csv head reads back identical values, 4 times, no mesh: result format "csv" records values without geometry
hdf5_ugrid depth reads back identical values, 4 times, mesh square with 4 nodes, 2 faces, 5 edges
hdf5_ugrid head reads back identical values, 4 times, mesh square with 4 nodes, 2 faces, 5 edges
netcdf_ugrid depth reads back identical values, 4 times, mesh square with 4 nodes, 2 faces, 5 edges
netcdf_ugrid head reads back identical values, 4 times, mesh square with 4 nodes, 2 faces, 5 edges
geopackage depth reads back identical values, 4 times, mesh square with 4 nodes, 0 faces, 0 edges
geopackage head reads back identical values, 4 times, mesh square with 4 nodes, 0 faces, 0 edges
run/results.nc: mesh square, 4 nodes, 2 faces, 4 elevations
run/results.h5: mesh square, 4 nodes, 2 faces, 0 elevations
reopened depth is served from hdf5_ugrid
reopened head is served from hdf5_ugrid
enqueue returned true; latched error 21: hdf5-ugrid: snapshot at t=2461370.500000 is missing registered field 'head'
all checks passed
```

No component was registered with the recorder, so the entries carry no component id and keep the
writers' own dimension names; with a component, `finishToFile()` replaces them by the outputs'
dimension ids, as in the CSV example. The HDF5 file gives back no elevations, for the
`node_coordinates` reason above. The last line shows a latched writer error: the snapshot was
accepted, the writer failed on it and was disabled, and only `errorCode()` and `errorMessage()`
say so.

@snippet ch13_writers/main.cpp failure

### ResultsModelComponent

@ref HydroCouple::SDK::ResultsModelComponent is a recorded run presented as a component. The
model's library is never loaded. `ResultsModelComponent::open(manifestPath, componentId, message)`
reads the manifest and constructs one that exposes the entries of `componentId` (all entries when
it is empty) under the same id; the constructor takes a manifest already in memory and an optional
different id.

@snippet ch13_record_csv/main.cpp reopen

`initialize()` opens one reader per artifact, creates one read-only output per item, and walks the
lifecycle Initializing, Initialized, Finishing, Finished in one call: a recorded run has already
computed its values. The items appear in both `results()` and `outputs()`, so a stored run can be
browsed and can also feed a new composition. An item recorded by several writers is exposed once,
served by the most capable format this build can read, in the order HDF5, NetCDF, GeoPackage, CSV;
`entryFor(itemId)` says which entry was chosen, and `times(itemId, values, message)` returns the
item's time values. Reading an item goes straight to the artifact; writing to it is refused ("item
\"ID\" belongs to a recorded run and is read-only").

- **Time units.** Each item's times are read when it is created. "julian_day" is used as stored;
  "elapsed_s" and "seconds_since_start" are converted to Julian days with the entry's
  `epoch_julian_day` and refused without one; any other units fail the item ("unsupported recorded
  calendar"). @anchor sdk_guide_record_times
- **Geometry.** An entry with a `geometry` block becomes a mesh-located item
  (`ITimeSeriesPolyhedralSurfaceComponentDataItem`) whose surface is built from the recorded mesh
  file; an entry without one, whose reader returns a mesh, becomes one too; otherwise the item is a
  plain time series. A layered entry (with a recorded vertical coordinate) becomes a layered item.
- **Units.** An entry with a `quantity` block gets a unit with that SI scale, offset and dimension
  powers; otherwise a unitless quantity named after the units text.
- **Failure.** The component ends Failed when the manifest records no results for the id (code
  1), no reader exists for a format (2), an artifact cannot be opened (3), no recorded format is
  readable in this build (4), or an item cannot be built (5, for example a missing geometry file or
  an unsupported calendar). Its error queue then holds two Fatal entries with the same message, one
  with that code and one with code 0 (entering Failed always queues one); the message names the
  item and the file. Calling `finish()` on a Failed component moves it to Finished, a transition
  the lifecycle does not allow.

Do not call `finish()` on a reopened component while its items are still read: `finish()` releases
the readers, and the items keep pointers to them, so reading an item afterwards reads freed memory.
The lifecycle the component already completed in `initialize()` does not need `finish()`.

The example's output:

```text
run chapter-13-run (Chapter 13 recording example): completed, sdk 2.0.0-beta.1
  component gauge ended Finished
  result gauge/flow in results.csv (csv), shape {6, 3}, dimensions {time, cell}, units ft³/s, time 2461370.5 to 2461370.708 julian_day
  result gauge/level in results.csv (csv), shape {6, 1}, dimensions {time, gauge}, units metres, time 2461370.5 to 2461370.708 julian_day
reopened flow: 6 times from 2461370.5 to 2461370.70833
reopened level: 6 times from 2461370.5 to 2461370.70833
flow at the last step: 2499.6666666666665, 2499.7916666666665, 2499.9166666666665
open composition: component gauge serves 2 outputs
moved run reopens identically
manifest without its CSV: item "flow": cannot open result artifact "incomplete/results.csv"
all checks passed
```

### Reopening through a composition

`compositionForRun(manifest, manifestPath)` builds the composition that reopens a run: one
`execution.mode` "open" block per component that contributed results, each pointing at the
manifest, and no connections. Hand it to a @ref HydroCouple::SDK::IO::ModelInitializer whose
resolver knows no component, and the initializer substitutes a `ResultsModelComponent` per block
(see @ref sdk_guide_compositions_open "Execution mode open"). This is how a user interface opens a
finished run for browsing:

@snippet ch13_record_csv/main.cpp open_composition

The runnable `examples/analysis_reopen` does this for the serial coupling example's run and prints
a summary of every item; @ref sdk_guide_example walks through it.

The same document can drive a *new* composition from stored results: add live components and
connections, and the recorded component acts as a boundary condition. A recorded output is history,
shaped `{ time, ... }`, while a live input usually wants one time level; the SDK does not decide
which level is "now". `TimeSliceOutput` presents one time level of a recorded item as an output of
the item's spatial shape, and the host sets its time index each step. The test
`ExecutionModeTest.StoredUpstreamReproducesTheAllLiveResult` runs a downstream component this way
and checks that it receives exactly the values the live upstream produced.

## Moving a run

A run is the manifest plus the artifacts it names, plus any `geometry_*.json` mesh files
`finishToFile()` wrote beside it. Artifact paths are relative to the manifest, so the directory can
be copied, archived or sent elsewhere as a whole and reopened there:

@snippet ch13_record_csv/main.cpp move

Keep the relative layout. If an item is listed but its values cannot be read, check the artifact
path relative to the manifest (`RunManifest::artifactPath()`) and whether this SDK build has a
reader for the entry's format (`resultFormatSupported()`). `composition.path` in the manifest is a
plain string, recorded as given and never resolved; it documents where the composition was, not
where it must be.

Reopening a run serves its recorded values; it does not let the run continue. Restarting a
simulation additionally needs the model's state and restart support from its components, which
`execution.mode` "resume" reserves for a future version.

## Diagnostics

### Snapshot and IOThread

| Message or code | Cause |
| --- | --- |
| Snapshot::capture: item 'ID' has a non-fixed-width DataKind (String/Opaque) that cannot be captured as raw bytes | Capturing a string or opaque item. |
| Snapshot::capture: item 'ID': ... | The item's `getValuesInto()` failed (for example a device-resident buffer). |
| layered item has no prepared vertical coordinate; shared recorded vertical coordinate changed shape; vertical interface shape mismatch; nonfinite recorded interface elevation | `captureGeometry()` on a layered item whose vertical coordinate is missing, changed or invalid. |
| `IOThread::errorMessage()` "NAME: MESSAGE" | The first writer failure; NAME is the writer's `name()`. |
| `enqueue()` returns false | The thread is not running, or `finish()` was called while waiting. |

### Writer codes

| Code | Writer | Meaning |
| --- | --- | --- |
| 1 | CSV | cannot open 'PATH' for writing |
| 2 | CSV | stream failure writing or finalizing 'PATH' |
| 3 | CSV | unsupported DataKind for field 'ID' |
| 10 | HDF5 | the mesh fails `MeshDefinition::validate()` (message starts "MeshDefinition: ") |
| 11 | HDF5 | H5Fcreate failed for 'PATH' |
| 12, 13, 14, 15 | HDF5 | topology, coordinate, connectivity or time dataset could not be created |
| 16 | HDF5 | field 'ID' registered on a LOCATION location, but the mesh has none |
| 17 | HDF5 | failed to create field dataset 'ID' |
| 20 | HDF5 | failed to append to /time |
| 21 | HDF5 | snapshot at t=TIME is missing registered field 'ID' |
| 22 | HDF5 | field 'ID' has N elements; the mesh LOCATION count is M |
| 23 | HDF5 | field 'ID' has an unsupported DataKind |
| 24 | HDF5 | failed to append to field 'ID' |
| 30 | GeoPackage | the attached mesh fails validation |
| 31 | GeoPackage | cannot open 'PATH' |
| 32 to 39 | GeoPackage | pragmas, conformance tables, result tables, simulation row, node layer (36 to 38), prepared insert or first transaction; the SQLite message |
| 40 | GeoPackage | inserting a variables row failed |
| 41 | GeoPackage | field 'ID' has an unsupported DataKind |
| 42, 43 | GeoPackage | inserting a value row or committing a transaction failed |
| 44, 45 | GeoPackage | the final commit or the summary failed |
| 50 | NetCDF | the mesh fails validation |
| 51 to 59 | NetCDF | the failing NetCDF call and nc_strerror() text (58 also "field 'ID' registered on a LOCATION location, but the mesh has none") |
| 60, 64 | NetCDF | writing the time value or a field failed |
| 61, 62, 63 | NetCDF | missing field, wrong element count, unsupported kind (same texts as HDF5) |
| 65 | NetCDF | nc_close failed |

### Manifests

| Message | Cause |
| --- | --- |
| Cannot open run manifest "PATH" | `RunManifest::read()` on a missing file. |
| Run manifest "PATH" is not valid JSON: ... | |
| Run manifest "PATH": Unsupported run manifest schema_version "X"; this SDK reads version 1.x | |
| Run manifest "PATH": Run manifest must carry a "run" object | |
| Run manifest "PATH": run.status must be "completed", "failed" or "stopped" | |
| Run manifest "PATH": Run manifest must carry a "results" array (it may be empty, but the catalog is what makes a run reopenable) | |
| Run manifest "PATH": results[] entry needs a string "item" (also "artifact", "format", "kind") | |
| Run manifest "PATH": results["ID"].kind "K" is not a fixed-width DataKind | |
| Run manifest "PATH": results["ID"] needs a non-empty "shape" array / .shape must hold non-negative integers | |
| Run manifest "PATH": results["ID"].dimensions must be parallel to shape (N vs M) | |
| Run manifest "PATH": results["ID"].location must be "node", "edge" or "face" | |
| Cannot write run manifest "PATH" / Failed while writing run manifest "PATH" | `RunManifest::write()`. |
| recording result metadata: ... | An exception in `finishToFile()`, for example a path without a directory part. |
| recorded dimension rank mismatch; ambiguous recorded output identity: ID; recorded surface has no mesh view; could not write recorded geometry; recorded mesh entity dimension is absent; unsupported recorded mesh location; recorded layer dimension is absent | `finishToFile()` could not describe an output's metadata. |

A manifest field of the wrong JSON type (a number where a string belongs, such as a numeric
`run.id` or `component`) is not reported as a message: the JSON library throws
`nlohmann::json::type_error` out of `RunManifest::read()` and `fromJson()`. Validate hand-edited
manifests against `run-1.0.json` (@ref sdk_ref_run_manifest).

### Readers and ResultsModelComponent

| Message | Cause |
| --- | --- |
| this SDK build has no reader for result format "F" — rebuild with the matching feature enabled | HDF5, NetCDF or GeoPackage support not built. |
| unknown result format "F" | |
| cannot open result artifact "PATH" (also "cannot open HDF5/NetCDF/GeoPackage result artifact") | The artifact is missing or unreadable; a moved run without its files. |
| result artifact "PATH" is empty | A CSV without a header. |
| result artifact "PATH" holds no item "ID"; HDF5 artifact ... holds no dataset "/ID"; NetCDF artifact ... holds no variable "ID" | The entry's variable is not in the file. |
| result artifact "PATH" holds N values for "ID"; the catalog records M (also GeoPackage "rows") | The artifact and the catalog disagree. |
| result read of "ID": start/count rank ... does not match the recorded rank N; selection out of range on dimension D; destination kind ... differs from recorded kind ...; destination holds N elements; the selection covers M | A bad `read()` request. |
| unsupported destination DataKind K for a result read | Destination kind other than Float64, Float32, Int32, Int64. |
| result "ID" records no time axis | `readTimes()` default on an entry without `time`. |
| result "ID" names no mesh | `readMesh()` on an HDF5 or NetCDF entry without `mesh`. |
| run "R" records no results for component "C" | `ResultsModelComponent` code 1. |
| item "ID": ... | Codes 2, 3 and 5: the reader's or the item's message. |
| no readable artifact for the recorded items (ID as F, ...) — rebuild with the matching features enabled | Code 4. |
| elapsed_s results require a declared Julian-day epoch; unsupported recorded calendar: U | Time units the component cannot convert. |

The CSV reader converts numbers with `std::stod`, so a malformed number in a CSV artifact throws
`std::invalid_argument` ("stod: no conversion") out of `open()`, and through
`ResultsModelComponent::initialize()`, instead of returning a message.
