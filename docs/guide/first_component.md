@page sdk_guide_first_component 3. Tutorial: a first component

[TOC]

This tutorial builds a complete HydroCouple component, a linear reservoir, and then drives it the
way a host does: it creates the component through its component info, sets its arguments, runs it
through its whole lifecycle and reads its outputs, using only the HydroCouple interfaces. Every
line of code on this page comes from the compiled example
`docs/examples/ch03_first_component/`, which the SDK's test suite builds and runs
(`ctest -R docs.example.ch03_first_component`), so the code and the output shown here are current.

By the end you will have written the nine methods every component implements, given the component
arguments with physical units, an input and two outputs, published it from a library through the
two C entry points hosts look for, and seen what a host sees when a value is rejected.
@ref sdk_guide_example then connects two components and runs them as a composition.

## The model

A linear reservoir stores a volume of water \f$S\f$ (m³), receives an inflow \f$I\f$ (m³/s) and
releases an outflow proportional to its storage:

\f[ \frac{dS}{dt} = I - k S, \qquad Q = k S \f]

where \f$k\f$ (1/s) is the recession constant. While the inflow stays constant, the equation has an
exact solution, so the component can advance a time step \f$\Delta t\f$ without any numerical error:

\f[ S(t + \Delta t) = \frac{I}{k} + \left(S(t) - \frac{I}{k}\right) e^{-k \Delta t} \f]

The storage relaxes toward the equilibrium \f$I/k\f$, at which outflow equals inflow. Because the
answer is known exactly, the example checks every step against it and fails its test if the two
differ.

The component has four arguments, one input and two outputs:

| Item | Kind | Units | Meaning |
| --- | --- | --- | --- |
| `recession_constant` | argument | 1/s | \f$k\f$; must be positive |
| `initial_storage` | argument | m³ | \f$S\f$ when the run starts; must not be negative |
| `time_step` | argument | s | \f$\Delta t\f$ of each `update()` |
| `duration` | argument | s | the run ends once this much time has been simulated |
| `inflow` | input | m³/s | \f$I\f$, held constant over each step |
| `outflow` | output | m³/s | \f$Q = k S\f$ at the end of each step |
| `storage` | output | m³ | \f$S\f$ at the end of each step |

## Step 1: declare the component

A component is a class derived from @ref HydroCouple::SDK::AbstractModelComponent. The base class
implements the `HydroCouple::IModelComponent` interface: identity, status and its change events,
the lists of arguments, inputs and outputs, the error queue, and the order in which
`initialize()` calls the methods you supply. You implement nine methods: four that create and
check the component's items, one that cleans up after a failed initialization, and the four
lifecycle methods a host calls after initialization.

@snippet ch03_first_component/linearreservoir.h class

Three points about this declaration hold for every component:

- **The component owns its items.** `addArgument()`, `addInput()` and `addOutput()` register an
  item with the base class, which lists it in `arguments()`, `inputs()` and `outputs()` but never
  deletes it. Hold each item in a `std::unique_ptr` member and pass `.get()` to the `add` call; an
  item created with a bare `new` and only registered is never freed.
- **Metadata must outlive the items that point to it.** Every item refers to a
  @ref HydroCouple::SDK::Dimension (what its index means) and to a value definition, here a
  @ref HydroCouple::SDK::Quantity (what its values mean, with a unit). The items store only
  pointers, so the component owns the dimension and quantities as members, declared before the
  items: members are destroyed in reverse order of declaration, so the items go first. A
  `Quantity` does not take ownership of its @ref HydroCouple::SDK::Unit either, so a unit you
  construct yourself is a member too (`m_perSecond`).
- **The constructor takes an id and, optionally, the component info.** The id is the instance's
  name in a composition. Passing the info lets `componentInfo()` report which component type the
  instance belongs to (step 6).

## Step 2: create the arguments

`createArguments()` creates the component's configuration items. The base class calls it exactly
once, the first time anything asks for `arguments()` while the component is still `Created`, or at
the start of `initialize()` if nothing asked earlier. That is what lets a host set argument values
before it initializes the component.

@snippet ch03_first_component/linearreservoir.cpp arguments

@ref HydroCouple::SDK::Argument1DDouble (`Argument1D<double>`) is a one-dimensional argument. Its
constructor takes the argument's id, the dimension of its single axis, its initial length, its value
definition and the component it belongs to. The id is the key a composition document uses to set the
value, so keep ids stable across versions of a component.

The quantities come from two places. The SDK's factories cover common quantities:
`Quantity::volumeInCubicMeters()`, `Quantity::flowInCMS()` and `Quantity::timeInSeconds()` return
new quantities with SI units, which the component wraps in `std::unique_ptr` because the caller
owns what a factory returns. The recession constant needs a unit the factories do not offer, so
the component builds one: a @ref HydroCouple::SDK::Unit named "per second" with a conversion factor
of 1 to SI, the symbol `1/s` and the dimension time to the power −1. Hosts compare these
dimensions when they check whether an output can feed an input (@ref sdk_guide_quantities).

The values written with `operator[]` are the defaults a host sees before it sets anything.

## Step 3: check the arguments

`initialize()` calls `initializeArguments()` after the arguments exist and the host has set them.
This is where a component reads its configuration, opens its input files and checks that what it
was given makes sense.

@snippet ch03_first_component/linearreservoir.cpp initialize_arguments

Returning `false` with a message stops initialization. The base class then sets the status to
`Failed`, queues the message on the component's error queue prefixed with
`Argument initialization failed: `, and calls `initializeFailureCleanUp()` so the component can
release anything it acquired before the failure. Write the message for the person who set the
value: name the argument, the rule and the value received. Step 8 shows what a host sees.

## Step 4: create the inputs and outputs

Once the arguments are accepted, `initialize()` empties the base class's lists of inputs and
outputs from an earlier initialization and calls `createInputs()` and `createOutputs()`. The
component frees the old items itself: assigning the new item to the same `std::unique_ptr` member
deletes the previous one. Inputs and outputs are created
after the arguments because their sizes often depend on them: a model with one value per cell
learns how many cells it has from its arguments.

@snippet ch03_first_component/linearreservoir.cpp ports

@ref HydroCouple::SDK::Input1DDouble and @ref HydroCouple::SDK::Output1DDouble take the same
constructor arguments as `Argument1DDouble`. The inflow input and the outflow output share the
`flowInCMS()` quantity, which is what lets an outflow of one reservoir feed the inflow of another
without conversion. The ids `inflow`, `outflow` and `storage` are what a composition's connections
name.

When all of this succeeds, `initialize()` sets the status to `Initialized`.

## Step 5: validate, prepare, update and finish

The four remaining methods are the lifecycle a host drives after initialization. Each one sets the
status on entry and on exit; the base class records every change, checks it against the
interface's table of legal transitions and announces it to anyone listening to the component's
status signal. A transition the table does not allow still happens, but queues an
`IllegalStatusTransition` error once for each pair of statuses, so a defect in a component shows
up in its diagnostics instead of silently.

`validate()` runs after the host has connected the component's inputs and outputs and before the
run starts. It returns a list of problems; an empty list and the status `Valid` mean the component
can run. This component has nothing to check beyond its arguments.

@snippet ch03_first_component/linearreservoir.cpp validate

`prepare()` sets up the run: the clock starts at zero and the outputs hold the initial state, so a
consumer that reads them before the first step sees the starting values. It ends in `Updated`,
which tells the host the component can be updated.

@snippet ch03_first_component/linearreservoir.cpp prepare

`update()` advances the model by one time step. It first calls `applyInputValues()`, which asks the
provider connected to each input for its current values and copies them in. An input with no
provider keeps the value it holds; in this tutorial the host writes the inflow itself. The method
ends in `Updated` while there is more to do and in `Done` once the run has reached its duration;
the message passed with the status is what a host shows as progress.

@snippet ch03_first_component/linearreservoir.cpp update

The `0.5 * dt` in the stopping test absorbs rounding in the accumulated time, so the run ends after
`duration / time_step` steps even when the sum of the steps is a hair short of the duration.

`finish()` closes the run: a model writes its final files and releases what `prepare()`
allocated. It ends in `Finished`.

@snippet ch03_first_component/linearreservoir.cpp finish

The statuses this component passes through, and the calls that move it:

<pre class="mermaid">
stateDiagram-v2
    [*] --> Created
    Created --> Initializing: initialize()
    Initializing --> Initialized: arguments accepted
    Initializing --> Failed: initializeArguments() returns false
    Initialized --> Validating: validate()
    Validating --> Valid
    Valid --> Preparing: prepare()
    Preparing --> Updated
    Updated --> Updating: update()
    Updating --> Updated: more to simulate
    Updating --> Done: duration reached
    Done --> Finishing: finish()
    Finishing --> Finished
    Finished --> [*]
</pre>

@ref sdk_guide_component lists every status, every legal transition and what a host may call in
each.

## Step 6: describe the component to hosts

A host does not construct components by class name. It asks a component info, an object that
describes a component type and creates instances of it. The SDK's
@ref HydroCouple::SDK::AbstractModelComponentInfo implements
`HydroCouple::IModelComponentInfo`; you set its metadata in the constructor and implement
`createComponentInstance()`.

@snippet ch03_first_component/linearreservoir.h info

@snippet ch03_first_component/linearreservoir.cpp info_impl

The id passed to the base constructor, `docs.examples.linear_reservoir`, names the component type;
compositions refer to it, so it must be unique and stable. The caption, description, developer and
version are what a host such as HydroCouple Composer shows in its component library. The instance
is returned as `std::unique_ptr`: the host owns it.

## Step 7: publish it from a library

A host loads components from shared libraries. HydroCouple fixes how a library publishes its
component: it exports two C functions, which `HYDROCOUPLE_DECLARE_COMPONENT(InfoType)` from
`hydrocouplecomponentabi.h` (included by `hydrocouplesdk/componentabi.h`) defines in one line.

@snippet ch03_first_component/linearreservoir.cpp declare

- `hydrocouple_component_abi_v1()` returns the library's ABI stamp: a string naming the interface
  ABI version, the compiler family, the standard library and, on Windows, the C runtime the library
  was built with. It is plain C, so a host can call it safely even when the library was built with
  an incompatible toolchain, and compare it with its own `HYDROCOUPLE_COMPONENT_ABI_STAMP` before it
  calls anything else.
- `hydrocouple_component_info_v1()` returns the component info. The library owns it (a
  function-local static) and it stays valid until the library is unloaded; the host must not
  delete it.

Because instances are allocated by the library's code, a host must destroy every instance before it
unloads the library that created it.

To build the component as a loadable library, compile its sources into a CMake `MODULE` library and
link the SDK:

```cmake
find_package(HydroCoupleSDK 2.0 REQUIRED)
add_library(linear_reservoir MODULE linearreservoir.cpp)
target_compile_features(linear_reservoir PRIVATE cxx_std_20)
target_link_libraries(linear_reservoir PRIVATE HydroCoupleSDK::HydroCoupleSDK)
```

The macro marks the two functions for export (`__declspec(dllexport)` on Windows, default
visibility elsewhere), so they stay visible in a library compiled with hidden symbol visibility,
as the SDK's own targets are. The
[HydroCoupleComponents manual](https://www.hydrocouple.org/HydroCoupleComponents/) walks through
packaging, installing and loading a component library in a host; this tutorial links the
component into the host program instead, which exercises the same entry points.

## Step 8: drive it from a host

`main.cpp` plays the host. It uses only interface types (`IModelComponent`, `IArgument`, `IInput`,
`IOutput`) and the free functions of `hydrocouplehelpers.h`, so it would work unchanged with a
component it knew nothing about at compile time.

First it checks the stamp and creates an instance. A host that had loaded the library would have
the same two function pointers from `dlsym` or `GetProcAddress`, looked up by the names
`HYDROCOUPLE_COMPONENT_ABI_SYMBOL` and `HYDROCOUPLE_COMPONENT_INFO_SYMBOL`.

@snippet ch03_first_component/main.cpp load

It sets the arguments. `IArgument::initialize()` takes the value as text and the type of that text;
a one-dimensional SDK argument accepts a JSON array, a JSON object with a `values` array, or the
path or URL of a file holding either. A composition document reaches the same call
(@ref sdk_guide_compositions).

@snippet ch03_first_component/main.cpp arguments

Then it initializes the component, which runs steps 2 to 4:

@snippet ch03_first_component/main.cpp initialize

No other component provides the inflow, so the host writes it. `Helpers::setValue()` copies one
value into a data item at the given index through the item's `setValuesFrom()`; its partner
`Helpers::getValue()` reads one value back. Both return `false` when the index or the value type
does not match the item (@ref sdk_guide_ports describes the full value-access contract).

@snippet ch03_first_component/main.cpp inflow

It validates, prepares and updates the component until the status leaves `Updated`, checking every
step against the exact solution:

@snippet ch03_first_component/main.cpp run

and finishes it:

@snippet ch03_first_component/main.cpp finish

Last, it shows what happens to a value the component rejects. A negative recession constant is
valid JSON, so `IArgument::initialize()` accepts it; the component's own check in
`initializeArguments()` rejects it, `initialize()` ends in `Failed`, and the reason is on the
error queue:

@snippet ch03_first_component/main.cpp failure

## Run it

Configure the SDK with examples and tests (the full set of options is in @ref sdk_guide_build),
build the example and run its test:

```shell
cmake -S . -B build -DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON
cmake --build build --target docs_example_ch03_first_component
ctest --test-dir build -R docs.example.ch03_first_component --output-on-failure
```

The test runs the program in `build/docs-examples/ch03_first_component/`. Run directly, it prints:

```text
created 'reservoir' from component type 'docs.examples.linear_reservoir'
initialized: 4 arguments, 1 input, 2 outputs
   t (s)       S (m3)     Q (m3/s)      exact S
     600     774.4058     0.774406     774.4058
    1200     650.5971     0.650597     650.5971
    1800     582.6494     0.582649     582.6494
    2400     545.3590     0.545359     545.3590
    3000     524.8935     0.524894     524.8935
    3600     513.6619     0.513662     513.6619
    4200     507.4978     0.507498     507.4978
    4800     504.1149     0.504115     504.1149
    5400     502.2583     0.502258     502.2583
    6000     501.2394     0.501239     501.2394
    6600     500.6802     0.500680     500.6802
    7200     500.3733     0.500373     500.3733
finished after 12 steps
rejected instance: status Failed
  error: Argument initialization failed: recession_constant must be positive (1/s), got -1.000000
```

Starting from 1,000 m³ with 0.5 m³/s of inflow and \f$k = 0.001\f$ 1/s, the storage falls toward
the equilibrium of 500 m³ and the outflow toward the inflow, and every step agrees with the exact
solution.

## What to change for a real model

- **State that depends on the past.** A component whose `update()` depends on earlier steps should
  list its prognostic items by overriding `states()`, which is what a checkpoint must capture
  (@ref sdk_guide_component).
- **Time.** This component counts seconds itself. A component that exchanges values with others in
  time exposes its time horizon and current time through the temporal items in
  @ref sdk_guide_time.
- **Many values.** Real models expose one value per cell, node or reach: size the items from the
  arguments and use bulk reads and writes (@ref sdk_guide_ports).
- **Space.** Items that carry geometry, such as a mesh or a network, are described in
  @ref sdk_guide_space.
- **Errors during a run.** A failure in `update()` sets the status to `Failed` with a message;
  further diagnostics go on the error queue (@ref sdk_guide_component).
