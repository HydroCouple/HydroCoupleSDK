@page sdk_ref_diagnostics 22. Errors and troubleshooting

[TOC]

This chapter explains how the SDK reports problems, lists the numeric codes it attaches to them,
and maps common symptoms to their causes. The messages of each area are listed in the chapter that
describes that area, in its diagnostics section; the links at the end of this chapter lead to them.

## How problems are reported

The HydroCouple standard's failure channel is the return value and the error queue, because
exceptions cannot cross the boundaries components often run behind: a process, an MPI rank, a C
entry point, another language. Exceptions are allowed only as a local convenience. The SDK uses
four channels.

1. **Return values with messages.** A method that can fail returns `false` (or a null pointer) and
   fills a message: `IArgument::initialize(text, type, message)`,
   `IComponentDataItem::getValuesInto(..., &message)`, `ModelInitializer::initializeFromFile(path,
   message)`, `RunManifest::read(path, manifest, message)`. Always print the message; it names
   the item, the rule and the value.
2. **The component's status.** A component that cannot continue sets its status to `Failed` with
   a message. `initialize()`, `prepare()` and `update()` all end this way on failure; a host checks
   `status()` after each call.
3. **The error queue.** Each component and workflow keeps a queue of
   `HydroCouple::ErrorEntry` records, which a host reads with `errors()` (passing `true` empties
   the queue). An entry has a severity, a numeric code, the id of the entity that raised it and a
   message:

   | Severity | Meaning |
   | --- | --- |
   | `Information` | Informational |
   | `Warning` | A recoverable anomaly; the operation went ahead |
   | `Error` | The operation failed; the component may continue |
   | `Fatal` | The component is in the `Failed` state |

   Setting the status to `Failed` with a message (`setStatus(Failed, message)`) queues a `Fatal`
   entry with that message automatically; the one-argument `setStatus(Failed)` queues nothing, so
   always give the reason.
   The interfaces ask a data item whose value-access method fails to queue an `Error` entry on its
   component as well; the SDK's typed items report such a failure only through the return value
   and the message, so read the message.
4. **Connection findings.** `validateConnection(output, input)` returns a list of findings, each
   with a severity and a code; `hasConnectionError(findings)` tells whether any blocks the
   connection. A workflow's `validate()` runs these checks for every connection and queues the
   findings on the workflow's error queue (@ref sdk_guide_workflow).

The IO thread reports a failing writer through its own `errorCode()` and `errorMessage()`
(@ref sdk_guide_record).

One SDK method also throws. When an input refuses an output in
`AbstractOutput::addConsumer()` (an output and input whose types, roles or other requirements do
not match), the output queues an `Error` entry on its component and throws `std::invalid_argument`
with the message `Output "<id>" cannot be consumed by input "<id>": <reason>`. A host that connects
items itself should check `canConsume()` first or catch the exception; `ModelInitializer` validates
connections before it makes them.

## Error codes

`ErrorEntry::code` is implementation-defined. The SDK divides the range so that a program reading
`errors()` can recognize the SDK's own diagnostics without parsing text:

| Codes | Used by |
| --- | --- |
| 0 | The `Fatal` entry queued with a `Failed` status, and validation problems, from the SDK's base classes |
| 1–999 | Components (yours included); `ResultsModelComponent` uses 1 to 5 |
| 1000–1099 | Lifecycle diagnostics of the SDK's base classes |
| 1100–1199 | Connection findings of `validateConnection()` |

The named codes are constants in `hydrocouplesdk/core/diagnosticcodes.h`, namespace
`HydroCouple::SDK::Diagnostic`:

| Code | Name | Severity and meaning |
| --- | --- | --- |
| 1001 | `IllegalStatusTransition` | Error. A component or workflow changed to a status its transition table does not allow from the current one. Reported once for each pair of statuses; the transition still happens. It points to a defect in the component's code, usually a missing intermediate status (for example `Updated` set without `Updating` first). |
| 1101 | `DataKindMismatch` | Error. The output and input hold different element types (for example `Float64` and `Int32`); values are never converted between types. |
| 1102 | `ValueDefinitionMismatch` | Error when one side is a quantity and the other a quality, or two qualities have different categories. Warning when one side declares no value definition (not checked for adapted outputs). |
| 1103 | `ValueKindMismatch` | Error when both sides declare a value kind and they differ; Warning when one or both declare none. |
| 1104 | `UnitMismatch` | Error when the units have different dimensions, or the same dimensions but a different scale or offset (m³/s against ft³/s); Warning when a side declares no unit or no dimensions, or a dimensionless unit meets a dimensioned one. @ref sdk_guide_adapted_outputs describes the adapted outputs that transform values between items. |
| 1105 | `TimeKindMismatch` | Error when the time kinds differ, or both are intervals of different lengths; Warning when a side leaves the time kind unknown (@ref sdk_guide_time). |
| 1106 | `TemporalPolicyMismatch` | Error when the provider's interpolation or extrapolation is one the consumer does not accept, or both ends leave it undeclared; Warning when one end leaves it undeclared. |
| 1107 | `HorizontalReferenceMismatch` | Error. Both sides declare a horizontal coordinate reference system and they differ (@ref sdk_guide_space). |
| 1108 | `VerticalReferenceMismatch` | Error when the vertical datums or vertical units differ; Warning when only one side declares a datum. |
| 1109 | `MeshLocationMismatch` | Error. The values sit on different mesh or network entities (nodes against faces, for example). |
| 1110 | `VectorBasisMismatch` | Error when the scalar, vector or tensor structure or the vector basis differs; Warning when a vector or tensor end declares no basis. |
| 1111 | `LayeringMismatch` | Error when layered items disagree on what a layer is or how many there are, or a layered item has no vertical coordinate; Warning when the vertical coordinate kind is undeclared. |
| 1112 | `DimensionRoleMismatch` | Error. An axis means different things on the two sides (time on one, entity on the other); axes whose role is unknown or other are skipped. |
| 1113 | `QuantityNameMismatch` | Warning only. Both sides name their quantity and the names differ (concept URIs if both declare one, otherwise CF standard names); one model's outflow is legitimately another's inflow. |

`ResultsModelComponent`, the component that reopens a recorded run, uses these codes, all
`Fatal` and all with the component `Failed`. Because it also sets the status to `Failed` with the
same message, each failure appears twice on its error queue: once with the code below and once with
code 0.

| Code | Meaning |
| --- | --- |
| 1 | The run manifest records no results for the requested component |
| 2 | No reader exists for a recorded item's format |
| 3 | A reader could not open the artifact a recorded item points to (moved, missing or unreadable) |
| 4 | The run's formats cannot be read by this SDK build; the message lists them and says to rebuild with the matching features |
| 5 | A recorded item could not be constructed from its catalog entry |

## Symptoms and causes

| Symptom | Likely cause | Where to look |
| --- | --- | --- |
| `initialize()` ends in `Failed` with `Argument initialization failed: ...` | The component's `initializeArguments()` rejected a value; the rest of the message is the component's reason | @ref sdk_guide_first_component, @ref sdk_guide_component |
| `IArgument::initialize()` returns `false` | The text is not valid for the input type (malformed JSON, a file that does not exist, a wrong number of values) | @ref sdk_guide_ports |
| `getValuesInto()` / `setValuesFrom()` return `false` | The buffer's kind differs from the item's `dataKind()`, the start/count do not match the item's rank or extents, or the element count differs from the selection | @ref sdk_guide_ports |
| An error with code 1001 | A component sets its statuses in an order the standard forbids | @ref sdk_guide_component |
| `ModelInitializer` returns `false` and nothing changed | The document failed parsing or validation: an id that does not resolve, an argument or port that does not exist, a malformed field | @ref sdk_guide_compositions, @ref sdk_ref_composition |
| `ModelInitializer` returns `false` naming a component that "failed to reach Initialized" | The document was valid but the component's own initialization failed; read the component's `errors()` | @ref sdk_guide_compositions |
| "This SDK build has no YAML support" or a YAML document rejected | The SDK was built without yaml-cpp; use the JSON form | @ref sdk_guide_build |
| A workflow's `validate()` returns problems with codes 1101–1113 | A connection is incompatible: types, units, time, space or structure | The code table above; @ref sdk_guide_workflow |
| The workflow never reaches `Done` | A component never reports `Done` (check its stopping condition), or a cycle does not converge | @ref sdk_guide_workflow |
| `IOThread::errorCode()` is not zero after `finish()` | A writer failed; `errorMessage()` names the writer and the reason, and that writer stopped writing | @ref sdk_guide_record |
| `RunManifest::read()` fails, or a reopened component fails with codes 1–5 | The manifest or its artifacts moved, the run recorded nothing for that component, or the build lacks the format's reader | @ref sdk_guide_record |
| `mesh packages require an SDK built with NetCDF` | Mesh packages are NetCDF files; rebuild with `USE_NETCDF` | @ref sdk_guide_mesh_packages |
| A host refuses a component library | The library's ABI stamp differs from the host's: rebuild the component with the same interface version and toolchain as the host | @ref sdk_guide_first_component |
| A program cannot find `HydroCoupleSDK.dll` (Windows) | The DLL is not next to the executable or on `PATH` | @ref sdk_guide_build |
| Configure-time errors | Missing interface headers, version misalignment, missing optional libraries | @ref sdk_guide_build |

## Messages by area

Each of these chapters ends with the messages its classes produce, with their causes and fixes:

- @ref sdk_guide_component — component lifecycle and status
- @ref sdk_guide_quantities — units, quantities and dimensions
- @ref sdk_guide_ports — arguments, inputs, outputs and value access
- @ref sdk_guide_adapted_outputs — adapted outputs and their factories
- @ref sdk_guide_time — time series and temporal items
- @ref sdk_guide_space — geometries, meshes, networks and rasters
- @ref sdk_guide_workflow — workflows and connection validation
- @ref sdk_guide_compositions — composition documents and `ModelInitializer`
- @ref sdk_guide_record — writers, the IO thread, run manifests and readers
- @ref sdk_guide_mesh_packages — mesh packages
- @ref sdk_guide_distributed — transports, proxies and partitioned items
- @ref sdk_guide_devices — device backends and buffers
- @ref sdk_guide_differentiation — gradients and differentiable workflows
- @ref sdk_guide_tools — the meshing tools
