@page sdk_guide_ports 7. Arguments, inputs and outputs

[TOC]

A component talks to the outside through *data items*. An *argument* configures it before it runs
(a roughness per reach, a time step, a file to read). An *output* exposes values it computes, and
an *input* accepts values another component computes. Inputs and outputs together are *exchange
items*: a provider exposes an `HydroCouple::IOutput` and a consumer accepts it through an
`HydroCouple::IInput`. In the serial example (@ref sdk_guide_example) the catchment publishes a
vector with one value per reach and the channel consumes that vector as lateral inflow; the
connection entry in the composition names the two instance IDs and their port IDs:

```json
{
  "from": { "component": "catchment", "output": "runoff" },
  "to":   { "component": "channel", "input": "lateral_inflow" }
}
```

This chapter describes the `data` module of the SDK, which implements those interfaces of the
[HydroCouple standard](https://www.hydrocouple.org/HydroCouple/): the base classes for arguments,
inputs and outputs, the typed storage behind them, the ready 1-D, 2-D and id-based items, the copy
engine that moves values between an item and a caller's buffer, and the value-changed events. It
explains how values are read and written, how a provider and a consumer are wired and exchange
values, and what each class refuses and why. Adapted outputs, which transform an output on its way
to an input, are the subject of @ref sdk_guide_adapted_outputs; the time-series items are in
@ref sdk_guide_time; spatial items are in @ref sdk_guide_space.

## The classes in this chapter

| Class | Implements | Purpose |
| --- | --- | --- |
| @ref HydroCouple::SDK::AbstractComponentDataItem | `IComponentDataItem` | Identity, dimensions, value definition, owning component, comments, the value-changed signal, JSON persistence |
| @ref HydroCouple::SDK::ComponentDataItemValueChanged | `IComponentDataItemValueChanged` | The event a value change sends: the item and the changed box |
| @ref HydroCouple::SDK::AbstractArgument | `IArgument` | Role, optional and read-only flags, file filters, input types, serialization |
| @ref HydroCouple::SDK::AbstractExchangeItem | `IExchangeItem` | Common base of inputs and outputs |
| @ref HydroCouple::SDK::AbstractInput | `IInput` | One provider; `retrieveValuesFromProvider()` and `applyData()` hooks |
| @ref HydroCouple::SDK::AbstractMultiInput | `IMultiInput` | Several providers, optionally in named roles |
| @ref HydroCouple::SDK::AbstractOutput | `IOutput` | Consumers, adapted outputs, the wiring entry point `addConsumer()` |
| @ref HydroCouple::SDK::ComponentDataItem1D, @ref HydroCouple::SDK::ComponentDataItem2D | (storage) | Contiguous typed storage and the data plane over it |
| @ref HydroCouple::SDK::IdBasedComponentDataItem | (storage) | One value per string identifier |
| @ref HydroCouple::SDK::Argument1D, @ref HydroCouple::SDK::Argument2D | `IArgument` | Typed arguments read from JSON, files and other items |
| @ref HydroCouple::SDK::IdBasedArgument | `IArgument`, `IIdBasedComponentDataItem` | An argument keyed by identifier |
| @ref HydroCouple::SDK::Input1D, @ref HydroCouple::SDK::Output1D | `IInput`, `IOutput` | 1-D typed exchange items |
| @ref HydroCouple::SDK::Input2D, @ref HydroCouple::SDK::Output2D | `IInput`, `IOutput` | 2-D typed exchange items, row-major |
| @ref HydroCouple::SDK::IdBasedInput, @ref HydroCouple::SDK::IdBasedOutput | `IInput`, `IOutput`, `IIdBasedComponentDataItem` | Exchange items keyed by identifier |
| @ref HydroCouple::SDK::TimeSliceOutput | `IOutput` | One time level of a time-major item, as an output one rank lower |
| `detail::readHyperslab()`, `detail::writeHyperslab()`, @ref HydroCouple::SDK::detail::validateSelection | | The copy engine every typed item uses (`bufferio.h`) |

The typed items exist for `int`, `double` and `std::string` as ready aliases, for example
@ref HydroCouple::SDK::Output1DDouble, @ref HydroCouple::SDK::Argument1DInt and
@ref HydroCouple::SDK::IdBasedInputString; the `int`, `double` and `std::string` instantiations of
each template are compiled into the library. Another numeric element type with a `DataKind`
(`float`, `int64_t`, `uint8_t`) works by instantiating the template in your own code.

Three compiled examples go with this chapter, and every code block below comes from them:
`docs/examples/ch07_exchange_items/main.cpp` (1-D and 2-D items, bulk reads, refusals, events, a
provider and a consumer), `docs/examples/ch07_arguments/main.cpp` (arguments) and
`docs/examples/ch07_id_based/main.cpp` (id-based items and a multi-input with roles). Each checks its
results and runs as a ctest test (`docs.example.ch07_exchange_items` and so on).

## The data-item model

Every SDK data item derives from @ref HydroCouple::SDK::AbstractComponentDataItem, and the concrete
items combine one of the three interface bases (argument, input, output) with a *storage mixin*: a
plain template class that holds the values and implements the data plane over them, without being
a data item itself.

<pre class="mermaid">
classDiagram
    class AbstractComponentDataItem
    class AbstractArgument
    class AbstractExchangeItem
    class AbstractInput
    class AbstractMultiInput
    class AbstractOutput
    class AbstractAdaptedOutput
    AbstractComponentDataItem <|-- AbstractArgument
    AbstractComponentDataItem <|-- AbstractExchangeItem
    AbstractComponentDataItem <|-- AbstractAdaptedOutput
    AbstractExchangeItem <|-- AbstractInput
    AbstractExchangeItem <|-- AbstractOutput
    AbstractInput <|-- AbstractMultiInput
    class ComponentDataItem1D~T~
    class ComponentDataItem2D~T~
    class IdBasedComponentDataItem~T~
    AbstractArgument <|-- Argument1D~T~
    ComponentDataItem1D~T~ <|-- Argument1D~T~
    AbstractArgument <|-- Argument2D~T~
    ComponentDataItem2D~T~ <|-- Argument2D~T~
    AbstractArgument <|-- IdBasedArgument~T~
    IdBasedComponentDataItem~T~ <|-- IdBasedArgument~T~
    AbstractInput <|-- Input1D~T~
    ComponentDataItem1D~T~ <|-- Input1D~T~
    AbstractOutput <|-- Output1D~T~
    ComponentDataItem1D~T~ <|-- Output1D~T~
    AbstractInput <|-- IdBasedInput~T~
    IdBasedComponentDataItem~T~ <|-- IdBasedInput~T~
    AbstractOutput <|-- IdBasedOutput~T~
    IdBasedComponentDataItem~T~ <|-- IdBasedOutput~T~
    AbstractOutput <|-- TimeSliceOutput
</pre>

The 2-D input and output (`Input2D`, `Output2D`) follow the same pattern as the 1-D ones with
`ComponentDataItem2D`; the time-series items of @ref sdk_guide_time follow it with
`TimeSeriesComponentDataItem`.

### What every item carries

| Member | Meaning |
| --- | --- |
| `id()` | Identifier, unique within the owning component. Composition documents name ports and arguments by it. An empty id given to a constructor is replaced by a generated UUID. |
| `caption()`, `description()` | Text for people; setting either fires the property-changed signal. |
| `modelComponent()` | The owning component, or nullptr for a standalone item. For an output it is the component that produces the values. |
| `dimensions()` | The axes of the item's index space, in storage order (see @ref sdk_guide_quantities for `Dimension`). |
| `valueDefinition()` | What the numbers are: a `Quantity` with a unit, or a `Quality` with categories. |
| `shape()` | The current extent of each axis. |
| `dataKind()` | The element type of the values. |
| `comments()` | Free-form strings, persisted with the item (an SDK extension). |

### Ownership and lifetime

Nothing a constructor receives is owned. The dimensions, the value definition and the component
are borrowed and must outlive the item; a `Quantity` does not delete its `Unit` either, so the code
that creates a quantity keeps both (the examples hold the unit in a `std::unique_ptr` taken from
`quantity->unit()`). The id-based and time-series items that need a dimension of their own
(`IdBasedInput`, `IdBasedOutput`, `TimeIdBasedComponentDataItem`) keep it as a member.

Connections are borrowed in both directions. An output stores pointers to its consumers and to its
adapted outputs; an input stores a pointer to its provider. Destroying an output does not tell its
consumers, so disconnect them first (`removeConsumer()`); destroying an SDK adapted output
deregisters it from its adaptee, and destroying an output detaches the SDK adapters still built on
it.

The SDK items are not thread-safe. The two signals of an item (property-changed and value-changed)
are individually safe to connect and emit from several threads, but the stored values are not
synchronized: read and write an item from one thread, or lock around it.

### Two ways to reach the values

The storage mixins give a component two paths to its own items:

- the **data plane**, `getValuesInto()` and `setValuesFrom()`, which every other component and host
  uses. It validates every request, works with any buffer layout, and a write through it fires the
  value-changed signal;
- **direct access** for the owning component's own code: `operator[]` (1-D and id-based),
  `operator()(i, j)` (2-D), `valueAt(id)` (id-based), `rawData()` (the `std::vector` itself), and
  `bulkGet()`/`bulkSet()`. The element accessors are bounds-checked with `std::vector::at()` and
  throw `std::out_of_range`; `rawData()` and the bulk copies are not checked. None of them fires a
  signal.

A component typically writes its outputs directly in its `update()` and lets the workflow push the
values on; the examples below do the same.

## Dimensions and shapes

`shape()` and `dimensions()` are parallel: entry *k* of one describes axis *k* of the other. The
role of an axis (time, entity, layer, row, column) is what a consumer reads to know what the axis
means, and @ref HydroCouple::SDK::Dimension infers it from well-known ids at construction: `"time"`
is Time, `"identifiers"`, `"nodes"`, `"faces"`, `"element"` and the like are Entity, `"rows"` is Row
and `"columns"` Column. A dimension with another id has role Unknown until `setRole()` is called;
the full list is in @ref sdk_guide_quantities.

| Item | `shape()` | Axis order |
| --- | --- | --- |
| `Argument1D`, `Input1D`, `Output1D` | `{ length() }` | the one dimension given |
| `Argument2D`, `Input2D`, `Output2D` | `{ iLength(), jLength() }` | rows (dimension X), then columns (dimension Y); row-major |
| `IdBasedArgument`, `IdBasedInput`, `IdBasedOutput` | `{ identifier count }` | identifiers |
| `TimeSeriesInput`, `TimeSeriesOutput` | `{ time steps, elementSize() }` | time, then elements |
| `TimeIdBasedComponentDataItem` | `{ time steps, identifier count }` | time, then identifiers |
| `TimeSeriesArgumentDouble` | `{ times }` | time |
| `TimeSliceOutput` | the source's shape without its first axis | the source's, after the first |

The canonical orderings are the standard's: the identifier axis of an id-based item is dimension 0,
and time is dimension 0 of a time-series item, so "the current time step, every entity" is one
contiguous row.

Extents can change. A 1-D item resized with `resizeDataArray()`, an argument that reads a longer
array, and a time series that records another step all report the new extent from their next
`shape()`. A reader therefore asks for the shape each time it reads rather than caching it.

## Reading and writing values

### The buffer descriptor

The data plane moves values between an item and memory the caller owns, described by a
`HydroCouple::BufferDescriptor`. The descriptor does not own the memory or its shape array; both
must outlive the call.

| Field | What the SDK items do with it |
| --- | --- |
| `data` | Base address of element (0, ..., 0) of the caller's buffer. |
| `kind` | Must equal the item's `dataKind()`; there is no conversion. |
| `rank`, `shape` | Describe the buffer. Only the element count (the product of `shape`, 1 for rank 0) must match the product of `count`; the rank may differ from the item's. |
| `stridesBytes` | nullptr means C-contiguous; otherwise one byte step per buffer dimension, honored element by element. |
| `itemSizeBytes` | Not checked by the SDK items; leave it 0 (the kind's natural size) or set it to that size. |
| `space` | Must be `MemorySpace::Host`; the SDK's typed items do not read or write device memory. |
| `backend`, `deviceId`, `queue` | Ignored by the host-only items. |

The selection is the box from `start` to `start + count` in every axis of the item: `start` and
`count` each have one entry per dimension of `shape()`, and element (0, ..., 0) of the buffer
receives element `start` of the item. When the buffer is contiguous, each innermost run of the
selection is copied with `std::copy`, which the compiler turns into `memcpy` for numeric types; a
strided buffer is addressed one element at a time.

### Element types

| C++ element type | `DataKind` | Notes |
| --- | --- | --- |
| `int` | `Int32` | |
| `double` | `Float64` | the kind every SDK adapted output uses |
| `float` | `Float32` | |
| `int64_t` | `Int64` | |
| `bool` | `Boolean` | one byte per element; the storage mixins cannot hold it (`std::vector<bool>` has no contiguous storage), so store `uint8_t` |
| `std::string` | `String` | host only: a transport or device cannot carry it |

`Helpers::DataKindOf<T>::kind` in `hydrocouplehelpers.h` maps a type to its kind; a type without
one is a compile error.

### A bulk read

The chapter's first example makes a five-reach output and reads three of its values:

@snippet ch07_exchange_items/main.cpp setup

@snippet ch07_exchange_items/main.cpp bulk_read

The same selection can land anywhere in the caller's memory. Here the three values go into every
other slot of a six-element array, described with a byte stride:

@snippet ch07_exchange_items/main.cpp strided_read

For the common cases, `hydrocouplehelpers.h` has typed wrappers over the same calls:
`Helpers::getValue()` reads one element at a full index, `Helpers::getValues()` reads a box into a
flat `std::span` in C order, and `setValue()`/`setValues()` write. `Helpers::makeContiguous()`
builds a contiguous host descriptor.

@snippet ch07_exchange_items/main.cpp typed_helpers

A 2-D item takes a 2-entry start and count. The buffer may have any rank: a 2 x 2 block goes into a
rank-2 buffer and a column into a rank-1 buffer of three elements.

@snippet ch07_exchange_items/main.cpp two_d

Output of these steps (from the ctest run of `docs.example.ch07_exchange_items`):

```text
runoff: rank 1, extent 5, kind Float64 yes
read values 1..3: 20 30 40
strided into 6 slots: 20 0 30 0 40 0
Helpers::getValue at 2: 30; Helpers::getValues: 10 20 30 40 50
block rows 1-2, columns 1-2: 11 12 21 22
column 3: 3 13 23
```

### What a read or write refuses {#sdk_guide_ports_refusals}

Every typed item validates a request with @ref HydroCouple::SDK::detail::validateSelection before
copying, and reports the first failure in this order. A refused call returns `false`, leaves the
caller's buffer and the item unchanged, and writes the message to `*message` when `message` is not
null. The SDK items do not queue an error on the owning component for a refused read; the caller
decides what a failure means.

| Message | Cause | Fix |
| --- | --- | --- |
| `kind mismatch` | The buffer's `kind` is not the item's `dataKind()`. | Allocate the item's element type; convert after the read. |
| `unsupported memory space` | The buffer is not in host memory. | Stage through a host buffer (@ref sdk_guide_devices). |
| `selection rank mismatch` | `start` or `count` does not have one entry per dimension of `shape()`. | Pass one entry per axis. |
| `selection out of bounds` | A negative start or count, or `start + count` past an axis's extent. | Read `shape()` first; extents can change. |
| `buffer element count mismatch` | The buffer holds a different number of elements than the product of `count`. | Size the buffer's shape to the selection. |

@snippet ch07_exchange_items/main.cpp refusals

```text
Float32 buffer: kind mismatch
start 4, count 3: selection out of bounds
count 2 into 3 slots: buffer element count mismatch
```

Adapted outputs and derived views refuse every write with their own message (for example
`LinearTransformAdaptedOutput is read-only`), and an adapted output that has not computed values
yet refuses reads (@ref sdk_guide_adapted_outputs).

## Value-changed events

An item is an `HydroCouple::ISignal` of `IComponentDataItemValueChanged` events. Each event is a
@ref HydroCouple::SDK::ComponentDataItemValueChanged, which carries the item and the `start` and `count` of the
box that changed, one entry per axis. An empty `start` and `count` mean the emitter did not say
which values changed, and a listener treats the whole item as changed.

@snippet ch07_exchange_items/main.cpp events

```text
setValuesFrom fired: start 2, count 2
```

Three rules govern listening:

- The signal holds a slot through a `std::weak_ptr`. Keep the `std::shared_ptr` to the slot for as
  long as you want events; a slot whose last owner releases it is dropped at the next emission.
  Connecting the same slot twice has no effect.
- The first argument a slot receives is the item's internal signal object, not the item. Use
  `event->componentDataItem()` to find the item.
- Slots run on the thread that changed the values, before the write returns. `blockSignals(true)`
  suppresses both the value-changed and the property-changed signal of an item; the events are
  dropped, not queued.

When the SDK items fire the event:

| Operation | Event |
| --- | --- |
| `setValuesFrom()` that succeeds | the written box |
| `Argument1D`, `Argument2D`, `IdBasedArgument`: `initialize()` from a string, JSON, file or URL | the whole item |
| `Input1D`, `Input2D`: `retrieveValuesFromProvider()` that copies values | the copied box |
| `TimeSeriesInput`: `retrieveValuesFromProvider()` | the new row |
| An SDK adapted output's `refresh()` | the whole item |
| Direct writes (`operator[]`, `rawData()`, `bulkSet()`), `readData()`, `IdBasedInput::retrieveValuesFromProvider()`, `initialize()` from another data item | none |

Metadata changes fire the separate property-changed signal (@ref sdk_guide_quantities) with the
property's name: `"Caption"`, `"Description"`, `"Comments"`, `"IsOptional"`, `"IsReadOnly"`,
`"FileFilters"`, `"Provider"`, `"Providers"`, `"Consumers"`, `"AdaptedOutputs"` or `"Arguments"`.

## Arguments

An argument is a data item that configures a component or an adapted output. A component creates
its arguments in `createArguments()` (or its constructor), so a host can set them while the
component is still `Created`; the component reads them in `initialize()` (@ref sdk_guide_component).
@ref HydroCouple::SDK::AbstractArgument adds the metadata the standard asks for:

| Member | Default | Meaning |
| --- | --- | --- |
| `role()`, `setRole()` | `Configuration` | What the argument is to the model: `Configuration`, `Parameter`, `InitialCondition`, `Forcing` or `Geometry`. Calibration and data-assimilation tools select arguments by it. |
| `isOptional()`, `setIsOptional()` | `false` | Whether a missing value is acceptable. |
| `isReadOnly()`, `setIsReadOnly()` | `false` | Whether hosts must leave the value alone. Metadata only: the SDK does not refuse a change. |
| `fileFilters()`, `addFileFilter()` | none | Filters such as `"JSON files (*.json)"` for a host's file dialog. |
| `validComponentDataItemTypes()`, `addValidComponentDataItemType<T>()` | none | The item types `initialize(item)` is meant for. Metadata only. |
| `currentArgumentInputType()` | `String` | The input type of the last successful string `initialize()`. |
| `sourceUri()` | empty | The remote URI the values were read from (see below). |

### Declaring and setting an argument

@snippet ch07_arguments/main.cpp declare

`initialize(value, type, message)` reads a value. The SDK's typed arguments read it the same way:
for `File` and `URL`, `value` names a path or URI and the text it names is read through
`IO::readReference()` (a local path or a `file:` URI is read directly; any other scheme needs the
host's `IO::UriResolver`); for every other type, `value` is the text itself, parsed as JSON. String
and JSON are therefore the same.

@snippet ch07_arguments/main.cpp initialize

```text
String: length 2, values [0.025,0.03]
JSON object: length 4, values [0.02,0.03,0.04,0.05]
File: length 3, values [0.03,0.035,0.04], input type File yes
```

The JSON each argument accepts:

| Argument | Accepted JSON | `toString()` |
| --- | --- | --- |
| `Argument1D` | an array `[0.03, 0.035]`, or `{"values": [...]}` | the array |
| `Argument2D` | an array of equal-length row arrays `[[1, 2], [3, 4]]`, or `{"values": [[...]]}` | the rows |
| `IdBasedArgument` | `{"J1": 1.5, "J2": 2.0}`, or `{"identifiers": [...], "values": [...]}` | the object form |
| `TimeSeriesArgumentDouble` | `{"times": [...], "values": [...]}`; a `.csv` file or URL is read as CSV | `{"times": ..., "values": ...}` |

The length of the result is the length of what was read, so an argument constructed with length 0
takes the size the host gives it. Elements are converted with `nlohmann::json::get`, so a string in
a numeric argument fails with the JSON library's message.

A JSON object with a non-empty string under the key \@uri is not read as values: the argument reads
the URI it names instead, as if `initialize(uri, URL)` had been called. That is how an argument
whose values came from a service survives a save and an open (@ref sdk_guide_compositions).

### Advertised input types

`isValidArgType()` is what a host asks before offering an input type. The base answer is fixed by
the metadata:

| Input type | Valid when |
| --- | --- |
| `String`, `JSON` | always |
| `File`, `XML`, `URL` | at least one file filter was added |
| `MEMORY_OBJECT` | at least one valid item type was registered |
| `YAML` | never |

@snippet ch07_arguments/main.cpp valid_types

```text
isValidArgType: String=1 File=1 JSON=1 YAML=0 XML=1 URL=1 MEMORY_OBJECT=0
```

The answer is metadata and does not limit `initialize()`: the typed arguments read `File` and `URL`
whether or not a filter was added, and XML or YAML text given to them is parsed as JSON (and so
fails). `TimeSeriesArgumentDouble` adds no file filter, so it reports `File` as unsupported although
it reads CSV and JSON files.

### Writing an argument back

Three members write an argument out:

- `toString()` is the compact text of the value, for display and logs.
- `serialize(type, value, message)` is the round-trippable form. The base supports `String`
  (`toString()`) and `JSON` (`writeData()`, pretty-printed); any other type fails with
  `unsupported serialization format for this argument`. `IO::ModelInitializer::serialize()` uses
  the JSON form to write a composition back.
- `writeData()` and `readData()` are the JSON object behind it: `"caption"`, `"description"`,
  `"comments"`, the values (`"values"`, and for an id-based argument `"identifiers"`), and \@uri when
  the values came from a remote URI. A local file is written inline as values, because the document
  can be moved next to it; a resource on a service cannot be got back from the document, so its URI
  is recorded.

`saveData()` writes the current values back to the file they were read from, when that was a local
file; values read from a remote URL are not written anywhere. It writes `toString()`, so a file that
held `{"values": [...]}` is rewritten as a bare array.

@snippet ch07_arguments/main.cpp save

@snippet ch07_arguments/main.cpp serialize

```text
saveData wrote: [0.033,0.035,0.04]
serialize(JSON):
{
  "caption": "Manning roughness per reach",
  "comments": [],
  "description": "",
  "values": [
    0.033,
    0.035,
    0.04
  ]
}
serialize(YAML): false (unsupported serialization format for this argument)
```

### When a value is refused

@snippet ch07_arguments/main.cpp errors

```text
scalar: Expected JSON array of values
malformed: [json.exception.parse_error.101] parse error at line 1, column 7: syntax error while parsing value - unexpected end of input; expected '[', '{', or a literal
string element: [json.exception.type_error.302] type must be number, but is string
missing file: Cannot open file: no_such_file.json
```

A refused value leaves the previous values in place, with one exception: when an element fails to
convert part way through, the argument has already been resized to the new length. Check the return
value and re-initialize after a failure.

### Two-dimensional arguments and copies from other items

@snippet ch07_arguments/main.cpp two_d

`initialize(item, message)` copies values from another data item, so one component's output can
initialize another's argument (composition documents call this an argument binding,
@ref sdk_guide_compositions). `Argument1D` takes any rank-1 item of its kind, `Argument2D` any
rank-2 item, `IdBasedArgument` any `IIdBasedComponentDataItem` (taking its identifiers too); each
resizes to the source. The copy fires no value-changed event and does not change
`currentArgumentInputType()`.

@snippet ch07_arguments/main.cpp from_item

```text
Argument2D shape {2, 3}, row 1 column 2 = 3
ragged rows: Rows must be arrays of equal length
from an output: [1.5,2.5]
from an Int32 output: Source data kind mismatch
```

### Typed arguments

An argument is a data item, so an argument that holds a time series or a mesh can answer the typed
interface a configurator looks for instead of presenting a column of numbers.
@ref HydroCouple::SDK::Temporal::TimeSeriesArgumentDouble is an argument that is also an
`ITimeSeriesComponentDataItem` (@ref sdk_guide_time), and
@ref HydroCouple::SDK::Spatial::PolyhedralSurfaceArgument is one that is also an
`IPolyhedralSurfaceComponentDataItem` (@ref sdk_guide_space). Both are read-only through the data
plane: their values are loaded with `initialize()`.

## Outputs and inputs

### Wiring a connection

@ref HydroCouple::SDK::AbstractOutput::addConsumer is the single entry point for a connection, as
the standard requires. Hosts, workflows and `IO::ModelInitializer` call it and never call
`setProvider()` themselves. For a plain input, the output asks the input's `canConsume(this, why)`
and then calls `setProvider(this)`; for a multi-input it asks `canConsume(this, why, role)` and then
calls `addProvider(this, role)`. Only when both succeed does it record the consumer and fire
`"Consumers"`. A consumer already connected is ignored.

When either step fails, nothing is recorded, a `Severity::Error` entry is queued on the output's
component (when it is an `AbstractModelComponent`), and `std::invalid_argument` is thrown with the
message `Output "OUTPUT" cannot be consumed by input "INPUT": REASON`. A role given for
an input that is not a multi-input is refused the same way, with the reason
`provider role "ROLE" given for an input that is not a multi-input`.

`removeConsumer()` is the counterpart: it forgets the consumer and calls `setProvider(nullptr)` (or
`removeProvider(this)` on a multi-input).

Each output keeps its consumers in connection order and identifies them by object, so two
components' inputs that share an id ("inflow" on two reaches) both connect.

### What each input accepts

`canConsume()` is the input's own rule, and the SDK's typed inputs check only what they need to copy
values:

| Input | Accepts a provider that | Messages |
| --- | --- | --- |
| `Input1D` | has the same `DataKind` and rank 1 | `Provider is null`, `Provider data kind mismatch`, `Provider is not one-dimensional` |
| `Input2D` | has the same `DataKind` and rank 2 | `Provider is null`, `Provider data kind mismatch`, `Provider is not two-dimensional` |
| `IdBasedInput` | has the same `DataKind`, rank 1 and at least as many values as the input has identifiers | `provider is null`, `provider DataKind differs`, `provider must be 1-D with at least N values` |
| `TimeSeriesInput` | is an `ITimeSeriesComponentDataItem` of the same kind with shape `{ times, elementSize() }` | `Provider is null`, `Provider is not a time-series data item`, `Provider data kind mismatch`, `Provider element count mismatch` |

None compares lengths (beyond the id-based count), units, dimension roles or time semantics. Those
are declarations, and @ref HydroCouple::SDK::validateConnection checks them across a proposed link
without wiring it; `hasConnectionError()` tells whether any finding is an error. A workflow runs the
same checks in its `validate()` (@ref sdk_guide_workflow). When the two ends do not match, the fix is
an adapted output between them (@ref sdk_guide_adapted_outputs).

### Exchanging values

<pre class="mermaid">
sequenceDiagram
    participant W as Workflow
    participant P as Provider component
    participant O as Output
    participant C as Consumer component
    participant I as Input
    W->>P: update()
    P->>O: write values (operator[], rawData(), setValuesFrom())
    W->>O: refresh adapted outputs (time-stepped workflow)
    W->>C: update()
    C->>I: retrieveValuesFromProvider()
    I->>O: getValuesInto(...)
    C->>I: applyData()
    Note over C: the component reads the input's storage and advances
</pre>

The provider's component writes its outputs while it updates. The ready outputs
(`Output1D`, `Output2D`, `IdBasedOutput`, `TimeSeriesOutput`) implement `updateValues()` as a no-op,
because there is nothing for them to compute: the component has already written the storage.
`AbstractOutput::updateValues(query)` calls `updateValues()` and then refreshes the adapted outputs
registered on the output; it does not inspect the query or advance the component. A pull-driven
workflow calls it on each provider before the consumer reads; a time-stepped workflow refreshes the
adapted outputs itself after each component update (@ref sdk_guide_workflow).

On the consumer side, `AbstractModelComponent::applyInputValues()` calls, for each input,
`retrieveValuesFromProvider()` and then `applyData()`. `retrieveValuesFromProvider()` reads the
provider's current values with `getValuesInto()`; it does not ask the provider to update. The typed
inputs copy what fits:

| Input | What `retrieveValuesFromProvider()` copies |
| --- | --- |
| `Input1D` | the first min(provider length, `length()`) values |
| `Input2D` | the overlapping top-left block |
| `IdBasedInput` | the first `length()` values, by position |
| `TimeSeriesInput` | the provider's last time step, appended as a new row |

When the input is unconnected, the provider does not fit, or the read fails, it returns without
changing anything and without a message. `applyData()` is an empty hook in all of them: a component
either overrides it or reads the input's storage in its own `update()`.

@snippet ch07_exchange_items/main.cpp exchange

```text
inflow after retrieve: 10 20 99 98 50
refused: Output "runoff" cannot be consumed by input "counts": Provider data kind mismatch
```

## Id-based items

An id-based item attaches each value to a string identifier instead of a position: a value per
junction, per reach, per gauge. @ref HydroCouple::SDK::IdBasedComponentDataItem stores the
identifiers and the values in two parallel vectors and keeps a hash map from identifier to position.
The identifier axis is dimension 0. `valueAt(id)` reads or writes by identifier and throws
`std::out_of_range` for an unknown one; `indexOf(id)` returns the position or -1. `addIdentifier()`
refuses a duplicate with `std::invalid_argument`; the constructor and `setIdentifiers()` do not
check, so keep identifiers unique. `setIdentifiers()` keeps the value of every identifier that
stays and gives new ones the default.

@snippet ch07_id_based/main.cpp id_output

@ref HydroCouple::SDK::IdBasedInput matches by position, not by name: value *i* of the provider lands
on identifier *i* of the input, whatever the provider's identifiers are, and the provider need not
be id-based. When two components order their entities differently, the mapping belongs in a
subclass that overrides `retrieveValuesFromProvider()` or in an adapted output.

@snippet ch07_id_based/main.cpp id_input

@ref HydroCouple::SDK::IdBasedArgument takes its identifiers from the value it reads. In the object
form the identifiers come back sorted, because `nlohmann::json` objects are ordered maps; the
parallel-array form keeps the order given. `toString()` always writes the object form, while
`writeData()` (and so `serialize(JSON)`) writes the arrays and keeps the order.

@snippet ch07_id_based/main.cpp id_argument

```text
output identifiers: J1 J2 J3; shape {3}; J2 at position 1
input J3 = 2
four-id input on a three-value provider: provider must be 1-D with at least 4 values
object form identifiers: J1 J2 J3
array form identifiers: J3 J1 J2; J1 = 100
toString: {"J1":100.0,"J2":99.2,"J3":98.5}
writeData identifiers: ["J3","J1","J2"]
```

## Multi-inputs

A multi-input accepts several providers, optionally each in a named *role* (an upstream inflow and a
lateral inflow, say). @ref HydroCouple::SDK::AbstractMultiInput stores the providers in a map keyed
by the role's id, or by the provider's own id when it was added without a role. Its base answers
are: no roles (`providerLabels()` is empty), no role required, and `provider()` returns nullptr,
because the providers are read through `providers()`. A subclass supplies the roles, the two
`canConsume()` overloads, and the data pull. The example sums two providers:

@snippet ch07_id_based/main.cpp multi_input

The roles are passed to `addConsumer()`; the output checks the role with the multi-input and records
the connection on both ends:

@snippet ch07_id_based/main.cpp roles

```text
providers 2, upstream required yes, lateral required no, total 3.75
provider() of a multi-input: nullptr
```

Two cautions follow from the map. `providers()` is in hash order, not connection order. And a
provider added under a key already in use replaces the first one in `providers()` without an error,
although both outputs still list the multi-input among their consumers: two providers that share an
id (two components' "runoff" outputs) and are added without roles collide. Give each provider a
role when several feed one multi-input.

## One time level of a record: TimeSliceOutput

A recorded run stores history, a variable shaped `{ time, entities... }`; a live input wants the
present, `{ entities... }`. @ref HydroCouple::SDK::TimeSliceOutput bridges the two: it wraps any
time-major item (an output, an argument, a reopened result) and exposes one time level of it as an
output one rank lower, so a stored run can drive a new composition as a boundary condition. The
level is chosen explicitly with `setTimeIndex()` (clamped to the record), `advance()`, or `setTime()`
(the nearest recorded time, when the source is a time series); the SDK has no global clock to infer
it from. Reads go to the source with the time index prepended; writes are refused.

The slice owns copies of the source's dimensions after the first and a new unitless value
definition named `ID_slice` (ID being the slice's id), so the source's unit is not carried over. A rank-1 source gives a
rank-0 slice, while `dimensions()` still lists one `"element"` dimension. @ref sdk_guide_time shows a
slice over a station record, and `examples/analysis_reopen` uses one to print the last level of each
reopened result (@ref sdk_guide_record).

## Writing your own item

Derive from the base that matches the role (`AbstractArgument`, `AbstractInput`,
`AbstractMultiInput` or `AbstractOutput`), and when the storage is a contiguous row-major array,
also from a storage mixin; the typed items are short templates and a good model. The data plane is
five members:

- `shape()` and `dataKind()`;
- `getValuesInto()`, which can call `detail::readHyperslab<T>(storage, shape, destination, start,
  count, message)` to get the standard validation and copy;
- `setValuesFrom()`, which can call `detail::writeHyperslab<T>()` and then
  `emitValueChanged(start, count)`; an item that is computed (as the example's multi-input is)
  returns false with a message instead;
- for an input, `canConsume()`, `retrieveValuesFromProvider()` and `applyData()`; for an output,
  `updateValues()`.

Give every axis a dimension, in storage order, with an id that names its role or an explicit
`setRole()`; give the item a value definition, because `validateConnection()` compares it. Fire
value-changed after every write a listener should see.

## Diagnostics

Messages the `data` module produces, in addition to the data-plane refusals listed in
@ref sdk_guide_ports_refusals above:

| Message | Produced by | Cause | Fix |
| --- | --- | --- | --- |
| `Output "OUTPUT" cannot be consumed by input "INPUT": REASON` (`std::invalid_argument`, and an Error on the component) | `AbstractOutput::addConsumer()` | The input's `canConsume()` or `setProvider()` refused | See the reason; insert an adapted output or fix the port |
| `provider role "ROLE" given for an input that is not a multi-input` | `addConsumer()` | A role on a plain input | Drop the role, or use a multi-input |
| `Expected JSON array of values` | `Argument1D::initialize()` | The JSON is not an array or `{"values": [...]}` | Wrap a scalar as `[x]` |
| `Expected JSON array of row arrays`, `Rows must be arrays of equal length` | `Argument2D::initialize()` | Not a rectangular array of rows | Give every row the same length |
| `Expected JSON object`, `identifiers/values must be arrays of equal length` | `IdBasedArgument::initialize()` | Not an object, or arrays of different lengths | Use one of the two documented forms |
| `Cannot open file: PATH` | any typed argument, File | The path does not exist or is unreadable | Check the path (relative paths resolve against the process's working directory) |
| `No resolver is installed for 'SCHEME' URIs, so this argument cannot read URI` | any typed argument, URL | A remote URI and no `IO::UriResolver` for its scheme | Install a resolver in the host |
| `Not a local file URI: URI` | any typed argument, URL | A `file:` URI naming another host | Use a local path |
| `Source data item is not one-dimensional` / `two-dimensional` / `id-based`, `Source data kind mismatch` | `initialize(item)` | The source does not fit the argument | Bind an item of the right rank and kind |
| `unsupported serialization format for this argument` | `serialize()` | A type other than String or JSON | Serialize as JSON |
| `item "ID" is a time slice of another item and is read-only` | `TimeSliceOutput::setValuesFrom()` | A write to a slice | Write to the source |
| JSON library messages (`[json.exception...]`) | any typed argument | Malformed JSON or an element of the wrong type | Fix the text |

The workflow-level messages that mention ports (an input with no provider, a connection finding)
are in @ref sdk_guide_workflow, and the composition messages in @ref sdk_guide_compositions; all are
collected in @ref sdk_ref_diagnostics.
