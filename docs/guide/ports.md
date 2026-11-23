@page sdk_guide_ports 3. Connect ports and exchange values

A provider exposes an `IOutput` and a consumer accepts it through an `IInput`. In the serial example the catchment publishes a vector with one value per reach. The channel consumes that vector as lateral inflow. The connection entry in the composition uses the two instance IDs and their port IDs.

```json
{
  "from": { "component": "catchment", "output": "runoff" },
  "to":   { "component": "channel", "input": "lateral_inflow" }
}
```

Use `validateConnection(output, input)` to inspect the proposed link and `hasConnectionError(findings)` to detect a blocking finding. Check the physical units, dimension order, geometry and time support for the quantities being coupled. When the models need a transformation, supply an adapted output that implements it. [Connection validation API](https://github.com/HydroCouple/HydroCoupleSDK/blob/removing_qt/include/hydrocouplesdk/component/connectionvalidator.h)

For bulk reads, query the exchange item’s `shape()` and data kind, allocate storage of the matching type, and describe that storage with a `BufferDescriptor`. Pass the destination, start indices and counts to `getValuesInto()`. The start and count describe the selection in the item’s dimensions. The descriptor describes the memory receiving those values. Check the returned status and diagnostic before using the buffer. [Worked Float64 buffer read](https://github.com/HydroCouple/HydroCoupleSDK/blob/removing_qt/examples/analysis_reopen/main.cpp)
