@page sdk_guide_workflow 11. Workflows

[TOC]

A *workflow* runs a composition: it initializes the components, checks that the composition as a
whole can run, prepares the components, steps them in a sensible order until they are done, and
finishes them. The HydroCouple standard defines it as `HydroCouple::IWorkflowComponent`. The SDK
implements the parts every strategy shares in @ref HydroCouple::SDK::AbstractWorkflowComponent and
offers two strategies:

- @ref HydroCouple::SDK::TimeSteppedWorkflow derives an execution schedule from the connections,
  collapses feedback loops into groups, and advances every component once per step. Use it for
  models that march forward in time, which is most of them. Composer uses it when a composition
  names no strategy.
- @ref HydroCouple::SDK::PullDrivenWorkflow starts each step from a *trigger* input at the most
  downstream point and pulls values upstream through the providers, the request-reply mode of
  OpenMI. Use it when one consumer decides what is computed and when.

This chapter describes the workflow status machine, what each lifecycle method does, both
strategies, connection validation with every finding it can report, cooperative pause and stop,
the workflow's status events and error queue, and how execution modes from a composition document
meet the workflow. Components are described in @ref sdk_guide_component; the documents that
describe a composition, and @ref HydroCouple::SDK::IO::ModelInitializer, which applies them, in
@ref sdk_guide_compositions.

## The classes in this chapter

| Class or function | Implements | Purpose |
| --- | --- | --- |
| @ref HydroCouple::SDK::AbstractWorkflowComponent | `HydroCouple::IWorkflowComponent` | Status machine, component registry, role slots, error queue, pause and stop, link validation |
| @ref HydroCouple::SDK::TimeSteppedWorkflow | | Schedule from the connection graph; feedback loops as iterated groups |
| @ref HydroCouple::SDK::PullDrivenWorkflow | | Request-reply chains from a trigger input |
| @ref HydroCouple::SDK::WorkflowStatusChangeEventArgs | `HydroCouple::IWorkflowComponentStatusChangeEventArgs` | The event a workflow emits on every status change |
| @ref HydroCouple::SDK::validateConnection, @ref HydroCouple::SDK::hasConnectionError | | Compare the declarations of a provider and a consumer |
| @ref HydroCouple::SDK::ConnectionFinding | | One finding: severity, code, message |
| @ref HydroCouple::SDK::Differentiation::DifferentiableWorkflow | | A time-stepped workflow that records its steps for gradients (@ref sdk_guide_differentiation) |

Two compiled examples belong to this chapter. `docs/examples/ch11_timestepped_cycle/main.cpp` runs
a time-stepped workflow over three components with a feedback loop and checks the result against a
plain loop. `docs/examples/ch11_pull_and_validation/main.cpp` runs a pull-driven chain, makes a
composition invalid with a unit mismatch and repairs it, shows how unconnected components are
ordered, and exercises pause, resume and stop.

## The workflow status machine

### Statuses

| Value | Status | Meaning |
| --- | --- | --- |
| 0 | `Created` | Just constructed; components can be added |
| 1 | `Initializing` | `initialize()` is initializing the components |
| 2 | `Initialized` | Components initialized; connect their items now |
| 3 | `Validating` | `validate()` is checking the composition |
| 4 | `Validated` | The composition is valid and can be prepared |
| 5 | `Preparing` | `prepare()` is preparing the components and building the schedule |
| 6 | `Prepared` | Ready for the first step |
| 7 | `Updating` | A step is running |
| 8 | `Updated` | A step completed; call `update()` again |
| 9 | `Paused` | Stopped at a synchronization point after a pause request; call `resume()` |
| 10 | `Done` | Every component is done, or a stop was requested |
| 11 | `Finishing` | `finish()` is finishing the components |
| 12 | `Finished` | Final; the workflow cannot be initialized again |
| 13 | `Failed` | An unrecoverable error; a `Fatal` entry is on the error queue |
| 14 | `Invalid` | `validate()` found problems; fix the composition and validate again |

The values are fixed; `Invalid` was appended in ABI 5, so the order of the list is not the order of
the lifecycle.

### Legal transitions

`HydroCouple::Helpers::isValidWorkflowStatusTransition()` is the normative table:

<pre class="mermaid">
stateDiagram-v2
    [*] --> Created
    Created --> Initializing: initialize()
    Initializing --> Initialized
    Initialized --> Validating: validate()
    Validating --> Validated
    Validating --> Invalid
    Invalid --> Validating: validate() again
    Validated --> Validating: validate() again
    Validated --> Preparing: prepare()
    Preparing --> Prepared
    Prepared --> Updating: update()
    Updated --> Updating: update()
    Updating --> Updated
    Updating --> Paused: pause requested
    Updating --> Done: done or stop requested
    Paused --> Updated: resume()
    Initialized --> Finishing: finish()
    Validated --> Finishing: finish()
    Invalid --> Finishing: finish()
    Prepared --> Finishing: finish()
    Updated --> Finishing: finish()
    Paused --> Finishing: finish()
    Done --> Finishing: finish()
    Failed --> Finishing: finish()
    Failed --> Initializing: initialize()
    Finishing --> Finished
    Finished --> [*]
</pre>

Every status except `Finished` and `Failed` may also go to `Failed`. `Initialized` may go back to
`Initializing` in the table, but the SDK's `initialize()` refuses to run from `Initialized`.
`Invalid` is not a failure: the caller rewires, adds or replaces components and validates again,
without re-initializing.

Like components, the SDK workflow checks every status change against the table, performs it even
when the table rejects it, and reports each rejected pair once as an `Error` with code
`Diagnostic::IllegalStatusTransition` (1001) and the message
`illegal workflow status transition A -> B (Helpers::isValidWorkflowStatusTransition rejects it)`.
Whenever the workflow becomes `Failed` it queues a `Fatal` entry, code 0, carrying the message of
the failure, or `workflow failed` when there is none.

### What each method requires

| Method | Legal from | In any other status |
| --- | --- | --- |
| `addModelComponent()`, `removeModelComponent()` | any | |
| `initialize()` | `Created`, `Failed` | Queues a `Warning`, `initialize() ignored in state S`, and returns |
| `validate()` | `Initialized`, `Validated`, `Invalid` | Returns one message, `validate() requires the Initialized, Validated or Invalid state (current: S)`; changes and queues nothing |
| `prepare()` | `Validated` | Fails the workflow: `prepare() requires the Validated state (current: S)` |
| `update()` | `Prepared`, `Updated` | Fails the workflow: `update() requires the Prepared or Updated state (current: S)` |
| `resume()` | `Paused` | Does nothing |
| `finish()` | any but `Finished` | Does nothing when `Finished` |
| `requestStop()`, `requestPause()`, `status()`, `errors()` | any, from any thread | |

Two consequences matter for a host loop. Stop calling `update()` once the status is `Done`: one
more call fails the workflow. And call `resume()` before stepping a `Paused` workflow: `update()`
from `Paused` fails it too.

## The lifecycle, step by step

### Building the composition

`addModelComponent(component, role, &message)` adds a component and sets its workflow back-pointer
(`component->workflow()` then returns the workflow). Components keep their insertion order, which
decides the order inside a feedback loop and, reversed, the order of components that do not depend
on each other.

It returns `false`, with the message set, when the component is null (`component is null`),
already managed (``component 'ID' is already managed``), or the role is unknown (``unknown role 'R'``)
or already filled (``role 'R' is already filled by 'ID'``). *Roles* are named slots a workflow
subclass declares with `addModelComponentLabel(label, required)`; `validate()` fails while a
required slot is empty, `modelComponentLabels()` lists them, `isRequiredModelComponent()` and
`componentForRole()` look them up. The SDK's two strategies declare no roles, so they take
components without one (`role` is `nullptr`).

The workflow does not own its components. Its destructor clears the back-pointer of every managed
component, so each component must still exist when the workflow is destroyed: destroy the workflow
first, or remove the component with `removeModelComponent()` before destroying it. The example
declares the workflow after its components for this reason.

### initialize()

`initialize()` clears pending stop and pause requests, sets `Initializing`, then visits the
components in insertion order. A component that is `Created` or `Failed` is initialized; a
component in any other status is left as it is, so a host may initialize components itself first
(as @ref HydroCouple::SDK::IO::ModelInitializer does). If a component is `Failed` afterwards, the
workflow fails with ``component 'ID' failed to initialize``; read the component's own `errors()` for
the reason. Then `onInitialize()` runs (a subclass hook; `false` fails the workflow with its
message) and the status becomes `Initialized`.

Components create their inputs and outputs in `initialize()`, so connections are made after it:

@snippet ch11_timestepped_cycle/main.cpp compose

### validate()

`validate()` checks the composition as a whole. It sets `Validating` and collects problems:

1. every required role filled (``required role 'R' is not filled``);
2. every component's own `validate()`, called whatever the component's status, each problem
   prefixed with ``component 'ID': ``;
3. every connection, through `validateConnections()` (next section);
4. the strategy's own checks, through `validateComposition()`: the pull-driven workflow requires a
   trigger.

Connection findings of `Warning` severity are queued on the workflow's error queue with their codes
and do not count as problems. Every other problem is queued as an `Error` (code 0 for the role,
component and strategy problems, the finding's code for connection errors). The status ends
`Validated` when there are no problems and `Invalid` otherwise, with the event message
`composition validation found N problem(s)`. The returned vector lists the problems in the order
roles, components, strategy, connection errors.

### prepare()

`prepare()` requires `Validated`. It sets `Preparing` and calls `prepare()` on every component in
insertion order, whatever the component's status; a component that is `Failed` afterwards fails
the workflow with ``component 'ID' failed to prepare``. Then `onPrepare()` runs; the time-stepped
workflow builds its schedule there. The status becomes `Prepared`.

### update()

`update()` is one *synchronization point*: one step of the whole composition. It requires
`Prepared` or `Updated`, sets `Updating` and calls the strategy's `performStep()`. When the step
fails, the workflow fails with the step's message, for example
``component 'ID' failed during update``. After a successful step it decides, in this order:

1. a stop was requested: `Done`, with the message `stopped on request`;
2. `isCompositionDone()` is true, by default when every component is `Done` or `Finished` (or
   there are none): `Done`;
3. a pause was requested: the request is consumed and the status becomes `Paused`, with the
   message `paused on request`;
4. otherwise `Updated`.

<pre class="mermaid">
sequenceDiagram
    participant Host
    participant Workflow as TimeSteppedWorkflow
    participant A as Provider component
    participant B as Consumer component
    Host->>Workflow: update()
    Note over Workflow: Updating
    Workflow->>A: update()
    A->>A: applyInputValues(), compute, write outputs
    Workflow->>A: refresh adapted outputs on A's outputs
    Workflow->>B: update()
    B->>A: read provider values (applyInputValues)
    B->>B: compute, write outputs
    Note over Workflow: stop? done? pause? else Updated
    Workflow-->>Host: status()
</pre>

A host loop steps until `Done` or `Failed`, and normally caps the number of steps; Composer stops
after a fixed maximum.

### finish()

`finish()` sets `Finishing` from any status but `Finished` (from `Created` the table rejects it, and
the rejection is reported), calls `finish()` on every component that is neither `Finished` nor
`Created`, runs `onFinish()` and sets `Finished`. A finished workflow cannot be initialized again;
create a new one.

## TimeSteppedWorkflow

### Building the schedule

At `prepare()` the time-stepped workflow turns the connections into a schedule:

1. Each managed component is a node. For every input of every managed component, each provider (all
   providers of a multi-input) gives an edge from the provider's component to the consumer's. A
   provider that is an adapted output is walked back through `adaptee()` to the output at the root
   of its chain, and the edge leaves the root's component; a third-party adapter that does not
   report the right `modelComponent()` therefore still orders correctly. Providers whose component
   is not managed, and a component's connections to itself, add no edge.
2. Tarjan's algorithm groups the nodes into *strongly connected components*: sets in which every
   member can reach every other through the edges. A group with more than one member is a feedback
   loop.
3. The groups are ordered so that providers run before their consumers (the reverse of the order
   Tarjan's algorithm emits them), and the members of each group are sorted by insertion order.

`schedule()` returns the result: a vector of groups, empty before `prepare()`. The ordering rule has
one consequence that is easy to miss: groups with no path between them run in reverse insertion
order. Three unconnected components added as x, y, z run z, y, x:

@snippet ch11_pull_and_validation/main.cpp order

```text
schedule without links
  added x y z, scheduled z y x
```

Connect components, or rely on the data they exchange, rather than on insertion order.

### One step

`performStep()` visits the groups in schedule order. Each component that is neither `Done` nor
`Finished` gets `update()` with an empty list of required outputs; when it ends `Failed`, the step
stops and the workflow fails. After each update the workflow calls `refresh()` on the adapted
outputs registered directly on that component's outputs (each SDK adapter refreshes the adapters
built on it), because a time-stepped run never calls `IOutput::updateValues()`, where adapted
outputs normally refresh.

The workflow does not move values into inputs. Each component reads its own inputs in `update()`,
normally with `AbstractModelComponent::applyInputValues()`, from whatever its providers hold at that
moment. A provider earlier in the schedule has already stepped, so its consumer reads the value of
the current step.

A component that sets `WaitingForData` is updated again in the next step. The SDK does not detect a
composition whose components all wait for each other: such a run continues until the host's step
cap. A component that is `Done` is skipped, and the adapted outputs on its outputs are no longer
refreshed.

### Feedback loops

Inside a feedback group the members run in insertion order, once per step by default. The first
member therefore reads its partners' values from the previous step: the coupling is *time-lagged*,
as in OpenMI. The example couples a reservoir and an operator that sets the reservoir's release from
its storage. Over a step of \f$\Delta t\f$ seconds,

\f[ S_n = S_{n-1} + \Delta t \, (I_n - R_{n-1}), \qquad R_n = c \max(S_n - S_{target}, 0) \f]

The reservoir, added before the operator, uses the release \f$R_{n-1}\f$ of the previous step; the
operator uses the storage \f$S_n\f$ of this one.

@snippet ch11_timestepped_cycle/main.cpp stepped

@snippet ch11_timestepped_cycle/main.cpp run

```text
time-lagged cycle (1 sweep)
  group: source
  group: reservoir operator  (feedback loop)
  step 1: inflow   4.0 m3/s, storage   2400.0 m3, release  0.000 m3/s
  step 2: inflow   9.0 m3/s, storage   7800.0 m3, release  5.600 m3/s
  step 3: inflow  12.0 m3/s, storage  11640.0 m3, release 13.280 m3/s
  step 4: inflow   8.0 m3/s, storage   8472.0 m3, release  6.944 m3/s
  step 5: inflow   5.0 m3/s, storage   7305.6 m3, release  4.611 m3/s
  step 6: inflow   3.0 m3/s, storage   6338.9 m3, release  2.678 m3/s
  workflow statuses: Initializing Initialized Validating Validated Preparing Prepared Updating Updated Updating Updated Updating Updated Updating Updated Updating Updated Updating Done Finishing Finished
```

With \f$\Delta t = 600\f$ s, \f$c = 0.002\f$ 1/s and \f$S_{target} = 5000\f$ m³, step 2 gives
\f$S_2 = 2400 + 600 \times (9 - 0) = 7800\f$ m³ and \f$R_2 = 0.002 \times 2800 = 5.6\f$ m³/s. The
example repeats the recurrence in a plain loop and checks every step:

@snippet ch11_timestepped_cycle/main.cpp reference

To swap the lag, add the operator first: it then reads the storage of the previous step and the
reservoir the release of the current one.

### Iteration count and convergence

`setIterationsPerGroup(n)` makes every feedback group run \f$n\f$ *sweeps* per step; a group of one
component always runs once. The default, 1, is the time-lagged coupling above. A value below 1 is
ignored without a message, and the value may change between steps.

A sweep is a full `update()` of every member. The workflow has no notion of repeating a step: it
does not save and restore component state between sweeps and does not test whether the values
converged; it runs exactly \f$n\f$ sweeps. A component whose `update()` advances its clock therefore
advances it \f$n\f$ times per workflow step:

@snippet ch11_timestepped_cycle/main.cpp sweeps

```text
two sweeps per cyclic group
  after one step: source 1 update, reservoir 2 updates, operator 2 updates
  the reservoir reached Done after 3 workflow steps; the run took 6
```

Use more than one sweep only with components whose `update()` re-solves the same interval when it
is called again within a step (an implicit coupling written for it), and decide yourself how many
sweeps reach the accuracy your coupled model needs; check it by comparing runs with different
counts, or by a residual the components compute. If your model requires convergence, specify and
verify that condition for the coupled model: the SDK provides no convergence test.

The composition document's `iterations_per_group` (@ref sdk_ref_composition) is parsed into
`IO::WorkflowSpec::iterationsPerGroup`, but no SDK class applies it: the host must pass it to
`setIterationsPerGroup()`.

## PullDrivenWorkflow

A pull-driven workflow is driven from one input, the *trigger*, declared with `setTrigger()` before
`validate()`. The trigger is usually a probe the host owns (an `Input1DDouble` with no component)
connected to the most downstream output of interest. `validate()` fails with
`no trigger input declared (see setTrigger())` when no trigger is set, and with
``trigger input 'ID' has no connected provider`` when it is not connected.

One `update()` services one request chain:

1. For each provider of the trigger, its component is *serviced*: first, recursively, every
   component that provides one of its inputs; then each such provider's
   `updateValues(input)`, then the input's `retrieveValuesFromProvider()` and `applyData()`; then the
   component's own `update()`.
2. Each provider of the trigger is asked for `updateValues(trigger)`, and the trigger retrieves and
   applies the values.

A component is serviced at most once per step; a visited set cuts feedback loops, so the loop
member reached second reads the first one's values from the previous step. A component that is
`Done` or `Finished` is not updated and its own inputs are not serviced. The walk follows each
output's `modelComponent()` and does not check that the component is managed by the workflow, so a
provider upstream of the trigger is updated even if it was never added. The trigger's own component,
when it has one, is not updated: the trigger only reads. The workflow is `Done` when every managed
component is, which is why the trigger is best left outside the managed components.

@snippet ch11_pull_and_validation/main.cpp pull

```text
pull-driven chain
  without a trigger: no trigger input declared (see setTrigger())
  pulled 11.0, updates so far: a b
  pulled 22.0, updates so far: a b a b
  pulled 33.0, updates so far: a b a b a b
```

The components were added as b, a; the pull still updates a before b, because b's input asks for
it. A composition document names the trigger with `workflow.trigger` (the `component` and `input`
ids), parsed into `IO::WorkflowSpec::triggerComponent` and `triggerInput`; as with the iteration
count, the host looks the input up and calls `setTrigger()`. The document's trigger must be an input
of a declared component. When the host also adds that component to the workflow, the workflow never
updates it, and reaches `Done` only if that component becomes `Done` or `Finished` some other way; a
host can leave the trigger's component out of the workflow, or override `isCompositionDone()`.

## Connection validation

### What is compared

@ref HydroCouple::SDK::validateConnection compares what a provider and a consumer declare about
their values and returns a list of findings. An `Error` means both ends declare and contradict each
other, so the coupled model would compute wrong numbers without an error (means read as instants,
NAVD88 heights read as NGVD29, face values read as node values). A `Warning` means a declaration is
missing, so the check could not be made. An empty list means every check that could be made passed.
@ref HydroCouple::SDK::hasConnectionError is `true` when any finding is an error.

The checks run in this order, and each runs only when both ends offer what it compares:

| Check | Runs when | Section |
| --- | --- | --- |
| Element type | always | @ref sdk_guide_quantities |
| Value definitions: value kind, quantity name, quantity or quality, categories, units | always (skipped for an adapted output without a value definition) | @ref sdk_guide_quantities |
| Time kind and interval, interpolation, extrapolation | both ends are `HydroCouple::Temporal::ITimeSeriesComponentDataItem` | @ref sdk_guide_time |
| Horizontal and vertical reference systems | both ends are spatial items with a reference system | @ref sdk_guide_space |
| Mesh location, scalar or vector structure, vector basis | both ends are polyhedral-surface items, or both network items; for grids, location only | @ref sdk_guide_space |
| Layering | both ends implement `HydroCouple::Spatial::ILayering` | @ref sdk_guide_space |
| Dimension roles | both ends have the same rank | @ref sdk_guide_quantities |

The provider compared is the consumer's immediate provider: through an adapted output, the adapter.
A spatial item's reference system is its first geometry's, its surface's, its network's first
vertex's, its raster's or its grid's; a reference system counts as declared when its SRID is
nonzero and its authority name is not empty.

### Every finding

Each message begins with ``link 'P' -> 'C': ``, where P and C are the provider and consumer ids. In
the table, *side* is `provider` or `consumer`, whichever end lacks the declaration.

| Code | Severity | Message | Cause |
| --- | --- | --- | --- |
| 1101 | Error | `element types differ (the data plane converts nothing)` | Different `dataKind()` |
| 1102 | Warning | `side declares no value definition; units and value kind cannot be checked` | `valueDefinition()` is null on one end |
| 1102 | Error | `provider is a quantity, consumer a quality` (or the reverse) | A quantity meets a quality |
| 1102 | Error | ``the qualities' category lists differ, so category index k names different things on the two ends`` | Different categories, or a different order |
| 1103 | Error | `provider values are A, consumer expects B` | Different declared `ValueKind` |
| 1103 | Warning | `side declares no ValueKind (the other says K); an adapter could not choose between averaging and summing` | One end leaves it `Unknown` |
| 1103 | Warning | `neither end declares a ValueKind` | Both leave it `Unknown` |
| 1104 | Error | `units differ in dimension (P vs Q)` | Different powers, both dimensioned |
| 1104 | Warning | `units differ in dimension (dimensionless vs Q); one side may simply not have declared its unit` | One unit dimensionless |
| 1104 | Error | ``units share a dimension but not a scale ('U' vs 'V'); wire a unit-conversion adapter between them`` | Same powers, different factor or offset |
| 1104 | Warning | `side quantity declares no unit` | A quantity without a unit |
| 1104 | Warning | `a unit declares no dimensions` | `dimensions()` is null |
| 1105 | Error | `provider timestamps are A, consumer expects B` | Different `TimeKind` |
| 1105 | Error | `both are K but over different intervals (a vs b days)` | Same interval kind, both interval lengths positive and different |
| 1105 | Warning | `side declares no TimeKind; the consumer must not assume Instantaneous` | One end `Unknown` |
| 1105 | Warning | `neither end declares what its timestamps mean (TimeKind)` | Both `Unknown` |
| 1106 | Error | `provider interpolates A between its instants; consumer accepts B` | The provider fabricates values between its instants in a way the consumer does not accept (a provider that interpolates `None` is accepted by any consumer) |
| 1106 | Error | `provider extrapolates A beyond its range; consumer accepts B` | The same beyond the range (a provider that `Refuse`s is accepted by any consumer) |
| 1106 | Error | `neither end declares a time interpolation policy`, `neither end declares a time extrapolation policy` | Both `Unknown`; an error by the standard's own rule |
| 1106 | Warning | `side declares no time interpolation policy`, `side declares no time extrapolation policy` | One end `Unknown` |
| 1107 | Error | `horizontal reference systems differ (A:n vs B:m); wire a reprojecting adapter between them` | Both declared, different authority or code |
| 1108 | Error | `vertical datums differ (A:n vs B:m); elevations need a datum shift` | Both declared, different |
| 1108 | Error | `same vertical datum, different vertical units` | Same datum, both units known and different |
| 1108 | Warning | `side declares no vertical datum while the other does; elevations cannot be checked` | Only one end declares a datum |
| 1109 | Error | `provider values sit on X, consumer expects them on Y` (grids: `provider grid values sit on X, consumer expects Y`) | Node, edge, face or volume values on different entities |
| 1110 | Error | `provider values are S, consumer expects T` | Scalar, multi-scalar, vector or tensor structure differs |
| 1110 | Error | `vector bases differ (A vs B); two numbers per entity mean different things on the two ends` | Vectors or tensors in different bases |
| 1110 | Warning | `side declares no vector basis for S values` | A vector or tensor end without a basis |
| 1111 | Error | `side is layered but has no vertical coordinate (ILayering requires one)` | A layered item without a vertical coordinate |
| 1111 | Error | `layer k means different things (A vs B); wire a vertical remapping adapter between them` | Different vertical coordinate kinds |
| 1111 | Error | `layer counts differ (a vs b)` | Different numbers of layers |
| 1111 | Warning | `side declares no vertical coordinate kind` | One kind `Unknown` |
| 1112 | Error | `axis N is A on the provider and B on the consumer` | Different roles on the same axis; `Unknown` and `Other` are skipped |
| 1113 | Warning | `provider measures URI, consumer expects URI` or ``provider measures 'NAME', consumer expects 'NAME'`` | Different concept URIs, or, when either lacks one, different CF standard names |

### In the workflow

`AbstractWorkflowComponent::validate()` calls `validateConnections()`, which by default runs
`validateConnection()` over the provider of every input of every managed component (every provider of
a multi-input). An input that belongs to no managed component, such as a pull trigger owned by the
host, is not checked. A subclass may override `validateConnections()` to add checks or to drop
findings it handles elsewhere; returning an empty vector turns the checks off.

Errors make the composition `Invalid`; warnings only reach the error queue. Fix the composition and
call `validate()` again:

@snippet ch11_pull_and_validation/main.cpp validate

```text
link validation
  status Invalid, 1 problem(s)
  error queue: code 1104: link 'out' -> 'in': units share a dimension but not a scale ('ft³/s' vs 'm³/s'); wire a unit-conversion adapter between them
  after rewiring: Validated
```

A host can run the same check when a user draws a connection, before any workflow exists, since
`validateConnection()` reads declarations only and changes nothing.

## Pause, resume and stop

`requestPause()` and `requestStop()` only set atomic flags, so they may be called from any thread
(a user-interface thread, a signal-handling thread that relays a signal) while `update()` runs on
the driving thread. They take effect at the next synchronization point, after the step in flight:

- after a stop request, the next completed step ends the run as `Done` (`stopped on request`), so
  `finish()` can write a consistent final state. The flag stays set until `initialize()`, which a
  `Done` workflow cannot call, so a stopped run stays stopped;
- after a pause request, the next completed step that is neither stopped nor done ends `Paused`
  (`paused on request`) and clears the request. A paused composition is at a consistent point, the
  moment to checkpoint components that support it. `resume()` returns to `Updated` (`resumed`).

@snippet ch11_pull_and_validation/main.cpp control

```text
control
  after a pause request: Paused
  after a stop request: Done, tick updated 2 times
  update() after Done: Failed
  error queue: update() requires the Prepared or Updated state (current: Done)
```

The last two lines show the rule from the start of the chapter: `update()` after `Done` fails the
workflow. `finish()` is still legal from `Failed`.

The protected accessors `stopRequested()` and `pauseRequested()` are declared but not defined in
this version of the library, so a subclass that calls them fails to link; read the effect through
`status()` instead.

## Status events and errors

Every status change emits one event to the slots connected with `connect()`, synchronously, on the
driving thread. The event is a @ref HydroCouple::SDK::WorkflowStatusChangeEventArgs with these
fields:

| Field | Value |
| --- | --- |
| `workflowComponent()` | The workflow, not owned |
| `previousStatus()`, `status()` | The statuses before and after the change |
| `message()` | The message of the change: a failure reason, `stopped on request`, `paused on request`, `resumed`, `composition validation found N problem(s)`, or empty |
| `hasProgressMonitor()`, `percentProgress()` | Always `false` and 0 for the SDK workflows |

@snippet ch11_timestepped_cycle/main.cpp events

The run above produced the events `Initializing Initialized Validating Validated Preparing Prepared`,
then `Updating Updated` per step, then `Updating Done Finishing Finished`. Unlike a component, a
workflow does not also fire its property-change signal with `"Status"`. `blockSignals()` silences
both of its signals.

`errors()` returns the workflow's own queue: the problems and connection findings of `validate()`,
the `Fatal` entry of every failure, the `Warning` of an ignored `initialize()`, and illegal
transitions. It does not include the components' queues; when the workflow reports
``component 'ID' failed ...``, read that component's `errors()` for the cause.

## Execution modes and where workflows come from

The usual host does not wire a workflow by hand. It reads a composition document with
@ref HydroCouple::SDK::IO::ModelInitializer, which applies argument values, initializes the
components and makes the connections through the host's instance lookup; the host checks the result
and message of `initializeFromFile(path, message)` before it builds the workflow. The SDK's
`serial_coupling` example supplies its own catchment and channel instances; a plugin-backed host such
as Composer creates each instance from its component type (@ref sdk_guide_component). The document,
`IO::CompositionSpec` and `IO::WorkflowSpec` are described in @ref sdk_guide_compositions and
@ref sdk_ref_composition.

Each component block of a document has an *execution mode*:

| Mode | What the initializer does | What the workflow sees |
| --- | --- | --- |
| `run` (default) | Initializes the live component | An ordinary component |
| `open` | Lets a live component that loads its own results do so; when it does not end `Finished`, or there is no live instance, substitutes a @ref HydroCouple::SDK::ResultsModelComponent opened from the block's run manifest | A component that is `Finished` from the start: the workflows never update it, `initialize()` and `finish()` skip it, and `isCompositionDone()` counts it as done. Its outputs still serve the recorded values |
| `resume` | Refuses the document: `execution mode "resume" is reserved and not yet implemented (see ICheckpointableModelComponent)` | |

`prepare()` is still called on every managed component, including a `Finished` one. A
`ResultsModelComponent`'s `prepare()` does nothing, but a component of your own that finishes during
`initialize()` (to serve its own results) must also tolerate a `prepare()` call, or stay out of the
workflow.

A recorded item has a time axis that a live input usually does not want; a
@ref HydroCouple::SDK::TimeSliceOutput presents one time level of it as an ordinary output, and the
host selects the level each step (@ref sdk_guide_record).

The document's `workflow` block names the strategy (`time_stepped`, `pull_driven` or `none`), the
trigger, the iteration count and an optional cap on the number of steps (`max_steps`, 0 for none). The SDK workflow classes
do not read the block; the host constructs the named workflow and applies the other fields with
`setTrigger()`, `setIterationsPerGroup()` and its own step cap.

## Running the examples

```text
cmake --build build --target docs_example_ch11_timestepped_cycle docs_example_ch11_pull_and_validation
ctest --test-dir build -R "docs.example.ch11_" --output-on-failure
```

Each program checks the values, orders and statuses it prints and returns nonzero if any is wrong.
Both end with `all checks passed`.

## Thread safety

| What | Rule |
| --- | --- |
| `status()`, `errors()` | Any thread |
| `requestStop()`, `requestPause()` | Any thread; not async-signal-safe, so a POSIX signal handler sets its own flag and lets a thread call them |
| Every other method | One driving thread |
| Status events | Delivered on the driving thread, during the call that changed the status |

## Diagnostics

| Message | Status after | Cause | Fix |
| --- | --- | --- | --- |
| `component is null`, ``component 'ID' is already managed``, ``unknown role 'R'``, ``role 'R' is already filled by 'ID'`` | unchanged (`addModelComponent()` returns `false`) | Adding a component | Add each component once; use a declared role |
| `initialize() ignored in state S` (`Warning`) | unchanged | `initialize()` outside `Created` and `Failed` | Create a new workflow to run again |
| ``component 'ID' failed to initialize`` | `Failed` | A component's `initialize()` failed | Read that component's `errors()` |
| `validate() requires the Initialized, Validated or Invalid state (current: S)` | unchanged | `validate()` too early or too late | Call it after `initialize()` and before `prepare()` |
| ``required role 'R' is not filled`` | `Invalid` | A required role is empty | Add a component into the role |
| ``component 'ID': MESSAGE`` | `Invalid` | A component's `validate()` returned a problem | See the component's message |
| `no trigger input declared (see setTrigger())`, ``trigger input 'ID' has no connected provider`` | `Invalid` | Pull-driven workflow without a usable trigger | Call `setTrigger()` with a connected input |
| ``link 'P' -> 'C': ...`` | `Invalid` for an error | A connection finding (table above) | Fix the declaration or insert an adapted output |
| `composition validation found N problem(s)` (event message) | `Invalid` | Summary of the problems returned by `validate()` | Fix them and validate again |
| `prepare() requires the Validated state (current: S)` | `Failed` | `prepare()` before a successful `validate()` | Validate first |
| ``component 'ID' failed to prepare`` | `Failed` | A component's `prepare()` failed | Read that component's `errors()` |
| `update() requires the Prepared or Updated state (current: S)` | `Failed` | `update()` after `Done`, while `Paused`, or before `prepare()` | Stop at `Done`; `resume()` first |
| ``component 'ID' failed during update`` | `Failed` | A component ended its `update()` `Failed` | Read that component's `errors()` |
| `illegal workflow status transition A -> B (...)` (`Error`, 1001) | performed | A transition the table rejects, such as `finish()` on a `Created` workflow | Follow the lifecycle |
| `workflow failed` | `Failed` | A failure without a message (subclass) | Give the failure a message |

@ref sdk_ref_diagnostics collects the codes and messages of every module.
