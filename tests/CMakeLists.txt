# SPDX-License-Identifier: MIT
# tests/CMakeLists.txt — HydroCoupleSDK Google Test suite

add_executable(HydroCoupleSDKTests
    # ── core (namespace HydroCouple) ──────────────────────────────────────
    hydrocouple/test_signal.cpp
    hydrocouple/test_description.cpp
    hydrocouple/test_identity.cpp
    hydrocouple/test_uuid.cpp
    hydrocouple/test_valuedefinition.cpp
    hydrocouple/test_dimension.cpp
    hydrocouple/test_argument.cpp
    hydrocouple/test_componentdataitem.cpp
    hydrocouple/test_exchangeitems.cpp
    hydrocouple/test_adaptedoutput.cpp
    hydrocouple/test_workflow.cpp

    # ── temporal ──────────────────────────────────────────────────────────
    temporal/test_datetime.cpp
    temporal/test_timeseries.cpp
    temporal/test_timeseriesexchangeitem.cpp

    # ── spatial ───────────────────────────────────────────────────────────
    spatial/test_point.cpp
    spatial/test_linestring_soa.cpp
    spatial/test_polygon.cpp
    spatial/test_quadedgepool.cpp
    spatial/test_vertexpool.cpp
    spatial/test_polyhedralsurface.cpp
    spatial/test_tin.cpp
    spatial/test_network.cpp
    spatial/test_raster.cpp
    spatial/test_regulargrid2d.cpp
    spatial/test_regulargrid3d.cpp
    spatial/test_geometryfactory.cpp

    # ── spatiotemporal ────────────────────────────────────────────────────
    spatiotemporal/test_timegeometry.cpp
    spatiotemporal/test_timeregulargrid.cpp

    # ── I/O ───────────────────────────────────────────────────────────────
    $<$<BOOL:${USE_NETCDF}>:io/test_netcdf_rw.cpp>
    $<$<BOOL:${USE_NETCDF}>:io/test_netcdf_threadsafe.cpp>
    $<$<BOOL:${USE_NETCDF}>:io/test_ugrid_netcdf.cpp>
    $<$<BOOL:${USE_HDF5}>:io/test_hdf5_rw.cpp>
    $<$<BOOL:${USE_HDF5}>:io/test_hdf5_threadsafe.cpp>
    $<$<BOOL:${USE_HDF5}>:io/test_ugrid_hdf5.cpp>

    # ── MPI (only built when USE_MPI=ON) ──────────────────────────────────
    $<$<BOOL:${USE_MPI}>:mpi/test_mpi_lifecycle.cpp>
    $<$<BOOL:${USE_MPI}>:mpi/test_mpi_updatevalues.cpp>
    $<$<BOOL:${USE_MPI}>:mpi/test_mpi_statuschange.cpp>
)

target_link_libraries(HydroCoupleSDKTests
    PRIVATE
        HydroCoupleSDK
        GTest::gtest_main
        $<$<BOOL:${BUILD_TOOLS}>:HydroCoupleTools>
)

target_include_directories(HydroCoupleSDKTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/../HydroCouple/include
)

include(GoogleTest)
gtest_discover_tests(HydroCoupleSDKTests)

# ── Tools tests (separate executable when BUILD_TOOLS=ON) ─────────────────
if(BUILD_TOOLS)
    add_executable(HydroCoupleToolsTests
        tools/test_terraindomain.cpp
        tools/test_delaunaymeshgenerator.cpp
        tools/test_rectilineargenerator.cpp
        tools/test_curvilineargenerator.cpp
        tools/test_terrainsampler.cpp
        tools/test_terrainthinner.cpp
        tools/test_meshtomesh.cpp
        tools/test_gridtomesh.cpp
    )
    target_link_libraries(HydroCoupleToolsTests
        PRIVATE
            HydroCoupleTools
            GTest::gtest_main
    )
    gtest_discover_tests(HydroCoupleToolsTests)
endif()
