# SPDX-License-Identifier: MIT
# src/CMakeLists.txt — HydroCoupleSDK source files

target_sources(HydroCoupleSDK
PRIVATE
    # ── stdafx ────────────────────────────────────────────────────────────
    stdafx.cpp

    # ── core (namespace HydroCouple) ──────────────────────────────────────
    hydrocouplesdk/core/description.cpp
    hydrocouplesdk/core/identity.cpp
    hydrocouplesdk/core/unit.cpp
    hydrocouplesdk/core/valuedefinition.cpp
    hydrocouplesdk/core/dimension.cpp
    hydrocouplesdk/component/componentinfo.cpp
    hydrocouplesdk/component/abstractmodelcomponentinfo.cpp
    hydrocouplesdk/data/abstractcomponentdataitem.cpp
    hydrocouplesdk/data/abstractargument.cpp
    hydrocouplesdk/data/abstractexchangeitem.cpp
    hydrocouplesdk/data/abstractinput.cpp
    hydrocouplesdk/data/abstractoutput.cpp
    hydrocouplesdk/data/abstractadaptedoutput.cpp
    hydrocouplesdk/component/componentstatuschangeeventargs.cpp
    hydrocouplesdk/data/exchangeitems1d.cpp
    hydrocouplesdk/data/exchangeitems2d.cpp
    hydrocouplesdk/data/argument1d.cpp
    hydrocouplesdk/data/argument2d.cpp
    hydrocouplesdk/data/idbasedargument.cpp
    hydrocouplesdk/data/abstractadaptedoutputfactory.cpp
    hydrocouplesdk/data/idbasedexchangeitems.cpp
    hydrocouplesdk/component/abstractmodelcomponent.cpp
    hydrocouplesdk/component/workflowstatuschangeeventargs.cpp
    hydrocouplesdk/component/abstractworkflowcomponent.cpp
    hydrocouplesdk/component/pulldrivenworkflow.cpp
    hydrocouplesdk/component/timesteppedworkflow.cpp
    hydrocouplesdk/component/resultsmodelcomponent.cpp

    # ── distributed (namespace HydroCouple::SDK::Distributed) ────────────
    hydrocouplesdk/distributed/valuepacker.cpp
    hydrocouplesdk/distributed/inproctransport.cpp
    hydrocouplesdk/distributed/componentworker.cpp
    hydrocouplesdk/distributed/proxymodelcomponent.cpp
    hydrocouplesdk/distributed/haloexchanger.cpp
    hydrocouplesdk/distributed/partitioneddataitem.cpp
    hydrocouplesdk/distributed/fluxpayload.cpp

    # ── device (namespace HydroCouple::SDK::Device) ───────────────────────
    hydrocouplesdk/device/devicebackend.cpp
    hydrocouplesdk/device/devicebufferregistry.cpp
    hydrocouplesdk/device/spacetransfer.cpp

    # ── io (namespace HydroCouple::SDK::IO) ───────────────────────────────
    hydrocouplesdk/io/snapshot.cpp
    hydrocouplesdk/io/meshdefinition.cpp
    hydrocouplesdk/io/iothread.cpp
    hydrocouplesdk/io/csvwriter.cpp
    hydrocouplesdk/io/resultentry.cpp
    hydrocouplesdk/io/resultreader.cpp
    hydrocouplesdk/io/runmanifest.cpp
    hydrocouplesdk/io/compositionspec.cpp
    hydrocouplesdk/io/modelinitializer.cpp


    # ── temporal (namespace HydroCouple::Temporal) ────────────────────────
    hydrocouplesdk/temporal/timedata.cpp
    hydrocouplesdk/temporal/timeseries.cpp
    hydrocouplesdk/temporal/timeseriesexchangeitems.cpp
    hydrocouplesdk/temporal/timeseriesadaptedoutput.cpp
    hydrocouplesdk/temporal/timeseriesargument.cpp

    # ── spatiotemporal (namespace HydroCouple::SDK::SpatioTemporal) ───────
    hydrocouplesdk/spatiotemporal/spatiotemporalitems.cpp

    # ── spatial (namespace HydroCouple::Spatial) ──────────────────────────
    hydrocouplesdk/spatial/linestring.cpp
    hydrocouplesdk/spatial/polygon.cpp
    hydrocouplesdk/spatial/polyhedralsurface.cpp
    hydrocouplesdk/spatial/tin.cpp
    hydrocouplesdk/spatial/network.cpp
    hydrocouplesdk/spatial/regulargrid2d.cpp
    hydrocouplesdk/spatial/regulargrid3d.cpp
)

# ── Kokkos device backend (optional) ──────────────────────────────────────
if(USE_KOKKOS)
    target_sources(HydroCoupleSDK PRIVATE
        hydrocouplesdk/device/kokkosbackend.cpp
    )
endif()

# ── MPI transport backend (optional) ──────────────────────────────────────
if(USE_MPI)
    target_sources(HydroCoupleSDK PRIVATE
        hydrocouplesdk/distributed/mpitransport.cpp
    )
endif()

# ── GDAL-backed raster sources (optional) ─────────────────────────────────
if(USE_GDAL)
    target_sources(HydroCoupleSDK PRIVATE
        hydrocouplesdk/spatial/raster.cpp
        hydrocouplesdk/spatial/rastercomponentdataitem.cpp
    )
endif()

# ── thread-safe NetCDF (conditional) ──────────────────────────────────────
# TODO: port to std::shared_mutex before re-enabling (legacy sources predate the Qt removal)
# if(USE_NETCDF)
#     target_sources(HydroCoupleSDK PRIVATE
#         threadsafenetcdf/threadsafencfile.cpp
#         threadsafenetcdf/threadsafencgroup.cpp
#         threadsafenetcdf/threadsafencvar.cpp
#         threadsafenetcdf/threadsafencdim.cpp
#         threadsafenetcdf/threadsafencatt.cpp
#     )
# endif()

if(USE_HDF5)
    target_sources(HydroCoupleSDK PRIVATE
        hydrocouplesdk/io/hdf5ugridwriter.cpp
    )
endif()

if(USE_GEOPACKAGE)
    target_sources(HydroCoupleSDK PRIVATE
        hydrocouplesdk/io/geopackagewriter.cpp
    )
endif()

if(USE_NETCDF)
    target_sources(HydroCoupleSDK PRIVATE
        hydrocouplesdk/io/netcdfugridwriter.cpp
    )
endif()
