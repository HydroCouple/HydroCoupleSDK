#!/usr/bin/env bash
# A3 falsifiers: ModelInitializer::buildAdaptedChain runtime behaviour.
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

# M1: initialize() runs before the step arguments are applied.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/\/\/ Adapted-output arguments are applied before initialize\(\), the/created->initialize(); \/\/ MUTANT: initialize first\n        \/\/ Adapted-output arguments are applied before initialize(), the/s;
              s/created->initialize\(\);\n        current = created;/current = created;/s' "$SRC"
rebuild
gate_ok 'AdaptedChainTest.AppliesChainStepArgumentsBeforeInitialize'
check "M1 initialize before arguments" red $?
restore

# M2: the consumer is wired to the raw provider, not the chain tail.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/        created->initialize\(\);\n        current = created;\n    \}\n\n    return current;/        created->initialize();\n        current = created;\n    }\n\n    return provider;/s' "$SRC"
rebuild
gate_ok 'AdaptedChainTest.BuildsTheChainFromTheSpecAndWiresTheConsumerToItsTail'
check "M2 tail replaced by raw provider" red $?
restore

# M3: the refusal stops naming the missing adapter.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/": no adapted output \\"" \+ step\.id \+/": no adapted output \\"" + std::string("?") +/s' "$SRC"
rebuild
gate_ok 'AdaptedChainTest.RefusesAnUnknownAdapterNamingBothSourcesSearched'
check "M3 refusal loses the name" red $?
restore

trap - EXIT
rebuild; gate_ok 'AdaptedChainTest.*'; check "restored" green $?

note "----"; note "pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
