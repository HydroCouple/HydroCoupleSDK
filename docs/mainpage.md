# HydroCouple SDK

The HydroCouple SDK is a C++20 library that implements the
[HydroCouple interfaces](https://www.hydrocouple.org/HydroCouple/): working base classes for model
components, typed arguments, inputs and outputs, time series, geometries and meshes, adapted outputs
between components, workflows that schedule a coupled run, composition documents that configure
one, writers that record it, and run manifests that let any program reopen it. A component author
writes only the model; a host writer gets loading, connecting, running and recording ready-made.

The @subpage sdk_guide explains all of it, from a first component to distributed and differentiable
coupling. Every code sample in it is a program the SDK's test suite compiles and runs.

**Start here**

- @ref sdk_guide_overview — what the SDK contains and how it relates to the interfaces, components
  and hosts.
- @ref sdk_guide_build — build, install and link the SDK.
- @ref sdk_guide_first_component — write a component and drive it from a host.
- @ref sdk_guide_example — couple two components, record the run and reopen it.

**Most used classes**

- @ref HydroCouple::SDK::AbstractModelComponent wraps a model;
  @ref HydroCouple::SDK::AbstractModelComponentInfo describes and creates it.
- @ref HydroCouple::SDK::Argument1D, @ref HydroCouple::SDK::Input1D and
  @ref HydroCouple::SDK::Output1D carry its configuration and exchanged values.
- @ref HydroCouple::SDK::IO::ModelInitializer applies a composition document;
  @ref HydroCouple::SDK::TimeSteppedWorkflow runs the components.
- @ref HydroCouple::SDK::IO::RunRecorder records a run;
  @ref HydroCouple::SDK::ResultsModelComponent reopens it.

**Related manuals**

- [HydroCouple interfaces](https://www.hydrocouple.org/HydroCouple/), the standard the SDK
  implements.
- [HydroCoupleComponents](https://www.hydrocouple.org/HydroCoupleComponents/), ready-made
  components built on the SDK.
- [HydroCouple Composer](https://www.hydrocouple.org/HydroCoupleComposer/), the desktop host that
  loads, connects and runs components.
