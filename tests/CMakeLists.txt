# SPDX-License-Identifier: MIT
# tests/CMakeLists.txt — HydroCoupleSDK Google Test suite

add_executable(HydroCoupleSDKTests
    hydrocouplesdk/test_bufferio.cpp
    hydrocouplesdk/test_dataitems.cpp
    hydrocouplesdk/test_component.cpp
    hydrocouplesdk/test_temporal.cpp
    hydrocouplesdk/test_io.cpp
    hydrocouplesdk/test_distributed.cpp
    hydrocouplesdk/test_workflow.cpp
)

target_link_libraries(HydroCoupleSDKTests
    PRIVATE
        HydroCoupleSDK
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(HydroCoupleSDKTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${HYDROCOUPLE_INCLUDE_DIR}
)

# Test file IO goes to a reviewable location inside the source tree
# (tests/artifacts/), not a temp folder.
target_compile_definitions(HydroCoupleSDKTests
    PRIVATE
        TEST_ARTIFACTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/artifacts"
)


if(USE_HDF5)
    target_sources(HydroCoupleSDKTests PRIVATE hydrocouplesdk/test_hdf5.cpp)
    target_link_libraries(HydroCoupleSDKTests PRIVATE ${HYDROCOUPLESDK_HDF5_TARGET})
endif()

if(USE_GEOPACKAGE)
    target_sources(HydroCoupleSDKTests PRIVATE hydrocouplesdk/test_geopackage.cpp)
    target_link_libraries(HydroCoupleSDKTests PRIVATE ${HYDROCOUPLESDK_SQLITE_TARGET})
endif()

if(USE_NETCDF)
    target_sources(HydroCoupleSDKTests PRIVATE hydrocouplesdk/test_netcdf.cpp)
    target_link_libraries(HydroCoupleSDKTests PRIVATE ${HYDROCOUPLESDK_NETCDF_TARGET})
endif()

include(GoogleTest)
gtest_discover_tests(HydroCoupleSDKTests)
