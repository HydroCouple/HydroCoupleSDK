@page sdk_guide_time 9. Time

[TOC]

A coupled run exchanges values that belong to instants: a flow at 06:00, a mean over the hour that
ended at 06:00, a total over a day. This chapter describes how the SDK represents those instants and
intervals, how it stores and interpolates series of values over time, and the items that exchange
time series between components: the `temporal` module and the time-slice output of the `data`
module. The interfaces they implement are in `hydrocoupletemporal.h` of the
[HydroCouple standard](https://www.hydrocouple.org/HydroCouple/). Adapting a time series to a
consumer's time (temporal interpolation) is in @ref sdk_guide_adapted_outputs; how a workflow
advances components through time is in @ref sdk_guide_workflow.

## How time is represented

Every time coordinate in the SDK is a `double` Julian day:

- **Unit and epoch.** Days since 4713 BC January 1, 12:00 UTC (the astronomical Julian day), in the
  proleptic Gregorian calendar, the standard's convention. 2000-01-01 12:00 UTC is 2451545.0, and
  midnight starts at .5: 2026-05-01 00:00 UTC is 2461161.5.
- **Time zone.** UTC, with no daylight saving and no leap seconds. Nothing in the SDK converts time
  zones; a component that reads local times converts them before they reach an item.
- **Durations** are in days as well: one hour is 1/24, one second 1/86400.
- **Resolution.** A double resolves a present-day Julian day to about 40 µs. Instants computed on two
  sides of a coupling rarely agree to the last bit, so the standard compares instants with
  `HydroCouple::Temporal::TIME_EQUALITY_TOLERANCE_DAYS`, 10⁻⁸ days (about 0.86 ms). The SDK's own
  series lookups apply no tolerance: they compare exactly.

Two derived calendars are offered for interoperability: the modified Julian day,
\f$\mathrm{MJD} = \mathrm{JD} - 2400000.5\f$ (days since 1858-11-17 00:00 UTC), and the MATLAB serial
date number, \f$\mathrm{datenum} = \mathrm{JD} - 1721058.5\f$. A persistence layer writing CF metadata
uses units of "days since ..." with `calendar = "standard"`.

## The classes in this chapter

| Class | Implements | Purpose |
| --- | --- | --- |
| @ref HydroCouple::SDK::Temporal::TimeData | `IDateTime` | An instant as a Julian day; calendar conversions |
| @ref HydroCouple::SDK::Temporal::TimeSpan | `ITimeSpan` | A start and a duration |
| @ref HydroCouple::SDK::Temporal::TimeSeries | (storage) | (time, value) pairs with linear, nearest and step interpolation, and CSV input and output |
| @ref HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem | (storage) | Time-major storage `{ time, element }` behind the time-series items |
| @ref HydroCouple::SDK::Temporal::TimeSeriesOutput, @ref HydroCouple::SDK::Temporal::TimeSeriesInput | `IOutput`/`IInput`, `ITimeSeriesComponentDataItem` | Time-series exchange items |
| @ref HydroCouple::SDK::Temporal::TimeIdBasedComponentDataItem | `ITimeIdBasedComponentDataItem` | Values per identifier per time step |
| @ref HydroCouple::SDK::Temporal::TimeSeriesArgumentDouble | `IArgument`, `ITimeSeriesComponentDataItem` | A scalar time series read from JSON or CSV |
| @ref HydroCouple::SDK::TimeSliceOutput | `IOutput` | One time level of a time-major item |
| @ref HydroCouple::SDK::Temporal::TemporalInterpolationAdaptedOutput, @ref HydroCouple::SDK::Temporal::TimeSeriesAdaptedOutput | `IAdaptedOutput` | Values between the instants a provider holds (@ref sdk_guide_adapted_outputs) |

Two compiled examples go with this chapter: `docs/examples/ch09_time_series/main.cpp` (instants,
spans, series, the time-series argument) and `docs/examples/ch09_time_series_items/main.cpp`
(time-series exchange items, a time-and-identifier record and a time slice). Both run as ctest tests
(`docs.example.ch09_time_series` and `docs.example.ch09_time_series_items`). The temporal
interpolation adapter is shown in `docs/examples/ch08_adapter_chain`.

## Instants: TimeData

@ref HydroCouple::SDK::Temporal::TimeData is an instant. It is built from a Julian day or from
calendar fields, and answers `julianDay()`, `modifiedJulianDay()` and `serialDate()`. It is also an
Identity (it has an id and a caption), and its setters (`setJulianDay()`, `setModifiedJulianDay()`,
`setSerialDate()`) fire `propertyChanged("JulianDay")`. Its class comment calls it immutable and safe
to share across threads; that holds only while nobody calls a setter, which is not synchronized.

Two static functions convert between the calendar and Julian days:

- `gregorianToJulianDay(year, month, day, hour, minute, second)` uses the Fliegel and Van Flandern
  integer algorithm for the day number and adds the time of day. Years use astronomical numbering (0
  is 1 BC). The fields are not range-checked and roll over arithmetically: hour 25 is 01:00 the next
  day.
- `julianDayToGregorian(jd, year, month, day, hour, minute, second)` is the inverse (Richards'
  algorithm). Hour and minute are truncated and the seconds carry the floating-point residue, so
  06:30 comes back as 06:30:00.000013; round the seconds when printing.

@snippet ch09_time_series/main.cpp julian

`TimeData::julianDayNow()` returns the current UTC instant, and the one-argument constructor uses it
when no Julian day is given. Its whole seconds are correct. The fraction of a second it adds is the
clock's tick count modulo 10⁹ divided by 10⁹, which is the sub-second part only on a clock that
counts nanoseconds; with the microsecond clock of libc++ (macOS) or the 100 ns clock of MSVC it is an
unrelated number below one second. Do not rely on its sub-second part.

## Intervals: TimeSpan

@ref HydroCouple::SDK::Temporal::TimeSpan is a start and a duration in days. It derives from
`TimeData`, as the standard's `ITimeSpan` derives from `IDateTime`: the inherited instant is the
start, so a span can be handed to anything that wants an instant and reads as "when this begins".
`endJulianDay()` (and its alias `endTime()`) is start plus duration. `TimeSpan::fromTimeRange(id,
start, end)` builds one from two instants. The standard requires a non-negative duration; the class
does not check, so a span with its end before its start has a negative duration. `setDuration()`
fires `propertyChanged("Duration")`.

@snippet ch09_time_series/main.cpp span

```text
2000-01-01 12:00 UTC: JD 2451545.0, MJD 51544.5, MATLAB datenum 730486.5
2026-05-01 06:30 is JD 2461161.77083333; back: 2026-05-01 06:30:0.000013 s
storm: starts JD 2461161.7708, lasts 0.75 days (18 h), ends JD 2461162.5208
```

## Series of values: TimeSeries

@ref HydroCouple::SDK::Temporal::TimeSeries stores samples \f$(t_k, v_k)\f$, \f$k = 0, \dots, n-1\f$,
with strictly increasing times, and returns a value at any time with one of three methods. It is the
storage behind the time-series argument and the temporal interpolation adapter, and implements no
HydroCouple interface itself.

`addValue(t, v)` appends a sample and throws `std::invalid_argument` ("TimeSeries: times must be
strictly increasing") when \f$t\f$ is not after the last time; the series is then unchanged. The
constructor that takes two vectors does not check either rule: unordered times give wrong lookups,
and fewer values than times makes `getValue()` read past the end of the values. Build a series from
data you do not control with `addValue()`.

### Interpolation

`getValue(t)` finds the samples around \f$t\f$ by binary search and applies the series's method:

| Query | Linear | Nearest | Constant |
| --- | --- | --- | --- |
| \f$t < t_0\f$, or an empty series | NaN | NaN | NaN |
| \f$t = t_0\f$ | \f$v_0\f$ | \f$v_0\f$ | \f$v_0\f$ |
| \f$t_{k-1} < t < t_k\f$ | \f$v_{k-1} + \dfrac{t - t_{k-1}}{t_k - t_{k-1}}\,(v_k - v_{k-1})\f$ | the nearer of \f$v_{k-1}\f$, \f$v_k\f$; \f$v_{k-1}\f$ on a tie | \f$v_{k-1}\f$ |
| \f$t = t_k\f$, \f$k \ge 1\f$ | \f$v_k\f$ | \f$v_k\f$ | \f$v_{k-1}\f$ |
| \f$t > t_{n-1}\f$ | NaN | \f$v_{n-1}\f$ | \f$v_{n-1}\f$ |

Linear is defined only inside the record. Nearest and Constant hold the last value after the end,
but not before the start. Constant is a step function that is continuous from the left: at exactly a
sample time after the first it still returns the value of the sample before, and the last value
appears only after the last time. A consumer that wants "the value of the most recent sample at or
before \f$t\f$" must query just after the sample time, or use Nearest when samples are regular.
`getValues(times)` applies `getValue()` to each time in order. The interpolation method can be changed
at any time with `setInterpolationMethod()`.

@snippet ch09_time_series/main.cpp methods

```text
t          -0.5   0.0   0.5   1.0   1.5   2.0   2.5
Linear      nan    10    15    20    25    30   nan
Nearest     nan    10    10    20    20    30    30
Constant    nan    10    10    10    20    20    30
```

### CSV text and files

`loadFromCSVText(text, skipHeader)` replaces the series with the rows of a CSV text: each line is
split at its first comma and both fields are read with `std::stod`, so the time is a plain number (a
Julian day, not a date string) and leading spaces are allowed. A line with no comma is skipped. A
field that does not parse, or a time that is not after the previous one, stops the load: it returns
false and keeps the rows read before it. `loadFromCSV(path, skipHeader)` reads a file and parses it
the same way; a file that cannot be opened returns false and leaves the series unchanged.

@snippet ch09_time_series/main.cpp csv_text

```text
CSV text: 3 samples from JD 2461161.50 to 2461162.00; at 06:00 4.2 mm/h
times out of order: load false, 1 sample kept
```

`saveToCSV(path)` writes a `Time,Value` header and one row per sample with the stream's default
precision of six significant digits. A Julian day such as 2460000.5 is written as `2.46e+06`: the
times of a series of real dates collapse, and reading the file back fails (the second row is not
after the first). Only series with small time coordinates survive the round trip; write Julian-day
series as JSON (`TimeSeriesArgumentDouble::toString()`) instead.

## What a timestamp means

A time series carries times and values but not whether a value is a reading at that instant, a mean
over the interval that ended there, or an accumulation; coupling a model that reports means to one
that expects instants produces plausible numbers and no error. Every `ITimeSeriesComponentDataItem`
therefore declares four things, and @ref HydroCouple::SDK::validateConnection compares them across a
link when both ends are time series:

| Declaration | Values | Meaning |
| --- | --- | --- |
| `timeKind()` | `Unknown`, `Instantaneous`, `IntervalMean`, `IntervalMinimum`, `IntervalMaximum`, `Accumulated` | What a value's time coordinate refers to; an interval statistic covers the interval ending at it |
| `intervalLength()` | days; negative (`INTERVAL_LENGTH_CONTIGUOUS`) for contiguous intervals of varying length; 0 for not declared | Length of the interval, for an interval kind |
| `timeInterpolation()` | `Unknown`, `None`, `Previous`, `Nearest`, `Linear` | On an output: what it does between its instants. On an input: the least it accepts |
| `timeExtrapolation()` | `Unknown`, `Refuse`, `HoldLast`, `Linear` | The same, outside the held range |

What the validator concludes (codes and messages in @ref sdk_guide_workflow):

| Check | Finding |
| --- | --- |
| Either end's time kind is Unknown | warning |
| Both declare kinds and they differ | error |
| Same interval kind, both lengths positive and different | error |
| Both interpolation policies Unknown | error |
| One interpolation policy Unknown | warning |
| Provider interpolates (not None) differently from what the consumer accepts | error; a provider that declares None is acceptable to any consumer |
| Extrapolation: the same three rules, with Refuse in the role of None | as above |

The SDK's classes start from these declarations:

| Class | Time kind | Interpolation | Extrapolation |
| --- | --- | --- | --- |
| `TimeSeriesComponentDataItem` (and so `TimeIdBasedComponentDataItem`) | Unknown | Unknown | Unknown |
| `TimeSeriesOutput`, `TimeSeriesInput` | Unknown | None | Refuse |
| `TimeSeriesArgumentDouble` | Unknown | Linear | HoldLast |

`setTimeKind(kind, intervalLengthDays)`, `setTimeInterpolation()` and `setTimeExtrapolation()`
change them. They are metadata: no SDK storage interpolates because of them. The output and input
declare None and Refuse because that is what they do: they serve held instants by index and never
make a value up. `TimeSeriesArgumentDouble` declares HoldLast but returns NaN after its last time
(below); declare what your component actually does.

## Time-series exchange items

@ref HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem is the storage mixin: one row of
`elementSize()` values per time step, time-major, so the shape is `{ timeCount(), elementSize() }` and
"time step *t*, every element" is one contiguous row. It keeps a shared `IDateTime` per step and a
contiguous cache of their Julian days behind `times()`.

- `addTimeStep(time, values)` appends a row. `values` must have `elementSize()` entries
  (`std::invalid_argument` otherwise); `time` must not be null; the order of times is not checked. It
  fires no signal.
- Each call replaces the object `timeSpan()` returns, so a pointer from an earlier `timeSpan()` no
  longer refers to the item's span (and dangles if nothing else holds it). Call `timeSpan()` again
  instead of keeping the pointer. The `times()` span and `rawData()` references are also invalidated
  when a row is added.
- `getValueAt(t, e)` and `setValueAt(t, e, v)` reach one value directly; out-of-range indices return
  `T{}` and are ignored, without an error. `clearTimeSteps()` empties the record and keeps the
  declarations.

@ref HydroCouple::SDK::Temporal::TimeSeriesOutput is what a component records into. Give its
constructor the time dimension first (with the id `"time"`, so its role is Time) and the element
count; append a row per step in `update()`. Its `updateValues()` does nothing, because the component
has already appended.

@snippet ch09_time_series_items/main.cpp output

@ref HydroCouple::SDK::Temporal::TimeSeriesInput accepts a time-series provider of the same kind and
element count. Its `retrieveValuesFromProvider()` appends the provider's *last* row, with the
provider's time, as a new row of its own, and fires value-changed for it. It records history: every
retrieve appends, whether or not the provider has a new step, so retrieving twice at one provider
time stores that time twice. A component that wants only the present reads the input's last row, or
clears it with `clearTimeSteps()` between steps.

@snippet ch09_time_series_items/main.cpp input

```text
discharge: shape {3, 2}, 3 times, span 0.0833 days, interpolation None yes, extrapolation Refuse yes
row at 02:00: 3 12
input recorded 2 steps: 0.0833 -> 3 12, 0.1250 -> 4 13
```

The SDK factory offers temporal interpolation on a time-series output of one column
(`{ times, 1 }`, Float64), which serves the value at a query time between the output's instants
(@ref sdk_guide_adapted_outputs).

## Values per identifier over time: TimeIdBasedComponentDataItem

@ref HydroCouple::SDK::Temporal::TimeIdBasedComponentDataItem implements
`ITimeIdBasedComponentDataItem`: a record of one value per identifier per time step, shaped
`{ time steps, identifier count }`. The identifiers are fixed at construction and their count is the
element size, so each `addTimeStep()` takes one value per identifier. It owns its two dimensions,
`"time"` (role Time) and `"identifiers"` (role Entity). It is a plain data item, neither input nor
output, for records that other code reads through the data plane: a state item, or a store behind a
time slice. Its temporal declarations start Unknown.

@snippet ch09_time_series_items/main.cpp time_id

## The present of a record: TimeSliceOutput

@ref HydroCouple::SDK::TimeSliceOutput (@ref sdk_guide_ports) presents one time level of a time-major
item as an output one rank lower. `setTime(t)` selects the level whose time is nearest \f$t\f$ (the
earlier on a tie, with no tolerance and no range check); `setTimeIndex()` clamps to the record;
`advance()` moves one level and returns false at the last. Choosing the level is the host's or the
workflow's job: the SDK has no global clock.

@snippet ch09_time_series_items/main.cpp slice

```text
gauges: shape {3, 3}; dimension 0 'time' (Time yes), dimension 1 'identifiers' (Entity yes)
slice: shape {3}, time index 1 of 3, values 1.5 0.9 2.4
setTimeIndex(99) -> 2; advance() at the end: false
slice value definition: 'stage_now_slice'
```

The slice's value definition is a new unitless quantity, not the source's.

## A time series as an argument: TimeSeriesArgumentDouble

@ref HydroCouple::SDK::Temporal::TimeSeriesArgumentDouble is a forcing read from configuration: a
boundary inflow, a rainfall record. It holds a `TimeSeries` of doubles and is both an argument and an
`ITimeSeriesComponentDataItem`, so a configurator that casts it finds a time series rather than a
column of numbers. Through the data plane it is a rank-1 Float64 item of the values, shape
`{ times }`; it is read-only there.

| Input type | `value` holds |
| --- | --- |
| String, JSON (and the other text types) | `{"times": [...], "values": [...]}`, optionally with `"timeKind"`, `"intervalLength"`, `"timeInterpolation"` and `"timeExtrapolation"` as the integers of their enumerations |
| File or URL ending in `.csv` (a query string after the name is ignored) | CSV text with a header line and `time,value` rows, times as Julian days |
| File or URL otherwise | the JSON object above |

A load replaces the series. A CSV load keeps the previous declarations; a JSON load sets the
declarations it names. A load that fails part way (a decreasing time, say) leaves the samples read
before the failure. The `IDateTime` objects `time(i)` returns are built on demand and cached; each is
reused only while its Julian day still equals the series's, so a reload never hands out stale dates.
`timeSpan()` is a zero span (start 0, duration 0) for an empty series.

`getInterpolatedValue(t)` is the series's linear interpolation: NaN before the first and after the
last time. The declared policies (Linear, HoldLast by default) do not change it, and HoldLast does
not describe it. `toString()` writes the times and values; `writeData()` adds the four declarations.
`saveData()` writes back to a local file it was read from, as CSV for a `.csv` path (with the
six-digit precision problem of `saveToCSV()`) or as JSON otherwise.

The argument adds no file filter, so `isValidArgType()` reports File and URL as unsupported although
`initialize()` reads both. `initialize(item)` copies the times and values of another Float64 time
series of shape `{ n }` or `{ n, 1 }`; pass only those shapes, because a source of rank 3 or more
passes the shape check and is copied into a buffer sized for its first dimension.

@snippet ch09_time_series/main.cpp argument_json

@snippet ch09_time_series/main.cpp argument_csv

@snippet ch09_time_series/main.cpp argument_errors

```text
argument: 3 times, span 0.50 days, Instantaneous yes, value at 03:00 2.25, after the end nan
from inflow.csv: 4 times; value at 12:00 4; isValidArgType(File) false
declared: interpolation Linear yes, extrapolation HoldLast yes
unequal arrays: times/values must have equal length
decreasing times: TimeSeries: times must be strictly increasing
```

The CSV file the example reads, `docs/examples/ch09_time_series/inflow.csv`:

@include ch09_time_series/inflow.csv

## Diagnostics

| Message | Produced by | Cause | Fix |
| --- | --- | --- | --- |
| `TimeSeries: times must be strictly increasing` (`std::invalid_argument`, or the message of a failed `initialize()`) | `TimeSeries::addValue()`, the time-series argument, temporal interpolation | A time at or before the previous one | Sort the samples; remove duplicates |
| `TimeSeriesComponentDataItem: values size must match elementSize` (`std::invalid_argument`) | `addTimeStep()` | A row with the wrong number of values | Pass `elementSize()` values |
| `Expected JSON object with "times" and "values" arrays` | `TimeSeriesArgumentDouble::initialize()` | Missing keys | Use the documented object |
| `times/values must have equal length` | `TimeSeriesArgumentDouble::initialize()` | Arrays of different lengths | Fix the document |
| `Failed to load CSV file: REFERENCE` | `TimeSeriesArgumentDouble::initialize()`, File or URL | A CSV field did not parse or the times decrease | Check the file; times must be numbers |
| `Source data item is not a time-series data item`, `Source data kind must be Float64`, `Source must be a scalar time series` | `TimeSeriesArgumentDouble::initialize(item)` | The source does not fit | Bind a `{ n, 1 }` Float64 time series |
| `TimeSeriesArgumentDouble is read-only; load values via initialize()` | `setValuesFrom()` | Writing through the data plane | Use `initialize()` |
| `Provider is not a time-series data item`, `Provider element count mismatch` | `TimeSeriesInput::canConsume()` | The provider is not a time series of the input's width | Match `elementSize()`, or use a 1-D input with a slice |

A NaN from a series queried outside its record is not reported as an error; check the query range
against `times()`. The connection findings about time kinds and policies are listed in
@ref sdk_guide_workflow, and all messages are collected in @ref sdk_ref_diagnostics.
