# SPDX-License-Identifier: MIT
# tests/CMakeLists.txt — HydroCoupleSDK Google Test suite

add_executable(HydroCoupleSDKTests
    hydrocouplesdk/test_bufferio.cpp
    hydrocouplesdk/test_dataitems.cpp
    hydrocouplesdk/test_uriresolver.cpp
    hydrocouplesdk/test_component.cpp
    hydrocouplesdk/test_temporal.cpp
    hydrocouplesdk/test_io.cpp
    hydrocouplesdk/test_adaptedoutputs.cpp
    hydrocouplesdk/test_compositionspec.cpp
    hydrocouplesdk/test_runmanifest.cpp
    hydrocouplesdk/test_resultsreopen.cpp
    hydrocouplesdk/test_executionmodes.cpp
    hydrocouplesdk/test_distributed.cpp
    hydrocouplesdk/test_fluxpayload.cpp
    hydrocouplesdk/test_device.cpp
    hydrocouplesdk/test_spatialadapters.cpp
    hydrocouplesdk/test_spatialarguments.cpp
    hydrocouplesdk/test_typedarguments.cpp
    hydrocouplesdk/test_spatiotemporal.cpp
    hydrocouplesdk/test_layereddataitem.cpp
    hydrocouplesdk/test_workflow.cpp
    hydrocouplesdk/test_differentiation.cpp
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


if(BUILD_TOOLS)
    target_sources(HydroCoupleSDKTests PRIVATE hydrocouplesdk/test_tools.cpp)
    target_link_libraries(HydroCoupleSDKTests PRIVATE HydroCoupleTools)
endif()

# Only a static archive carries the second-copy hazard described under
# USE_NETCDF below; a shared library is linked like any other.
function(hydrocouplesdk_link_if_shared consumer target)
    get_target_property(_type ${target} TYPE)
    if(_type STREQUAL "SHARED_LIBRARY")
        target_link_libraries(${consumer} PRIVATE ${target})
    endif()
endfunction()

if(USE_HDF5)
    target_sources(HydroCoupleSDKTests PRIVATE hydrocouplesdk/test_hdf5.cpp)

    # Headers only, for the reason given under USE_NETCDF below: a second
    # copy of the static HDF5 archive in this executable is a second set of
    # HDF5's globals. The NetCDF suite is where that surfaced, but nothing
    # made the HDF5 suite immune -- it happened not to hand a handle across
    # the two -- so the duplicate goes rather than being left to be found
    # again by whoever writes the test that does.
    # A list, not one target: a static HDF5 carries zlib beside it.
    foreach(_headers_only IN LISTS HYDROCOUPLESDK_HDF5_TARGET)
        if(TARGET ${_headers_only})
            target_include_directories(HydroCoupleSDKTests PRIVATE
                $<TARGET_PROPERTY:${_headers_only},INTERFACE_INCLUDE_DIRECTORIES>)
            hydrocouplesdk_link_if_shared(HydroCoupleSDKTests ${_headers_only})
        endif()
    endforeach()
endif()

if(USE_GEOPACKAGE)
    target_sources(HydroCoupleSDKTests PRIVATE hydrocouplesdk/test_geopackage.cpp)
    target_link_libraries(HydroCoupleSDKTests PRIVATE ${HYDROCOUPLESDK_SQLITE_TARGET})
endif()

if(USE_NETCDF)
    target_sources(HydroCoupleSDKTests PRIVATE
        hydrocouplesdk/test_netcdf.cpp
        hydrocouplesdk/test_ugridreader.cpp)

    # Headers only, deliberately: the netCDF and HDF5 that vcpkg supplies are
    # static archives, and linking them here as well as into libHydroCoupleSDK
    # puts TWO copies of each in one process. The copies do not share HDF5's
    # global state, so a file this suite wrote through the library could not
    # be reopened by the executable's own copy -- nc_open returned NC_EHDFERR
    # on a file the SDK's reader opened happily, and the macOS linker had
    # already said as much ("ignoring duplicate libraries: libhdf5.a").
    #
    # The tests call nc_* to check the file with the library rather than with
    # our own reader, which is the point of them, so the calls stay and the
    # symbols are resolved from the shared library instead. That is exactly
    # one instance, and it is the same one that wrote the file. Every nc_*
    # the suite uses is among those the library exports.
    #
    # A dynamic triplet (the macos presets' arm64-osx-dynamic) is the
    # opposite case: the SDK exports none of netCDF's or HDF5's symbols, and
    # a shared library is one instance however many images link it, so there
    # the targets are linked. See hydrocouplesdk_link_if_shared above.
    foreach(_headers_only IN LISTS HYDROCOUPLESDK_NETCDF_TARGET)
        if(TARGET ${_headers_only})
            target_include_directories(HydroCoupleSDKTests PRIVATE
                $<TARGET_PROPERTY:${_headers_only},INTERFACE_INCLUDE_DIRECTORIES>)
            hydrocouplesdk_link_if_shared(HydroCoupleSDKTests ${_headers_only})
        endif()
    endforeach()
endif()

hydrocouplesdk_colocate_runtime_dlls(HydroCoupleSDKTests)

include(GoogleTest)
gtest_discover_tests(HydroCoupleSDKTests)

# ── MPI-launched transport tests (separate executable: own main owns MPI) ──
if(USE_MPI)
    add_executable(HydroCoupleSDKMpiTests
        hydrocouplesdk/test_mpitransport.cpp
    )
    target_link_libraries(HydroCoupleSDKMpiTests
        PRIVATE
            HydroCoupleSDK
            GTest::gtest
            MPI::MPI_CXX
    )
    target_include_directories(HydroCoupleSDKMpiTests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${HYDROCOUPLE_INCLUDE_DIR}
    )
    hydrocouplesdk_colocate_runtime_dlls(HydroCoupleSDKMpiTests)

    # Rank count is configurable, and the launcher's own flags come from
    # FindMPI's MPIEXEC_PREFLAGS. CI machines usually have fewer cores than
    # ranks, and OpenMPI refuses to oversubscribe unless told to
    # (--oversubscribe), while MPICH rejects that flag outright — so the
    # flag belongs to the environment, not to this file.
    set(HYDROCOUPLESDK_MPI_TEST_RANKS 3 CACHE STRING
        "Ranks used by the MPI transport test suite (3 exercises the ring)")

    add_test(NAME MpiTransportSuite
        COMMAND ${MPIEXEC_EXECUTABLE}
                ${MPIEXEC_NUMPROC_FLAG} ${HYDROCOUPLESDK_MPI_TEST_RANKS}
                ${MPIEXEC_PREFLAGS}
                $<TARGET_FILE:HydroCoupleSDKMpiTests>
                ${MPIEXEC_POSTFLAGS}
    )
endif()

# The LibTorch layer's suite (Phase G2.3): its own executable, so the main
# suite never links LibTorch.
if(HYDROCOUPLESDK_WITH_TORCH)
    add_executable(HydroCoupleSDKTorchTests
        hydrocouplesdk/test_torchdifferentiation.cpp
    )
    target_link_libraries(HydroCoupleSDKTorchTests
        PRIVATE
            HydroCoupleSDKTorch
            GTest::gtest
            GTest::gtest_main
    )
    target_include_directories(HydroCoupleSDKTorchTests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${HYDROCOUPLE_INCLUDE_DIR}
    )
    if(HYDROCOUPLESDK_TORCH_RPATH)
        set_target_properties(HydroCoupleSDKTorchTests PROPERTIES
            BUILD_RPATH "${HYDROCOUPLESDK_TORCH_RPATH}")
    endif()
    gtest_discover_tests(HydroCoupleSDKTorchTests)
    hydrocouplesdk_colocate_runtime_dlls(HydroCoupleSDKTorchTests)
endif()
