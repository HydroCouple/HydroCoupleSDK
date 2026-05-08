# SPDX-License-Identifier: MIT
# src/CMakeLists.txt — HydroCoupleSDK source files

target_sources(HydroCoupleSDK
PRIVATE
    # ── stdafx ────────────────────────────────────────────────────────────
    stdafx.cpp

    # ── core (namespace HydroCouple) ──────────────────────────────────────
    hydrocouple/datatype.cpp
    hydrocouple/safevariant.cpp
    hydrocouple/description.cpp
    hydrocouple/identity.cpp
    hydrocouple/unit.cpp
    hydrocouple/valuedefinition.cpp
    hydrocouple/dimension.cpp
    hydrocouple/componentinfo.cpp
    hydrocouple/abstractmodelcomponentinfo.cpp
    hydrocouple/abstractcomponentdataitem.cpp
    hydrocouple/abstractargument.cpp
    hydrocouple/abstractexchangeitem.cpp
    hydrocouple/abstractinput.cpp
    hydrocouple/abstractoutput.cpp
    hydrocouple/abstractadaptedoutput.cpp
    hydrocouple/componentstatuschangeeventargs.cpp
    hydrocouple/exchangeitems1d.cpp
    hydrocouple/exchangeitems2d.cpp
    hydrocouple/argument1d.cpp
    hydrocouple/argument2d.cpp
    hydrocouple/abstractadaptedoutputfactory.cpp
    hydrocouple/abstractmodelcomponent.cpp

    # ── temporal (namespace HydroCouple::Temporal) ────────────────────────
    hydrocouple/temporal/timedata.cpp
    hydrocouple/temporal/timeseries.cpp
    hydrocouple/temporal/timeseriesexchangeitems.cpp
    hydrocouple/temporal/timeseriesadaptedoutput.cpp
    hydrocouple/temporal/timeseriesargument.cpp

    # ── spatial (namespace HydroCouple::Spatial) ──────────────────────────
    hydrocouple/spatial/linestring.cpp
    hydrocouple/spatial/polygon.cpp
    hydrocouple/spatial/polyhedralsurface.cpp
    hydrocouple/spatial/tin.cpp
    hydrocouple/spatial/network.cpp
    hydrocouple/spatial/raster.cpp
    hydrocouple/spatial/regulargrid2d.cpp
    hydrocouple/spatial/regulargrid3d.cpp
    hydrocouple/spatial/rastercomponentdataitem.cpp
)

# ── thread-safe NetCDF (conditional) ──────────────────────────────────────
# TODO: upgrade to std::shared_mutex (currently uses Qt OpenMP critical sections)
# if(USE_NETCDF)
#     target_sources(HydroCoupleSDK PRIVATE
#         threadsafenetcdf/threadsafencfile.cpp
#         threadsafenetcdf/threadsafencgroup.cpp
#         threadsafenetcdf/threadsafencvar.cpp
#         threadsafenetcdf/threadsafencdim.cpp
#         threadsafenetcdf/threadsafencatt.cpp
#     )
# endif()
