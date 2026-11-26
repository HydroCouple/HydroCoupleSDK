@page sdk_guide_compositions 12. Compositions

[TOC]

A *composition* is a set of component instances, the values of their arguments, and the
connections between their outputs and inputs. A *composition document* writes a composition down
as JSON or YAML, so that a desktop host, a command-line runner, a test and a cloud worker all read
the same file. This chapter explains the document, the classes the SDK parses it into, how
@ref HydroCouple::SDK::IO::ModelInitializer applies it to live components, how arguments refer to
data that is not in the document (\@uri references, \@ref sidecars and \@from bindings), and every
message the IO layer produces when a document is wrong. @ref sdk_ref_composition lists every field
of the document in one table per block.

Everything in this chapter lives in the namespace `HydroCouple::SDK::IO`, in the headers
`compositionspec.h`, `modelinitializer.h`, `uriresolver.h` and `sidecar.h` under
`include/hydrocouplesdk/io/`. None of it needs an optional build feature except YAML, which needs
yaml-cpp (a core dependency of the vcpkg manifest, see @ref sdk_guide_build).

## From a document to a running composition

A document passes two checks before anything is touched, and the SDK applies only what concerns
components. The rest of the document is handed to the host as validated data.

<pre class="mermaid">
flowchart TB
    doc["composition.json or composition.yaml"]
    yaml["YAML converted to JSON"]
    parse["CompositionSpec::parse()<br/>structure: shapes, enums, required fields,<br/>declared ids, bindings, cycles"]
    semantic["ModelInitializer: semantic checks<br/>resolver has every 'run' component,<br/>argument ids exist, roles fit"]
    apply["apply components, stage by stage<br/>arguments, then initialize()"]
    wire["wire connections<br/>adaptation chains, then addConsumer()"]
    host["host builds what spec() carries<br/>workflow, writers, run manifest"]
    doc --> yaml --> parse
    doc --> parse
    parse --> semantic --> apply --> wire --> host
</pre>

- **Structural validation** is @ref HydroCouple::SDK::IO::CompositionSpec::parse. It needs no
  component and no library: a document editor or a continuous-integration job can run it on any
  file. It produces a @ref HydroCouple::SDK::IO::CompositionSpec.
- **Semantic validation** is the first half of `ModelInitializer::initialize()`. It asks the host
  for each component instance and checks the document against them. Nothing is applied until both
  checks pass.
- **Application** sets every argument, initializes every component, and wires every connection.
- The `workflow`, `writers` and `run` blocks are not applied by the SDK. The host reads them from
  `ModelInitializer::spec()` and builds the workflow (@ref sdk_guide_workflow), the output writers
  and the run recorder (@ref sdk_guide_record) itself, because writers are build-feature dependent
  and workflows are not part of the IO layer.

## The composition document

A document is one JSON object. Its blocks are:

| Key | Required | Holds | Acted on by |
| --- | --- | --- | --- |
| `schema_version` | no (read as "1.0" when absent) | Specification version, "1.x" | `CompositionSpec::parse()` |
| `metadata` | no | Provenance: id, caption, description, author, created, versions | carried |
| `components` | yes | One block per component instance | `ModelInitializer` |
| `connections` | no | Provider output to consumer input links, with optional adaptation chains | `ModelInitializer` |
| `workflow` | no | Which workflow strategy drives the run, and its settings | the host |
| `writers` | no | Output writers to construct | the host |
| `run` | no | Where the run manifest is written | the host |

This document, from the chapter's first example, uses every block:

@include ch12_composition_spec/composition.json

Read it from the top. The `terrain` component gets an inline argument. The `channel` component's
`bed_levels` argument is *bound* to the `levels` output of `terrain`: its value is whatever
`terrain` produces when it runs, so `terrain` is initialized and run first. Its `roughness`
argument is read from `roughness.json` beside the document. The `gauge` component is not run at
all: its values come from a previously recorded run (`execution.mode` "open"). The catchment's
runoff reaches the channel through a linear-transform adapted output that multiplies it by 1000.
The last three blocks tell the host to drive the run time-stepped for at most 48 steps, to record
two writers, and to write a run manifest.

### JSON and YAML

YAML documents use the same schema. `ModelInitializer::initializeFromFile()` reads a file whose
extension is exactly `.yaml` or `.yml` with yaml-cpp and converts it to JSON before anything else
happens, so a component's argument code only ever sees JSON. Every other extension is parsed as
JSON. Without yaml-cpp, a YAML file fails with the message listed under
@ref sdk_guide_compositions_diagnostics "Diagnostics"; `ModelInitializer::yamlSupported()` tells a
host in advance.

The conversion turns each YAML scalar into the narrowest JSON value that reads it: a boolean,
then a 64-bit integer, then a double, and only then a string. Quoting does not change this: the
conversion does not see YAML quotes. Two consequences matter in practice:

- `schema_version: "1.1"` becomes the number 1.1, and the parser refuses it ("\"schema_version\"
  must be a string"). Leave `schema_version` out of YAML documents. A document without it is read
  as 1.0, and 1.0 documents may use every 1.1 feature, argument bindings included (the parser does
  not gate features by version).
- A string argument value that looks like a number or a boolean (a gauge id `"0042"`, a flag
  `"yes"`) arrives at the argument as a number or a boolean. Use JSON for such values.

The YAML file of the second example, `ch12_model_initializer/composition.yaml`, shows the form:

@include ch12_model_initializer/composition.yaml

### The schemas, versions 1.0 and 1.1

The SDK ships JSON Schema documents (draft 2020-12) for editors and external tools. They are in
`share/schema/` in the source tree and in `share/hydrocouplesdk/schema/` of an installed SDK:

| File | `$id` | Default `schema_version` | Adds |
| --- | --- | --- | --- |
| `composition-1.0.json` | https://hydrocouple.org/schema/composition-1.0.json | "1.0" | the base specification |
| `composition-1.1.json` | https://hydrocouple.org/schema/composition-1.1.json | "1.1" | argument bindings |

Version 1.1 differs from 1.0 in one feature. Its `arguments` object validates any payload that
carries the key \@from against a new definition, `argumentBinding`: an object with exactly one key,
\@from, whose value has a required non-empty `component` and `output` and an optional `selector`
that must be null. Version 1.0 accepts any argument payload. Everything else in the two files is
the same apart from formatting.

The SDK writes version "1.1" (`kCompositionSchemaVersion`). It reads any document whose version
starts with "1" before the first period, so "1.0", "1.1", "1.7" and also "1" are accepted, and "2.0"
is refused. `CompositionSpec::toJson()` writes back the version it read, except that a document
read as "1.0" that contains an argument binding is written as "1.1", so that a strict 1.0 reader is
never told it can read a binding. `ModelInitializer::serialize()` always writes "1.1".

The parser, not the schema, decides what the SDK accepts, and the two differ in both directions:

- The parser **ignores** keys it does not know. The schemas set `additionalProperties` to false on
  the top level and on most blocks, so a schema validator refuses a misspelled key the SDK would
  silently skip. Run a schema validator in your editor or CI when typos matter.
- The parser accepts any string as a writer `type`, any string as `metadata.created`, and an empty
  string as a component id. The schemas restrict these to the four writer kinds, a date-time and a
  non-empty id.
- The parser **adds** checks a schema cannot express: every connection endpoint, binding provider
  and pull-driven trigger names a declared component; component ids are unique; bindings form no
  cycle; a pull-driven workflow names both a trigger component and a trigger input (the schema
  requires only the `trigger` object).

## The parsed form: CompositionSpec

@ref HydroCouple::SDK::IO::CompositionSpec is the typed form of a document. It and its parts are
plain value types: they hold no live objects, copy freely, and compare member by member.

| Type | Document part | Holds |
| --- | --- | --- |
| @ref HydroCouple::SDK::IO::CompositionSpec | the whole document | `schemaVersion`, `metadata`, `components`, `connections`, `workflow`, `writers`, `runManifest` |
| @ref HydroCouple::SDK::IO::CompositionMetadata | `metadata` | seven optional strings; `empty()` |
| @ref HydroCouple::SDK::IO::ComponentSpec | one `components[]` block | `id`, `caption`, `info`, `mode`, `resultsManifest`, `arguments` (JSON) |
| @ref HydroCouple::SDK::IO::ComponentInfoRef | a block's `info` | `library`, `componentInfoId`; `empty()` |
| @ref HydroCouple::SDK::IO::ConnectionSpec | one `connections[]` block | `fromComponent`, `output`, `toComponent`, `input`, `role`, `adaptedOutputs` |
| @ref HydroCouple::SDK::IO::AdaptedOutputSpec | one `adapted_outputs[]` step | `factory`, `id`, `arguments` (JSON) |
| @ref HydroCouple::SDK::IO::WorkflowSpec | `workflow` | `strategy`, `id`, `triggerComponent`, `triggerInput`, `iterationsPerGroup`, `maxSteps` |
| @ref HydroCouple::SDK::IO::WriterSpec | one `writers[]` block | `type`, `path`, `config` (the whole block) |
| @ref HydroCouple::SDK::IO::ArgumentBindingSpec | an \@from payload | `component`, `argument`, `provider`, `output` (derived, never stored) |
| @ref HydroCouple::SDK::IO::ExecutionMode | `execution.mode` | `Run`, `Open`, `Resume` |
| @ref HydroCouple::SDK::IO::WorkflowStrategy | `workflow.strategy` | `None`, `PullDriven`, `TimeStepped` |

Argument payloads stay JSON (`nlohmann::json`) in the spec, exactly as authored, because only the
argument that receives a payload knows what it means.

### Parsing and inspecting a document

The example `ch12_composition_spec` reads the document above and prints what the parser found. It
links no component.

@snippet ch12_composition_spec/main.cpp parse

`parse()` resets the spec first. On failure it returns false and the message names the offending
element by its path in the document, for example
`connections[0].to.component "ghost" is not declared in components[]`. On success the message is
cleared. A spec is then read field by field:

@snippet ch12_composition_spec/main.cpp inspect

The helpers `executionModeName()`/`executionModeFromName()` and
`workflowStrategyName()`/`workflowStrategyFromName()` convert between the enumerations and their
document spellings ("run", "open", "resume"; "none", "pull_driven", "time_stepped"). Spellings are
exact and case-sensitive. `CompositionSpec::component(id)` returns the block with that id or
nullptr.

The arguments of a component are listed in the order `nlohmann::json` stores object keys, which is
sorted by key, not the order of the document. `ModelInitializer` applies them in that order too, so
an argument must not depend on another argument of the same component having been applied first.

### Writing a spec back

`CompositionSpec::toJson()` is the symmetric write side. It always writes `schema_version`,
`components` (every block with its `arguments`, possibly empty) and `connections` (possibly an
empty array). It writes `metadata`, `info`, `execution`, `workflow`, `writers` and `run` only when
they hold something: `execution` when the mode is not "run" or a results path is set, `workflow`
when the strategy is not "none", and inside `workflow` only the values that differ from their
defaults. A minimal document therefore round-trips to a minimal document.

@snippet ch12_composition_spec/main.cpp roundtrip

`ComponentSpec`, `ConnectionSpec`, `AdaptedOutputSpec` and `ComponentInfoRef` compare member by
member, argument JSON and adaptation chains included. This is change detection for a host that
diffs two documents (a canvas deciding what to rebuild), not identity: a component's identity is its
`id`, and a connection's identity is its two endpoints plus its role. Two connections on the same
endpoints and role would wire the same input twice, so hosts deduplicate by endpoints and treat
the chain as content.

@snippet ch12_composition_spec/main.cpp version

### Output

Copied from the ctest run of `docs.example.ch12_composition_spec`:

```text
schema_version 1.1, composition "Lower Creek coupled model" by HydroCouple SDK manual
component terrain: mode run, library libterrain_component.dylib, elevations (inline)
component catchment: mode run, coefficients (inline)
component channel: mode run, bed_levels (binding), roughness (uri)
component gauge: mode open, results prior/run.json
connection catchment.runoff -> channel.lateral_inflow via hydrocouple.sdk.adapters/linear_transform
connection gauge.flow -> channel.boundary_flow
workflow time_stepped "lower-creek-workflow", 2 iterations per group, 48 steps at most
writer csv -> out/results.csv
writer geopackage -> out/results.gpkg
run manifest out/run.json
binding channel.bed_levels <- terrain.levels
stage 0: terrain catchment gauge
stage 1: channel
round trip preserves every component and connection
minimal document: read as 1.0, written as "1.0"
document with a binding: read as 1.0, written as "1.1"
refused: Unsupported composition schema_version "2.0"; this SDK reads version 1.x
refused: Composition document must be an object with a "components" array
refused: Duplicate component id "a" in components[]
refused: components["a"].execution.mode must be one of "run", "open", "resume" (got "replay")
refused: components["a"].execution: mode "open" requires a "results" run manifest path
refused: connections[0].to.component "ghost" is not declared in components[]
refused: components["a"].arguments.x.@from binds "a" to its own output
refused: Argument bindings form a cycle: "a" (argument "x") needs "b"; "b" (argument "y") needs "a".
refused: workflow.strategy "pull_driven" requires workflow.trigger.component and .input
refused: writers[0] ("csv") requires a "path"
all checks passed
```

The refusals come from this table of broken documents in the same program:

@snippet ch12_composition_spec/main.cpp errors

## Component blocks

A component block names one instance. Its `id` is required and unique within the document; every
other part of the document refers to the instance by it.

- `caption` is an optional display name. The SDK carries it; it does not set the component's
  caption.
- `info` (@ref HydroCouple::SDK::IO::ComponentInfoRef) says how a host without a code-side
  registry could create the instance: `library`, a shared-library path, and `component_info_id`,
  the id of the `HydroCouple::IComponentInfo` inside it. Naming a library makes a document
  executable content. The SDK never loads a library, neither while validating nor while
  initializing: it only calls the host's resolver (see
  @ref sdk_guide_compositions_instances "Supplying component instances"). A host that loads libraries
  named in documents must treat that as opt-in and check them against an allow list.
- `execution` sets the *execution mode*, what the component does when the composition is
  initialized:

| `execution.mode` | Meaning | Requires |
| --- | --- | --- |
| "run" (default) | Compute normally. Results are never silently reused, which is what keeps a rerun reproducible from its inputs. | an instance from the resolver |
| "open" | Serve previously recorded results and finish immediately. | `execution.results`, the path of a run manifest |
| "resume" | Restart from a checkpoint. Reserved: the parser accepts it and `ModelInitializer` refuses it as not yet implemented. | |

- `arguments` maps argument ids to payloads; the next section lists the payload forms.

### Argument payloads

Arguments are the unit of persistence in HydroCouple: a component's whole configuration flows
through its `arguments()`, and the SDK never serializes a component's internals. Each payload in
a block's `arguments` object takes one of four forms:

| Form | Example | What happens |
| --- | --- | --- |
| Inline value | `{ "values": [0.35, 0.20, 0.55] }` | The payload is passed to `IArgument::initialize(text, JSON, message)`. What it may contain is up to the argument; the SDK's typed arguments accept an object with `values` or a bare array (@ref sdk_guide_ports). |
| \@uri reference | `{ "@uri": "roughness.json" }` | The argument reads the named file or URI and parses its content as it would an inline value. A relative path is rebased on the document's directory first. See @ref sdk_guide_compositions_uri "URI references". |
| \@ref sidecar | `{ "@ref": "channel_bed.bin", "kind": "Float64", "shape": [10000000] }` | The initializer reads the binary file and hands it to the argument as a read-only data item; bulk values never enter the JSON tree. See @ref sdk_guide_compositions_sidecar "Sidecar references". |
| \@from binding | `{ "@from": { "component": "terrain", "output": "levels" } }` | The value is another component's output: the provider runs to completion first. See @ref sdk_guide_compositions_bindings "Argument bindings". |

An argument id that does not exist on the component is refused before anything is applied
("Component \"channel\" has no argument \"friction\""). Arguments that the document does not
mention keep the values the component gave them.

## Connections

A connection block links a provider output (`from.component`, `from.output`) to a consumer input
(`to.component`, `to.input`). The parser checks that both components are declared; the output and
input ids are checked when the connection is wired, because exchange items exist only after a
component has initialized.

- `to.role` names a provider role when the input is a `HydroCouple::IMultiInput` that
  distinguishes its providers (an upstream and a downstream boundary, for example). The role must
  be one of the input's `providerLabels()`. Leave it out for single inputs and for multi-inputs
  that do not use roles. A role given for an input that is not a multi-input is refused when the
  target component already has its inputs at validation time, and ignored otherwise (the input is
  then wired without a role).
- `from.adapted_outputs` is an ordered *adaptation chain*: each step wraps the output before it,
  and the consumer is wired to the last step. A step names the adapted output `id` its factory
  offers, optionally the `factory` id, and the step's `arguments`. @ref sdk_guide_adapted_outputs
  describes the SDK's adapted outputs; how `ModelInitializer` finds them is described
  @ref sdk_guide_compositions_chains "below".

## Workflow, writers and run

These three blocks are validated and carried, never acted on by the SDK.

- `workflow` (@ref HydroCouple::SDK::IO::WorkflowSpec) chooses a strategy: "none" (the host drives
  the components itself; the default), "pull_driven" (request-reply from a trigger input, which
  must be named by `trigger.component` and `trigger.input`), or "time_stepped" (a schedule derived
  from the connection graph). `iterations_per_group` (an integer of at least 1, default 1) is the
  number of sweeps per cyclic group of a time-stepped workflow: 1 gives a time-lagged coupling,
  more give a fixed-point iteration. `max_steps` (default 0, meaning until the workflow reports
  Done) caps the number of orchestration steps the host runs. @ref sdk_guide_workflow describes the
  workflows. No SDK class reads this block: `ModelInitializer` only parses it, and building and
  configuring a workflow from it is the host's code. HydroCouple Composer's run setup reads only
  `strategy`, creating a `PullDrivenWorkflow` for "pull_driven" and a `TimeSteppedWorkflow` for
  every other value ("none" included); `iterations_per_group`, `max_steps` and `trigger` are
  edited in its execution panel but not applied by its run setup.
- `writers` lists output writers (@ref HydroCouple::SDK::IO::WriterSpec): `type` is "csv",
  "hdf5_ugrid", "netcdf_ugrid" or "geopackage" and `path` the destination; any other key
  (`simulation`, `mesh`, `fields`, or a key of your own) is kept in `WriterSpec::config`, which
  holds the whole block. @ref sdk_guide_record describes the writers and what a host does with
  each block.
- `run.manifest` (`CompositionSpec::runManifest`) is the path of the run manifest the host writes
  with `RunRecorder::finishToFile()`. An empty value means the host records no manifest.

## Applying a document: ModelInitializer {#sdk_guide_compositions_initializer}

@ref HydroCouple::SDK::IO::ModelInitializer reads a document, validates it against live
components, applies it, and writes the symmetric document back out. It is constructed with a
*resolver*, a function from a document id to a live `HydroCouple::IModelComponent` that the host
owns. The initializer never creates or loads a model component; the only components it creates are
the substitutes for "open" blocks, which it owns.

The second example, `ch12_model_initializer`, has two small component classes in `components.h`:
a `Producer` with an argument `scale` and an output `series` (each update sets
`series[i] = scale[i] * step`, and it reports Done after three steps), and a `Consumer` with
arguments `initial` and `weights` and an input `inflow`. Its document:

@include ch12_model_initializer/composition.json

The host keeps one instance per document id and hands the initializer a lookup:

@snippet ch12_model_initializer/main.cpp instances

and applies the document:

@snippet ch12_model_initializer/main.cpp initialize

`initializeFromFile()` reads the file, records its directory as the *document directory* (the
base for relative resources, see below), converts YAML, and calls `initialize()`. The three entry
points differ only in what they start from:

| Function | Starts from | Document directory | Exceptions |
| --- | --- | --- | --- |
| `initializeFromFile(path, message)` | a file; YAML by extension | set to the file's absolute parent | caught, reported as a message |
| `initializeFromJsonText(text, message)` | JSON text | unchanged (see `setDocumentDirectory()`) | caught, reported as "JSON parse error: ..." |
| `initialize(document, message)` | a parsed `nlohmann::json` | unchanged | propagate |

The prefix "JSON parse error" is used for every `std::exception` caught by
`initializeFromJsonText()`, including type errors raised later by a malformed sidecar reference,
so read the rest of the message.

### What initialize() does, in order

<pre class="mermaid">
sequenceDiagram
    participant Host
    participant Init as ModelInitializer
    participant Provider as Bound provider
    participant Comp as Component
    participant Out as Provider output
    Host->>Init: initializeFromFile(path)
    Init->>Init: CompositionSpec::parse()
    Init->>Host: resolver(id) for every block
    Init->>Init: semantic checks (nothing applied yet)
    loop each stage, each component in document order
        Init->>Comp: argument->initialize(payload)
        opt the argument is a binding
            Init->>Provider: validate(), prepare(), update() until Done, finish()
            Init->>Comp: argument->initialize(provider output)
        end
        Init->>Comp: initialize()
    end
    loop each connection
        Init->>Init: build the adaptation chain
        Init->>Out: addConsumer(input, role)
    end
    Init-->>Host: true, or false and a message
</pre>

1. **Parse** with `CompositionSpec::parse()`. The result replaces `spec()`.
2. **Check** against live components. The resolver is asked for every component. A "run" block
   without an instance fails ("Unknown component id ..."); an "open" block may have none. A
   "resume" block fails. Every argument id must exist on its component. A connection whose
   endpoints have no instance and are not "open" blocks fails; a role is checked against the
   target input's provider labels when the target already has inputs. A component in the Created
   state has none yet, so for it a multi-input role is checked at wiring time, and a role on an
   input that turns out not to be a multi-input is ignored.
3. **Apply components** in binding stages (see @ref sdk_guide_compositions_bindings "Argument bindings").
   For a "run" block, every argument payload is applied, then the component's `initialize()` runs
   and the component must reach Initialized; otherwise the message lists the component's own
   errors. For an "open" block, see @ref sdk_guide_compositions_open "Execution mode "open"".
4. **Wire connections** in document order: look up the output and input (both must exist now),
   build the adaptation chain, resolve the role, and call `addConsumer(input, role)` on the last
   output of the chain. `addConsumer()` checks that the input can consume the output; an exception
   it throws becomes the message.

If any step fails, `initialize()` returns false. Components already applied stay as they are: the
initializer does not roll back. Call `initialize()` once per initializer: each call replaces
`spec()` but adds to `openedComponents()` and `adaptedOutputs()` instead of clearing them.

After a successful call, `spec()` holds what the host must act on, and `component(id)` returns the
live instance behind an id (the substitute for an "open" block, otherwise the resolver's
instance):

@snippet ch12_model_initializer/main.cpp inspect

### Supplying component instances {#sdk_guide_compositions_instances}

The resolver (`ModelInitializer::ComponentResolver`) is called during validation and again during
application, sometimes several times for one id. It must:

- return the same host-owned instance for the same id every time, and nullptr for an id it does not
  know;
- be callable: an empty `std::function` makes the first call throw `std::bad_function_call`;
- return instances whose `id()` equals the document id. The initializer looks instances up by the
  document id, but `serialize()` and @ref HydroCouple::SDK::IO::RunRecorder use `id()`, so a
  mismatch produces documents and manifests that name the wrong component;
- return instances in the Created state, so that their arguments can still be set, and keep them
  alive as long as the composition runs.

A host with a static set of components (a test, a dedicated runner such as the serial example of
@ref sdk_guide_example) writes the lookup by hand, as above. A *plugin-backed host* creates the
instances from component libraries before calling the initializer. HydroCouple Composer does
this: for each component block it asks its component registry for a new instance of the component
info named by `info.component_info_id` (or by the block id when there is none), gives the instance
the document id while it is still Created, sets its reference directory to the document's
directory, stores it in a map, and passes a resolver that looks up that map. The library-loading convention (the
`hydrocouple_component_info_v1` entry point and its ABI stamp) is described in
@ref sdk_guide_build. Whatever loads libraries is the host's code, under the host's allow list.

### Execution mode "open" {#sdk_guide_compositions_open}

An "open" block says the component's values come from a recorded run. When the block is applied:

1. If the resolver has an instance, its `initialize()` is called. A component that loads its own
   results and reaches Finished during `initialize()` is taken at its word and used as it is.
2. Otherwise, or when the instance does not reach Finished, the initializer opens a
   @ref HydroCouple::SDK::ResultsModelComponent from the manifest named by `execution.results`
   (a relative path resolves against the document directory). It exposes the manifest's entries for
   the block's id, takes that id, and must reach Finished. The substitute is owned by the
   initializer and listed by `openedComponents()`. Every reference to the id, connections and
   bindings included, resolves to it.

The arguments of an "open" block are checked against the resolver's instance but never applied:
the component is initialized with the values it already has. The absence of an instance is normal:
an analysis program that links no model at all can reopen a run this way, which is what
@ref sdk_guide_record shows with `compositionForRun()`.

### Argument bindings {#sdk_guide_compositions_bindings}

A binding (schema 1.1) makes an argument's value the output of another component, for example a
mesh produced by a mesh generator or an initial state produced by a spin-up model. Write it as an
object whose only key is \@from:

```json
{ "@from": { "component": "profile", "output": "series" } }
```

`component` and `output` are required non-empty strings; `selector` is reserved and must be null
(a value is refused as not yet implemented). The provider must be declared in the document, may be
declared after its consumer, and must not be the consumer itself.

*Binding stages* order initialization. `bindingStages(spec, stages, message)` groups the component
ids so that stage N holds the components whose longest chain of providers is N deep; a
binding-free composition is one stage in document order. The initializer applies stage by stage,
so a provider is always applied before its consumers. Bindings that form a cycle are refused and
the message names every edge of the cycle. `argumentBindings(spec)` lists every binding as an
@ref HydroCouple::SDK::IO::ArgumentBindingSpec, derived from the argument payloads each time it is
called, so a spec edited through its arguments cannot drift from a cached list.

When the consumer's bound argument is applied, the provider is run to Finished, unless it is
already Finished (an "open" substitute, or a provider an earlier binding ran):
`validate()` (any problem fails the binding and is listed), `prepare()`, `update()` until the
status is Done, Finished or Failed, and `finish()` when Done. A provider that has not stopped after
1,000,000 updates fails with a named error instead of hanging. The provider's output is then
passed to `IArgument::initialize(const IComponentDataItem&)`, which copies its values (the SDK's
1-D argument resizes to the output's length). A provider that fails, or an output the provider
does not have, fails the consumer's apply with a message naming the component, the argument and
the provider.

A provider is run once however many arguments bind to it. Because it is Finished afterwards, it
cannot also take part in a time-stepped run as a live component; use a binding for values that
are produced once, before the run.

`setProgressCallback()` reports the staged apply as it happens, on the initializing thread:
`(stage, stageCount, componentId, activity)` with a zero-based stage, the activity "initializing"
when a component's apply begins and "running provider" (with the provider's id, reported in the
consumer's stage) for every binding. Composer turns it into "stage 1 of 2" progress messages.

The serialized document keeps the binding, never the value it produced, so the next run reruns the
provider rather than reusing a stale result.

### Adaptation chains {#sdk_guide_compositions_chains}

For each step of a connection's `adapted_outputs`, the initializer collects candidate factories:
first the `adaptedOutputFactories()` of the component info of the current output's component (a
component may deliberately shadow a standalone factory id this way), then the standalone factories
returned by the resolver installed with `setAdapterFactoryResolver()`. It takes the first candidate
whose id equals the step's `factory` (any candidate, when the step names none) and that offers the
step's `id` for this output and input in `getAvailableAdaptedOutputIds()`, creates the adapted
output, applies the step's arguments, and initializes it. The created adapted outputs are owned by
the initializer (`adaptedOutputs()`) and live as long as it does.

A host that wants the SDK's own adapted outputs (linear transform, relaxation, temporal
interpolation) installs `SdkAdaptedOutputFactory::instance()`, as the example does; Composer adds
one factory per loaded adapter library. Without any candidate, a chain fails with "adapted output
... requested but the provider's component exposes no adapter factories and no standalone adapter
factories are installed".

### Running the example

Copied from the ctest run of `docs.example.ch12_model_initializer`:

```text
stage 1/2: initializing profile
stage 1/2: initializing upstream
stage 2/2: initializing channel
stage 2/2: running provider profile
profile is Finished; channel.initial = [30, 60, 90]
upstream.scale = [2, 4, 6] (from scale.json)
channel.weights = [0.5, 0.25, 0.25] (from memory:weights)
channel.inflow provider: linear_transform#1
host must build: workflow time_stepped, 1 writer(s), run manifest out/run.json
validateDocument: Component "channel" has no argument "friction"
written back: channel.initial = {"@from":{"component":"profile","output":"series"}}
              channel.weights @uri = "memory:weights"
              upstream.scale values = [2.0,4.0,6.0]
              chain step = "linear_transform"
composition.yaml applied the same values: [30, 60, 90]
all checks passed
```

`profile` is in the first stage with `upstream`; `channel` waits for it. When `channel`'s binding
is applied, `profile` runs its three steps, so `initial` receives `10, 20, 30` times 3. `upstream`
has no consumer binding, so it stays Initialized, ready for the workflow. The adapted output's id
is the factory's offering id plus an ordinal, `linear_transform#1`.

### Validating without applying

`validateDocument(document, message)` runs the parse and the semantic checks and changes nothing,
not even `spec()`. It calls the resolver but never applies an argument, so a document editor can
check a document against the components it has, as often as it likes:

@snippet ch12_model_initializer/main.cpp validate

Checks that need initialized components (does the output exist, can the input consume it) happen
only in `initialize()`.

## Writing a composition back

`serialize(componentIds, message)` produces the document that describes the live composition, and
`serializeToFile(componentIds, path, message)` writes it as indented JSON (creating missing
directories, overwriting an existing file). The ids give the order of the components in the
output.

- **Arguments** come from each component's `arguments()`, through `IArgument::serialize(JSON)`.
  An argument that was a binding in the last applied document is written as that binding. An
  argument read from a remote URI is written with its \@uri (the SDK's arguments also write their
  current values beside it, which a later read ignores in favor of the URI). An argument read from
  a local file is written as its values, so the document no longer depends on the file.
- **Connections** are reproduced from the live wiring of each component's inputs, chains included:
  the initializer walks from the input's provider back through the adapted outputs to the
  provider component's output, and writes each step with its factory and the *live* arguments of
  the adapted output, so a value changed after wiring is kept. Roles and offering ids come from
  the authored connection with the same endpoints.
- **Everything else** — caption, `info`, `execution`, `metadata`, `workflow`, `writers` and `run` —
  is copied from `spec()`, and `schema_version` is "1.1".

@snippet ch12_model_initializer/main.cpp serialize

Each id is looked up through the resolver, not through `component()`. A composition reopened from
a recorded run with a resolver that knows none of its components therefore cannot be serialized:
`serialize()` returns a null JSON value and the message
`serialize: unknown component id "gauge"`. Serialize `compositionForRun()`'s spec with
`CompositionSpec::toJson()` instead.

## URI references {#sdk_guide_compositions_uri}

An argument payload `{ "@uri": "..." }` names where the argument's value is, instead of holding it.
The SDK's typed arguments (1-D and 2-D arguments, id-based arguments, time-series arguments and
the mesh argument) recognize it (`jsonUriReference()`), read the reference, and parse what they
read as they would an inline value. Other keys in the object are ignored: when the reference cannot
be read, the argument fails rather than falling back to stale values beside it.

`readReference(reference, contents, message)` is the single function that reads a reference. What
it does depends on the reference's scheme, which `uriScheme()` extracts: the text before the first
colon, lower-cased, when it is at least two characters long, starts with a letter and holds only
letters, digits, '+', '-' and '.'. The two-character minimum keeps a Windows path such as
`C:\data\flow.json` a path.

| Reference | Read by | Notes |
| --- | --- | --- |
| a plain path, absolute or relative | the SDK, in binary mode | A relative path in a payload read through `ModelInitializer` is first rebased on the document directory (`resolvedArgument()`); read any other way it is relative to the current directory. |
| `file:///path`, `file://localhost/path`, `file:/path` | the SDK | `/C:/...` loses its leading slash; a `file://host/...` naming another host fails ("Not a local file URI"). |
| any other scheme (`https:`, `s3:`, `sensor:`...) | the installed @ref HydroCouple::SDK::IO::UriResolver | Without a resolver that `handles()` the scheme, the read fails with "No resolver is installed for '...' URIs, so this argument cannot read ...". |

The SDK has no HTTP client, and deliberately no transport stack under every model that links it.
It defines what resolving a URI means and leaves the doing to the host. A host implements
`UriResolver` (two functions: `handles(scheme)` and `resolve(uri, contents, message)`) and installs
it once, before any component is initialized:

@snippet ch12_model_initializer/main.cpp resolver

@snippet ch12_model_initializer/main.cpp install

`setUriResolver()` keeps a non-owning pointer in a plain global: the resolver must outlive every
component that may read a URI, and the pointer must not be changed while another thread may be
reading. `setUriResolver(nullptr)` removes it. HydroCouple Composer installs a resolver backed by
HydroCoupleOgc's fetch layer; a command-line runner may install none and get the refusal above
instead of a misleading "Cannot open file".

Which references are written back depends on `isWritableReference()`, which is true for plain
paths and `file:` URIs. An argument read from a writable reference remembers the path so that
`saveData()` can write the values back, and serializes its values inline; an argument read from a
remote reference remembers the URI (`sourceUri()`) and serializes it as \@uri, so a saved
composition reopens with a fresh read from the service. Setting values outright clears the
remembered URI.

## Sidecar references {#sdk_guide_compositions_sidecar}

A *sidecar* keeps a large numeric argument out of the document. When a host calls
`setSidecarDirectory(directory)`, `serialize()` writes every argument with a fixed-width element
kind whose values exceed the threshold (`setSidecarThresholdBytes()`, default 1024 bytes, element
size times element count) to `directory/<component>_<argument>.bin` and writes a reference in its
place:

```json
{ "@ref": "channel_bed_levels.bin", "kind": "Float64", "shape": [10000000], "byteOrder": "little" }
```

The file holds the values as raw contiguous bytes in the writing machine's byte order, written in
one stream write through the typed data plane (`writeSidecar()`). On initialization, a payload that
carries \@ref (directly or under `values`, `isSidecarRef()`) is parsed (`parseSidecarRef()`), its
relative path resolved against the sidecar directory (or the document directory, or the current
directory, in that order), the file read whole and checked to hold exactly
element size × element count bytes (`readSidecarBytes()`), and the bytes handed to the argument as
a read-only data item through `IArgument::initialize(const IComponentDataItem&)`. A ten-million
element argument therefore never becomes a ten-million element JSON array.

`byteOrder` is written as "little" and not read back; the bytes are copied without conversion, so a
sidecar is portable only between machines of the same byte order. Kinds are spelled "Float64",
"Float32", "Int64", "UInt64", "Int32", "UInt32", "Int16", "UInt16", "Int8", "UInt8" and "Boolean".

## Relative paths

| Path | Resolves against |
| --- | --- |
| an \@uri plain path in an argument or chain-step payload | the document directory, when one is set |
| an \@ref sidecar path | the sidecar directory, else the document directory, else the current directory |
| `execution.results` of an "open" block | the document directory, when one is set |
| writer `path` and `run.manifest` | whatever the host decides; the SDK does not resolve them |

The document directory is set by `initializeFromFile()` to the file's absolute parent, and by
`setDocumentDirectory(directory)` for a document edited in memory (Composer sets it to the
document's folder). It is stored absolute and normalized; an empty value means the current
directory. Loading does not rewrite the document: `spec()` keeps the relative references as they
were authored, so a saved document stays portable.

## Diagnostics {#sdk_guide_compositions_diagnostics}

Every failure is a false return with a message; none of the functions in this chapter writes to a
log. Messages that name a document element use its path, `components["id"]` for a component and
`connections[i]` for a connection (zero-based).

### CompositionSpec::parse()

| Message | Cause | Fix |
| --- | --- | --- |
| Composition document must be a JSON object | The root is an array, a string or a number. | Make the root an object. |
| "schema_version" must be a string (e.g. "1.0") | A numeric version, often from YAML. | Quote it in JSON; leave it out of YAML. |
| Unsupported composition schema_version "X"; this SDK reads version 1.x | The major version is not 1. | Use a 1.x document or a newer SDK. |
| "metadata" must be an object | | |
| metadata.KEY must be a string | A metadata field of another type. | |
| Composition document must be an object with a "components" array | `components` is missing or not an array. | Add `"components": [...]`. |
| Every components[] entry must be an object with a string "id" | | Give every block an `id`. |
| Duplicate component id "X" in components[] | Two blocks share an id. | Rename one. |
| components["X"].info must be an object / .execution must be an object | | |
| components["X"].execution.mode must be one of "run", "open", "resume" (got "Y") | A misspelled mode. | |
| components["X"].execution: mode "open" requires a "results" run manifest path | | Add `execution.results`. |
| components["X"].arguments must be an object of argumentId → payload | | |
| ... is a binding, so "\@from" must be its only key — a reference carries no other payload | A binding object with other keys. | Remove the other keys. |
| ....\@from must be an object with string "component" and "output" | | |
| ....\@from.component must be a non-empty string (also .output) | | |
| ....\@from.selector is reserved and not yet implemented; bind the whole output | A non-null selector. | Remove it. |
| ....\@from carries unknown key "K" | | |
| ....\@from must name a "component" and an "output" | | |
| ....\@from.component "X" is not declared in components[] | The provider has no block. | |
| ....\@from binds "X" to its own output | A self-binding. | |
| Argument bindings form a cycle: "a" (argument "x") needs "b"; ... | Providers depend on each other. | Break the cycle. |
| "connections" must be an array / connections[i] must be an object | | |
| connections[i] must carry "from"/"to" objects with a string "component" id | | |
| connections[i].from.component "X" is not declared in components[] (also .to) | | |
| connections[i].from.adapted_outputs must be an array | | |
| connections[i].from.adapted_outputs[j] must be an object with a string "id" | | |
| "workflow" must be an object | | |
| workflow.strategy must be one of "none", "pull_driven", "time_stepped" (got "Y") | | |
| workflow.iterations_per_group must be an integer ≥ 1 | 0, a negative or a fractional value. | |
| workflow.max_steps must be a non-negative integer | | |
| workflow.strategy "pull_driven" requires workflow.trigger.component and .input | | Add `trigger`. |
| workflow.trigger.component "X" is not declared in components[] | | |
| "writers" must be an array / writers[i] must be an object with a string "type" | | |
| writers[i] ("T") requires a "path" | A writer without a path, or an empty one. | |
| "run" must be an object | | |
| CONTEXT.KEY must be a string / must be an object | Any optional field of the wrong JSON type. | |

### ModelInitializer

| Message | Cause | Fix |
| --- | --- | --- |
| Cannot open composition document: PATH | The file does not exist or is unreadable. | |
| JSON parse error: ... | Invalid JSON, or any exception thrown while applying (see above). | |
| YAML parse error in PATH: ... | Invalid YAML, or any exception thrown while applying a YAML document. | |
| Composition document 'PATH' is YAML, but this build has no yaml-cpp support. ... | SDK built without yaml-cpp. | Rebuild with the vcpkg manifest or use JSON. |
| Unknown component id "X" — the resolver returned no instance for it | A "run" block the host cannot supply. | Fix the id or the resolver. |
| Component "X": execution mode "resume" is reserved and not yet implemented (see ICheckpointableModelComponent) | | Use "run" or "open". |
| Component "X" has no argument "A" | A misspelled argument id. | |
| connections[i]: unknown component id | An endpoint with no instance that is not an "open" block. | |
| connections[i].to.role "R" is not a provider role of input "I" | | Use one of the input's provider labels. |
| connections[i].to.role was given but input "I" is not an IMultiInput | | Remove the role. |
| Component "X", argument "A": ... | The argument refused its payload; the rest is the argument's message. | |
| Component "X", argument "A": provider "P" has no live instance | | |
| Component "X", argument "A": provider "P" failed validation: (list) | | |
| Component "X", argument "A": provider "P" did not report Done within 1000000 update steps | | |
| Component "X", argument "A": provider "P" failed while producing its outputs (list) | | |
| Component "X", argument "A": provider "P" has no output "O" | | |
| Component "X", argument "A" rejected the bound output "O": ... | Wrong rank or kind for the argument. | |
| Component "X" failed to reach Initialized after initialize() (list) | The component's own initialization failed; its errors follow. | |
| Component "X" (execution mode "open"): Cannot open run manifest "PATH" (and other manifest errors) | | See @ref sdk_guide_record. |
| Component "X" (execution mode "open") failed to load its recorded results (list) | The substitute could not open its artifacts. | |
| connections[i]: component "X" has no output "O" / has no input "I" | | |
| connections[i]: adapted output "S" requested but the provider's component exposes no adapter factories and no standalone adapter factories are installed | | `setAdapterFactoryResolver()`. |
| connections[i]: no adapted output "S" is offered for output "O" (searched N factories on component "X" and M standalone factories, factory "F" requested) | No factory offers that id for this pair. | |
| connections[i]: factory "F" returned no adapted output for "S" | | |
| connections[i]: adapted output "S" has no argument "A" / , argument "A": ... | | |
| connections[i]: input "I" has no provider role "R" | | |
| connections[i]: (exception text from addConsumer) | The input cannot consume the output. | See @ref sdk_guide_ports. |
| serialize: unknown component id "X" | The resolver does not know the id (an opened substitute, for example). | |
| serialize: component "X", argument "A": ... | An argument failed to serialize or to write its sidecar. | |
| Cannot open 'PATH' for writing | `serializeToFile()` destination. | |

### References and sidecars

| Message | Cause |
| --- | --- |
| Cannot open file: PATH | A local path or `file:` URI that does not exist. |
| Not a local file URI: URI | A `file://host/...` naming another host. |
| No resolver is installed for 'SCHEME' URIs, so this argument cannot read REFERENCE | No resolver, or one that does not handle the scheme. |
| (the resolver's message) | The resolver refused. |
| Sidecar reference must carry "\@ref", "kind", and "shape" | |
| Sidecar reference has unknown kind "K" | |
| Cannot open sidecar 'PATH' | |
| Sidecar 'PATH' holds N bytes but kind/shape describe M | The file does not match the reference. |
| Cannot open sidecar 'PATH' for writing / Stream failure writing sidecar 'PATH' | |
