#!/usr/bin/env bash
# A6 falsifiers: TimeSteppedWorkflow refreshes adapted outputs.
set -u
cd "$(dirname "$0")/.."

SRC=src/hydrocouplesdk/component/timesteppedworkflow.cpp
BUILD=build/darwin
GATE='TimeSteppedWorkflowTest.TimeSteppedRunRefreshesAdaptedOutputsAfterEachUpdate'

pass=0; fail=0
note() { printf '%s\n' "$*"; }
rebuild() { cmake --build "$BUILD" --target HydroCoupleSDKTests >/dev/null 2>&1; }
gate_ok() { "$BUILD/tests/HydroCoupleSDKTests" --gtest_filter="$GATE" >/dev/null 2>&1; }
check() {
    local label=$1 expect=$2 rc=$3
    if [ "$expect" = red ] && [ "$rc" -ne 0 ]; then note "CAUGHT   $label"; pass=$((pass+1))
    elif [ "$expect" = green ] && [ "$rc" -eq 0 ]; then note "GREEN    $label"; pass=$((pass+1))
    else note "SURVIVED $label"; fail=$((fail+1)); fi
}
restore() { [ -f "$SRC.bak" ] && mv "$SRC.bak" "$SRC" && touch "$SRC"; }

rebuild; gate_ok; check "baseline" green $?
trap 'restore; rebuild' EXIT

# M1: the refresh pass is deleted.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/                for \(HydroCouple::IOutput \*output : component->outputs\(\)\)\n                \{\n                    if \(!output\)\n                        continue;\n                    for \(HydroCouple::IAdaptedOutput \*adapted :\n                         output->adaptedOutputs\(\)\)\n                        if \(adapted\)\n                            adapted->refresh\(\);\n                \}\n//s' "$SRC"
rebuild
gate_ok; check "M1 refresh pass deleted" red $?
restore

# M2: refresh runs BEFORE the component's update (stale values served).
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/                component->update\(\{\}\);/                for (HydroCouple::IOutput *output : component->outputs())\n                {\n                    if (!output)\n                        continue;\n                    for (HydroCouple::IAdaptedOutput *adapted :\n                         output->adaptedOutputs())\n                        if (adapted)\n                            adapted->refresh();\n                }\n                component->update({});/s;
              s/                    return false;\n                \}\n\n                \/\/ The push-driven schedule bypasses.*?\n                \}\n/                    return false;\n                }\n/s' "$SRC"
rebuild
gate_ok; check "M2 refresh before update" red $?
restore

trap - EXIT
rebuild; gate_ok; check "restored" green $?

note "----"; note "pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
