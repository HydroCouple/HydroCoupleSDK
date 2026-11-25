#!/usr/bin/env bash
# U2-S falsifiers: typed arguments answering their typed interfaces.
#
# Two kinds of mutation here. Most flip an answer and must turn a gate
# red. Three do not compile at all once applied — removing a base class
# leaves a pure virtual behind, which is exactly what the static_asserts
# in test_typedarguments.cpp exist to catch; for those a build failure IS
# the catch, and is reported as one.
#
# Every mutation must bite; a pattern that no longer matches is reported
# as NOT APPLIED rather than read as a survivor.
set -u
cd "$(dirname "$0")/../.."

BUILD=${BUILD:-build}
TARGET=${TARGET:-HydroCoupleSDKTests}
BIN=${BIN:-$BUILD/tests/HydroCoupleSDKTests}

TS=src/hydrocouplesdk/temporal/timeseriesargument.cpp
TSH=include/hydrocouplesdk/temporal/timeseriesargument.h
PS=src/hydrocouplesdk/spatial/polyhedralsurfaceargument.cpp
PSH=include/hydrocouplesdk/spatial/polyhedralsurfaceargument.h

pass=0; fail=0
note() { printf '%s\n' "$*"; }
builds() { cmake --build "$BUILD" --target "$TARGET" >/dev/null 2>&1; }
gate_ok() { timeout 300 "$BIN" --gtest_filter="$1" >/dev/null 2>&1; }
restore() {
    for f in "$TS" "$TSH" "$PS" "$PSH"; do
        [ -f "$f.bak" ] && mv "$f.bak" "$f" && touch "$f"
    done
    return 0
}
trap 'restore; builds' EXIT

mutate() {
    cp "$1" "$1.bak"; perl -0pi -e "$2" "$1"
    if cmp -s "$1" "$1.bak"; then note "NOT APPLIED in $1"; fail=$((fail+1)); return 1; fi
    return 0
}

# A mutation that must turn a gate red.
run() {
    if mutate "$2" "$3"; then
        if ! builds; then
            note "SURVIVED $1 (did not build; expected a red gate)"; fail=$((fail+1))
        elif gate_ok "$4"; then
            note "SURVIVED $1"; fail=$((fail+1))
        else
            note "CAUGHT   $1"; pass=$((pass+1))
        fi
    fi
    restore
}

# A mutation that must fail to compile: the static_asserts are the gate.
nobuild() {
    if mutate "$2" "$3"; then
        if builds; then
            note "SURVIVED $1 (compiled; a pure virtual went unnoticed)"; fail=$((fail+1))
        else
            note "CAUGHT   $1 (refused to build, as it must)"; pass=$((pass+1))
        fi
    fi
    restore
}

builds || note "BASELINE BUILD FAILED"
gate_ok 'TypedArgumentTest.*'
if [ $? -eq 0 ]; then note "GREEN    baseline"; pass=$((pass+1)); else note "BASELINE RED"; fail=$((fail+1)); fi

# ── the interfaces themselves ────────────────────────────────────────────
nobuild "S1 the time-series argument stops answering the interface" "$TSH" \
  's/    : public HydroCouple::SDK::AbstractArgument,\n      public virtual HydroCouple::Temporal::ITimeSeriesComponentDataItem/    : public HydroCouple::SDK::AbstractArgument/s'

nobuild "S2 the mesh argument stops answering the interface" "$PSH" \
  's/    : public HydroCouple::SDK::AbstractArgument,\n      public virtual HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem/    : public HydroCouple::SDK::AbstractArgument/s'

# ── the time series ──────────────────────────────────────────────────────
run "S3 the dates are cached once and never checked again" "$TS" \
  's/    if \(!slot \|\| slot->julianDay\(\) != julianDay\)/    if (!slot)/s' \
  'TypedArgumentTest.ReloadingASeriesDoesNotHandOutTheOldDates'

run "S4 the span is cached once and never checked again" "$TS" \
  's/    if \(!m_timeSpan \|\| m_timeSpan->startTime\(\) != start \|\|\n        m_timeSpan->duration\(\) != duration\)/    if (!m_timeSpan)/s' \
  'TypedArgumentTest.ReloadingASeriesDoesNotHandOutTheOldDates'

run "S5 an index past the end returns the last date instead of nothing" "$TS" \
  's/    if \(timeIndex < 0 \|\|\n        static_cast<size_t>\(timeIndex\) >= julian\.size\(\)\)\n    \{\n        return nullptr;\n    \}/    if (timeIndex < 0 || julian.empty())\n    {\n        return nullptr;\n    }\n\n    if (static_cast<size_t>(timeIndex) >= julian.size())\n    {\n        timeIndex = static_cast<int64_t>(julian.size()) - 1;\n    }/s' \
  'TypedArgumentTest.ATimeSeriesArgumentReportsItsTimes'

run "S6 an empty series has no span at all" "$TS" \
  's/    const double start = julian\.empty\(\) \? 0\.0 : julian\.front\(\);/    if (julian.empty())\n    {\n        return nullptr;\n    }\n\n    const double start = julian.front();/s' \
  'TypedArgumentTest.AnEmptySeriesCoversNothingRatherThanRefusingToSay'

run "S7 the time dimension is forgotten" "$TS" \
  's/IDimension \*TimeSeriesArgumentDouble::timeDimension\(\) const\n\{\n    return m_timeDim;\n\}/IDimension *TimeSeriesArgumentDouble::timeDimension() const\n{\n    return nullptr;\n}/s' \
  'TypedArgumentTest.ATimeSeriesArgumentReportsItsTimes'

# ── the surface ──────────────────────────────────────────────────────────
run "S8 the surface is built without its connectivity, so it has no faces" "$PS" \
  's/        m_surface = std::make_unique<PolyhedralSurfaceAdapter>\(\n            polyhedralSurfaceFromMesh\(m_mesh\), m_mesh\);/        m_surface = std::make_unique<PolyhedralSurfaceAdapter>(\n            polyhedralSurfaceFromMesh(m_mesh));/s' \
  'TypedArgumentTest.TheSurfaceCarriesTheConnectivityItWasBuiltFrom'

run "S9 no mesh reads as an empty surface" "$PS" \
  's/    if \(m_mesh\.nodeX\.empty\(\)\)\n    \{[\s\S]*?return nullptr;\n    \}\n\n//s' \
  'TypedArgumentTest.NoMeshIsNotAnEmptySurface'

run "S10 adopting a mesh keeps the surface built from the last one" "$PS" \
  's/    m_mesh = std::move\(mesh\);\n\n    \/\/ Dropped rather than rebuilt[\s\S]*?m_surface\.reset\(\);/    m_mesh = std::move(mesh);/s' \
  'TypedArgumentTest.ReplacingAMeshReplacesTheSurfaceItBuilt'

run "S11 the vertex axis is a second dimension of its own" "$PS" \
  's/    return const_cast<Dimension \*>\(&m_nodeDimension\);/    return const_cast<Dimension *>(\&m_patchDimension);/s' \
  'TypedArgumentTest.AMeshArgumentReportsItsSurface'

run "S12 patches and edges share one axis" "$PS" \
  's/HydroCouple::IDimension \*PolyhedralSurfaceArgument::edgeDimension\(\) const\n\{\n    return const_cast<Dimension \*>\(&m_edgeDimension\);\n\}/HydroCouple::IDimension *PolyhedralSurfaceArgument::edgeDimension() const\n{\n    return const_cast<Dimension *>(\&m_patchDimension);\n}/s' \
  'TypedArgumentTest.AMeshArgumentReportsItsSurface'

run "S13 node coordinates are described as one value per face" "$PS" \
  's/    return HydroCouple::Spatial::MeshDataObjectType::Vertex;/    return HydroCouple::Spatial::MeshDataObjectType::Cell;/s' \
  'TypedArgumentTest.AMeshArgumentReportsItsSurface'

run "S14 a position is described as a scalar" "$PS" \
  's/    return HydroCouple::Spatial::SpatialDataType::Vector;/    return HydroCouple::Spatial::SpatialDataType::Scalar;/s' \
  'TypedArgumentTest.AMeshArgumentReportsItsSurface'

note ""
note "u2s: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
