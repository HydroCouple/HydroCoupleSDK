@page sdk_guide_differentiation 17. Differentiation

[TOC]

Calibration, data assimilation, sensitivity analysis and training a machine-learned closure all
need the derivative of something computed from a coupled run (a misfit, a cost, a forecast) with
respect to the run's parameters and initial state. The HydroCouple SDK computes that derivative for
a time-stepped composition in reverse mode, across components written independently, and across
the adapted outputs between them. Each component supplies the derivative of its own step; the SDK
composes them through the connections and through time.

The chapter has three parts. The contract a differentiable component implements comes first,
illustrated by a component with a hand-written vector-Jacobian and Jacobian-vector product. Then
@ref HydroCouple::SDK::Differentiation::DifferentiableWorkflow, which records a run and
differentiates it, with its checkpointing policies and the compositions it refuses. Last, the
optional LibTorch layer, which differentiates a component written in ATen operations
automatically and exposes a whole composition to LibTorch as one differentiable function.

The core classes are in the namespace `HydroCouple::SDK::Differentiation` of the library
`HydroCoupleSDK` and depend on nothing beyond it. The Torch layer is a separate library,
`HydroCoupleSDKTorch`, built with `HYDROCOUPLESDK_WITH_TORCH=ON` (@ref sdk_ref_cmake). The compiled
examples are `docs/examples/ch17_differentiable_workflow/` (always built) and
`docs/examples/ch17_torch_composition/` (built with the Torch layer).

## Products, not Jacobians

A component's step maps the state it held before the step, the values of its inputs and the
values of its arguments to its new state and its outputs:

\f[ (s_{n-1},\, u_n,\, p) \;\mapsto\; (s_n,\, y_n). \f]

Collect the step's inputs into one vector \f$x = (u_n, p, s_{n-1})\f$ and its results into
\f$z = (y_n, s_n)\f$, with Jacobian \f$J = \partial z / \partial x\f$. No component is asked for
\f$J\f$, which would be as large as its state squared. It is asked for products with it:

- the **Jacobian-vector product** (JVP, forward mode) \f$\dot z = J\,\dot x\f$ maps a tangent of
  the inputs to the tangent of the results;
- the **vector-Jacobian product** (VJP, reverse mode) \f$\bar x = J^{\top} \bar z\f$ maps a
  cotangent of the results (a sensitivity of some scalar to them) to the cotangent of the inputs.

The two are transposes of each other, which gives the **dot-product test** every implementation
should pass: for any tangent \f$\dot x\f$ and any cotangent \f$\bar z\f$,

\f[ \langle \bar z,\; J \dot x \rangle \;=\; \langle J^{\top} \bar z,\; \dot x \rangle \f]

to rounding. The test needs no reference solution, so it catches an adjoint that disagrees with
its forward linearization even when both look plausible.

Reverse mode is what gradients need: for a scalar loss \f$L = \sum_n \ell_n(y_n)\f$ over N steps,
one reverse sweep gives the derivative with respect to every parameter at once. Seeded with
\f$\bar y_n = \partial \ell_n / \partial y_n\f$ at each step and with the state cotangent
\f$\bar s_n\f$ carried back from the step after, each step's VJP returns
\f$(\bar u_n, \bar p_n, \bar s_{n-1})\f$; the input cotangents \f$\bar u_n\f$ are passed to the
component that produced those inputs, and

\f[ \frac{dL}{dp} = \sum_{n=1}^{N} \bar p_n, \qquad \frac{dL}{ds_0} = \bar s_0. \f]

## The contract

The interfaces are part of the HydroCouple standard (`hydrocouple.h`, see the
[HydroCouple interfaces](https://www.hydrocouple.org/HydroCouple/)); the SDK implements the
orchestration around them.

### Differentiable components

A component that reports derivatives implements `HydroCouple::IDifferentiableModelComponent` and
advertises `HydroCouple::Capability::Differentiable` in `capabilities()`. It lists the items the
derivative reaches:

| Method | Returns | Role of those items |
| --- | --- | --- |
| `differentiableInputs()` | inputs the derivative reaches; others are constants | `Input` |
| `differentiableArguments()` | arguments (parameters) the derivative reaches | `Argument` |
| `differentiableOutputs()` | outputs whose derivative the component reports | `Output` |
| `differentiableStates()` | the data items that make up the state carried from step to step | `StateBefore`, `StateAfter` |

and it implements `vjp(seeds, results, message)` and `jvp(seeds, results, message)`. Each takes
two spans of `HydroCouple::DifferentialEntry`: an item, its `HydroCouple::DifferentialRole`, and a
`BufferDescriptor` spanning the item's whole shape, of kind Float64 (or Float32) in a memory space
the component can serve. A state item appears in two roles, `StateBefore` (its value at the start
of the step) and `StateAfter` (at the end), which is why the role is explicit.

| Call | `seeds` hold | `results` hold |
| --- | --- | --- |
| `vjp()` | cotangents of `Output` and `StateAfter` entries | buffers to overwrite with cotangents of `Input`, `Argument` and `StateBefore` entries |
| `jvp()` | tangents of `Input`, `Argument` and `StateBefore` entries | buffers to overwrite with tangents of `Output` and `StateAfter` entries |

The rules that make components compose:

- **The linearization point is the most recent `update()`.** Both products describe that step and
  do not change the component's state. To differentiate an earlier step, an orchestrator restores
  the state before it and calls `update()` again.
- **Results are written, not accumulated.** Every result buffer is overwritten in full; an absent
  seed is zero.
- **Unknown entries are refused.** An entry naming an item the component does not list, or in a
  role it does not have, fails the call with a message.

### Checkpointable components

Differentiating more than the last step requires going back. A component that can save and
restore its state implements `HydroCouple::ICheckpointableModelComponent` and advertises
`Capability::Checkpointing`: `saveState(token, message)` returns an opaque token,
`restoreState(token, message)` returns the component to it (status `Updated`), and
`releaseState(token, message)` frees whatever the token names. Saving and restoring are legal when
the component is `Updated` or `Done`. A restore is not a step, so after it the component has
nothing to differentiate until its next `update()`.

### Differentiable and checkpointable adapters

An adapted output between two components is differentiated too. One that implements
`HydroCouple::IDifferentiableAdaptedOutput` offers `differentiableArguments()`,
`differentiableStates()`, `vjp()` and `jvp()` with the roles `Output` (itself), `Input` (its
adaptee), `Argument` and the two state roles, linearized at its most recent `refresh()`. An adapter
whose values depend on earlier refreshes lists that state in `states()`, and to be replayed it also
implements `HydroCouple::ICheckpointableAdaptedOutput`. Adapters have no capability set: the SDK
finds these interfaces by casting.

Among the SDK's adapters (@ref sdk_guide_adapted_outputs),
@ref HydroCouple::SDK::LinearTransformAdaptedOutput is differentiable and stateless (arguments
`multiplier` and `offset`), and @ref HydroCouple::SDK::RelaxationAdaptedOutput is differentiable,
stateful and checkpointable (argument `relaxation_factor`). The temporal interpolation adapter is
not differentiable, because its record of earlier values grows.

## Writing a differentiable component

The example's component, in `docs/examples/ch17_differentiable_workflow/reservoir.h`, holds two
reservoir cells. Each step advances the storage \f$S\f$ (m³) with inflow \f$I\f$ (m³/h),
recession constant \f$k\f$ (1/h) and a quadratic loss \f$c = 0.02\f$:

\f[ S' = S + \Delta t\,\bigl(I - kS - cS^2\bigr), \qquad Q = k\,S'. \f]

The \f$S^2\f$ term makes every derivative depend on the state the step started from, so a product
taken at the wrong step cannot pass a check. The class derives from
@ref HydroCouple::SDK::AbstractModelComponent and from both interfaces:

@snippet ch17_differentiable_workflow/reservoir.h class

Four points in it matter for every differentiable component:

- **The step keeps its linearization point.** `update()` stores the values the step started from
  (`m_before`, `m_inflow`, `m_k`); the products are evaluated there.
- **A restore forgets it.** `restoreState()` clears `m_haveStep`, so a product requested after a
  restore and before the next step is refused instead of answered at the wrong point.
- **Forcing belongs to the component.** The forced reservoir adds its own boundary inflow, indexed
  by its step count, and the checkpoint restores the count, so a replayed step sees the same
  forcing. A value written into an input from outside between steps is not recorded by the workflow
  and would not be replayed.
- **The token is the state.** The storage and outflow are printed with 17 significant digits, so a
  restore is exact and `releaseState()` has nothing to free.
- **The component owns its items.** They are `std::unique_ptr` members declared after the
  dimension and quantity they point to, and `addArgument()`, `addInput()` and `addOutput()` receive
  `.get()`: the base class only registers items and never deletes them.

The lists name the items the derivative reaches:

@snippet ch17_differentiable_workflow/reservoir.h lists

The VJP reads the seeds into row-major vectors, evaluates the adjoint of the step and writes the
results. With \f$T = \bar S' + k\,\bar Q\f$ the total cotangent of the new storage:

\f[ \bar I = \Delta t\,T, \qquad \bar k = \bar Q\,S' - \Delta t\,S\,T, \qquad
    \bar S = \bigl(1 - \Delta t\,k - 2\,\Delta t\,c\,S\bigr)\,T. \f]

@snippet ch17_differentiable_workflow/reservoir.h vjp

The JVP is the forward linearization of the same step:

\f[ \dot S' = \bigl(1 - \Delta t\,k - 2\,\Delta t\,c\,S\bigr)\dot S
    + \Delta t\,\bigl(\dot I - \dot k\,S\bigr), \qquad \dot Q = \dot k\,S' + k\,\dot S'. \f]

@snippet ch17_differentiable_workflow/reservoir.h jvp

### Buffer helpers

`differentials.h` holds the helpers the component uses; they move whole-item buffers to and from
row-major host vectors through the same hyperslab code as the data plane, so strided or pitched
derivative buffers are honored. They handle Float64 host buffers only.

| Helper | Does |
| --- | --- |
| @ref HydroCouple::SDK::Differentiation::readDifferential | Copies an entry's buffer into a row-major vector of the item's element count. |
| @ref HydroCouple::SDK::Differentiation::writeDifferential | Overwrites an entry's buffer from a row-major vector. |
| @ref HydroCouple::SDK::Differentiation::itemOf | The `IComponentDataItem` identity an entry's `item` is compared with. Compare `e.item == itemOf(myInput)`, never with the typed pointer: with virtual inheritance the addresses differ. |
| @ref HydroCouple::SDK::Differentiation::hostBuffer | A contiguous Float64 host descriptor over a vector and a shape (both must outlive it). |
| @ref HydroCouple::SDK::Differentiation::elementCount | The product of a shape's extents. |

Each helper returns false with the hyperslab engine's reason ("kind mismatch", "unsupported memory
space", "buffer element count mismatch") when the buffer does not fit.

## DifferentiableWorkflow

@ref HydroCouple::SDK::Differentiation::DifferentiableWorkflow is a
@ref HydroCouple::SDK::TimeSteppedWorkflow (@ref sdk_guide_workflow) that records its steps and can
run them backwards. A host uses it exactly as a time-stepped workflow: add the components,
initialize, connect, validate, prepare, then call `update()` once per step. The components are
declared before the workflow, because the workflow's destructor uses the components it manages and
they must outlive it.

@snippet ch17_differentiable_workflow/main.cpp rig

### Recording

Before every step the workflow checkpoints the composition: it calls `saveState()` on every
component that advertises `Capability::Checkpointing` (in schedule order) and on every
checkpointable adapter hanging off a managed component's outputs. Then it runs the step as its base
class does and keeps the checkpoint under the key \f$k\f$, the number of steps before it. A step
that fails releases its checkpoint and records nothing. `prepare()` and `clearRecording()` forget
the record; `recordedSteps()` counts it.

### The reverse sweep

@ref HydroCouple::SDK::Differentiation::DifferentiableWorkflow::backward visits the recorded steps
last first. For each step it brings the composition to that step's linearization point: the last
step needs nothing when nothing has run since, and any other step is restored from the nearest
checkpoint before it and replayed forward. Then it sweeps the step's schedule in reverse:

1. It calls the caller's seed function with the step's 1-based index. The composition holds that
   step's state and outputs, so the function can read outputs to form the cotangent of a misfit,
   and deposits cotangents with @ref HydroCouple::SDK::Differentiation::Seeds::add (repeated adds
   accumulate).
2. For each component, last in the schedule first, it reverses the adapted outputs hanging off the
   component's outputs, leaves first, so every consumer's share has reached an adapter before the
   adapter's own VJP runs. Each adapter's VJP runs at most once per step and adds its adaptee's
   cotangent to the adaptee.
3. It calls the component's `vjp()` with the cotangents that have reached its differentiable
   outputs and the state cotangent carried back from the following step. A component no cotangent
   reaches is skipped.
4. It adds each input's cotangent to the output that input reads (a root output, or an adapted
   output on the way to it), sums the argument cotangents into the gradients, and carries the
   state cotangents to the previous step.

<pre class="mermaid">
sequenceDiagram
    participant H as Host
    participant W as DifferentiableWorkflow
    participant A as Component A
    participant B as Component B (reads A)
    H->>W: backward(seeds)
    Note over W: save the final state
    Note over W,B: step 3 is the latest update, so no replay
    W->>H: seeds(3): dL/dQ_B at step 3
    W->>B: vjp(Q_B bar, S_B bar)
    B-->>W: I_B bar, k_B bar, S_B bar before
    W->>A: vjp(Q_A bar = I_B bar, S_A bar)
    A-->>W: k_A bar, S_A bar before
    W->>A: restoreState(after step 1)
    W->>B: restoreState(after step 1)
    Note over W: replay step 2
    W->>H: seeds(2)
    W->>B: vjp ...
    W->>A: vjp ...
    Note over W: restore after step 0, replay step 1, sweep step 1
    W->>A: restoreState(final)
    W->>B: restoreState(final)
    W-->>H: Gradients
</pre>

When `backward()` returns successfully, every component and every stateful adapter is back where
the forward run left it, and the run may continue as if `backward()` had not happened. The record
is kept, so `backward()` may be called again (it replays the last step too, since the composition
is restored rather than linearized), and further steps extend the record.

### Gradients

@ref HydroCouple::SDK::Differentiation::Gradients holds what one pass produced:

| Member | Holds |
| --- | --- |
| `arguments` | One @ref HydroCouple::SDK::Differentiation::ArgumentGradient per argument a cotangent reached, in the order first reached: `owner` (the component or adapted output), `argument`, its `shape`, and `values`, the gradient summed over steps, row-major. |
| `initialStates` | One @ref HydroCouple::SDK::Differentiation::StateGradient per state item a cotangent reached: `owner`, exactly one of `component` and `adapter`, the `state` item, `shape` and `values`, the gradient with respect to the state before step 1. Component states come first, in schedule order. |
| `find(owner, argument)` | The argument's entry, or null. |
| `findState(owner, state)` | The state's entry, or null. |

Null is not zero: an argument with no entry was not reached at all, which usually means a
connection the sweep could not cross. An adapter's state before step 1 is what its `initialize()`
made of its adaptee's prepared values; the workflow does not differentiate that dependence.

### The example

The example runs 12 steps of the two reservoirs, A feeding B, and differentiates the loss
\f$L = \tfrac12 \sum_n \sum_i (Q_{B,n,i} - Q^{obs}_{n,i})^2\f$:

@snippet ch17_differentiable_workflow/main.cpp backward

It then checks each gradient against central differences of the loss, \f$(L(p + \epsilon) -
L(p - \epsilon)) / 2\epsilon\f$ with \f$\epsilon = 10^{-6}\f$, computed by forward runs of fresh
compositions only, a path that never calls a VJP, so a wrong adjoint, a wrong routing of
cotangents or a wrong replay cannot agree with it by construction:

@snippet ch17_differentiable_workflow/main.cpp check

and runs the dot-product test of B's most recent step through the component's own `jvp()` and
`vjp()`. After `backward()` the composition is restored, not linearized, so the example steps it
once more first:

@snippet ch17_differentiable_workflow/main.cpp dottest

The test `docs.example.ch17_differentiable_workflow` prints:

```text
loss 0.7565344022 over 12 steps; 12 checkpoints, 11 replays, peak 13
param         backward()     central diff    rel err
kA[0]       0.1494982409     0.1494982409   2.82e-10
kA[1]      -2.5291810288    -2.5291810288   2.62e-11
kB[0]       0.2412349505     0.2412349505   4.87e-12
kB[1]      -0.2072881733    -0.2072881733   7.51e-11
SA0[0]      0.0188909400     0.0188909400   8.75e-10
SB0[1]      0.0634094637     0.0634094637   2.23e-10
Logarithmic: 5 checkpoints, 20 replays, peak 8
<w, J t> = 0.226863395659773
<J^T w, t> = 0.226863395659773
after one gradient step on kB: loss 0.7556029241 (was 0.7565344022)
released 92 checkpoint tokens, 0 refused
```

The relative errors are those of the finite differences themselves (truncation and rounding at
\f$\epsilon = 10^{-6}\f$), not of the gradient.

## Checkpointing policies

@ref HydroCouple::SDK::Differentiation::CheckpointPolicy chooses how many checkpoints a recorded
run holds. Set it with `setCheckpointPolicy()`; a change applies to steps recorded after it, and
switching to `Logarithmic` thins the record at once.

| Policy | Held after N steps | Replays in `backward()` | Use when |
| --- | --- | --- | --- |
| `EveryStep` (default) | N | N − 1 | Memory is not the limit: the fastest backward pass. |
| `Logarithmic` | O(log N): at most two per power of two, plus the first and the newest | O(N log N) | Long runs or large states. |

Under `Logarithmic` the record keeps checkpoint \f$k\f$ only while \f$k\f$ is a multiple of the
largest power of two not above \f$d\f$, its distance from the newest checkpoint; the first
(\f$k = 0\f$) and the newest (\f$d = 0\f$) always survive. `backward()` reverses each gap between
kept checkpoints by recursive bisection: it replays to the middle, saves a temporary checkpoint,
reverses the upper half, releases the temporary and reverses the lower half. Every step is still
differentiated at exactly its own linearization point, so the gradients are identical to the bit;
the example compares them with `!=` and finds no difference:

@snippet ch17_differentiable_workflow/main.cpp ladder

The counters describe the cost: `storedCheckpoints()` and `checkpointedSteps()` the record,
`replays()` the steps the last `backward()` re-ran, and `peakCheckpoints()` the most held at once
during it (the record, the final state it returns to, and the bisection's temporaries). For the
example's 12 steps, `EveryStep` holds 12 checkpoints and replays 11 steps; `Logarithmic` holds 5,
replays 20 and peaks at 8.

### Releasing checkpoints

A component whose token names storage (a file under its reference directory, a buffer) must free it
in `releaseState()`, because the workflow saves far more checkpoints than it restores. The workflow
releases every token exactly once, as soon as nothing can restore it: a checkpoint the ladder
drops, a bisection temporary when its half is done, the final state `backward()` returns to, and
every remaining checkpoint on `clearRecording()`, `prepare()` and `finish()`. The token of an
adapter that has since left the composition is dropped unreleased (the adapter may be gone).
`releasedTokens()` and `refusedReleases()` count releases over the workflow's lifetime. The
workflow's destructor releases nothing, because the components may already be gone: call
`finish()` or `clearRecording()` first.

## Repeating a run

The workflow lifecycle does not allow `prepare()` again once a workflow has stepped.
@ref HydroCouple::SDK::Differentiation::DifferentiableWorkflow::rewind returns every component and
every stateful adapter to the checkpoint before step 1 and forgets the record, so the same run can
be repeated with new argument values, as a training or calibration loop does:

@snippet ch17_differentiable_workflow/main.cpp rewind

Arguments must be read by their owner at every update or refresh for this to work; the SDK's
components and adapters read them so. `rewind()` fails when nothing is recorded or a restore
fails.

## What the workflow refuses

`backward()` refuses, with a message naming the offender, rather than return a gradient that would
be wrong. It checks
@ref HydroCouple::SDK::Differentiation::DifferentiableWorkflow::differentiabilityProblems before
any VJP runs and joins the problems with "; ". Call it after `prepare()` to check a composition
early; a problem that needs more than one step to exist (a component that cannot checkpoint)
appears once a second step is recorded.

| Problem | Message |
| --- | --- |
| Components in a feedback loop (a schedule group of two or more) | `feedback loop between 'A', 'B': iterative coupling is not differentiable yet` |
| A component without `Capability::Differentiable` | `component 'X' is not differentiable` |
| More than one step and a component without `Capability::Checkpointing` | `component 'X' lacks Capability::Checkpointing, so only a single recorded step can be differentiated` |
| A differentiable multi-input | `input 'I' of 'X' is a multi-input, not yet supported` |
| A non-differentiable adapter on a differentiable connection | `adapted output 'A' on the connection into 'X.I' is not differentiable` |
| A component reading its own output (a lagged self-coupling) | `'X.I' reads 'X''s own output 'O': a lagged self-coupling is not differentiable yet` |
| A stateful adapter on a connection from a component outside the workflow | `adapted output 'A' carries state but adapts 'O', whose component is outside the workflow, so its refreshes cannot be replayed` |
| An input reading an output its provider does not differentiate | `'X.I' consumes 'P.O', which 'P' does not differentiate` |
| More than one step and a stateful adapter that cannot checkpoint | `adapted output 'A' carries state but is not an ICheckpointableAdaptedOutput, so only a single recorded step can be differentiated` |
| A component that finished before the last recorded step | `component 'X' had finished by step N, so that step is not the same function for every component` |

An input whose provider belongs to no component in the workflow is a constant; a stateless adapter
on such a connection is differentiated at its latest refresh, as a function of its arguments only.

Failures during the pass:

| Message | Cause |
| --- | --- |
| `nothing to differentiate: no step has been recorded` | `backward()` before any `update()`, or after `clearRecording()`. |
| `checkpoint of 'X' failed: ...`, `checkpoint of adapted output 'A' failed: ...` | A `saveState()` refused. |
| `component 'X' lacks Capability::Checkpointing, so an earlier step cannot be replayed` | A restore was needed and a component cannot. |
| `adapted output 'A', checkpointed earlier, has left the composition, so an earlier step cannot be replayed` | An adapter was removed after it was checkpointed. |
| `adapted output 'A' carries state but joined the composition after the checkpoint, so an earlier step cannot be replayed` (or `... is not an ICheckpointableAdaptedOutput, ...`) | A stateful adapter has no token in the checkpoint. |
| `restore of 'X' failed: ...`, `restore of adapted output 'A' failed: ...` | A `restoreState()` refused. |
| `restoreState left 'X' finished, so replaying step N would skip it` | A component's restore did not return it to `Updated`. |
| `component 'X' failed during update` | A replayed step failed. |
| `'X'.vjp at step N refused: ...`, `adapted output 'A'.vjp at step N refused: ...` | A product call returned false. |
| `a cotangent reached adapted output 'A' at step N (seeded on it or on an adapter it feeds), but it is not differentiable` | A seed on a non-differentiable adapter or a child of one. |
| `nothing to rewind: no step has been recorded` | `rewind()` with an empty record. |

`Seeds::add()` returns false, rather than failing the pass, for a null output or a cotangent whose
length differs from the output's element count. A seed on an output its component does not list in
`differentiableOutputs()` is ignored.

## Forward mode

The workflow drives reverse mode only: no SDK code chains `jvp()` through a composition. Forward
mode is available component by component: after a step, `jvp()` gives the tangent of that step's
results, as in the dot-product test above. A caller needing a directional derivative of a whole
run either chains the components' JVPs itself or takes the gradient from `backward()` and forms its
dot product with the direction.

## The LibTorch layer

The optional library `HydroCoupleSDKTorch` sits on top of the Torch-free core and adds two classes
in `HydroCouple::SDK::Differentiation::Torch`. Configure with `-DHYDROCOUPLESDK_WITH_TORCH=ON`; by
default CMake asks the Python interpreter that `FindPython3` finds for the `torch` package's CMake
prefix and uses the LibTorch inside it, so the SDK and Python share one LibTorch in a process. `Torch_DIR` points at another
installation, and `HYDROCOUPLESDK_TORCH_ROOT` (a directory with `include/` and `lib/`) bypasses
LibTorch's CMake configuration, linking the CPU libraries directly. The library is built in the
build tree only; it is not installed or exported. Link `HydroCoupleSDKTorch` (it brings
`HydroCoupleSDK` and LibTorch with it). The core library never includes a Torch header.

### AtenModelComponent

@ref HydroCouple::SDK::Differentiation::Torch::AtenModelComponent is a component whose physics is
written once, as a function of tensors:

\f[ \mathrm{step}(\text{states}, \text{inputs}, \text{arguments}) \;\to\; (\text{states}', \text{outputs}). \f]

A subclass creates its items, lists them, and implements `step()`; the base class implements
everything else a differentiable, checkpointable component needs and advertises both capabilities:

- `update()` reads every listed state, input and argument into a Float64 CPU tensor, runs `step()`
  with gradient recording off, checks the result (one tensor per state and output, each of its
  item's shape) and writes the new states and outputs back. It keeps the tensors it stepped from.
  A thrown exception, a missing tensor or a wrong shape fails the component with the reason.
- `vjp()` calls `step()` again at the stored point with fresh leaf tensors that require gradients
  and returns `torch::autograd::grad` of the seeded results. No adjoint is written by hand.
- `jvp()` uses the double-backward identity: \f$g(w) = J^{\top} w\f$ is linear in \f$w\f$, so
  \f$\partial \langle g(w), t\rangle / \partial w = J t\f$.
- `saveState()` writes the step count, every listed state and every Float64 output into the token
  as text; `restoreState()` parses all of it before writing anything; `releaseState()` has nothing
  to free.

`step()` must be a pure function of its tensors and of `stepIndex()` (the 1-based index of the
step being taken), because the products call it again. Forcing that is not a tensor (a boundary
time series) is the subclass's own business; index it by `stepIndex()` and the default checkpoint,
which restores the step count, replays it exactly. `afterStep()` sees the detached results, for
outputs the component does not differentiate, and `doneAfterStep()` ends the run. Items must be
Float64 and in host memory. `readItem()` and `writeItem()` move a whole item to and from a tensor.

The example writes the same reservoir as the hand-written component:

@snippet ch17_torch_composition/main.cpp aten

### TorchComposition

@ref HydroCouple::SDK::Differentiation::Torch::TorchComposition lets LibTorch code treat a whole
composition run as one differentiable function. Bind the parameters (arguments with
`bindArgument()`, initial states with `bindInitialState()`) and the outputs to observe
(`observe()`); each returns its position in the tensors `operator()` takes or returns:

@snippet ch17_torch_composition/main.cpp compose

Calling it rewinds the workflow (the first call needs a freshly prepared one instead), writes each
parameter tensor into its item, runs the configured number of steps and returns each observed
output's values at every step, stacked as `[steps, item shape...]`. Calling `backward()` on a loss
of those tensors hands the incoming gradients to `DifferentiableWorkflow::backward()` as per-step
seeds and returns the gradients it reports; a parameter no cotangent reached gets zeros. LibTorch
sees one node: the components keep whatever differentiation they have (hand-written, the SDK
adapters' adjoints, or ATen autograd), and checkpointing stays the workflow's policy.

@snippet ch17_torch_composition/main.cpp loss

Finite differences through the same function check the node, and a LibTorch optimizer drives the
SDK's reverse engine:

@snippet ch17_torch_composition/main.cpp fd

@snippet ch17_torch_composition/main.cpp train

With the same parameters and initial storage as the hand-written example, the ATen component gives
the same loss and the same gradient to every printed digit. The test
`docs.example.ch17_torch_composition` prints:

```text
loss 0.7565344022; dL/dkB = [0.2412349505, -0.2072881733]
kB[0]: autograd node 0.2412349505, central difference 0.2412349505
kB[1]: autograd node -0.2072881733, central difference -0.2072881733
SGD: loss 0.7565344022 -> 0.7487328191 in 20 steps (25 runs), kB = [0.240389, 0.423206]
```

The rules a caller keeps:

- A bound argument must be read by its owner at every update or refresh, not latched at
  `initialize()`. `TorchComposition` never initializes an adapter again, because that would refresh
  it and every adapter below it, moving a stateful one off the state the rewind restored.
- A bound initial state is written after the rewind, before step 1; outputs computed from the
  state before step 1 (in `prepare()`) are not recomputed.
- Each call rewinds the workflow, so only the graph of the most recent call can be differentiated;
  `backward()` through an older one throws.
- Values are Float64 in host memory. Parameters of another dtype or device are converted on the
  way in; gradients come back as Float64 CPU tensors.

Every failure throws `c10::Error` through `TORCH_CHECK`: "nothing is observed", a parameter count
that differs from the bindings, "rewind failed: ...", "parameter i: ..." (a tensor of the wrong
shape), "the composition failed at step n (a component failed, finished early, or the workflow is
not prepared)", "cannot read ...", "this graph is from run g but the composition has since been run
again (run h); only the most recent run can be differentiated", "the gradient of ... does not match
its shape" and "backward failed: ..." with the workflow's message. Null pointers given to the
binding calls, and a non-positive step count, throw as well.

The LibTorch suite `tests/hydrocouplesdk/test_torchdifferentiation.cpp` checks the ATen component
against the hand-written one and against finite differences, and the composition's gradients
against `DifferentiableWorkflow::backward()` called directly.

## What to read next

@ref sdk_guide_workflow describes the time-stepped workflow this one extends, and
@ref sdk_guide_adapted_outputs the adapters a gradient crosses. The API reference of
@ref HydroCouple::SDK::Differentiation::DifferentiableWorkflow documents every member, including
the internal steps of the sweep.
