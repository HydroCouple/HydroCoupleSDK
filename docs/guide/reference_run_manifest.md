@page sdk_ref_run_manifest 20. Run manifest reference

[TOC]

This chapter lists every field of a run manifest (`run.json`): its JSON type, whether the reader
requires it, who writes it, and what it means. The authorities are the parser,
`RunManifest::fromJson()` and `ResultEntry::fromJson()`, the writer, `RunRecorder` with
`RunManifest::toJson()`, and the JSON Schema `run-1.0.json` (in `share/schema/` of the source tree
and `share/hydrocouplesdk/schema/` of an installed SDK; `$id`
https://hydrocouple.org/schema/run-1.0.json). @ref sdk_guide_record explains how a manifest is
written and used. All C++ types named here are in `HydroCouple::SDK::IO`.

Conventions:

- **Required** is what the parser refuses to do without; "schema" in the notes means the JSON
  Schema requires more. The parser ignores unknown keys; the schema forbids them on every object
  except `composition.embedded`.
- A present optional field of the wrong JSON type is not reported as a message: the JSON library
  throws `nlohmann::json::type_error` out of `RunManifest::read()`.
- **Written by** names the code that sets the field: `finish()` is `RunRecorder::finish()` (and
  therefore also `finishToFile()`), `finishToFile()` the extra metadata only `finishToFile()` adds,
  and "writer" the catalog of the writer that produced the artifact.

## Top level

| Field | Type | Required | Default | Written by | Meaning |
| --- | --- | --- | --- | --- | --- |
| `$schema` | string | no | | not written | The schema URI, for editors. |
| `schema_version` | string | no (schema: yes) | "1.0" | always, "1.0" (`kRunManifestSchemaVersion`) | Manifest version. The part before the first period must be "1". Schema pattern `^1\.[0-9]+$`. |
| `run` | object | **yes** | | always | What ran and how it ended; see below. |
| `versions` | object | no | | when either version is set | Versions that produced the run. |
| `composition` | object | no | | when a path, hash or document is set | The composition that produced the run. |
| `components` | array | no | empty | when components were registered | One outcome per component. |
| `results` | array | **yes** (may be empty) | | always | The results catalog: one entry per recorded item and writer. |

## run

| Field | Type | Required | Default | Written by | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `id` | string | no (schema: yes, at least one character) | "" | always | `RunManifest::id` | Run identifier: the id given to `RunRecorder`, or a generated UUID. |
| `caption` | string | no | "" | when set (`setCaption()`) | `RunManifest::caption` | Display name. |
| `started` | string | no | "" | the `RunRecorder` constructor | `RunManifest::started` | Start time, ISO 8601 UTC to the second, "YYYY-MM-DDTHH:MM:SSZ". |
| `finished` | string | no | "" | `finish()` | `RunManifest::finished` | End time, same format. |
| `wall_seconds` | number ≥ 0 | no | 0 | `finish()`, when positive | `RunManifest::wallSeconds` | Elapsed wall-clock seconds from the recorder's construction to `finish()`. |
| `status` | string: "completed", "failed", "stopped" | no (schema: yes) | "completed" | always | `RunManifest::status` | How the run ended, as the host declared it. Results of a failed or stopped run remain readable. Another spelling is refused. |
| `host` | string | no | "" | never by `RunRecorder` | `RunManifest::host` | Free text naming the machine; a host may set it. |

## versions

| Field | Type | Default | Written by | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- |
| `sdk` | string | "" | the `RunRecorder` constructor | `RunManifest::sdkVersion` | HydroCouple SDK version (`HYDROCOUPLESDK_VERSION`). |
| `interface` | string | "" | the `RunRecorder` constructor | `RunManifest::interfaceVersion` | Interface version; the recorder writes the SDK version, the two being released in lockstep. |

A `versions` value that is not an object is ignored.

## composition

| Field | Type | Default | Written by | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- |
| `path` | string | "" | `setComposition()` | `RunManifest::compositionPath` | The composition's path, as given. Never resolved or rebased; moving a run does not change it. |
| `sha256` | string (schema: 64 lower-case hex digits) | "" | never by the SDK | `RunManifest::compositionSha256` | Hash of the composition document, for drift detection. A host may set it; no SDK code computes or checks it. |
| `embedded` | object | null | `setComposition(path, document, true)` | `RunManifest::compositionEmbedded` | The whole composition document. |

A `composition` value that is not an object is ignored.

## components[]

One @ref HydroCouple::SDK::IO::ComponentOutcome per component registered with
`RunRecorder::addComponent()`, in registration order.

| Field | Type | Default | Written by | Meaning |
| --- | --- | --- | --- | --- |
| `id` | string | "" (schema: required, non-empty) | `finish()` | The component's `id()`. |
| `caption` | string | "" | `finish()`, when not empty | The component's `caption()`. |
| `final_status` | string | "" | `finish()` | Status at `finish()`, spelled as `AbstractModelComponent::statusToString()` does ("Finished", "Failed", "Done"...). |
| `errors` | array of strings | empty | `finish()`, when there are any | The messages of the component's `errors()` at `finish()` (read without clearing them). |

## results[]

One @ref HydroCouple::SDK::IO::ResultEntry per item per writer: an item recorded by two writers
has two entries, and a reader chooses between them.

| Field | Type | Required | Default | Written by | Meaning |
| --- | --- | --- | --- | --- | --- |
| `component` | string | no | "" | `finish()`, by attribution | Component that produced the item. An entry without one is attributed to the first registered component with an output or result of that id; "" when none has. |
| `item` | string | **yes** | | writer | The recorded item's `id()` (`FieldSlice::itemId`). Schema: at least one character. |
| `artifact` | string | **yes** | | writer | File holding the values, relative to the manifest's directory (writers record the file name only). An absolute path is used as is. |
| `format` | string | **yes** | | writer | "csv", "hdf5_ugrid", "netcdf_ugrid" or "geopackage": selects the reader. The parser accepts any string; an unknown format fails when the entry is read. |
| `variable` | string | no | the item id | writer | Name inside the artifact: CSV item column, HDF5 dataset, NetCDF variable, GeoPackage `variables.item_id`. All SDK writers set it to the item id. |
| `kind` | string | **yes** | | writer | Stored element kind: "Int8", "UInt8", "Int16", "UInt16", "Int32", "UInt32", "Int64", "UInt64", "Float32", "Float64" or "Boolean". "Float64" for the HDF5, NetCDF and GeoPackage writers; the captured kind for CSV. |
| `shape` | array of integers ≥ 0 | **yes**, non-empty | | writer | Full extent, row-major, time first: `{ levels, values per level }` for the SDK writers. |
| `dimensions` | array of strings | no | empty | writer; replaced by `finishToFile()` | A name per dimension, parallel to `shape` when not empty. Writers use "time" plus "element"/"dimN" (CSV), the location (UGRID) or "object" (GeoPackage); `finishToFile()` uses "time" plus the output's dimension ids. |
| `units` | string | no | "" | UGRID and GeoPackage writers from `addField()`; `finishToFile()` | Units text. `finishToFile()` sets the unit's caption when the entry has none or the same one. |
| `description` | string | no | "" | UGRID writers (the standard name), GeoPackage (`addField()`); `finishToFile()` (the output's caption) | Human-readable description. |
| `mesh` | string | no | "" | UGRID writers, GeoPackage writer | UGRID mesh name the values are attached to. The GeoPackage writer always writes its mesh definition's name, "mesh" when no mesh was attached. |
| `location` | string: "node", "edge", "face" | no | | UGRID writers; `finishToFile()` for mesh-located items | Mesh location of the values. Another spelling is refused. |
| `time` | object | no | | writer | Time coordinate of the leading dimension; see below. |
| `quantity` | object | no | | `finishToFile()` | The unit's conversion to SI and its dimensions; see below. Must be an object when present. |
| `geometry` | object | no | | `finishToFile()` for mesh-located items | The recorded mesh; see below. Must be an object when present. |
| `sha256` | string (schema: 64 lower-case hex digits) | no | "" | never by the SDK | Hash of the artifact. Stored and round-tripped only: no SDK code computes or checks it, although the schema describes a mismatch as reported. |

### results[].time

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `count` | integer ≥ 0 | 0 | Number of time levels written. |
| `start` | number | 0 | First snapshot time, in `units`. |
| `end` | number | 0 | Last snapshot time, in `units`. |
| `units` | string | "julian_day" | Units of the time values. The SDK convention is "julian_day"; the CSV writer records what `setTimeUnits()` set. `ResultsModelComponent` also accepts "elapsed_s" and "seconds_since_start" with `epoch_julian_day`, and refuses anything else. |
| `epoch_julian_day` | number | absent | Julian day of elapsed time zero, required for an elapsed-seconds clock. `finishToFile()` sets it from the simulation period of a component that has one. |

A reader that stores no time coordinate (GeoPackage) rebuilds a uniform axis from `count`, `start`
and `end`. The HDF5 and NetCDF readers read the stored times, and the CSV reader the rows' times.

### results[].quantity

Written by `finishToFile()` when the output's value definition is a quantity with a unit. Read by
`ResultsModelComponent`, which rebuilds the unit from it. The schema requires all four fields; the
reader requires them when the object is present.

| Field | Type | Meaning |
| --- | --- | --- |
| `scale` | number > 0 | Factor that converts a stored value to SI (0.0283168 for cubic feet per second). |
| `offset` | number | Offset added after scaling (temperatures). |
| `powers` | array of 10 numbers | Power of each fundamental dimension, in the order Length, Mass, Time, ElectricCurrent, Temperature, AmountOfSubstance, LuminousIntensity, Currency, Unitless, PlaneAngle. |
| `value_kind` | integer | `HydroCouple::ValueKind`: 0 Unknown, 1 Intensive, 2 Extensive, 3 Flux, 4 Density. |

### results[].geometry

Written by `finishToFile()` for an output that is a `HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem`.

| Field | Type | Required (schema) | Meaning |
| --- | --- | --- | --- |
| `artifact` | string | yes | Recorded mesh file, relative to the manifest; one file per surface, named "geometry_UUID_N.json". |
| `entity_axis` | integer ≥ 1 | yes | Position of the mesh-entity dimension in `shape` (0 is time). |
| `layer_axis` | integer ≥ 1 | no | Position of the layer dimension, for layered items. |
| `vertical` | object, a `results[]` entry | no | The recorded layer interface elevations of a layered item: Float64, shape `{ levels, columns, layers + 1 }`. |
| `vertical_kind` | integer | no | `HydroCouple::Spatial::VerticalCoordinateKind`: 0 Unknown, 1 Sigma, 2 ZLevel, 3 ZStar, 4 Hybrid, 5 Isopycnal, 6 DepthBelowSurface. |

The recorded mesh file is a JSON object with `schema` "hc.recorded-mesh/1.0", `node_x`, `node_y`,
`node_z` (arrays of numbers), `face_offsets` and `face_nodes` (compressed face rows),
`edge_nodes` (a flat array of node index pairs), and, when the surface has a spatial reference,
`reference` with `authority`, `id`, `text`, `units`, `vertical_authority`, `vertical_id`,
`vertical_text` and `vertical_units` (units as `HydroCouple::IUnit::DistanceUnits` integers: 0
Meters, 1 Kilometers, 2 Feet, 3 NauticalMiles, 4 Yards, 5 Miles, 6 Degrees, 7 Centimeters, 8
Millimeters, 9 Inches, 10 Unknown).

## A complete manifest

The manifest the CSV example of @ref sdk_guide_record writes, with one of its two entries:

```json
{
  "components": [
    { "caption": "Gauge", "final_status": "Finished", "id": "gauge" }
  ],
  "results": [
    {
      "artifact": "results.csv",
      "component": "gauge",
      "dimensions": [ "time", "cell" ],
      "format": "csv",
      "item": "flow",
      "kind": "Float64",
      "quantity": {
        "offset": 0.0,
        "powers": [ 3.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 ],
        "scale": 0.0283168,
        "value_kind": 0
      },
      "shape": [ 6, 3 ],
      "time": { "count": 6, "end": 2461370.7083333335, "start": 2461370.5, "units": "julian_day" },
      "units": "ft³/s",
      "variable": "flow"
    }
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

Keys appear sorted because the JSON library stores object keys in order; readers must not depend on
key order.
