@page sdk_guide SDK developer guide

This guide explains how to build HydroCouple 2.0 components and hosts in C++ with the SDK: how to
wrap a model as a component, describe the values it exchanges in time and space, connect it to
other models, run a coupled composition, record the run so others can reopen it, and go further
with distributed, device and differentiable coupling.

The chapters build on each other, but each states what it assumes. Every code sample is part of a
program in the SDK repository (`docs/examples/` and `examples/`) that the test suite compiles and
runs, and the output shown is that program's output. Build them with
`-DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON` and run `ctest -L docs-example`.

## Getting started

- @subpage sdk_guide_overview
- @subpage sdk_guide_build
- @subpage sdk_guide_first_component
- @subpage sdk_guide_example

## Concepts

- @subpage sdk_guide_component
- @subpage sdk_guide_quantities
- @subpage sdk_guide_ports
- @subpage sdk_guide_adapted_outputs
- @subpage sdk_guide_time
- @subpage sdk_guide_space

## Running models

- @subpage sdk_guide_workflow
- @subpage sdk_guide_compositions
- @subpage sdk_guide_record
- @subpage sdk_guide_mesh_packages

## Advanced topics

- @subpage sdk_guide_distributed
- @subpage sdk_guide_devices
- @subpage sdk_guide_differentiation
- @subpage sdk_guide_tools

## Reference

- @subpage sdk_ref_composition
- @subpage sdk_ref_run_manifest
- @subpage sdk_ref_cmake
- @subpage sdk_ref_diagnostics
