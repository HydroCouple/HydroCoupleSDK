# SPDX-License-Identifier: MIT
# src/CMakeLists.txt — HydroCoupleSDK source files

# ── C files (spatial triangulation — compiled as C99, not C++) ────────────
set_source_files_properties(
    spatial/triangle.c
    spatial/tricall.c
    PROPERTIES
        LANGUAGE C
        C_STANDARD 99
        SKIP_PRECOMPILE_HEADERS ON  # stdafx.h has C++ headers; skip for C TUs
)

target_sources(HydroCoupleSDK
PRIVATE
    # ── stdafx ────────────────────────────────────────────────────────────
    stdafx.cpp

    # ── hydrocouple (namespace HydroCouple) — C++20 migrated sources ──────
    # Files are added here as they are migrated from include/core/ (Qt) to
    # include/hydrocouple/ (C++20).  Old Qt-based files in src/core/ are
    # kept as reference but are NOT compiled until migration is complete.
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

    # ── temporal (namespace HydroCouple::Temporal) ──────────────────────
    hydrocouple/timedata.cpp
    hydrocouple/timeseries.cpp
    hydrocouple/timeseriesexchangeitems.cpp
    hydrocouple/timeseriesadaptedoutput.cpp
    hydrocouple/timeseriesargument.cpp

    # ── utilities ─────────────────────────────────────────────────────────
    # hydrocoupleexceptions.cpp   # TODO: migrate (uses QString)
    # progresschecker.cpp         # TODO: migrate (uses QObject signals)
    # splineinterpolator.cpp      # TODO: migrate (uses QObject)

    # ── spatial (namespace HydroCouple::Spatial) ──────────────────────────
    hydrocouple/linestring.cpp
    hydrocouple/polygon.cpp
    hydrocouple/polyhedralsurface.cpp
    hydrocouple/tin.cpp
    hydrocouple/network.cpp
    hydrocouple/raster.cpp
    hydrocouple/regulargrid2d.cpp
    hydrocouple/regulargrid3d.cpp
    hydrocouple/rastercomponentdataitem.cpp

    # ── spatial C files (triangulation) ────────────────────────────────────
    spatial/triangle.c
    spatial/tricall.c
)

# ── thread-safe NetCDF (conditional) ──────────────────────────────────────
# TODO: migrate to std::shared_mutex (currently uses Qt OpenMP critical sections)
# if(USE_NETCDF)
#     target_sources(HydroCoupleSDK PRIVATE
#         threadsafenetcdf/threadsafencfile.cpp
#         threadsafenetcdf/threadsafencgroup.cpp
#         threadsafenetcdf/threadsafencvar.cpp
#         threadsafenetcdf/threadsafencdim.cpp
#         threadsafenetcdf/threadsafencatt.cpp
#     )
# endif()
