# SPDX-License-Identifier: MIT
# src/tools/CMakeLists.txt — HydroCoupleTools source files

target_sources(HydroCoupleTools
PRIVATE
    # ── domain specification ──────────────────────────────────────────────
    domain/terraindomain.cpp
    domain/constraintsegment.cpp
    domain/regionmarker.cpp

    # ── mesh generation ───────────────────────────────────────────────────
    mesh/meshresult.cpp
    mesh/triangulation/delaunaymeshgenerator.cpp
    mesh/rectilinear/rectilineargrid2dgenerator.cpp
    mesh/rectilinear/rectilineargrid3dgenerator.cpp
    mesh/curvilinear/curvilineargrid2dgenerator.cpp
    mesh/curvilinear/curvilineargrid3dgenerator.cpp

    # ── terrain processing ────────────────────────────────────────────────
    terrain/terrainsampler.cpp
    terrain/terrainthinner.cpp
    terrain/terrainanalysis.cpp

    # ── interpolation ─────────────────────────────────────────────────────
    interpolation/methods/bilinearinterpolator.cpp
    interpolation/methods/barycentricinterpolator.cpp
    interpolation/methods/idwinterpolator.cpp
    interpolation/methods/nearestneighborinterpolator.cpp
    interpolation/methods/naturalneighborinterpolator.cpp
    interpolation/spatial/meshtomeshinterpolator.cpp
    interpolation/spatial/gridtomeshinterpolator.cpp
    interpolation/spatial/meshtogridinterpolator.cpp
    interpolation/spatial/gridtogridinterpolator.cpp
)
