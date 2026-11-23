@page sdk_guide_build 1. Build and link the SDK

The 2.0 SDK implements the HydroCouple interfaces in C++20. A model wrapper can use its lifecycle, exchange-item and recording classes without linking Qt. Start with matching HydroCouple interface headers, a C++20 compiler, CMake and the JSON dependency. Point CMake at the installed packages or use the repository’s dependency instructions. [SDK build guide](https://github.com/HydroCouple/HydroCoupleSDK/blob/removing_qt/Readme.md)

In a consumer project, declare your library target first, then link the installed SDK through its exported CMake target.

```cmake
find_package(HydroCoupleSDK REQUIRED)
target_compile_features(my_component PRIVATE cxx_std_20)
target_link_libraries(my_component PRIVATE HydroCoupleSDK::HydroCoupleSDK)
```

When configuring a source build, set `HYDROCOUPLE_INCLUDE_DIR` to the directory containing `hydrocouple.h` if CMake cannot locate the interfaces. To build the worked example below, enable `BUILD_EXAMPLES`. [Build options and dependency discovery](https://github.com/HydroCouple/HydroCoupleSDK/blob/removing_qt/CMakeLists.txt)
