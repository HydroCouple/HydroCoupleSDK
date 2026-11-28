#!/bin/bash
# S1.3 falsifier — PolyhedralSurfaceArgument.
set -u
cd "$(dirname "$0")/.."
SRC=src/hydrocouplesdk/spatial/polyhedralsurfaceargument.cpp
BAK=$SRC.s13bak
GATES='PolyhedralSurfaceArgumentTest.*'
BUILD="cmake --build build/darwin --target HydroCoupleSDKTests -j 8"
RUN="./build/darwin/tests/HydroCoupleSDKTests --gtest_filter=$GATES"

cp "$SRC" "$BAK"
restore() { cp "$BAK" "$SRC"; }
trap 'restore; rm -f "$BAK"; $BUILD >/dev/null 2>&1' EXIT

verdict() {
    if ! $BUILD >/dev/null 2>&1; then echo "M$1: BUILD FAILED"; return; fi
    if $RUN >/dev/null 2>&1; then echo "M$1: SURVIVED"; else echo "M$1: CAUGHT"; fi
}

mutate() {
    restore
    perl -0pi -e "$1" "$SRC"
    if cmp -s "$SRC" "$BAK"; then return 1; fi
    return 0
}

# M1 incoherent meshes accepted
mutate 's/(bool meshIsCoherent\(const HydroCouple::SDK::IO::MeshDefinition &mesh,\n                    std::string &message\)\n\{)/$1 return true;/s' \
    && verdict 1 || echo "M1: NOT APPLIED"

# M2 connectivity dropped when adopting from a surface item
mutate 's/mesh\.faceNodes\.assign\(faces\.begin\(\), faces\.end\(\)\);/mesh.faceNodes.clear(); mesh.faceNodeOffsets.clear();/' \
    && verdict 2 || echo "M2: NOT APPLIED"

# M3 a file-less mesh serializes anyway
mutate 's/if \(m_sourceReference\.empty\(\)\)\n        \{\n            message = "This mesh came from a component output/if (false)\n        {\n            message = "This mesh came from a component output/s' \
    && verdict 3 || echo "M3: NOT APPLIED"

# M4 inline JSON payloads accepted silently
mutate 's/message = "A mesh is never inlined into a document; pass "\n                  "\{\\"\@uri\\": \\"<mesh file>\\"\} or bind the argument with "\n                  "\@from";\n        return false;/message.clear();\n        return true;/s' \
    && verdict 4 || echo "M4: NOT APPLIED"

# M5 the data plane interleaves y where x belongs
mutate 's/interleaved\[static_cast<size_t>\(node\) \* 3 \+ 0\] =\n            m_mesh\.nodeX\[static_cast<size_t>\(node\)\];/interleaved[static_cast<size_t>(node) * 3 + 0] =\n            m_mesh.nodeY[static_cast<size_t>(node)];/s' \
    && verdict 5 || echo "M5: NOT APPLIED"

# M6 z coordinates dropped when adopting
mutate 's/mesh\.nodeZ\.assign\(nodeZ\.begin\(\), nodeZ\.end\(\)\);/mesh.nodeZ.clear();/' \
    && verdict 6 || echo "M6: NOT APPLIED"
