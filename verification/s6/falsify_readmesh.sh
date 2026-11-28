#!/bin/bash
# Slice 6 falsification: the readers answer for the geometry their entries
# name, and the items carry it.
#
# Every mutation below leaves a reader that still returns a mesh, still opens
# every artifact, and still passes every value test in the suite. What they
# change is whether the geometry is the one the values belong to.

set -u
cd "$(dirname "$0")/../.." || exit 1

BUILD=build/darwin
TEST=$BUILD/tests/HydroCoupleSDKTests
FILTER='ResultsReopenTest.*'
LOG=verification/s6/falsify_readmesh.log
: > "$LOG"

FILES=(src/hydrocouplesdk/io/resultreader.cpp
       src/hydrocouplesdk/component/resultsmodelcomponent.cpp
       src/hydrocouplesdk/spatial/meshadapters.cpp)

backup() { for f in "${FILES[@]}"; do cp "$f" "$f.orig"; done; }
restore() { for f in "${FILES[@]}"; do cp "$f.orig" "$f"; touch "$f"; done; }
cleanup() { restore; for f in "${FILES[@]}"; do rm -f "$f.orig"; done; }
trap cleanup EXIT

backup

run_mutation() {
  local name="$1"; shift
  restore
  "$@" || { echo "MUTATION $name: could not be applied" | tee -a "$LOG"; return; }

  local changed=0
  for f in "${FILES[@]}"; do
    cmp -s "$f" "$f.orig" || changed=1
  done

  if [ "$changed" -eq 0 ]; then
    echo "MUTATION $name: NOT APPLIED (stale pattern) -- inconclusive" | tee -a "$LOG"
    return
  fi

  for f in "${FILES[@]}"; do touch "$f"; done

  if ! cmake --build $BUILD --target HydroCoupleSDKTests -j 8 >> "$LOG" 2>&1; then
    echo "MUTATION $name: BUILD FAILED (inconclusive)" | tee -a "$LOG"
    return
  fi

  local out_file="$LOG.run"
  $TEST --gtest_filter="$FILTER" > "$out_file" 2>&1 &
  local pid=$!
  local waited=0

  while kill -0 $pid 2>/dev/null && [ $waited -lt 180 ]; do
    sleep 1
    waited=$((waited + 1))
  done

  if kill -0 $pid 2>/dev/null; then
    kill -9 $pid 2>/dev/null
    wait $pid 2>/dev/null
    echo "MUTATION $name: CAUGHT (hung -- the suite never finished)" | tee -a "$LOG"
    return
  fi

  wait $pid
  local status=$?

  local out
  out=$(cat "$out_file")
  rm -f "$out_file"
  echo "=== $name ===" >> "$LOG"
  echo "$out" | grep '^\[  FAILED  \]' >> "$LOG"

  if echo "$out" | grep -q "\[  FAILED  \]"; then
    echo "MUTATION $name: CAUGHT -- $(echo "$out" | grep '^\[  FAILED  \] ResultsReopen' | head -2 | sed 's/\[  FAILED  \] //' | tr '\n' ' ')" | tee -a "$LOG"
  elif [ "$status" -ne 0 ]; then
    echo "MUTATION $name: CAUGHT (the suite died, status $status)" | tee -a "$LOG"
  else
    echo "MUTATION $name: *** SURVIVED ***" | tee -a "$LOG"
  fi
}

# 1. An entry that names no mesh is given the first mesh in the file, so a
#    file holding a network and a floodplain hands back whichever came first.
m_first_mesh_wins() {
  perl -0pi -e 's/    if \(entry\.mesh\.empty\(\)\)\n    \{.*?\n    \}\n\n    return readUGRIDMesh/    return readUGRIDMesh/s' src/hydrocouplesdk/io/resultreader.cpp
}

# 2. The default says an artifact has geometry it does not have, so a CSV of
#    values reports a mesh of nothing.
m_default_claims_geometry() {
  perl -0pi -e 's/records values without geometry";\n    return false;/records values without geometry";\n    return true;/' src/hydrocouplesdk/io/resultreader.cpp
}

# 3. GeoPackage nodes come back in reverse index order -- every count right,
#    every value attached to the wrong point.
m_nodes_reversed() {
  perl -0pi -e 's/"SELECT geom FROM mesh_nodes ORDER BY node_index";/"SELECT geom FROM mesh_nodes ORDER BY node_index DESC";/' src/hydrocouplesdk/io/resultreader.cpp
}

# 4. The GeoPackage envelope is assumed absent, so a point from any other
#    tool is read out of the middle of its bounding box.
m_no_envelope_skip() {
  perl -0pi -e 's/    const int header = 8 \+ kEnvelopeDoubles\[envelope\] \* 8;/    const int header = 8;/' src/hydrocouplesdk/io/resultreader.cpp
}

# 5. The WKB coordinate is taken from the header's first bytes rather than
#    past the byte-order and type fields.
m_wkb_offset_off() {
  perl -0pi -e 's/    std::memcpy\(&x, wkb \+ 5, 8\);\n    std::memcpy\(&y, wkb \+ 13, 8\);/    std::memcpy(\&x, wkb, 8);\n    std::memcpy(\&y, wkb + 8, 8);/' src/hydrocouplesdk/io/resultreader.cpp
}

# 6. A GeoPackage with no mesh_nodes rows reports an empty mesh as a success,
#    so a consumer draws nothing and is told nothing.
m_empty_is_fine() {
  perl -0pi -e 's/        if \(read\.nodeX\.empty\(\)\)\n        \{.*?\n        \}\n\n        \/\/ Nodes and no connectivity/        \/\/ Nodes and no connectivity/s' src/hydrocouplesdk/io/resultreader.cpp
}

# 7. Every item claims the spatial interface, so an artifact of values alone
#    says it can be mapped and then hands back nothing.
m_everything_is_spatial() {
  perl -0pi -e 's/        const bool         spatial = reader->readMesh\(\*entry, mesh, meshMessage\);/        std::ignore = reader->readMesh(*entry, mesh, meshMessage);\n        const bool spatial = true;/' src/hydrocouplesdk/component/resultsmodelcomponent.cpp
}

# 8. The values are reported as living on the faces whatever the catalog
#    says, so a node field is drawn on the wrong entities.
m_always_cells() {
  perl -0pi -e 's/            default:\n                m_objectType = MeshDataObjectType::Vertex;\n                break;/            default:\n                m_objectType = MeshDataObjectType::Cell;\n                break;/' src/hydrocouplesdk/component/resultsmodelcomponent.cpp
}

# 9. All three entity dimensions are answered, so an item describes the mesh
#    three times over and says nothing about where its values are.
m_all_dimensions() {
  perl -0pi -e 's/        return m_objectType == MeshDataObjectType::Cell \? entityDimension\(\)\n                                                        : nullptr;/        return entityDimension();/' src/hydrocouplesdk/component/resultsmodelcomponent.cpp
}

# 10. The entity dimension is taken as the first, which is time.
m_entity_is_time() {
  perl -0pi -e 's/        return dimensions\(\)\.empty\(\) \? nullptr : dimensions\(\)\.back\(\);\n    \}\n\n    std::unique_ptr<Spatial::PolyhedralSurfaceAdapter> m_surface;/        return dimensions().empty() ? nullptr : dimensions().front();\n    }\n\n    std::unique_ptr<Spatial::PolyhedralSurfaceAdapter> m_surface;/' src/hydrocouplesdk/component/resultsmodelcomponent.cpp
}

# 11. Mesh nodes become surface vertices in reverse, so value i is drawn at
#     the wrong node while every count still agrees.
m_vertices_reversed() {
  perl -0pi -e 's/    for \(int64_t node = 0; node < mesh\.nodeCount\(\); \+\+node\)\n        surface\.addVertex\(nodeAt\(node\)\);/    for (int64_t node = mesh.nodeCount() - 1; node >= 0; --node)\n        surface.addVertex(nodeAt(node));/' src/hydrocouplesdk/spatial/meshadapters.cpp
}

# 12. A face's ring is left open, so a patch is a polyline pretending to be a
#     polygon.
m_ring_left_open() {
  perl -0pi -e 's/        if \(end > begin\)\n            ring\.addPoint\(nodeAt\(mesh\.faceNodes\[static_cast<size_t>\(begin\)\]\)\);//' src/hydrocouplesdk/spatial/meshadapters.cpp
}

# 13. The surface's bulk view is rebuilt from its vertices, so a surface with
#     faces and edges reports a view with neither -- an empty mesh rather
#     than a missing feature.
m_view_loses_connectivity() {
  perl -0pi -e 's/    m_meshView = std::make_unique<MeshViewAdapter>\(\n        supplied\.nodeCount\(\) > 0 \? std::move\(supplied\) : std::move\(mesh\)\);/    m_meshView = std::make_unique<MeshViewAdapter>(std::move(mesh));/' src/hydrocouplesdk/spatial/meshadapters.cpp
}

# 14. The item builds its surface without handing over the mesh it read, so
#     the connectivity is dropped one call before it would have been kept.
m_item_withholds_mesh() {
  perl -0pi -e 's/              Spatial::polyhedralSurfaceFromMesh\(mesh\), mesh\)\)/              Spatial::polyhedralSurfaceFromMesh(mesh)))/' src/hydrocouplesdk/component/resultsmodelcomponent.cpp
}

# 15. Edge endpoints are flattened with both ends taken from the origin.
m_edge_ends_collapse() {
  perl -0pi -e 's/        m_edgeNodes\.push_back\(edge\[1\]\);/        m_edgeNodes.push_back(edge[0]);/' src/hydrocouplesdk/spatial/meshadapters.cpp
}

run_mutation "1-unnamed-entry-takes-first-mesh" m_first_mesh_wins
run_mutation "2-default-claims-geometry"        m_default_claims_geometry
run_mutation "3-geopackage-nodes-reversed"      m_nodes_reversed
run_mutation "4-envelope-not-skipped"           m_no_envelope_skip
run_mutation "5-wkb-offset-wrong"               m_wkb_offset_off
run_mutation "6-empty-mesh-reads-as-success"    m_empty_is_fine
run_mutation "7-every-item-claims-geometry"     m_everything_is_spatial
run_mutation "8-values-always-on-faces"         m_always_cells
run_mutation "9-every-entity-dimension-answers" m_all_dimensions
run_mutation "10-entity-dimension-is-time"      m_entity_is_time
run_mutation "11-vertices-reversed"             m_vertices_reversed
run_mutation "12-face-ring-left-open"           m_ring_left_open
run_mutation "13-view-loses-connectivity"      m_view_loses_connectivity
run_mutation "14-item-withholds-its-mesh"      m_item_withholds_mesh
run_mutation "15-edge-ends-collapse"           m_edge_ends_collapse

restore
touch "${FILES[@]}"
cmake --build $BUILD --target HydroCoupleSDKTests -j 8 >> "$LOG" 2>&1
echo
echo "--- restored, clean run ---"
$TEST --gtest_filter="$FILTER" 2>&1 | tail -3
