@page sdk_ref_composition 19. Composition document reference

[TOC]

This chapter lists every field of a composition document: its JSON type, whether it is required,
its default, the specification version that introduced it, and what it means. The authorities are
the parser, @ref HydroCouple::SDK::IO::CompositionSpec::parse, and the JSON Schema files
`composition-1.0.json` and `composition-1.1.json` (in `share/schema/` of the source tree and
`share/hydrocouplesdk/schema/` of an installed SDK). Where they differ, the table says what the
parser does and the notes say what the schema says. @ref sdk_guide_compositions explains how the
blocks are used.

Conventions:

- **Required** is what the parser refuses to do without. "Schema" in the notes means the JSON
  Schema requires or restricts more than the parser.
- **Default** is the value the parser uses when the field is absent. A field whose value is JSON
  `null` is treated as absent everywhere the parser reads an optional string or object.
- **Since** is the specification version: every field exists in 1.0 except the argument binding,
  added in 1.1.
- The parser ignores keys it does not know; the schemas forbid them (`additionalProperties`
  false) everywhere except inside argument payloads, adapted-output arguments and writer blocks.
- **Parsed into** names the member of the parsed spec (`HydroCouple::SDK::IO` types) that holds the
  field.

## Top level

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `$schema` | string | no | | 1.0 | not read | The schema URI, for editors. |
| `schema_version` | string | no | "1.0" | 1.0 | `CompositionSpec::schemaVersion` | Specification version. The part before the first period must be "1". Schema: pattern `^1\.[0-9]+$`, default "1.0" in the 1.0 schema and "1.1" in the 1.1 schema. |
| `metadata` | object | no | empty | 1.0 | `CompositionSpec::metadata` | Provenance; see below. |
| `components` | array of objects | **yes** | | 1.0 | `CompositionSpec::components` | Component instances; see below. May be empty. |
| `connections` | array of objects | no | empty | 1.0 | `CompositionSpec::connections` | Provider-to-consumer links. |
| `workflow` | object | no | strategy "none" | 1.0 | `CompositionSpec::workflow` | Orchestration settings for the host. |
| `writers` | array of objects | no | empty | 1.0 | `CompositionSpec::writers` | Output writers for the host to build. |
| `run` | object | no | | 1.0 | | Run recording settings. |

The root must be a JSON object ("Composition document must be a JSON object").

## metadata

Every field is an optional string; a present field of another type is refused
("metadata.KEY must be a string"). The SDK carries the block and does not interpret it.

| Field | Type | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- |
| `id` | string | "" | 1.0 | `CompositionMetadata::id` | Stable identifier of the composition. |
| `caption` | string | "" | 1.0 | `CompositionMetadata::caption` | Display name. |
| `description` | string | "" | 1.0 | `CompositionMetadata::description` | Free text. |
| `author` | string | "" | 1.0 | `CompositionMetadata::author` | Who wrote the composition. |
| `created` | string | "" | 1.0 | `CompositionMetadata::created` | When it was written. Schema: format date-time (ISO 8601); the parser does not check the format. |
| `sdk_version` | string | "" | 1.0 | `CompositionMetadata::sdkVersion` | SDK that wrote the document. |
| `interface_version` | string | "" | 1.0 | `CompositionMetadata::interfaceVersion` | HydroCouple interface version it targets. |

## components[]

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `id` | string | **yes** | | 1.0 | `ComponentSpec::id` | Instance identifier, unique within the document; the resolver is asked for it. Schema: at least one character; the parser accepts "". |
| `caption` | string | no | "" | 1.0 | `ComponentSpec::caption` | Display name, carried only. |
| `info` | object | no | empty | 1.0 | `ComponentSpec::info` | Instantiation hint; see below. |
| `execution` | object | no | mode "run" | 1.0 | `ComponentSpec::mode`, `resultsManifest` | Execution mode; see below. |
| `arguments` | object | no | {} | 1.0 | `ComponentSpec::arguments` | Argument id to payload; see below. Must be an object when present. |

### components[].info

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `library` | string | no | "" | 1.0 | `ComponentInfoRef::library` | Path of the shared library that provides the component. Executable content: the SDK never loads it; a host that does must allow-list it. |
| `component_info_id` | string | no | "" | 1.0 | `ComponentInfoRef::componentInfoId` | Id of the `HydroCouple::IComponentInfo` in that library. |

### components[].execution

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `mode` | string: "run", "open", "resume" | no | "run" | 1.0 | `ComponentSpec::mode` | "run": compute normally. "open": serve recorded results from `results` and finish. "resume": reserved; parsed, then refused by `ModelInitializer`. |
| `results` | string | when `mode` is "open" | "" | 1.0 | `ComponentSpec::resultsManifest` | Path of the run manifest that supplies the recorded results. Relative to the document's directory. |

### components[].arguments

Each key is an argument id of the component; each value is a payload in one of these forms. The
parser checks only the form of bindings; `ModelInitializer` checks that every id exists on the
component.

| Form | Recognized by | Since | Meaning |
| --- | --- | --- | --- |
| any JSON value | otherwise | 1.0 | Passed to `IArgument::initialize(text, JSON, message)`; the argument decides what it accepts (the SDK's typed arguments take `{ "values": [...] }` or a bare array). |
| object with \@uri | `jsonUriReference()` | 1.0 | The argument reads the reference instead; see below. |
| object with \@ref (or `values` holding one) | `isSidecarRef()` | 1.0 | A binary sidecar file; see below. |
| object with \@from | `isArgumentBinding()` | **1.1** | The output of another component; see below. |

In the 1.0 schema every payload is valid; the 1.1 schema validates a payload that carries \@from
against the binding definition below.

### Argument binding (\@from), since 1.1

The payload object must have exactly one key, \@from, whose value is an object:

| Field | Type | Required | Default | Since | Meaning |
| --- | --- | --- | --- | --- | --- |
| `component` | string | **yes** | | 1.1 | Provider component id. Non-empty, declared in `components[]` (before or after the consumer), not the consumer itself. |
| `output` | string | **yes** | | 1.1 | Provider output id. Non-empty; checked to exist when the binding is applied. |
| `selector` | null | no | null | 1.1 | Reserved. Any value other than null is refused as not yet implemented. |

Any other key inside \@from, or beside it in the payload, is refused. Bindings must not form a
cycle. `argumentBindings()` lists them as @ref HydroCouple::SDK::IO::ArgumentBindingSpec values.

### URI reference (\@uri)

| Field | Type | Meaning |
| --- | --- | --- |
| \@uri | non-empty string | A path or URI the argument reads. A plain path or `file:` URI is read from disk; a relative path is rebased on the document's directory by `ModelInitializer`. Any other scheme is read by the installed `UriResolver`. |

Other keys in the object are ignored when the argument reads the reference. The SDK's arguments
write \@uri back, with their values beside it, only for references that are not local files.

### Sidecar reference (\@ref)

Written by `ModelInitializer::serialize()` in sidecar mode and read by `initialize()`:

| Field | Type | Required | Meaning |
| --- | --- | --- | --- |
| \@ref | string | **yes** | Sidecar file name; relative to the sidecar directory, else the document directory, else the current directory. |
| `kind` | string | **yes** | Element kind: "Float64", "Float32", "Int64", "UInt64", "Int32", "UInt32", "Int16", "UInt16", "Int8", "UInt8" or "Boolean". |
| `shape` | array of integers | **yes** | Row-major extents; the file must hold exactly element size × product of extents bytes. |
| `byteOrder` | string | no | Written as "little"; not read. |

## connections[]

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `from` | object | **yes** | | 1.0 | | The provider end. |
| `to` | object | **yes** | | 1.0 | | The consumer end. |

### connections[].from

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `component` | string | **yes** | | 1.0 | `ConnectionSpec::fromComponent` | Provider component id, declared in `components[]`. Schema: at least one character. |
| `output` | string | no | "" | 1.0 | `ConnectionSpec::output` | Provider output id. Optional in both parser and schema, but wiring fails without a matching output. |
| `adapted_outputs` | array of objects | no | empty | 1.0 | `ConnectionSpec::adaptedOutputs` | Ordered adaptation chain; the consumer is wired to the last step. |

### connections[].from.adapted_outputs[]

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `id` | string | **yes** | | 1.0 | `AdaptedOutputSpec::id` | Adapted-output id the factory offers (for the SDK's factory: "linear_transform", "relaxation", "temporal_interpolation"). Schema: at least one character. |
| `factory` | string | no | "" (any factory) | 1.0 | `AdaptedOutputSpec::factory` | Factory id (the SDK's: "hydrocouple.sdk.adapters"). Empty searches every candidate. |
| `arguments` | object | no | {} | 1.0 | `AdaptedOutputSpec::arguments` | Argument id to payload for the adapted output, applied before it initializes. |

### connections[].to

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `component` | string | **yes** | | 1.0 | `ConnectionSpec::toComponent` | Consumer component id, declared in `components[]`. |
| `input` | string | no | "" | 1.0 | `ConnectionSpec::input` | Consumer input id; wiring fails without a matching input. |
| `role` | string or null | no | "" | 1.0 | `ConnectionSpec::role` | Provider role, when the input is a `HydroCouple::IMultiInput` that labels its providers; must be one of its `providerLabels()`. |

## workflow

Carried for the host; `ModelInitializer` does not build a workflow.

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `strategy` | string: "none", "pull_driven", "time_stepped" | no | "none" | 1.0 | `WorkflowSpec::strategy` | "none": the host drives the components. "pull_driven": request-reply from the trigger input. "time_stepped": a schedule derived from the connection graph. |
| `id` | string | no | "workflow" | 1.0 | `WorkflowSpec::id` | Identifier of the workflow component. |
| `trigger` | object | for "pull_driven" | | 1.0 | | The most downstream input, which starts each pull. |
| `iterations_per_group` | integer ≥ 1 | no | 1 | 1.0 | `WorkflowSpec::iterationsPerGroup` | Sweeps per cyclic (feedback) group: 1 is time-lagged, more is a fixed-point iteration. |
| `max_steps` | integer ≥ 0 | no | 0 | 1.0 | `WorkflowSpec::maxSteps` | Cap on orchestration steps; 0 runs until the workflow reports Done. |

### workflow.trigger

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `component` | string | for "pull_driven" | "" | 1.0 | `WorkflowSpec::triggerComponent` | The trigger input's component, declared in `components[]`. |
| `input` | string | for "pull_driven" | "" | 1.0 | `WorkflowSpec::triggerInput` | The trigger input's id. |

The schema requires only the `trigger` object for "pull_driven"; the parser requires both fields
and checks that the component is declared.

## writers[]

Carried for the host as @ref HydroCouple::SDK::IO::WriterSpec; the whole block, every key included,
is kept in `WriterSpec::config`. @ref sdk_guide_record describes the writers.

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `type` | string | **yes** | | 1.0 | `WriterSpec::type` | Writer kind: "csv", "hdf5_ugrid", "netcdf_ugrid" or "geopackage". Schema: one of the four; the parser accepts any string. Non-CSV writers need the matching SDK build feature. |
| `path` | string | **yes** (non-empty) | | 1.0 | `WriterSpec::path` | Destination file. |
| `simulation` | string | no | | 1.0 | `config["simulation"]` | GeoPackage simulation name. |
| `mesh` | any | no | | 1.0 | `config["mesh"]` | Mesh name or inline mesh definition for the UGRID writers; the SDK defines no format for it. |
| `fields` | array of objects | no | | 1.0 | `config["fields"]` | Per-item metadata: `item` (string, required by the schema), `units` (string), `description` (string). |
| any other key | any | no | | 1.0 | `config` | Allowed by the schema (`additionalProperties` true) and kept. |

## run

| Field | Type | Required | Default | Since | Parsed into | Meaning |
| --- | --- | --- | --- | --- | --- | --- |
| `manifest` | string | no | "" | 1.0 | `CompositionSpec::runManifest` | Path of the run manifest the host writes when the run finishes. The schema says relative paths resolve against the document; the SDK does not resolve it, the host does. Empty means no manifest. |

## What CompositionSpec::toJson() writes

| Block | Written when |
| --- | --- |
| `schema_version` | always: the spec's version, or "1.1" when a "1.0" spec contains a binding |
| `metadata` | some field is set; only the set fields |
| `components[]` | always; each with `id` and `arguments`, `caption` and `info` when set, `execution` when the mode is not "run" or `results` is set |
| `connections[]` | always (possibly empty); each `from` with `component` and `output`, `adapted_outputs` when not empty (each step with `id`, `factory` and `arguments` when set); each `to` with `component` and `input`, `role` when set |
| `workflow` | the strategy is not "none"; `strategy` and `id` always, `trigger` when a trigger component is set, `iterations_per_group` when not 1, `max_steps` when not 0 |
| `writers[]` | not empty; each block's `config` with `type` and `path` set |
| `run` | `manifest` is set |

`ModelInitializer::serialize()` writes the same shape from the live components, with
`schema_version` "1.1".
