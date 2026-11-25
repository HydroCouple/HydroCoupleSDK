#!/usr/bin/env bash
# A4 falsifiers: the AdapterFactoryResolver seam.
set -u
cd "$(dirname "$0")/.."

SRC=src/hydrocouplesdk/io/modelinitializer.cpp
BUILD=build/darwin

pass=0; fail=0
note() { printf '%s\n' "$*"; }
rebuild() { cmake --build "$BUILD" --target HydroCoupleSDKTests >/dev/null 2>&1; }
gate_ok() { "$BUILD/tests/HydroCoupleSDKTests" --gtest_filter="$1" >/dev/null 2>&1; }
check() {
    local label=$1 expect=$2 rc=$3
    if [ "$expect" = red ] && [ "$rc" -ne 0 ]; then note "CAUGHT   $label"; pass=$((pass+1))
    elif [ "$expect" = green ] && [ "$rc" -eq 0 ]; then note "GREEN    $label"; pass=$((pass+1))
    else note "SURVIVED $label"; fail=$((fail+1)); fi
}
restore() { [ -f "$SRC.bak" ] && mv "$SRC.bak" "$SRC" && touch "$SRC"; }

rebuild; gate_ok 'AdaptedChainTest.*'; check "baseline" green $?
trap 'restore; rebuild' EXIT

# M1: lookup order inverted — standalone factories searched first.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/        std::vector<HydroCouple::IAdaptedOutputFactory \*> candidates;\n        size_t infoCount = 0;\n        if \(info\)\n        \{\n            for \(HydroCouple::IAdaptedOutputFactory \*factory :\n                 info->adaptedOutputFactories\(\)\)\n                candidates\.push_back\(factory\);\n            infoCount = candidates\.size\(\);\n        \}\n        if \(m_adapterFactoryResolver\)\n        \{\n            for \(HydroCouple::IAdaptedOutputFactory \*factory :\n                 m_adapterFactoryResolver\(\)\)\n                candidates\.push_back\(factory\);\n        \}/        std::vector<HydroCouple::IAdaptedOutputFactory *> candidates;\n        size_t infoCount = 0;\n        if (m_adapterFactoryResolver)\n        {\n            for (HydroCouple::IAdaptedOutputFactory *factory :\n                 m_adapterFactoryResolver())\n                candidates.push_back(factory);\n        }\n        if (info)\n        {\n            for (HydroCouple::IAdaptedOutputFactory *factory :\n                 info->adaptedOutputFactories())\n                candidates.push_back(factory);\n            infoCount = candidates.size();\n        }/s' "$SRC"
rebuild
gate_ok 'AdaptedChainTest.StandaloneFactoriesAreSearchedAfterComponentInfoFactories'
check "M1 lookup order inverted" red $?
restore

# M2: the resolver is never consulted.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/        if \(m_adapterFactoryResolver\)\n        \{\n            for \(HydroCouple::IAdaptedOutputFactory \*factory :\n                 m_adapterFactoryResolver\(\)\)\n                candidates\.push_back\(factory\);\n        \}\n/\n/s' "$SRC"
rebuild
gate_ok 'AdaptedChainTest.StandaloneFactoriesAreSearchedAfterComponentInfoFactories'
check "M2 resolver never consulted" red $?
restore

# M3: the refusal stops distinguishing the standalone source.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/" standalone factories"/" other factories"/s' "$SRC"
rebuild
gate_ok 'AdaptedChainTest.RefusesAnUnknownAdapterNamingBothSourcesSearched'
check "M3 refusal loses the source split" red $?
restore

trap - EXIT
rebuild; gate_ok 'AdaptedChainTest.*'; check "restored" green $?

note "----"; note "pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
