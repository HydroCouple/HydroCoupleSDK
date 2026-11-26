@page sdk_guide_quantities 6. Quantities, units and dimensions

[TOC]

Every data item in HydroCouple says what its numbers are. A *value definition* states whether
the values measure a quantity in a physical unit or name categories, how they behave when an
adapter regrids or aggregates them, and which quantity they are; each *dimension* of the item's
index space states what its axis means. Two components written by different people can then be
connected, and the connection checked, without either knowing the other.

This chapter describes the `core` module of the SDK, which implements those declarations: units
and their dimensions, quantities and qualities, the dimensions of an index space, and the
description, identity and signal classes that every SDK object is built on. It ends with what
@ref HydroCouple::SDK::validateConnection concludes when the two ends of a connection declare
different things.

The SDK never converts values. A unit states how to reach SI; converting from one unit to another
is the job of an adapted output (@ref sdk_guide_adapted_outputs) or of the host, and a connection
between two units that differ is reported as an error until one is in place.

## The classes in this chapter

| Class | Implements | Purpose |
| --- | --- | --- |
| @ref HydroCouple::SDK::Description | `HydroCouple::IDescription` | Caption, description and the property-change signal; the base of every SDK class that has a caption |
| @ref HydroCouple::SDK::Identity | `HydroCouple::IIdentity` | Adds an id |
| @ref HydroCouple::SDK::Dimension | `HydroCouple::IDimension` | One axis of an item's index space and what it means |
| @ref HydroCouple::SDK::UnitDimensions | `HydroCouple::IUnitDimensions` | The powers of the fundamental dimensions in a unit |
| @ref HydroCouple::SDK::Unit | `HydroCouple::IUnit` | A physical unit: dimensions, factor and offset to SI, symbols |
| @ref HydroCouple::SDK::ValueDefinition | `HydroCouple::IValueDefinition` | Value kind, quantity names, missing and default values |
| @ref HydroCouple::SDK::Quantity | `HydroCouple::IQuantity` | Values that are amounts of a unit, with bounds |
| @ref HydroCouple::SDK::Quality | `HydroCouple::IQuality` | Values that are indexes into a list of categories |
| @ref HydroCouple::SDK::Signal, @ref HydroCouple::SDK::FunctionSlot | `HydroCouple::ISignal`, `HydroCouple::ISlot` | Thread-safe notifications, and lambdas as slots |
| @ref HydroCouple::SDK::generateUUID | | Random ids |

The compiled example of this chapter is `docs/examples/ch06_units_and_quantities/main.cpp`. It
converts values between units, wires pairs of items whose declarations differ, and prints what the
validator finds for each.

## Descriptions and identities

@ref HydroCouple::SDK::Description holds a caption (a short title for people) and a description
(longer text). Neither identifies anything. Both setters fire the property-change signal with the
property's name, `"Caption"` or `"Description"`, even when the value does not change. Descriptions
are not copyable, because each owns its signal and its connections, and neither is any SDK class
derived from it: identities, dimensions, units, value definitions, data items, components and
workflows.

@ref HydroCouple::SDK::Identity adds an id. The standard requires an id to be unique only within
its context (the inputs of one component, the components of one composition) and never empty, and
the SDK enforces the second rule only: a constructor given an empty id stores a generated UUID
instead, and nothing checks uniqueness. The id is fixed for users of the class; a subclass can
replace it with the protected `setId()`, which ignores an empty value and fires `"Id"`.
`AbstractModelComponent::setInstanceId()` is the public way to rename a component
(@ref sdk_guide_component).

@ref HydroCouple::SDK::generateUUID returns a random version 4 UUID in lowercase hexadecimal,
36 characters long. Each thread has its own generator, seeded once from `std::random_device`, so
the function is thread-safe; the ids are unique in practice but not cryptographically
unpredictable.

## Dimensions: what each axis means

An item's values form an index space whose extent is its `shape()`: a 1-D item of 3 reaches has
shape `{3}`, a time series on 100 mesh faces over 24 steps has shape `{24, 100}`. Each axis is
described by a @ref HydroCouple::SDK::Dimension, which holds no length. The dimension says what the
axis means through its *role*, so that a generic consumer (a NetCDF writer, a partitioner, the
connection validator) can tell a time axis from an entity axis without knowing the item's class.

The SDK infers the role from the dimension's id when it is constructed, matching these ids exactly
and case-sensitively:

| Role | Meaning | Inferred from the ids |
| --- | --- | --- |
| `Time` | Time steps | `time`, `times` |
| `Entity` | Geometries, mesh nodes, edges, faces, network elements, identifiers | `identifiers`, `identifier`, `geometries`, `nodes`, `node`, `edges`, `edge`, `faces`, `face`, `volumes`, `patches`, `vertices`, `entities`, `element`, `elements` |
| `Layer` | Vertical layers, top first | `layers`, `layer` |
| `Band` | Raster bands | `band`, `bands` |
| `Row` | Raster rows, structured-grid y index | `y_cells`, `rows`, `row`, `y` |
| `Column` | Raster columns, structured-grid x index | `x_cells`, `columns`, `column`, `x` |
| `Depth` | Structured-grid z index | `z_cells`, `z` |
| `Component` | Vector or tensor components | `coordinate`, `component`, `components` |
| `Realization` | Ensemble members | none; set it with `setRole()` |
| `Other` | Declared, but none of the above | none |
| `Unknown` | Not declared | any other id |

`setRole()` changes the role and fires `"Role"`. The connection validator compares the roles of
two items axis by axis when they have the same rank, and skips an axis whose role is `Unknown` or
`Other` on either side.

A dimension's *length type* says whether the axis's extent is fixed (`Static`) or may change while
the model runs (`Dynamic`, the default). It is descriptive; no SDK code resizes or refuses a resize
because of it. `copy()` duplicates a dimension with its id, caption, description, length type and
role.

Items borrow their dimensions. A component keeps its `Dimension` objects as members and passes
pointers to the items it creates, so a dimension must outlive every item that points to it; one
dimension may be shared by many items.

@snippet ch06_units_and_quantities/main.cpp dimensions

## Units

### Fundamental dimensions

A unit's dimensions are the powers of the fundamental dimensions it is made of. A
@ref HydroCouple::SDK::UnitDimensions holds one power for each of them:

| Fundamental dimension | Symbol in messages | Example power |
| --- | --- | --- |
| `Length` | L | 3 for m³/s |
| `Mass` | M | 1 for kg/m³ |
| `Time` | T | -1 for m³/s |
| `ElectricCurrent` | I | |
| `Temperature` | Θ | 1 for °C |
| `AmountOfSubstance` | N | 1 for mol/L |
| `LuminousIntensity` | J | |
| `Currency` | $ | 1 for dollars per cubic meter |
| `Unitless` | | a named slot; nothing in the SDK reads it |
| `PlaneAngle` | rad | 1 for degrees of arc |

Every power starts at 0. `setPower(dimension, power)` accepts fractional powers and fires
`"Power"`. Plane angle is a basis of its own so that angles and geographic coordinates do not turn
silently into plain numbers. A unit whose powers are all zero is *dimensionless*.

### Factor and offset

A @ref HydroCouple::SDK::Unit states how to convert its values to the SI unit of the same
dimensions. With the factor \f$A\f$ (`conversionFactorToSI()`) and the offset \f$B\f$
(`offsetToSI()`):

\f[ x_{SI} = A\,x + B \f]

Feet have \f$A = 0.3048\f$, \f$B = 0\f$; degrees Celsius have \f$A = 1\f$, \f$B = 273.15\f$ (to kelvin).
Converting from a unit \f$u\f$ to a unit \f$v\f$ of the same dimensions goes through SI:

\f[ x_v = \frac{A_u\,x_u + B_u - B_v}{A_v} \f]

A unit also carries a UDUNITS-2 symbol (`symbol()`, for example `m3 s-1`, what a CF `units`
attribute holds) and a UCUM code (`ucumCode()`, for example `m3/s`); each is empty when not
declared, and neither is parsed or checked against the dimensions. No SDK writer reads them in this
version: a run manifest records a quantity's unit by its caption, factor, offset and powers
(@ref sdk_guide_record).

The factories return new units owned by the caller:

| Factory | Caption | Factor to SI | Powers | Symbol | UCUM |
| --- | --- | --- | --- | --- | --- |
| `lengthInMeters()` | metres | 1 | L | `m` | `m` |
| `lengthInFeet()` | feet | 0.3048 | L | `ft` | `[ft_i]` |
| `areaInSquareMeters()` | square metres | 1 | L² | `m2` | `m2` |
| `volumeInCubicMeters()` | cubic metres | 1 | L³ | `m3` | `m3` |
| `flowInCMS()` | m³/s | 1 | L³ T⁻¹ | `m3 s-1` | `m3/s` |
| `flowInCFS()` | ft³/s | 0.0283168 | L³ T⁻¹ | `ft3 s-1` | `[ft_i]3/s` |
| `timeInDays()` | days | 86400 | T | `d` | `d` |
| `timeInSeconds()` | seconds | 1 | T | `s` | `s` |
| `unitlessCoefficient()` | unitless | 1 | none | `1` | `1` |

The factor of `flowInCFS()` is rounded to 0.0283168; the exact value is 0.028316846592 (0.3048³).

For any other unit, construct one with a caption, factor and offset, and set its powers through
`dimensionsInternal()`:

@snippet ch06_units_and_quantities/main.cpp units

The example converts with the formula above:

@snippet ch06_units_and_quantities/main.cpp convert

```text
conversions through SI
  10 ft      = 3.0480 m
  20 degC    = 68.0000 degF
  10 ft3/s   = 0.283168 m3/s (factor 0.0283168)
  flow powers: L^3 T^-1, symbol "m3 s-1", UCUM "m3/s"
```

### Ownership of units

Three facts about ownership come from the code, and a component that creates many units must
respect them:

- The constructor that takes a caption allocates a `UnitDimensions`, and the unit's destructor
  does not free it, so every such unit leaks its dimensions object when it is destroyed. The
  amount is small (ten doubles and a signal) but it is per unit.
- The default constructor allocates no dimensions object: `dimensions()` returns `nullptr`, which
  the connection validator reports as a warning ("a unit declares no dimensions") and which the
  run recorder does not check before reading it. Construct units with a caption.
- `copy()` duplicates a unit's caption, factor, offset, symbol, UCUM code and dimensions, but not
  its description.

## Value definitions

### ValueDefinition

@ref HydroCouple::SDK::ValueDefinition is the base of quantities and qualities. It records:

| Property | Default | Meaning |
| --- | --- | --- |
| `type()` | given at construction | The C++ value type, as `std::type_info`. An SDK convenience: the element type the standard uses is the owning item's `dataKind()`. |
| `valueKind()` | `Unknown` | How values behave under regridding and aggregation (table below) |
| `standardName()` | empty | An entry of the CF Standard Name Table |
| `conceptURI()` | empty | An absolute URI naming the quantity in a published vocabulary (QUDT, CSDMS, SWEET, HY_Features) |
| `missingValue()` | -999999 | The value that marks missing data (numeric data kinds only) |
| `defaultValue()` | 0 | The value for new data (numeric data kinds only) |

Each setter fires the property's name (`"ValueKind"`, `"StandardName"`, `"ConceptURI"`,
`"MissingValue"`, `"DefaultValue"`). The missing and default values are descriptive: no SDK code
writes them into data, and the SDK's data items keep their own default element value.

The *value kind* tells an adapter how to move values between grids, and is the reason a
temperature and a mass cannot be resampled the same way:

| `ValueKind` | Meaning | Resampling |
| --- | --- | --- |
| `Unknown` | Not declared | An adapter must refuse or ask; the validator warns |
| `Intensive` | Independent of extent: temperature, concentration, elevation | Averages |
| `Extensive` | Proportional to extent: mass, volume, heat content | Sums |
| `Flux` | Per unit area and time | Integrates over the area it crosses |
| `Density` | Per unit volume | Integrates over the volume it fills |

Declare the kind on every value definition you create; the validator reports an undeclared kind
on either end of a connection.

`dataKindForType(type)` maps the C++ types the data plane supports to the standard's data kinds:
`double` to `Float64`, `float` to `Float32`, the signed and unsigned 8-, 16-, 32- and 64-bit
integers to `Int8` through `UInt64`, `bool` to `Boolean`, `std::string` to `String`, and anything
else to `Opaque`.

### Quantity

A @ref HydroCouple::SDK::Quantity describes values that are amounts of a unit, with a minimum
and a maximum (by default the lowest and highest finite doubles). The bounds are descriptive: no
SDK code checks values against them, and setting one does not check it against the other.

A quantity stores the `Unit *` it is given and never deletes it; its destructor does nothing. The
caller owns the unit and keeps it alive at least as long as the quantity. The factories
(`flowInCMS()`, `lengthInMeters()` and the others) and `copy()` allocate a new unit that nothing
deletes, so every quantity made that way leaks its unit, and the unit's dimensions, when it is
destroyed. To avoid the leak, own the unit and construct the quantity over it, as the example does:

@snippet ch06_units_and_quantities/main.cpp quantities

The quantity factories exist for brevity in tests and examples:

| Factory | Caption | Unit | Concept URI (QUDT quantity kind) |
| --- | --- | --- | --- |
| `flowInCMS()` | Flow (m³/s) | `Unit::flowInCMS()` | `http://qudt.org/vocab/quantitykind/VolumeFlowRate` |
| `flowInCFS()` | Flow (ft³/s) | `Unit::flowInCFS()` | `.../VolumeFlowRate` |
| `lengthInMeters()` | Length (m) | `Unit::lengthInMeters()` | `.../Length` |
| `lengthInFeet()` | Length (ft) | `Unit::lengthInFeet()` | `.../Length` |
| `areaInSquareMeters()` | Area (m²) | `Unit::areaInSquareMeters()` | `.../Area` |
| `volumeInCubicMeters()` | Volume (m³) | `Unit::volumeInCubicMeters()` | `.../Volume` |
| `timeInDays()` | Time (days) | `Unit::timeInDays()` | `.../Time` |
| `timeInSeconds()` | Time (s) | `Unit::timeInSeconds()` | `.../Time` |
| `unitLess(caption)` | the caption | `Unit::unitlessCoefficient()` | none |

The factories declare no CF standard name and leave the value kind `Unknown`. A QUDT quantity kind
is what such a generic quantity knows; a component that means something narrower (channel
discharge rather than any volume flow rate) sets its own concept URI and standard name.

### Quality

A @ref HydroCouple::SDK::Quality describes categorical values: land use, a soil class, a rating.
Its `categories()` are labels, and a value \f$k\f$ stored in the data item means `categories()[k]`,
so the values are integers; the quality's type is `int32_t` and an item that uses one should hold
`Int32` data. `isOrdered()` says whether the categories have an order (ordinal) or not (nominal);
the validator does not compare it. `setCategories()` and `addCategory()` fire `"Categories"` and do
not remap values already stored; duplicate labels are not rejected. The value kind starts
`Unknown`, as for every value definition; the standard expects a quality to declare `Intensive`,
because categories do not sum.

### Copies

| Function | Result |
| --- | --- |
| `ValueDefinition::copy(src)` | Always a plain `ValueDefinition`, even when `src` is a quantity or a quality: copies the caption, description, missing and default values, value kind and both names, and drops the unit, bounds and categories |
| `Quantity::copy(src)` | A quantity with a copy of the unit (which nothing deletes), the bounds, and everything `ValueDefinition::copy()` copies |
| `Quality::copy(src)` | A quality with the categories and the order flag, and everything `ValueDefinition::copy()` copies |

Each returns a new object owned by the caller, or `nullptr` for a null source.

## How a connection compares two declarations

@ref HydroCouple::SDK::validateConnection reads the declarations of a provider and a consumer and
returns a list of @ref HydroCouple::SDK::ConnectionFinding records. A finding is an `Error` when
both ends declare and the declarations contradict each other, so the coupled model would compute
wrong numbers without any error; it is a `Warning` when a declaration is missing, so the check could
not be made. @ref HydroCouple::SDK::hasConnectionError tells whether any finding is an error.
The workflow runs the function over every connection in `validate()`; @ref sdk_guide_workflow
describes how the findings reach the error queue, and the temporal and spatial checks are described
in @ref sdk_guide_time and @ref sdk_guide_space. The checks on value definitions run in this
order:

1. **Element type.** Different `dataKind()` values are an error (1101): the data plane converts
   no element types.
2. **Presence.** When either end has no value definition, a warning (1102) ends the value checks.
   An adapted output without a value definition is not checked at all: it owns its value semantics.
3. **Value kind.** Two declared kinds that differ are an error (1103); a kind undeclared on one or
   both ends is a warning.
4. **Quantity name.** When both ends declare a concept URI, the URIs must be equal; otherwise,
   when both declare a CF standard name, the names must be equal. A difference is only a warning
   (1113), because one model's outflow is legitimately another's inflow. Captions never decide it.
5. **Quantity or quality.** A quantity on one end and a quality on the other is an error (1102).
   Two qualities must have equal category lists, label by label and in order (1102); nothing more is
   compared for them.
6. **Units.** For two quantities: a missing unit, or a unit without dimensions, is a warning
   (1104). Then the powers are compared on every fundamental dimension except `Unitless`, with an
   absolute tolerance of \f$10^{-12}\f$. Different powers are an error, unless one unit is
   dimensionless, which is only a warning because a dimensionless unit is also what an undeclared
   placeholder looks like. Equal powers with a different factor or offset are an error; factors and
   offsets \f$a\f$ and \f$b\f$ count as equal when
   \f[ |a - b| \le 10^{-12} \max(1, |a|, |b|) \f]

@snippet ch06_units_and_quantities/main.cpp compare

```text
connection findings
m3/s -> m3/s
  no findings
ft3/s -> m3/s
  Error 1104: link 'out' -> 'in': units share a dimension but not a scale ('ft³/s' vs 'm³/s'); wire a unit-conversion adapter between them
m3/s -> m
  Error 1104: link 'out' -> 'in': units differ in dimension (L^3 T^-1 vs L^1)
unitless -> m3/s
  Warning 1104: link 'out' -> 'in': units differ in dimension (dimensionless vs L^3 T^-1); one side may simply not have declared its unit
Intensive -> Extensive
  Error 1103: link 'out' -> 'in': provider values are Intensive, consumer expects Extensive
Unknown kind -> Intensive
  Warning 1103: link 'out' -> 'in': provider declares no ValueKind (the other says Intensive); an adapter could not choose between averaging and summing
other concept URI
  Warning 1113: link 'out' -> 'in': provider measures <http://qudt.org/vocab/quantitykind/Rainfall>, consumer expects <http://qudt.org/vocab/quantitykind/VolumeFlowRate>
no value definition
  Warning 1102: link 'out' -> 'in': provider declares no value definition; units and value kind cannot be checked
quality -> quantity
  Error 1102: link 'out' -> 'in': provider is a quality, consumer a quantity
categories reordered
  Error 1102: link 'out' -> 'in': the qualities' category lists differ, so category index k names different things on the two ends
Int32 -> Float64
  Error 1101: link 'out' -> 'in': element types differ (the data plane converts nothing)
axis time -> nodes
  Error 1112: link 'out' -> 'in': axis 0 is Time on the provider and Entity on the consumer
```

Every message starts with `link 'P' -> 'C': `, naming the provider and consumer ids. Only the
consumer's immediate provider is compared. When an adapted output sits between two components, the
adapter is what is compared, which is correct: it exists to change units, grids or time bases. A
ft³/s output feeding an m³/s input is fixed by inserting a linear-transform adapted output with
multiplier 0.0283168 (@ref sdk_guide_adapted_outputs). That adapter declares no value definition, so
the validator skips its value checks: whether the multiplier is right is the responsibility of whoever
configures it.

The validator only reads declarations. It does not need the items to be connected, reads no values
and changes nothing, so a host can call it when a user draws a connection.

## Persistence

`toJson()` and `fromJson()` store a value definition as a JSON object, which components use to save
their configuration:

| Key | Written by | Read back by `fromJson()` |
| --- | --- | --- |
| `caption`, `description` | all | yes, through the setters (signals fire) |
| `missingValue`, `defaultValue` | all | yes, without a signal |
| `valueKind` | all, as the enumerator's integer | yes, without a signal |
| `standardName`, `conceptURI` | all, only when declared | yes, without a signal |
| `categories`, `isOrdered` | `Quality` | yes, without a signal |
| `minValue`, `maxValue` | `Quantity` | yes, without a signal |
| `unit` (`caption`, `conversionFactorToSI`, `offsetToSI`, and `symbol`, `ucumCode` when declared) | `Quantity` with a unit | no |

The C++ type and the unit's dimensions are not written, and `Quantity::fromJson()` keeps the unit the
quantity was constructed with. A present key of the wrong JSON type throws
`nlohmann::json::type_error`.

@snippet ch06_units_and_quantities/main.cpp json

```text
JSON
  {"caption":"Discharge","conceptURI":"http://qudt.org/vocab/quantitykind/VolumeFlowRate","defaultValue":0.0,"description":"","maxValue":1.7976931348623157e+308,"minValue":0.0,"missingValue":-999999.0,"unit":{"caption":"m³/s","conversionFactorToSI":1.0,"offsetToSI":0.0,"symbol":"m3 s-1","ucumCode":"m3/s"},"valueKind":1}
  restored: caption "Discharge", min 0, unit still "metres"
```

## Signals and slots

The standard's notifications are signals and slots: a `HydroCouple::ISignal` calls every connected
`HydroCouple::ISlot`. @ref HydroCouple::SDK::Signal is the SDK's implementation, used for the
property-change signal of every SDK object and for the status signals of components and workflows.

| Behavior | Detail |
| --- | --- |
| Ownership | The signal keeps a `std::weak_ptr` to each slot. The caller owns the slot and keeps its `shared_ptr` alive; a slot whose owner released it is dropped at the next emission or disconnection. |
| Connecting | `connect()` ignores a null slot and a slot already connected. `disconnect()` ignores a slot that is not connected. |
| Emission | Slots run synchronously on the emitting thread, in connection order. The signal copies the list of live slots under its lock and calls them outside it, so a slot may connect or disconnect slots; a slot connected during an emission is first called by the next one. |
| Exceptions | A slot that throws stops the emission; the exception reaches the code that changed the property. |
| Blocking | `blockSignals(true)` makes emissions do nothing. Nothing is queued; changes made while blocked are applied but never announced. |
| Threads | Every method locks a `std::shared_mutex`, so connecting, disconnecting and emitting from several threads is safe. The objects that own signals are not otherwise thread-safe. |
| Sender | The `sender` a slot receives is the signal object itself, which for an SDK object is a private member; capture the object you connected to if the slot needs it. |

@ref HydroCouple::SDK::FunctionSlot wraps a lambda or any other callable as a slot. Every setter in
the `core` and `component` modules fires the property-change signal with the property's name:

| Class | Property names |
| --- | --- |
| `Description` | `Caption`, `Description` |
| `Identity` | `Id` |
| `Dimension` | `Role`, `LengthType` |
| `UnitDimensions` | `Power` |
| `Unit` | `ConversionFactorToSI`, `OffsetToSI`, `Symbol`, `UcumCode` |
| `ValueDefinition` | `ValueKind`, `StandardName`, `ConceptURI`, `MissingValue`, `DefaultValue` |
| `Quality` | `Categories`, `IsOrdered` |
| `Quantity` | `MinValue`, `MaxValue` |
| `ComponentInfo` | `LibraryFilePath`, `License` (its other setters fire nothing) |
| `AbstractModelComponent` | `Status`, `Inputs`, `Outputs`, `Arguments`, `Workflow`, `ReferenceDirectory`, `MpiAllocatedProcesses` |

@snippet ch06_units_and_quantities/main.cpp signal

```text
property changes
  changed: StandardName
  changed: MaxValue
```

## Running the example

```text
cmake --build build --target docs_example_ch06_units_and_quantities
ctest --test-dir build -R docs.example.ch06_units_and_quantities --output-on-failure
```

The program checks each conversion, each finding's code and severity, the JSON round trip and the
notifications, and returns nonzero if any is wrong. The part of its output not shown above (the UUID
differs on every run):

```text
dimensions and identities
  role of "faces": Entity; role of "depth_bins": Unknown
  an empty id became "53744eaa-c06b-44b7-bb49-f820acba02a3" (36 characters)

all checks passed
```

## Thread safety

None of the classes in this chapter lock their properties: set captions, units, kinds and roles
from one thread, normally while the component initializes, and read them from others afterwards.
The signals are the exception; see the table above. `generateUUID()` is thread-safe.

## Diagnostics

The value-definition checks of `validateConnection()`, with their fixes:

| Code | Finding | Fix |
| --- | --- | --- |
| 1101 Error | `element types differ (the data plane converts nothing)` | Use items of the same data kind on both ends |
| 1102 Warning | `provider declares no value definition; units and value kind cannot be checked` (or `consumer`) | Give the item a quantity or quality |
| 1102 Error | `provider is a quality, consumer a quantity` (or the reverse) | Connect categorical values only to categorical values |
| 1102 Error | `the qualities' category lists differ, so category index k names different things on the two ends` | Use one category list, in one order, or a mapping adapter |
| 1103 Error | `provider values are A, consumer expects B` | Correct the kind on one end, or adapt between them |
| 1103 Warning | `provider declares no ValueKind (the other says K); ...` or `neither end declares a ValueKind` | Call `setValueKind()` on the value definitions |
| 1104 Error | `units differ in dimension (P vs Q)` | Connect matching quantities |
| 1104 Error | `units share a dimension but not a scale ('U' vs 'V'); wire a unit-conversion adapter between them` | Insert a linear-transform adapted output (@ref sdk_guide_adapted_outputs) |
| 1104 Warning | `units differ in dimension (dimensionless vs Q); one side may simply not have declared its unit` | Give the placeholder its real unit |
| 1104 Warning | `provider quantity declares no unit` (or `consumer`), `a unit declares no dimensions` | Construct the quantity with a unit, and the unit with a caption |
| 1112 Error | `axis N is R on the provider and S on the consumer` | Use matching axis ids, or set the roles with `setRole()` |
| 1113 Warning | `provider measures URI, consumer expects URI` or `provider measures 'NAME', consumer expects 'NAME'` | Nothing, when the difference is intended (outflow to inflow); otherwise correct the names |

@ref sdk_ref_diagnostics lists every code the SDK uses.
