@page sdk_guide_component 5. Components and their lifecycle

[TOC]

A *model component* is a model wrapped so that other software can configure it, connect it to
other models and step it, without knowing what model it is. The HydroCouple standard defines it
as the interface `HydroCouple::IModelComponent`; the SDK implements the strategy-independent part
of that interface in @ref HydroCouple::SDK::AbstractModelComponent, so a model wrapper only writes
what is specific to its model. This chapter describes the status machine every component follows,
what the base class does and what a wrapper overrides, the status events and the error queue,
component infos and the plugin entry points through which a host finds and creates components,
and the SDK's results-backed component.

@ref sdk_guide_first_component builds a first component step by step; this chapter is the
reference for the rules that tutorial follows. Arguments, inputs and outputs are described in
@ref sdk_guide_ports, and running several components together in @ref sdk_guide_workflow.

## The classes in this chapter

| Class or header | Implements | Purpose |
| --- | --- | --- |
| @ref HydroCouple::SDK::AbstractModelComponent | `HydroCouple::IModelComponent` | Base class of every model wrapper: status, registries, error queue, status signal, transition checks |
| @ref HydroCouple::SDK::ComponentInfo | `HydroCouple::IComponentInfo` | Metadata about a component type: developer, version, license, tags |
| @ref HydroCouple::SDK::AbstractModelComponentInfo | `HydroCouple::IModelComponentInfo` | Component info that creates instances and lists adapted-output factories |
| @ref HydroCouple::SDK::ComponentStatusChangeEventArgs | `HydroCouple::IComponentStatusChangeEventArgs` | The event a component emits on every status change |
| @ref HydroCouple::SDK::ResultsModelComponent | `HydroCouple::IModelComponent` | A recorded run, reopened as a component |
| `hydrocouplesdk/componentabi.h` | | The two C entry points a component library exports, and the ABI stamp |
| `hydrocouplesdk/core/diagnosticcodes.h` | | Numeric codes of the SDK's diagnostics |

The compiled example of this chapter, `docs/examples/ch05_component_lifecycle/main.cpp`, takes one
component through every path described below and prints each status event. Its output is shown in
@ref sdk_guide_component_run "Running the example".

## The status machine

A component's status says what it is doing and what may be asked of it. It is a value of
`HydroCouple::IModelComponent::ComponentStatus`. The numeric values are fixed by the standard:
proxies carry them across processes and the language bindings mirror them, so a value never
changes meaning and new statuses are appended.

### Statuses

| Value | Status | Meaning | Set in the SDK by |
| --- | --- | --- | --- |
| 0 | `Created` | Just constructed, or finished and ready to be initialized again. Only `arguments()`, `status()`, `capabilities()`, `errors()` and `componentInfo()` may be used. | The constructor; your `finish()` when the component can restart |
| 1 | `Initializing` | `initialize()` is running. | `AbstractModelComponent::initialize()` |
| 2 | `Initialized` | Arguments read, inputs and outputs created. A host connects items now. | `AbstractModelComponent::initialize()` |
| 3 | `Validating` | `validate()` is checking that what `update()` needs will be there. | Your `validate()` |
| 4 | `Valid` | Ready to prepare. Messages returned with this status are warnings. | Your `validate()` |
| 5 | `WaitingForData` | `update()` could not proceed because a provider is behind; the orchestrator must update the provider and call `update()` again. | Your `update()` |
| 6 | `Invalid` | Something `update()` needs is missing. Fix the connections and validate again. | Your `validate()` |
| 7 | `Preparing` | `prepare()` is allocating run state and publishing initial outputs. | Your `prepare()` |
| 8 | `Updating` | `update()` is computing. | Your `update()` |
| 9 | `Updated` | A step completed, or, right after `prepare()`, ready for the first step. There is no separate "Prepared" status. | Your `prepare()` and `update()` |
| 10 | `Checkpointing` | Saving or restoring state through `HydroCouple::ICheckpointableModelComponent`; a save returns to the status it was entered from, a restore lands in `Updated`. | Components that implement checkpointing |
| 11 | `Done` | The last step has been computed; a further `update()` must leave the state unchanged. | Your `update()` |
| 12 | `Finishing` | `finish()` is writing final files and releasing resources. | Your `finish()` |
| 13 | `Finished` | Final. The component cannot be initialized again. Its data items may still be readable (a recorded run is `Finished` from the start). | Your `finish()`; `ResultsModelComponent::initialize()` |
| 14 | `Failed` | An unrecoverable error. A `Fatal` entry is on the error queue. The component may be initialized again or finished. | `initialize()` on failure; any of your methods |

`AbstractModelComponent` performs only the transitions of `initialize()`. The other lifecycle
methods are pure virtual: your `validate()`, `prepare()`, `update()` and `finish()` set every status
they pass through, with `setStatus()`.

### Legal transitions

The standard makes the transition table normative: `HydroCouple::Helpers::isValidComponentStatusTransition()`
in `hydrocouplehelpers.h` encodes it, and an implementation must not make a transition it rejects.

<pre class="mermaid">
stateDiagram-v2
    [*] --> Created
    Created --> Initializing: initialize()
    Initializing --> Initialized
    Initialized --> Initializing: initialize() again
    Initialized --> Validating: validate()
    Validating --> Valid
    Validating --> Invalid
    Valid --> Validating: validate() again
    Invalid --> Validating: validate() again
    Valid --> Preparing: prepare()
    Preparing --> Updated
    Updated --> Updating: update()
    Updating --> Updated
    Updating --> WaitingForData
    WaitingForData --> Updating: update() again
    Updating --> Done: last step
    Updated --> Checkpointing
    Done --> Checkpointing
    Checkpointing --> Updated
    Checkpointing --> Done
    Initialized --> Finishing: finish()
    Valid --> Finishing: finish()
    Invalid --> Finishing: finish()
    Updated --> Finishing: finish()
    Done --> Finishing: finish()
    Failed --> Finishing: finish()
    Finishing --> Finished
    Finishing --> Created: can restart
    Failed --> Initializing: initialize() again
    Finished --> [*]
</pre>

Any status except `Finished` (and `Failed` itself) may also go to `Failed`; the diagram leaves
those arrows out. A failure can be detected while a component rests: a proxy whose remote peer
dies while it is `Updated` fails without any call being made.

| From | Allowed next statuses |
| --- | --- |
| `Created` | `Initializing`, `Failed` |
| `Initializing` | `Initialized`, `Failed` |
| `Initialized` | `Validating`, `Initializing`, `Finishing`, `Failed` |
| `Validating` | `Valid`, `Invalid`, `Failed` |
| `Valid` | `Preparing`, `Validating`, `Finishing`, `Failed` |
| `Invalid` | `Validating`, `Finishing`, `Failed` |
| `Preparing` | `Updated`, `Failed` |
| `Updating` | `Updated`, `Done`, `WaitingForData`, `Failed` |
| `WaitingForData` | `Updating`, `Failed` |
| `Updated` | `Updating`, `Checkpointing`, `Finishing`, `Failed` |
| `Checkpointing` | `Updated`, `Done`, `Failed` |
| `Done` | `Checkpointing`, `Finishing`, `Failed` |
| `Finishing` | `Finished`, `Created`, `Failed` |
| `Finished` | none |
| `Failed` | `Initializing`, `Finishing` |

### What a host may call in each status

| Method | Legal from | Notes |
| --- | --- | --- |
| `arguments()`, `status()`, `capabilities()`, `errors()`, `componentInfo()` | every status | `AbstractModelComponent::arguments()` creates the arguments on its first call, even while `Created` |
| `inputs()`, `outputs()`, `states()` | after `initialize()` succeeds | Empty before; rebuilt by each `initialize()` |
| `initialize()` | `Created`, `Failed`, `Initialized` | The SDK's `initialize()` returns without doing anything in any other status |
| `validate()` | `Initialized`, `Valid`, `Invalid` | Call it after the connections are made; repeat it after they change |
| `prepare()` | `Valid` | At most once per `initialize()`/`finish()` cycle |
| `update()` | `Updated`, `WaitingForData` | The SDK workflows never update a `Done` or `Finished` component |
| `finish()` | `Initialized`, `Valid`, `Invalid`, `Updated`, `Done`, `Failed` | So a composition can be torn down from any resting status |

### How the SDK checks transitions

Every `setStatus()` call goes through one private function that stores the new status atomically
and compares the pair with the normative table. A transition the table rejects is still
performed, because refusing it would strand an orchestrator that waits for `Done`, but it is
reported on the error queue as an `Error` with code `Diagnostic::IllegalStatusTransition` (1001)
and the message `illegal status transition <from> -> <to> (Helpers::isValidComponentStatusTransition rejects it)`.
Each distinct pair is reported once per component, so a component that takes the same shortcut on
every step reports it once. Setting the status a component already has is not a transition: it
refreshes the message or the progress and is never reported.

The most common defect it catches is a missing intermediate status. The example's
`ShortcutPond` goes from `Valid` straight to `Updated` in `prepare()`:

@snippet ch05_component_lifecycle/main.cpp illegal

```text
illegal transition
  [shortcut] Created -> Initializing: Initializing component
  [shortcut] Initializing -> Initialized: Component initialized successfully
  [shortcut] Initialized -> Validating
  [shortcut] Validating -> Valid
  [shortcut] Valid -> Updated: prepared without Preparing
  error queue: Error 1001 from shortcut: illegal status transition Valid -> Updated (Helpers::isValidComponentStatusTransition rejects it)
```

## Writing a component on AbstractModelComponent

### What the base class does and what you write

| You write | Called by | When | What it must do |
| --- | --- | --- | --- |
| `createArguments()` | the first `arguments()` call, or `initialize()` | once per instance, possibly while `Created` | Create and register every argument with `addArgument()`, with defaults |
| `initializeArguments(message)` | `initialize()` | while `Initializing` | Read and check the argument values, open files, build the model; return `false` with a reason to fail |
| `initializeFailureCleanUp()` | `initialize()` | after `initializeArguments()` returned `false`, with the status already `Failed` | Release what `initializeArguments()` acquired |
| `createInputs()` | `initialize()` | after `initializeArguments()` succeeds | Create and register the inputs with `addInput()` |
| `createOutputs()` | `initialize()` | right after `createInputs()` | Create and register the outputs with `addOutput()` |
| `validate()` | the host or the workflow | `Initialized`, `Valid`, `Invalid` | `Validating`, then `Valid` or `Invalid`; return and queue the problems |
| `prepare()` | the host or the workflow | `Valid` | `Preparing`, allocate run state, publish initial outputs, `Updated` |
| `update(requiredOutputs)` | the host or the workflow | `Updated`, `WaitingForData` | `Updating`, read inputs, compute, write outputs, then `Updated`, `Done`, `WaitingForData` or `Failed` |
| `finish()` | the host or the workflow | see the table above | `Finishing`, release resources, then `Finished` or `Created` |

Optional overrides: `results()` (items that are not exchange items; empty by default), `states()`
(the prognostic items a checkpoint must capture; empty by default), `capabilities()` (the optional
interfaces the component implements; empty by default) and `initialize()` itself, which
@ref HydroCouple::SDK::ResultsModelComponent replaces.

The example's component is a detention pond: it stores water, \f$S\f$ in m³, and releases
\f$Q = k S\f$ in m³/s, where \f$k\f$ is a recession constant in 1/s. One explicit step of length
\f$\Delta t\f$ is

\f[ S_{n+1} = S_n + \Delta t \, (I_n - k S_n) \f]

with inflow \f$I_n\f$. It is stable for \f$k \Delta t \le 1\f$, which the component checks.

@snippet ch05_component_lifecycle/main.cpp component

### Construction and arguments

The constructors take an id, an optional caption and an optional
@ref HydroCouple::SDK::AbstractModelComponentInfo. The info is borrowed: `componentInfo()` returns
it, and nothing else in the base class uses it. An empty id is replaced by a generated UUID.

Construction calls no hook. The arguments are created the first time anyone asks for them:
`arguments()` calls `createArguments()` once, even though it is a `const` method, and
`initialize()` calls it if it has not run. This is what lets a host list and set arguments while
the component is `Created`. Consequently `createArguments()` must not depend on argument values,
and anything it uses (units, quantities, dimensions) must already exist, which is why the pond
creates those as members. Give each argument its default value in `createArguments()`; the host
replaces it with `IArgument::initialize()` before `initialize()`.

A host that creates several instances of one component type renames each before initialization
with `setInstanceId(id, message)`. It succeeds only while the status is `Created` and the id is not
empty; otherwise it returns `false` with the message `a nonempty instance ID can only be assigned
before initialization`. Composer uses it when the composition names an instance.

### initialize(), step by step

`AbstractModelComponent::initialize()` is the one lifecycle method the base class implements:

1. It returns at once, changing nothing and queuing nothing, unless the status is `Created`,
   `Failed` or `Initialized`. Check `status()` after calling it.
2. It sets `Initializing` with the message `Initializing component`.
3. It runs `createArguments()` if no argument exists yet.
4. It calls `initializeArguments(message)`. If that returns `false`, it sets `Failed` with the
   message `Argument initialization failed: ` followed by your message, which queues a `Fatal`
   entry, calls `initializeFailureCleanUp()` and returns.
5. It clears the input and output registries (without deleting anything), calls `createInputs()`,
   `createOutputs()` and then `initializeAdaptedOutputs()`, which initializes the adapted outputs
   already registered on each output.
6. It sets `Initialized` with the message `Component initialized successfully`.

If a hook throws, the exception propagates out of `initialize()` and the status stays
`Initializing`, which no transition other than `Initialized` or `Failed` leaves. Catch errors in
your hooks and report them with a `false` return instead.

### When initialization fails

A failed initialization leaves the component `Failed`, with one `Fatal` entry on the error queue
and the inputs and outputs it had before: none on a first initialization, the previous ones when
an `Initialized` component was initialized again (the registries are cleared only after
`initializeArguments()` succeeds). From `Failed` a host may fix the arguments and call `initialize()`
again, or call `finish()` and discard the component.

@snippet ch05_component_lifecycle/main.cpp failure

```text
failed initialization
  [pond-2] Created -> Initializing: Initializing component
  [pond-2] Initializing -> Failed: Argument initialization failed: recession_constant must be in (0, 0.000278] 1/s, got 0.500000
  error queue: Fatal 0 from pond-2: Argument initialization failed: recession_constant must be in (0, 0.000278] 1/s, got 0.500000
  [pond-2] Failed -> Finishing
  [pond-2] Finishing -> Created: run state released
```

`initializeFailureCleanUp()` runs only when `initializeArguments()` returns `false`; nothing calls
it after a failure in `validate()`, `prepare()` or `update()`, so release resources there yourself
or in `finish()`.

### validate()

`validate()` runs after the host has connected the items and before `prepare()`. It checks what
`update()` will need: required inputs connected, argument combinations that only make sense
together, files that `prepare()` will open. Set `Validating` first, then `Valid` or `Invalid`, and
return one message per problem. Messages returned with `Valid` are warnings; with `Invalid` at
least one is fatal to the run. The standard asks for every returned message on the error queue
too (an `Error` entry when `Invalid`, a `Warning` when `Valid`), so that a proxy forwarding a
remote `validate()` loses nothing; the base class does not do this for you.

The pond warns when its inflow is not connected:

```text
  [pond-1] Initialized -> Validating
  [pond-1] Validating -> Valid
  error queue: Warning 0 from pond-1: inflow is not connected; the pond drains with zero inflow
```

A component's `validate()` checks the component alone. Whether the two ends of each link agree on
units, value kind and time semantics is checked by the workflow (@ref sdk_guide_workflow) or by a
host with @ref HydroCouple::SDK::validateConnection.

### prepare()

`prepare()` runs once, from `Valid`. Set `Preparing`, allocate the run state, set the initial
values of the outputs (a consumer may read them before the first step), and set `Updated`. After
`prepare()` succeeds the component must answer every request for values, and any device buffer it
exports must keep its address until `finish()`.

### update()

`update()` advances the model by one step. Set `Updating`; read the inputs; compute; write the
outputs; then set `Updated`, or `Done` after the last step. The SDK's input classes do not fetch
values by themselves: call `applyInputValues()`, which asks every registered input to retrieve its
provider's current values and apply them. The time-stepped workflow relies on this, because it
updates components but does not move values into their inputs.

`setStatus(status, message, progress)` publishes a percentage with the event; the pond reports
`100 n / N` after step \f$n\f$ of \f$N\f$. A component that finds a provider has not yet produced the
values it needs may set `WaitingForData` and return; the orchestrator updates the provider and
calls `update()` again. On an unrecoverable error, set `Failed` with a message.

The `requiredOutputs` argument names the outputs the caller needs refreshed; an empty list leaves
the choice to the component. Both SDK workflows always pass an empty list.

### finish() and restarting

`finish()` is the last call of a run. Set `Finishing`, close files and release the run state, then
set `Finished` if the component cannot run again, or `Created` if it can. A component that returns
to `Created` may be initialized again:

@snippet ch05_component_lifecycle/main.cpp reinitialize

```text
re-initialization
  [pond-1] Created -> Initializing: Initializing component
  [pond-1] Initializing -> Initialized: Component initialized successfully
  recession_constant is still [0.0002]; inflow item replaced: yes
  [pond-1] Initialized -> Finishing
  [pond-1] Finishing -> Created: run state released
```

`createArguments()` never runs twice, so arguments and the values a host gave them survive a
re-initialization. The inputs and outputs are created again, so every connection made to the
previous items is gone and the host must connect the new ones. `isInitialized()` stays `true`
from the first successful initialization on.

### Owning arguments, inputs and outputs

The base class keeps three registries, each an id lookup plus a list in registration order.
They hold raw pointers and own nothing: `addInput()` does not take ownership, and neither
`clearInputs()` nor the destructor deletes anything. The component owns its items. The pond keeps
them in `std::unique_ptr` members, so replacing one in `createInputs()` deletes the previous item
and the component's destructor deletes the rest. Items borrow their dimension and value
definition, so keep those alive at least as long as the items (members declared before the
items).

| Registry call | Behavior |
| --- | --- |
| `addInput(input)`, `addOutput(output)`, `addArgument(argument)` | Registers by id; `nullptr` is ignored. An item whose id is already registered is not added (the first one stays), though the property-change signal still fires. |
| `removeInput(input)` and the others | Unregisters when that object is the one registered under its id; returns `false` otherwise. The item is not deleted and its connections are not touched. |
| `clearInputs()` and the others | Unregisters everything, deletes nothing, fires no signal. |
| `inputsInternal()` and the others | A copy of the id-to-item map. |

Deleting an input does not tell its provider: an SDK output keeps a raw pointer to each consumer.
Before a component deletes connected items (on re-initialization or destruction), the host must
disconnect them with `removeConsumer()` on the provider.

### Helpers the base class provides

| Helper | Behavior |
| --- | --- |
| `applyInputValues()` | Calls `retrieveValuesFromProvider()` and then `applyData()` on every registered input, in registration order. An input without a provider keeps its values. |
| `updateOutputValues(required)` | Calls each output's `updateValues()` (all outputs when the list is empty). Adapted outputs are not refreshed by it. |
| `initializeAdaptedOutputs()` | Initializes the adapted outputs registered on each output; `initialize()` calls it. |
| `referenceDirectory()`, `setReferenceDirectory(dir)` | The directory relative paths resolve against, normally the composition document's directory; stored as given. |
| `getAbsoluteFilePath(path)` | `path` when absolute, otherwise the reference directory joined with `path`, not normalized. |
| `getRelativeFilePath(path)` | `std::filesystem::relative(path, referenceDirectory())`; `path` unchanged when no reference directory is set. Throws `std::filesystem::filesystem_error` when the file system cannot resolve the paths. |
| `statusToString(status)` | The status's name, for messages. |
| `workflow()`, `setWorkflow()` | The managing workflow, set by `AbstractWorkflowComponent::addModelComponent()`. |
| `index()`, `setIndex()` | An ordering number for a host; nothing in the SDK reads it. |
| `isInitialized()` | `true` after the first successful `initialize()`. |
| `isPrepared()` | Always `false` in this version: nothing sets the flag behind it. Track preparation yourself. |
| `mpi...()`, `gpu...()` | Rank, allocated ranks and a GPU allocation table recorded for the subclass. Nothing in the SDK reads or sets them; the distributed classes in @ref sdk_guide_distributed do not use them. |

## Status events and messages

Every `setStatus()` call emits one event to the slots connected to the component's status signal.
The event is a @ref HydroCouple::SDK::ComponentStatusChangeEventArgs in a `std::shared_ptr`:

| Field | Value |
| --- | --- |
| `component()` | The component, not owned; valid while the component lives |
| `previousStatus()` | The status before the call; equal to `status()` for a refresh |
| `status()` | The new status, which the component already holds when the slot runs |
| `message()` | The message passed to `setStatus()`; empty for the one-argument overload |
| `hasProgressMonitor()` | `true` when a progress greater than 0 was passed |
| `percentProgress()` | The progress, nominally 0 to 100; not range-checked |

Slots run synchronously, on the thread that called `setStatus()`, in the order they were
connected; a slot that throws stops the emission and the exception reaches the component. The
signal keeps only a weak reference to each slot, so the caller keeps the slot's `shared_ptr` alive
for as long as it wants events; a temporary slot is released at once and never called.
@ref HydroCouple::SDK::FunctionSlot turns a lambda into a slot:

@snippet ch05_component_lifecycle/main.cpp run

```text
full run
  [pond-1] Created -> Initializing: Initializing component
  [pond-1] Initializing -> Initialized: Component initialized successfully
  [pond-1] Initialized -> Validating
  [pond-1] Validating -> Valid
  error queue: Warning 0 from pond-1: inflow is not connected; the pond drains with zero inflow
  [pond-1] Valid -> Preparing
  [pond-1] Preparing -> Updated: ready to step
  [pond-1] Updated -> Updating
  [pond-1] Updating -> Updated: step 1 (33%)
  storage 280.0 m3, outflow 0.0560 m3/s
  [pond-1] Updated -> Updating
  [pond-1] Updating -> Updated: step 2 (67%)
  storage 78.4 m3, outflow 0.0157 m3/s
  [pond-1] Updated -> Updating
  [pond-1] Updating -> Done: step 3 (100%)
  storage 22.0 m3, outflow 0.0044 m3/s
  [pond-1] Done -> Finishing
  [pond-1] Finishing -> Created: run state released
```

With \f$k = 2 \times 10^{-4}\f$ 1/s and \f$\Delta t = 3600\f$ s each step multiplies the storage by
\f$1 - k \Delta t = 0.28\f$: 1000, 280, 78.4, 21.95 m³. The example checks each value against
\f$S_0 (1 - k \Delta t)^n\f$.

A slot's `sender` argument is the component's internal signal object, not the component; use
`event->component()` to reach the component. Besides the status signal, every component has the
property-change signal of @ref HydroCouple::SDK::Description (chapter @ref sdk_guide_quantities).
`setStatus()` fires it with `"Status"` after the status event, and the registry and
configuration setters fire it with `"Inputs"`, `"Outputs"`, `"Arguments"`, `"Workflow"`,
`"ReferenceDirectory"` and `"Id"`. `blockSignals(true)` silences both signals; status changes,
transition checks and the error queue are unaffected.

Status events are informative. The standard says a host must not build critical behavior on them;
read `status()` after each call instead.

## The error queue

The error queue is the standard's normative failure channel, because exceptions cannot cross a
process, an MPI rank, a C entry point or a language boundary. Each component keeps a queue of
`HydroCouple::ErrorEntry` records: a severity (`Information`, `Warning`, `Error`, `Fatal`), a code,
the source (the component's id) and a message.

- `errors(clearAfterRead)` returns a copy of the queue in insertion order and empties it when
  `clearAfterRead` is `true`. It is guarded by a mutex and may be called from any thread, in any
  status.
- `reportError(severity, code, message)` (protected) queues an entry. Use codes below 1000 for your
  own diagnostics; 1000 and above are the SDK's (`hydrocouplesdk/core/diagnosticcodes.h`).
- `queueError(severity, code, message)` is the public form, for a data item to report on behalf of
  its component; @ref HydroCouple::SDK::AbstractOutput uses it when a connection is refused.
- `setStatus(Failed, message)` and `setStatus(Failed, message, progress)` queue a `Fatal` entry,
  code 0, with the message. The one-argument `setStatus(Failed)` does not, so always fail with a
  message.
- An illegal transition queues an `Error` with code 1001.

The standard also asks a component to queue an `Error` whenever a method that returns `bool`
returns `false`, and to queue every message `validate()` returns. The base class does neither for
you.

## Component infos and how hosts create instances

A host does not construct components by class name. It asks a *component info*, an object that
describes a component type and creates instances of it. @ref HydroCouple::SDK::ComponentInfo holds
the metadata; @ref HydroCouple::SDK::AbstractModelComponentInfo adds the factory side of
`HydroCouple::IModelComponentInfo`. The setters in the table are protected, except the caption and
description setters of every description and `setLibraryFilePath()`.

| Metadata | Getter | Set with |
| --- | --- | --- |
| Id of the component type | `id()` | constructor |
| Caption, description | `caption()`, `description()` | `setCaption()`, `setDescription()` |
| Developer or vendor | `developer()` | `setDeveloper()` |
| Version | `version()` | `setVersion()` |
| License text or SPDX id | `license()` | `setLicenseText()` |
| Copyright notice | `copyright()` | `setCopyright()` |
| Home page, contact | `url()`, `email()` | `setUrl()`, `setEmail()` |
| Citations | `documentation()` | `addDocumentation()`, `setDocumentation()` |
| Classification tags | `tags()` (sorted) | `addTag()`, `setTags()` |
| Icon, relative to the library | `iconFilePath()` | `setIconFilePath()` |
| Library the type was loaded from | `libraryFilePath()` | `setLibraryFilePath()` (public; the host sets it) |

Give the info a stable id, preferably reverse-domain; hosts and composition documents identify
the component type by it, and the default constructor's random UUID changes every run. A subclass
fills the metadata once, in its constructor; after that the info is read-only and may be read from
several threads.

`AbstractModelComponentInfo` leaves `createComponentInstance()` pure virtual; your info implements
it. The returned instance is owned by the caller, and must be destroyed before the library that
created it is unloaded. Passing `this` to the component's constructor makes `componentInfo()`
answer:

@snippet ch05_component_lifecycle/main.cpp info

The info also lists adapted-output factories (`adaptedOutputFactories()`), offered to hosts for
adapting this component's outputs. A subclass registers them with `addAdaptedOutputFactory()`;
the list is keyed by factory id, holds raw non-owning pointers, and its order is unspecified.
@ref sdk_guide_adapted_outputs describes the factories.

A host then creates and configures instances in one of two ways. A program that links the
component's code constructs it directly, as the workflow examples in @ref sdk_guide_workflow do.
A composition-driven host gives @ref HydroCouple::SDK::IO::ModelInitializer a resolver that returns
the instance for each id in the document (@ref sdk_guide_compositions). Composer creates instances
from the infos of the libraries it loaded and names each one through the component's
`hydrocouple.instance_id` argument when it declares one, otherwise with `setInstanceId()`.

## Publishing a component library: the plugin ABI

A component library is a shared library that exports two C functions:

| Symbol (string macro) | Signature | Purpose |
| --- | --- | --- |
| `hydrocouple_component_abi_v1` (`HYDROCOUPLE_COMPONENT_ABI_SYMBOL`) | `const char *(void)` | Returns the library's ABI stamp. Pure C, safe to call across any mismatch. |
| `hydrocouple_component_info_v1` (`HYDROCOUPLE_COMPONENT_INFO_SYMBOL`) | `HydroCouple::IComponentInfo *(void)` | Returns the library's component info. Safe only once the stamp matches. |

`hydrocouplesdk/componentabi.h` includes the interface package's `hydrocouplecomponentabi.h`,
which owns the convention; the SDK header adds nothing. Its macro
`HYDROCOUPLE_DECLARE_COMPONENT(InfoType)` emits both functions; place it once, in one translation
unit, with a default-constructible info class:

@snippet ch05_component_lifecycle/main.cpp declare

The info is a function-local static: the library owns it, the host must not delete it, and it
stays valid until the library is unloaded.

The *ABI stamp* exists because passing C++ objects across a shared-library boundary is defined only
when both sides were built with the same toolchain and standard library, and no program can
recover once a mismatched C++ function has been called. The stamp is a string built by the
preprocessor, `HYDROCOUPLE_COMPONENT_ABI_STAMP`, made of these fields:

| Field | Example | What it records |
| --- | --- | --- |
| `hc-abi/` | `2` | Revision of the loading convention itself |
| `iface=` | `5` | Interface ABI (`HydroCouple::HYDROCOUPLE_ABI_VERSION`); a `static_assert` keeps the macro and the constant equal |
| `cxx=` | `clang-21` | Compiler family, with the major version for GCC and Clang; `msvc-v14x` for every binary-compatible MSVC |
| `stdlib=` | `libc++1` | Standard library and its ABI-affecting settings (libstdc++ dual ABI and debug mode, libc++ ABI version, MSVC iterator-debug level) |
| `crt=` | `md` | Windows only: the C runtime. `/MT` builds are stamped so a host can name the problem, but they cannot work |
| `ptr=` | `8` | Pointer width in bytes |

A host loads a library in this order:

1. Open the library (`dlopen`, `LoadLibrary`).
2. Look up `hydrocouple_component_abi_v1`, call it, and compare the result with its own
   `HYDROCOUPLE_COMPONENT_ABI_STAMP`. Refuse the library and close it on any difference. This is
   the only symbol that may be called before the comparison.
3. Look up `hydrocouple_component_info_v1`, call it, and `dynamic_cast` the result to
   `HydroCouple::IModelComponentInfo` (or another info interface).
4. Set `libraryFilePath()` on the info, so run manifests can record where the type came from.
5. Create instances with `createComponentInstance()`; destroy every instance before closing the
   library.

@snippet ch05_component_lifecycle/main.cpp host

```text
library stamp: hc-abi/2;iface=5;cxx=clang-21;stdlib=libc++1;ptr=8
component type org.hydrocouple.examples.detention-pond, version 1.0.0, license MIT
```

The example links its component into the executable, so it calls the two functions directly; a
host that loads a file obtains the same pointers from `dlsym` or `GetProcAddress`. Libraries
written before the stamp existed export `CreateComponentInfo`
(`HYDROCOUPLE_COMPONENT_LEGACY_INFO_SYMBOL`), which returns an `IModelComponentInfo *` and has no
stamp; HydroCouple's Python loader uses it, and Composer accepts such libraries on a best-effort
basis, reporting them as unstamped. How Composer reports a refused library is described in the
[Composer manual](https://www.hydrocouple.org/HydroCoupleComposer/).

## Running the example {#sdk_guide_component_run}

```text
cmake --build build --target docs_example_ch05_component_lifecycle
ctest --test-dir build -R docs.example.ch05_component_lifecycle --output-on-failure
```

The program checks every value and transition it prints and returns nonzero if one is wrong. Its
complete output, from the test run (the stamp depends on the toolchain):

@include ch05_component_lifecycle/main.cpp

```text
library stamp: hc-abi/2;iface=5;cxx=clang-21;stdlib=libc++1;ptr=8
component type org.hydrocouple.examples.detention-pond, version 1.0.0, license MIT

full run
  [pond-1] Created -> Initializing: Initializing component
  [pond-1] Initializing -> Initialized: Component initialized successfully
  [pond-1] Initialized -> Validating
  [pond-1] Validating -> Valid
  error queue: Warning 0 from pond-1: inflow is not connected; the pond drains with zero inflow
  [pond-1] Valid -> Preparing
  [pond-1] Preparing -> Updated: ready to step
  [pond-1] Updated -> Updating
  [pond-1] Updating -> Updated: step 1 (33%)
  storage 280.0 m3, outflow 0.0560 m3/s
  [pond-1] Updated -> Updating
  [pond-1] Updating -> Updated: step 2 (67%)
  storage 78.4 m3, outflow 0.0157 m3/s
  [pond-1] Updated -> Updating
  [pond-1] Updating -> Done: step 3 (100%)
  storage 22.0 m3, outflow 0.0044 m3/s
  [pond-1] Done -> Finishing
  [pond-1] Finishing -> Created: run state released

re-initialization
  [pond-1] Created -> Initializing: Initializing component
  [pond-1] Initializing -> Initialized: Component initialized successfully
  recession_constant is still [0.0002]; inflow item replaced: yes
  [pond-1] Initialized -> Finishing
  [pond-1] Finishing -> Created: run state released

failed initialization
  [pond-2] Created -> Initializing: Initializing component
  [pond-2] Initializing -> Failed: Argument initialization failed: recession_constant must be in (0, 0.000278] 1/s, got 0.500000
  error queue: Fatal 0 from pond-2: Argument initialization failed: recession_constant must be in (0, 0.000278] 1/s, got 0.500000
  [pond-2] Failed -> Finishing
  [pond-2] Finishing -> Created: run state released

illegal transition
  [shortcut] Created -> Initializing: Initializing component
  [shortcut] Initializing -> Initialized: Component initialized successfully
  [shortcut] Initialized -> Validating
  [shortcut] Validating -> Valid
  [shortcut] Valid -> Updated: prepared without Preparing
  error queue: Error 1001 from shortcut: illegal status transition Valid -> Updated (Helpers::isValidComponentStatusTransition rejects it)

all checks passed
```

## ResultsModelComponent

@ref HydroCouple::SDK::ResultsModelComponent presents a recorded run as an ordinary component, so
an analysis tool can browse and plot results, and a new composition can use a stored run as a
boundary condition, without loading the model's library. How runs are recorded, and the run
manifest it reads, are described in @ref sdk_guide_record; this section covers its behavior as a
component.

| Member | Behavior |
| --- | --- |
| `ResultsModelComponent(manifest, componentId, id)` | Copies the manifest. `componentId` selects the catalog entries to expose (empty exposes all); `id` defaults to `componentId`, so a reopened component keeps its run-time identity. Caption "Recorded results". Opens nothing. |
| `open(manifestPath, componentId, message)` | Reads the manifest and constructs; `nullptr` with `message` set when the manifest cannot be read. |
| `initialize()` | Opens the readers and builds one recorded item per catalog entry, then walks `Initializing`, `Initialized`, `Finishing`, `Finished`. It does not check the status first: call it once, on a `Created` instance. |
| `validate()`, `prepare()`, `update()` | Do nothing and change no status; `validate()` returns no messages. |
| `finish()` | Closes the readers and sets `Finished` if the status is anything else. |
| `results()`, `outputs()` | The recorded items, in catalog order, in both lists. |
| `manifest()`, `sourceComponentId()` | What the component was opened from. |
| `entryFor(itemId)` | The catalog entry behind an item, or `nullptr`. |
| `times(itemId, values, message)` | The item's times in Julian days; `false` with `no recorded item "ID"` for an unknown id. |

When an item was recorded by several writers it is exposed once, from the first format this build
can read in the order `hdf5_ugrid`, `netcdf_ugrid`, `geopackage`, `csv`. Recorded items are
read-only (`setValuesFrom()` returns `false`), report `TimeKind::Unknown`,
`TimeInterpolation::None` and `TimeExtrapolation::Refuse`, and read values from the artifact on
each request. Entries recorded in elapsed seconds are converted to Julian days with the entry's
epoch; an entry without one fails the whole component with code 5.

Two cautions come from the code. The recorded items are created with `new`, registered as outputs
and never deleted, so each opened component leaks them when destroyed. And `finish()` closes the
readers the items read through, so reading a value after `finish()` uses a destroyed reader; read
everything first. The workflows never call `finish()` on a component that is already `Finished`,
so a reopened run inside a workflow is safe.

## Thread safety

| What | Rule |
| --- | --- |
| `status()` | Any thread, any time (atomic) |
| `errors()`, `reportError()`, `queueError()` | Any thread (mutex) |
| Connecting and disconnecting slots | Any thread; slots run on the thread that sets the status |
| Lifecycle methods, registries, arguments, reference directory | One driving thread; the registries have no lock |
| Component info metadata | Write once in the constructor, then read from any thread |

## Diagnostics

| Message | Cause | Fix |
| --- | --- | --- |
| `Argument initialization failed: <your message>` (status event and `Fatal` entry, code 0) | `initializeArguments()` returned `false` | Correct the argument the message names, then `initialize()` again |
| `illegal status transition A -> B (Helpers::isValidComponentStatusTransition rejects it)` (`Error`, code 1001) | The component set a status the table does not allow after the current one, typically skipping `Preparing`, `Updating`, `Validating` or `Finishing` | Set every intermediate status in order |
| `a nonempty instance ID can only be assigned before initialization` (from `setInstanceId()`) | The id was empty, or the component is no longer `Created` | Rename instances right after creating them |
| `initialize()` returns and the status is unchanged | `initialize()` called in a status other than `Created`, `Failed` or `Initialized` | `finish()` to `Created` first, or create a new instance |
| Status stuck at `Initializing` | A hook threw an exception out of `initialize()` | Catch inside the hooks and return `false` with a message |
| `Output "ID" cannot be consumed by input "ID": REASON` (`std::invalid_argument`, and an `Error` entry on the output's component) | `addConsumer()` refused the connection | See @ref sdk_guide_ports |
| `ResultsModelComponent` codes 1 to 5 | The manifest names no results, a format cannot be read, or an artifact cannot be opened | See the table in @ref HydroCouple::SDK::ResultsModelComponent and @ref sdk_guide_record |

@ref sdk_ref_diagnostics collects the codes and messages of every module.
