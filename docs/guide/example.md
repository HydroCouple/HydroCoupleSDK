@page sdk_guide_example 5. Run the catchment and channel

From the SDK source root, configure a single-configuration build with the interfaces and dependencies available to CMake. The example uses synthetic, unitless values at three reaches. It writes both model outputs at each of 24 updates.

```shell
cmake -S . -B build-reference -DBUILD_EXAMPLES=ON -DBUILD_TESTS=OFF
cmake --build build-reference --target serial_coupling analysis_reopen
./build-reference/examples/serial_coupling examples/serial_coupling/composition.json demo-run
./build-reference/examples/analysis_reopen demo-run/run.json
```

The schedule should list `catchment` followed by `channel`. The reopened catalog should contain two Float64 arrays, each with 24 rows of time and three columns of reaches. Across all entries, the maximum runoff is **6.6000** and the maximum channel flow is **6.5309**. If you see different values, check the coefficients and routing weights against the supplied input. [Example input](https://github.com/HydroCouple/HydroCoupleSDK/blob/removing_qt/examples/serial_coupling/composition.json)

[Download the recorded example (ZIP)](serial-coupling-run.zip) to inspect the same input, CSV and run manifest. Extract the archive and pass its `run.json` to `analysis_reopen`, or follow the [Composer result-review instructions](../HydroCoupleComposer/guide/results.html).
