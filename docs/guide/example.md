@page sdk_guide_example 4. Tutorial: two coupled components

[TOC]

This tutorial runs a coupled model end to end: a catchment that turns rainfall into runoff on three
reaches, and a channel that routes that runoff downstream. A composition document configures both
components and connects them; a workflow decides the order they run in; the results are written to
files while the model runs; and a run manifest records the run so that a separate program, with no
model code at all, can reopen and analyze it.

The code is the SDK's runnable example `examples/serial_coupling/` and its companion
`examples/analysis_reopen/`. The test suite runs both (`ctest -R examples.`), so the commands and
output on this page are current. @ref sdk_guide_first_component explains the pieces of a single
component; this page is about what happens between components.

## What runs

<pre class="mermaid">
flowchart LR
    doc["composition.json / composition.yaml"] --> init["ModelInitializer"]
    init -->|"arguments, initialize()"| catchment
    init -->|"arguments, initialize()"| channel
    catchment -->|"runoff → lateral_inflow"| channel
    wf["TimeSteppedWorkflow"] -->|"update() in schedule order"| catchment
    wf --> channel
    catchment -->|Snapshot| io["IOThread"]
    channel -->|Snapshot| io
    io --> csv["results.csv"]
    io --> gpkg["results.gpkg"]
    io --> h5["results.h5"]
    rec["RunRecorder"] --> manifest["run.json"]
    manifest --> reopen["analysis_reopen"]
</pre>

- **CatchmentComponent** (`catchment`) has one argument, `coefficients`, a runoff coefficient for
  each of three reaches, and one output, `runoff`. Each of its 24 updates computes a synthetic
  storm's rainfall for that hour and multiplies it by each reach's coefficient.
- **ChannelComponent** (`channel`) has one argument, `routing_weight`, per reach, one input,
  `lateral_inflow`, and one output, `flow`. Each update takes the catchment's runoff and blends it
  with the previous flow: \f$Q_{new} = w\,Q_{old} + (1 - w)\,I\f$.
- The **composition document** sets both components' arguments and connects `runoff` to
  `lateral_inflow`.
- The **workflow** finds from that connection that the catchment must run before the channel in
  each step.
- The **writers** store both outputs at every step; the **run manifest** catalogs what was stored
  and where.

## The two components

Both follow the pattern of @ref sdk_guide_first_component — they derive from
@ref HydroCouple::SDK::AbstractModelComponent, create their items in `createArguments()`,
`createInputs()` and `createOutputs()`, and set their status in each lifecycle method. The catchment:

@snippet serial_coupling/main.cpp catchment

The channel differs in one way that matters for coupling: its `update()` begins with
`applyInputValues()`, which pulls the catchment's current runoff into `lateral_inflow` through the
connection the composition made.

@snippet serial_coupling/main.cpp channel

Both components describe their values with `Quantity::unitLess()`, so the items themselves carry no
physical unit; the writers below attach units to the fields they store (see "Units" under the
reopened results).

These two components create their items with `new` and only register them with the base class,
which never deletes registered items, so the items live until the program exits. That is harmless
in a program that runs one composition and ends. A component that a host creates and destroys
repeatedly should own its items in `std::unique_ptr` members, as the linear reservoir in
@ref sdk_guide_first_component does.

## The composition document

A composition document is JSON or YAML. This one lists the two components by id with their
argument values, and one connection:

@include serial_coupling/composition.json

The YAML file is the same document; the SDK converts YAML to JSON when it reads it, when the SDK
was built with yaml-cpp (`IO::ModelInitializer::yamlSupported()` reports whether it was):

@include serial_coupling/composition.yaml

Each component's `id` must match the id of an instance the host can supply, each argument key must
match an argument id of that component, and each connection names a component's output and another
component's input by id. @ref sdk_guide_compositions describes everything a document can hold,
including component types for hosts that create instances themselves, adapted outputs between
components, workflows and writers; @ref sdk_ref_composition lists every field.

## Applying the document

@ref HydroCouple::SDK::IO::ModelInitializer applies a document to live components. It does not
create components in this example: its constructor takes a resolver, a function that returns the
host's instance for an id. Here the host owns both components on the stack.

@snippet serial_coupling/main.cpp initializer

`initializeFromFile()` reads the file (converting YAML when needed) and works in two stages. First it
parses the document and checks it against the live components: every id must resolve, every
argument must exist, every connection must name an existing output and input. Nothing is changed
until both checks pass, so a document with a typo leaves the components untouched and returns
`false` with a message naming the problem. Then it sets each component's arguments, calls each
component's `initialize()`, and connects each output to its input. A component whose
initialization fails also makes `initializeFromFile()` return `false`.

## Writers on a background thread

Results are written by format writers behind an @ref HydroCouple::SDK::IO::IOThread. The
simulation thread captures a step's values into an owned
@ref HydroCouple::SDK::IO::Snapshot and puts it on the IO thread's bounded queue (32 snapshots here);
the IO thread hands each snapshot to every writer. The model is not held up by file IO unless the
queue is full: then `enqueue()` waits for room, so the simulation slows to the writers' pace rather
than dropping a step.

@snippet serial_coupling/main.cpp writers

The CSV writer is always available. The GeoPackage and HDF5 UGRID writers exist only when the SDK
was built with `USE_GEOPACKAGE` and `USE_HDF5`, which the build reports to code through the
`HYDROCOUPLESDK_HAVE_GEOPACKAGE` and `HYDROCOUPLESDK_HAVE_HDF5` definitions. Both need a mesh, here
three reach centroids joined by two edges, and the UGRID writers need their fields declared before
the IO thread starts, because they create their datasets up front. Each writer is also given to the
@ref HydroCouple::SDK::IO::RunRecorder, which will catalog what it wrote.
@ref sdk_guide_record describes each writer and its options.

## The workflow

@ref HydroCouple::SDK::TimeSteppedWorkflow runs the components. `addModelComponent()` hands it
each component; it then has the same lifecycle as a component: `initialize()`, `validate()`,
`prepare()`, `update()` and `finish()`.

@snippet serial_coupling/main.cpp workflow

During `prepare()` the workflow builds its schedule from the connections between the components it
manages: a component runs after every component that provides one of its inputs. The schedule is a
list of groups; each group is one component, or several components that depend on each other in a
cycle and are iterated together. Here the catchment provides the channel's inflow, so the schedule
is `[ catchment ] [ channel ]`. @ref sdk_guide_workflow describes scheduling, cycles and the
pull-driven alternative.

## The run

Each `workflow.update()` updates every component once, in schedule order. The loop runs until the
workflow is `Done`, which it becomes when every component is `Done` or `Finished`; a guard stops it
after one step more than the model should need, in case a component never finishes. After each step
the host captures both outputs into a snapshot stamped with the step number and queues it.

@snippet serial_coupling/main.cpp run

If the workflow fails, its error queue says why. `enqueue()` returns `false` only when the IO thread
is not running. A writer that fails while writing is switched off, the other writers carry on, and
the first failure is kept: `errorCode()` and `errorMessage()` report it, which is why the example
checks `errorCode()` after the IO thread has finished.

## Finishing and recording the run

The order of the last steps matters. `workflow.finish()` finishes the components. `ioThread.finish()`
drains the queue and closes every writer; only then are the files complete and the writers'
catalogs of what they stored final. `RunRecorder::finishToFile()` then writes the run manifest,
`run.json`, with the run's status.

@snippet serial_coupling/main.cpp manifest

The manifest records what ran (the composition, the components, the SDK version), how it ended, and
a results catalog: for every recorded data item, which artifact holds its values, in what format,
with what shape, dimensions, units and time axis. @ref sdk_ref_run_manifest lists every field.

Finally the example writes the composition back out. `serializeToFile()` saves the listed
components' current arguments and connections as a composition document, so a host can save what
a user configured interactively and load it again later.

@snippet serial_coupling/main.cpp roundtrip

## Run it

Configure the SDK with examples and tests (@ref sdk_guide_build), build the two programs and run
their tests:

```shell
cmake -S . -B build -DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON -DUSE_HDF5=ON -DUSE_GEOPACKAGE=ON
cmake --build build --target serial_coupling analysis_reopen
ctest --test-dir build -R examples. --output-on-failure
```

The tests write the run to `build/docs-examples/serial_coupling/`. To run the program yourself from
its build directory, give it a composition document and an output directory:

```shell
cd build/examples
./serial_coupling composition.json demo-run
```

```text
initialized composition from "composition.json"
execution schedule: [ catchment ] [ channel ]
completed 24 steps; outputs in "/path/to/build/examples/demo-run"
run manifest written to run.json
composition round-tripped to composition.out.json
```

`demo-run/` then holds `results.csv`, `results.gpkg` and `results.h5` (the last two only in builds
with GeoPackage and HDF5), `run.json` and `composition.out.json`. `composition.yaml` gives the same
run in a build with YAML support; without it the program says so and exits with status 2.

## Reopening the run

`analysis_reopen` is the other half of the example: a program that links only the SDK, knows
nothing about the catchment or the channel, and still reads every result. This is what a
visualization or analysis tool does with a finished run.

It reads the manifest:

@snippet analysis_reopen/main.cpp read_manifest

It builds a composition in which every recorded component has execution mode `open`
(`IO::compositionForRun()`), and applies it with a `ModelInitializer` whose resolver returns
nothing, because there are no model components to offer. For an `open` component the initializer
creates a @ref HydroCouple::SDK::ResultsModelComponent instead: a component whose outputs are the
recorded items, read from the artifacts the manifest names. It is `Finished` as soon as it is
initialized, because its values already exist.

@snippet analysis_reopen/main.cpp reopen

It reads each recorded item through the ordinary data-item interface, the same `getValuesInto()`
any consumer uses, into a flat buffer described by a `BufferDescriptor`:

@snippet analysis_reopen/main.cpp read_all

and summarizes each item, then reads only the last time level of each through a
@ref HydroCouple::SDK::TimeSliceOutput, a view that fixes the time index of a recorded item:

@snippet analysis_reopen/main.cpp browse

Run on the run above:

```shell
./analysis_reopen demo-run/run.json
```

```text
run        : 22682632-5349-4869-9cf5-b4cd8a5dac0e
caption    : serial coupling example
status     : completed
sdk        : 2.0.0-beta.1
finished   : 2026-11-26T19:18:16Z
components : 2, recorded items: 6

component "catchment" (finished)
  runoff [m3/s]  shape {24, 3}  min 0.0267  max 6.6000  mean 2.1938  (from results.h5)
    last level: 0.0467 0.0267 0.0733

component "channel" (finished)
  flow [m3/s]  shape {24, 3}  min 0.0353  max 6.5309  mean 2.1929  (from results.h5)
    last level: 0.0618 0.0353 0.0972

summarized 6 recorded item(s) with no model component linked.
```

The run id and finishing time differ on every run. Each variable has shape {24, 3}: 24 time levels
of three reaches. The peak runoff is 6.6000, on the reach with the largest coefficient, and routing
lowers and delays it to a peak flow of 6.5309.

The manifest lists six recorded items because three writers each stored both variables. A reopened
component exposes each variable once, from the most capable format the build can read; here that
is the HDF5 file. A build without HDF5 or GeoPackage records and reopens the CSV entries.

**Units.** The `[m3/s]` comes from the catalog entry the variable is served from: the HDF5 and
GeoPackage writers were told the fields are m3/s when the fields were declared. The CSV writer takes
units from the items' own quantities, which are unitless in this example, so the same variable
served from the CSV entry reports `[unitless]`. A real model should give its items physical
quantities (@ref sdk_guide_quantities) so that every format agrees.

## Try it without building

[Download the recorded example (ZIP)](../../serial-coupling-run.zip): the composition document, the
CSV results and the run manifest of a CSV-only run. Extract it and pass its `run.json` to
`analysis_reopen`, or open it in HydroCouple Composer
([Composer user guide](https://www.hydrocouple.org/HydroCoupleComposer/)).

## Where to go next

- @ref sdk_guide_compositions — component types, adapted outputs, argument bindings, workflows and
  writers in a composition document.
- @ref sdk_guide_workflow — scheduling, cycles and iteration, the pull-driven workflow.
- @ref sdk_guide_record — the writers, the run manifest and reopening runs in detail.
- @ref sdk_guide_ports — reading and writing values in bulk.
