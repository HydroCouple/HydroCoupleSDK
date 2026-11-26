@page sdk_guide_adapted_outputs 8. Adapted outputs

[TOC]

Two models rarely publish a value exactly as the other wants it. One reports flow in m³/s and the
other reads ft³/s; one records an hourly series and the other needs the value at its own time step;
a feedback loop between two components oscillates unless the exchanged value is damped. An
*adapted output* sits between an output and an input and performs that transformation. It is an
output in its own right (`HydroCouple::IAdaptedOutput` derives from `HydroCouple::IOutput`), built
over another output called its *adaptee*, so a consumer connects to it exactly as it would to the
original output, and adapted outputs stack into a *chain*.

This chapter describes the adapted-output part of the SDK's `data` and `temporal` modules: the base
class, the factories that create adapted outputs, the SDK's own factory and its three adapters
(linear transform, relaxation, temporal interpolation), the older `TimeSeriesAdaptedOutput`, how a
chain is built by hand and from a composition document, and how its values are refreshed. The
exchange items it builds on are in @ref sdk_guide_ports; how a composition names a chain is in
@ref sdk_guide_compositions; differentiating through adapters is in @ref sdk_guide_differentiation.

## The classes in this chapter

| Class | Implements | Purpose |
| --- | --- | --- |
| @ref HydroCouple::SDK::AbstractAdaptedOutput | `IAdaptedOutput` | Adaptee, factory, arguments, consumers, chained adapters, refresh propagation |
| @ref HydroCouple::SDK::AbstractAdaptedOutputFactory | `IAdaptedOutputFactory` | Base of every factory: offerings and creation |
| @ref HydroCouple::SDK::AbstractAdaptedOutputFactoryComponent | `IAdaptedOutputFactoryComponent` | A factory delivered as a plugin of its own |
| @ref HydroCouple::SDK::AbstractAdaptedOutputFactoryComponentInfo | `IAdaptedOutputFactoryComponentInfo` | The metadata and constructor of such a plugin |
| @ref HydroCouple::SDK::SdkAdaptedOutputFactory | `IAdaptedOutputFactory` | The SDK's factory: `linear_transform`, `relaxation`, `temporal_interpolation` |
| @ref HydroCouple::SDK::LinearTransformAdaptedOutput | `IAdaptedOutput`, `IDifferentiableAdaptedOutput` | \f$y = m\,x + b\f$ element by element |
| @ref HydroCouple::SDK::RelaxationAdaptedOutput | `IAdaptedOutput`, `IDifferentiableAdaptedOutput`, `ICheckpointableAdaptedOutput` | Under-relaxation across refreshes |
| @ref HydroCouple::SDK::Temporal::TemporalInterpolationAdaptedOutput | `IAdaptedOutput` | A time series's value at a query time |
| @ref HydroCouple::SDK::Temporal::TimeSeriesAdaptedOutput | `IAdaptedOutput` | Interpolates a `TimeSeries` supplied in code (no factory) |

Three compiled examples go with this chapter: `docs/examples/ch08_adapter_chain/main.cpp` builds a
two-step chain by hand, `docs/examples/ch08_relaxation/main.cpp` exercises the relaxation adapter's
state, checkpoint and derivative, and `docs/examples/ch08_composition_chain/main.cpp` declares a
chain in a composition document. Each runs as a ctest test (`docs.example.ch08_adapter_chain` and
so on).

## How an adapted output works

### Lifecycle

<pre class="mermaid">
sequenceDiagram
    participant H as Host or ModelInitializer
    participant F as Factory
    participant A as Adapted output
    participant O as Adaptee (output)
    participant I as Consumer input
    H->>F: getAvailableAdaptedOutputIds(output, input)
    F-->>H: offerings
    H->>F: createAdaptedOutput(offering, output, input)
    F->>A: construct over the adaptee
    F->>O: addAdaptedOutput(A)
    F-->>H: unique_ptr (H owns A)
    H->>A: arguments()[k]->initialize(json)
    H->>A: initialize()
    A->>O: getValuesInto(...)
    H->>A: addConsumer(I)
    loop every exchange
        O->>A: refresh() (from refreshAdaptedOutputs())
        A->>O: getValuesInto(...)
        A->>A: refresh its own adapted outputs
        I->>A: getValuesInto(...) (retrieveValuesFromProvider())
    end
    H->>A: destroy (deregisters from O)
</pre>

1. **Discovery.** A host asks a factory what it can make for an output, and optionally for a given
   input: `getAvailableAdaptedOutputIds(provider, consumer)` returns *offerings*, identities with an
   id and a caption that the factory owns. An offering describes a kind of adaptation, not an
   instance.
2. **Creation.** `createAdaptedOutput(offering, provider, consumer)` returns a new adapter in a
   `std::unique_ptr`. By the standard's contract the adapter is already registered with the provider
   (`provider->addAdaptedOutput()`), and the caller owns it for as long as the connection exists.
3. **Configuration.** The adapter's `arguments()` hold their defaults. The host sets them with
   `IArgument::initialize()` before the next step, as it would a component's arguments.
4. **Initialization.** `initialize()` computes the first values from the arguments and the
   adaptee's current values. The standard requires a stack to be initialized from the adaptee down.
5. **Connection.** A consumer connects to the adapter with `addConsumer()`, the same wiring entry
   point an ordinary output has (@ref sdk_guide_ports); a chain's consumer connects to its tail.
6. **Refresh.** Whenever the adaptee has new values, `refresh()` recomputes. Each SDK adapter ends its
   refresh by refreshing the adapters built on it, so one call at the head reaches the whole chain.
7. **Destruction.** Destroying an SDK adapter removes it from its adaptee and detaches the adapters
   built on it, so a chain can be torn down from either end. Consumers are not disconnected: call
   `removeConsumer()` first.

### What AbstractAdaptedOutput provides

@ref HydroCouple::SDK::AbstractAdaptedOutput is to adapted outputs what `AbstractOutput` is to
outputs. It keeps the adaptee and the creating factory (both borrowed), the consumers in connection
order, the adapters chained on it keyed by id, and the arguments a subclass registers with
`addArgument()` in its constructor (also keyed by id; `arguments()` is in hash order). Its
`addConsumer()` and `removeConsumer()` behave exactly as `AbstractOutput`'s, including the
`std::invalid_argument` on refusal, whose message begins `Adapted output "ID" cannot be consumed by
input`. `states()` is empty unless a subclass overrides it.

The adapter's `modelComponent()` is its adaptee's component, captured at construction, so an
adapter's errors go to the provider's component and a pull-driven workflow can find the component
to advance behind an adapter. The SDK's concrete adapters report no dimensions and no value
definition: `dimensions()` is empty and `valueDefinition()` is nullptr. A consumer or a validator
that reads the units of its provider finds none through an adapter (the connection validator then
skips the value-definition checks, @ref sdk_guide_workflow).

The constructor does not register the adapter with its adaptee; a factory does. An adapter built
directly in code (with `new` or `std::make_unique`) is registered by calling
`adaptee->addAdaptedOutput(adapter)`, and until then refreshes of the adaptee do not reach it.

### Refreshing a chain

Values are pushed, not pulled. An adapter computes its values in `refresh()` and serves those values
until the next refresh; reading it never recomputes. What triggers a refresh:

| Caller | When |
| --- | --- |
| `AbstractOutput::refreshAdaptedOutputs()` | called by the output's `updateValues(query)`, or directly by a component or host after writing the output |
| `TimeSteppedWorkflow` | after every component update, on the adapters registered directly on that component's outputs (@ref sdk_guide_workflow) |
| An SDK adapter's `refresh()` | at its end, on the adapters chained on it |
| `AbstractAdaptedOutput::updateValues(query)` | when a pull-driven workflow asks the chain's tail for values: it calls the adapter's `updateValues()` (which for the SDK adapters is `refresh()`) and then refreshes the chained adapters again |
| the host | any time, for example after changing an argument or a query time |

Two consequences matter in practice. First, an adapter recomputes from its adaptee's *current*
values and never asks the adaptee to update; the chain is only as fresh as its head. Second, a
stateful adapter (relaxation) blends once per refresh, so the number of refreshes per exchange is
part of its result. `AbstractAdaptedOutput::updateValues(query)` on an adapter that has children
refreshes each child twice (once from the parent's `refresh()`, once from the explicit
`refreshAdaptedOutputs()` after it): a relaxation adapter chained under another adapter and reached
this way blends twice per exchange. The time-stepped workflow refreshes once.

Changing an adapter's query time or argument does not refresh anything by itself. In the chain
below, moving the temporal interpolation's query time changes what the interpolation serves at once
(it interpolates on read), but the linear transform after it still holds the value it computed at
its last refresh until it is refreshed.

## Factories

### Where a factory comes from

A factory is any `HydroCouple::IAdaptedOutputFactory`; the SDK's base is
@ref HydroCouple::SDK::AbstractAdaptedOutputFactory, an Identity whose id composition documents use
to name it. A factory reaches a run in one of three ways:

- a component's info lists it (`AbstractModelComponentInfo::addAdaptedOutputFactory()`), which offers
  it on that component's outputs;
- a host installs it as a *standalone* factory with
  `IO::ModelInitializer::setAdapterFactoryResolver()`, which offers it on every output;
- it is a plugin of its own: an @ref HydroCouple::SDK::AbstractAdaptedOutputFactoryComponentInfo
  creates @ref HydroCouple::SDK::AbstractAdaptedOutputFactoryComponent instances (a host such as
  Composer loads the library and passes the instances to the resolver).

When a composition step names no factory, the chain builder searches the component-info factories
first, so a component can deliberately shadow a standalone factory's offering with its own
(@ref sdk_guide_compositions). A factory must outlive every adapter it created, because each adapter
keeps a pointer to it.

### The SDK's factory

@ref HydroCouple::SDK::SdkAdaptedOutputFactory has the id `hydrocouple.sdk.adapters`. Hosts use the
process-wide object from `SdkAdaptedOutputFactory::instance()`.

| Offering id | Offered when the provider is | Creates | Arguments |
| --- | --- | --- | --- |
| `linear_transform` | Float64 with a rank-1 shape | `LinearTransformAdaptedOutput` | `multiplier` (1), `offset` (0) |
| `relaxation` | Float64 with a rank-1 shape | `RelaxationAdaptedOutput` | `relaxation_factor` (1) |
| `temporal_interpolation` | Float64, an `ITimeSeriesComponentDataItem`, shape `{ times, 1 }` | `TemporalInterpolationAdaptedOutput` | `method` (`"linear"`) |

When a consumer is named, nothing is offered unless its `dataKind()` is Float64; its shape is not
compared. Each created adapter's id is the offering id, `#` and a counter of the factory
(`linear_transform#1`, `relaxation#2`), so several adapters of one kind on one output do not replace
each other in the output's registry, which is keyed by id. `createAdaptedOutput()` returns nullptr
for an unknown offering id, a null offering or a null provider, without queuing an error; it does
not check the provider's fitness again, so an adapter created over a provider it was not offered for
holds no values and refuses reads. The factory's counter makes it unsafe to create adapters from two
threads at once.

The factory's class comment says the caller must unregister an adapter before destroying it. The
code does not require it: each SDK adapter deregisters itself in its destructor.

## A chain built by hand

`docs/examples/ch08_adapter_chain` interpolates an hourly flow record (m³/s) at a query time and
converts the result to ft³/s for a consumer. First the provider, a time series of three hourly
values:

@snippet ch08_adapter_chain/main.cpp provider

A time series of shape `{3, 1}` is offered only temporal interpolation; the element-wise adapters
want rank 1:

@snippet ch08_adapter_chain/main.cpp offerings

@snippet ch08_adapter_chain/main.cpp interpolate

The interpolated value is a rank-1 Float64 item of one element, so the second step is offered the
element-wise adapters. The transform's multiplier converts m³/s to ft³/s:

@snippet ch08_adapter_chain/main.cpp transform

@snippet ch08_adapter_chain/main.cpp consume

When the provider records a new hour and pushes, the interpolation appends the sample and refreshes
the transform. The query time is moved after that push, so the transform serves the old value until
it is refreshed:

@snippet ch08_adapter_chain/main.cpp refresh

Past the last sample, linear interpolation has no value; the read succeeds and returns NaN:

@snippet ch08_adapter_chain/main.cpp outside

@snippet ch08_adapter_chain/main.cpp teardown

Output (from the ctest run of `docs.example.ch08_adapter_chain`):

```text
offered on the {3, 1} time series: temporal_interpolation
adapter id temporal_interpolation#1, 3 samples, value at 01:30 = 13 m3/s
offered on the interpolated value: linear_transform, relaxation
consumer reads 459.0907 ft3/s (13 m3/s)
after the new hour: 4 samples; before transform refresh 459.0907, after 565.0347 ft3/s (16 m3/s)
query past the record: nan
after teardown: 0 adapters on the interpolation, 0 on flow
```

## The linear transform

@ref HydroCouple::SDK::LinearTransformAdaptedOutput applies, to every element \f$i\f$ of a rank-1
Float64 adaptee,

\f[ y_i = m\,x_i + b, \f]

with \f$m\f$ the `multiplier` and \f$b\f$ the `offset`. Unit conversion is the motivating use:
m³/s to ft³/s is \f$m = 35.3147\f$, °C to °F is \f$m = 1.8\f$, \f$b = 32\f$.

| Argument | Default | Meaning |
| --- | --- | --- |
| `multiplier` | 1 | \f$m\f$; an `Argument1DDouble` of length 1, set as `[2.0]` or `{"values": [2.0]}` |
| `offset` | 0 | \f$b\f$, in the units of the result |

Both are read at every refresh, not latched at `initialize()`, so a calibration loop can change them
between runs without initializing the adapter again; a change applies from the next refresh. Only
element 0 of each argument is used. An argument set to an empty array makes the next refresh throw
`std::out_of_range`.

`initialize()` is the first refresh. A refresh reads the adaptee's whole slab; when the adaptee is
gone, is not rank 1 or refuses the read, the adapter keeps its previous values and fires nothing.
`shape()` is the adaptee's current shape and `dataKind()` is Float64. Before the first successful
refresh a read fails with `LinearTransformAdaptedOutput "ID" has no values; initialize() has not
run`; a write always fails with `LinearTransformAdaptedOutput is read-only`. The transform records
no units: its value definition is null, so a consumer cannot tell from it that the values are now
ft³/s.

The transform is differentiable (`HydroCouple::IDifferentiableAdaptedOutput`). At the most recent
refresh, with \f$\bar{y}\f$ the cotangent of its output,

\f[ \bar{x}_i = m\,\bar{y}_i, \qquad \bar{m} = \sum_i \bar{y}_i\,x_i, \qquad \bar{b} = \sum_i \bar{y}_i, \f]

and forward, \f$\dot{y}_i = m\,\dot{x}_i + \dot{m}\,x_i + \dot{b}\f$. `differentiableArguments()` is
the multiplier and the offset, `differentiableStates()` is empty, and a vjp or jvp before any
refresh is refused with `has no refresh to differentiate`.

## The relaxation adapter

@ref HydroCouple::SDK::RelaxationAdaptedOutput damps a value across refreshes: with \f$\lambda\f$ the
`relaxation_factor`, refresh \f$k\f$ computes

\f[ y^{(k)}_i = (1-\lambda)\,y^{(k-1)}_i + \lambda\,x^{(k)}_i. \f]

It damps oscillation in a cyclic exchange group, which a time-stepped workflow sweeps several times
per step, and between two components in a stepped composition it acts as a first-order lag.
\f$\lambda = 1\f$ (the default) passes the adaptee through, \f$\lambda = 0\f$ freezes the first
values, and values between damp; the factor is not range-checked. It is read at every refresh.

`initialize()` clears the history and refreshes once. A refresh with no history (the first one, or
any refresh after the adaptee changed length) passes the adaptee through and starts blending from
there. Like the transform, the adapter is rank 1 and Float64, keeps its values when the adaptee
cannot be read, and refuses writes.

The adapter is deliberately stateful: two refreshes at the same adaptee values produce different
results when \f$\lambda < 1\f$. Its state is its own previous values, so `states()` and
`differentiableStates()` both list the adapter itself, and an orchestrator that returns a
composition to an earlier step knows it must return this adapter too.

@snippet ch08_relaxation/main.cpp create

@snippet ch08_relaxation/main.cpp blend

@snippet ch08_relaxation/main.cpp state

### Checkpoints

The adapter is an `HydroCouple::ICheckpointableAdaptedOutput`. `saveState()` returns a token that
*is* the state: the tag `relaxation-v1`, the value count, and each value's IEEE-754 bit pattern as 16
hexadecimal digits. The token is exact for every double and independent of locale, so another
process can restore it, and `releaseState()` has nothing to free. `restoreState()` refuses a token
that is not a relaxation token, holds a number of values other than the adaptee's current length
(an empty token restores anywhere), is truncated, or has trailing text; a refused token changes
nothing. A restore replaces the values without firing an event or refreshing anything, and the next
vjp or jvp is refused until a refresh.

@snippet ch08_relaxation/main.cpp checkpoint

### Derivative

With \f$t = \bar{y}_{\mathrm{out}} + \bar{y}_{\mathrm{state}}\f$ the sum of the cotangents of the
output and of the state carried to the next refresh, a blending refresh has

\f[ \bar{x} = \lambda\,t, \qquad \bar{y}^{(k-1)} = (1-\lambda)\,t, \qquad
    \bar{\lambda} = \sum_i t_i \bigl(x^{(k)}_i - y^{(k-1)}_i\bigr), \f]

and a passthrough refresh has \f$\bar{x} = t\f$ and nothing else. Seeds name the adapter in roles
Output and StateAfter; results name the adaptee (Input), the adapter (StateBefore) and the
`relaxation_factor` argument (Argument). The buffers are whole-item Float64.

@snippet ch08_relaxation/main.cpp derivative

Output (from the ctest run of `docs.example.ch08_relaxation`):

```text
after initialize: 1 2
refresh 1: 2 4
refresh 2: 2.5 5
refresh 3: 2.75 5.5
states(): 1 item, the adapter itself: yes
token: relaxation-v1 2 4006000000000000 4016000000000000
restored: 2.75 5.5
three values for a two-value adaptee: RelaxationAdaptedOutput "relaxation#1": the token holds 3 values but the adaptee has a different shape
vjp: x_bar 0.5 0.5, y_prev_bar 0.5 0.5, lambda_bar 0.75
```

## Temporal interpolation

@ref HydroCouple::SDK::Temporal::TemporalInterpolationAdaptedOutput serves the value of a
single-column Float64 time series at a *query time*, interpolated from the samples it has recorded.
It is the adapter for a consumer that needs a provider's value between the provider's own instants.

| Argument | Default | Values |
| --- | --- | --- |
| `method` | `"linear"` | `"linear"`, `"nearest"`, `"previous"` (hold from the left); any other text means linear. Read once, by `initialize()`. |

`initialize()` latches the method, clears the record and copies every sample the adaptee already
holds. `refresh()` appends the adaptee's last sample when its time is after the last one recorded and
otherwise does nothing; that skip is what makes a host that refreshes twice per step harmless. Only
the adaptee's last sample is taken, so steps the adaptee added between two refreshes, other than the
last, are not recorded. After an append it fires value-changed and refreshes its chained adapters.

The query time is the adapter's own setting, a Julian day set with `setQueryTime()` (0 initially).
Nothing in the SDK sets it from the consumer's `times()` or a workflow clock, so the host or the
consuming component sets it before each read. A read interpolates at that moment, with the
`TimeSeries` rules of @ref sdk_guide_time. Linear gives NaN before the first and after the last
sample, nearest and previous hold the last sample after the end, and previous at exactly a sample
time (after the first) returns the sample before it. A NaN is served as a successful read. With no
samples a read fails with `TemporalInterpolationAdaptedOutput "ID" holds no samples; initialize() has
not run or the adaptee has no time steps`.

`shape()` is `{1}` and `dataKind()` is Float64. `states()` returns the record as a read-only
`{ samples, 2 }` item of (time, value) rows. Because that state grows by a row per refresh, the
adapter is neither differentiable nor checkpointable, and an orchestrator that must return a
composition to an earlier step refuses a connection through it. The adaptee's times must increase:
a sample at or before the last recorded time throws `std::invalid_argument` from
`TimeSeries::addValue()` during `initialize()`.

## TimeSeriesAdaptedOutput

@ref HydroCouple::SDK::Temporal::TimeSeriesAdaptedOutput is an older adapter that interpolates a
`TimeSeries` handed to its constructor, not its adaptee: the provider is only the output it is
registered on. It has no factory, no arguments, no dimensions and no value definition, and its
`refresh()` only fires value-changed. Build it in code, register it with
`provider->addAdaptedOutput()`, set the query time, and read the one value. For `int` a NaN result is
converted with `static_cast`, which is undefined behavior, so use the `double` form. New code uses
`TemporalInterpolationAdaptedOutput`, which samples its adaptee and can be named in a composition.

## Chains in a composition document

A connection's `from` block may list `adapted_outputs`, an ordered chain of steps. Each step names an
offering `id`, optionally a `factory` id, and `arguments` keyed by argument id, each value the JSON
the argument's `initialize()` accepts. @ref sdk_guide_compositions describes the document and
@ref HydroCouple::SDK::IO::ModelInitializer, which for each step: collects the candidate factories
(the provider component's info first, then the standalone resolver), takes the first that offers the
step's id for the current output, creates the adapter, applies the step's arguments as JSON,
initializes it, and continues from it. The consumer is wired to the last adapter. The initializer
owns the adapters it creates.

The example declares two steps on one connection:

@snippet ch08_composition_chain/main.cpp document

@snippet ch08_composition_chain/main.cpp build

@snippet ch08_composition_chain/main.cpp push

`IO::ModelInitializer::serialize()` writes the chain back: each step's offering id (not the instance
id with its counter), its factory id, and the arguments as the live adapter holds them, so an
argument changed after the build is what the document records.

@snippet ch08_composition_chain/main.cpp serialize

@snippet ch08_composition_chain/main.cpp unknown

Output (from the ctest run of `docs.example.ch08_composition_chain`):

```text
built: yes; tail relaxation#2 <- linear_transform#1 <- flow
downstream.inflow: 25 45 65
serialized steps:
[
  {
    "arguments": {
      "multiplier": {
        "caption": "",
        "comments": [],
        "description": "",
        "values": [
          2.0
        ]
      },
      "offset": {
        "caption": "",
        "comments": [],
        "description": "",
        "values": [
          5.0
        ]
      }
    },
    "factory": "hydrocouple.sdk.adapters",
    "id": "linear_transform"
  },
  {
    "arguments": {
      "relaxation_factor": {
        "caption": "",
        "comments": [],
        "description": "",
        "values": [
          1.0
        ]
      }
    },
    "factory": "hydrocouple.sdk.adapters",
    "id": "relaxation"
  }
]
unknown step: connections[0]: no adapted output "unit_conversion" is offered for output "flow" (searched 0 factories on component "a" and 1 standalone factories)
```

## Writing an adapter and a factory

An adapter derives from `AbstractAdaptedOutput` and implements:

- the constructor, which passes the id, the adaptee and the factory to the base and registers its
  arguments with `addArgument()` (the arguments are members of the adapter, which owns them);
- `initialize()` and `refresh()`. `refresh()` reads the adaptee through `getValuesInto()`, stores the
  result, calls `emitValueChanged()` and ends with `refreshAdaptedOutputs()`, so the chain below
  follows;
- `updateValues()`, usually as `refresh()`;
- the data plane: `shape()`, `dataKind()`, `getValuesInto()` over the stored result (with
  `detail::readHyperslab()`), and a `setValuesFrom()` that refuses;
- `states()` when a refresh depends on earlier refreshes, and the differentiable and checkpointable
  interfaces when it can support them.

Give the adapter a value definition when it changes units, so a validator downstream can check the
consumer against it; the SDK's own adapters do not.

A factory derives from `AbstractAdaptedOutputFactory`, owns one identity per offering, returns the
offerings that fit a provider from `getAvailableAdaptedOutputIds()`, and in `createAdaptedOutput()`
creates the adapter with a distinct id, calls `provider->addAdaptedOutput()` on it and returns it. The
factory test `test_adaptedoutputs.cpp` (its `ShadowFactory`) is a complete small factory.

## Diagnostics

| Message | Produced by | Cause | Fix |
| --- | --- | --- | --- |
| `LinearTransformAdaptedOutput "ID" has no values; initialize() has not run` (and the same for `RelaxationAdaptedOutput`) | `getValuesInto()` | No successful refresh yet: not initialized, or the adaptee is not a readable rank-1 Float64 item | Initialize after the adaptee has values; check the offering fits |
| `TemporalInterpolationAdaptedOutput "ID" holds no samples; ...` | `getValuesInto()` | Not initialized, or the adaptee is not a time series or has no steps | Initialize after the adaptee records a step |
| `... is read-only` | `setValuesFrom()` on any adapter | Writing to an adapter | Write to the adaptee |
| `has no refresh to differentiate` | `vjp()`, `jvp()` | No refresh since construction or `restoreState()` | Refresh first |
| `seed names an item or role this adapter does not differentiate`, `result names ...` | `vjp()`, `jvp()` | A seed or result for another item or role | Use the roles listed above |
| `not a relaxation token`, `the token holds N values but the adaptee has a different shape`, `truncated or malformed token`, `trailing data in token` | `RelaxationAdaptedOutput::restoreState()` | A token from elsewhere, or the adaptee changed length | Restore tokens of this adapter at the same shape |
| `Adapted output "ID" cannot be consumed by input "INPUT": REASON` (`std::invalid_argument`) | `addConsumer()` | The consumer refused the adapter | See the reason |
| ``adapted output "ID" requested but the provider's component exposes no adapter factories and no standalone adapter factories are installed`` | `ModelInitializer` | No factory at all | Install `SdkAdaptedOutputFactory::instance()` as a standalone factory |
| `no adapted output "ID" is offered for output "OUTPUT" (searched ...)` | `ModelInitializer` | No candidate offers the step for this output | Check the offering id and the output's kind and shape |
| `factory "F" returned no adapted output for "ID"` | `ModelInitializer` | The factory offered the step and then created nothing | Fix the factory |
| `adapted output "ID" has no argument "A"` | `ModelInitializer` | A step argument the adapter does not have | Use the argument ids above |

A NaN from a temporal interpolation outside its record, or from a linear transform of a NaN, is not
an error to the SDK; check values at the consumer. The workflow-level messages about adapters on
differentiable connections are in @ref sdk_guide_differentiation, and all messages are collected in
@ref sdk_ref_diagnostics.
