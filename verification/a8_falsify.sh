#!/usr/bin/env bash
# A8 falsifiers: an adapted connection's schedule edge resolves to its root.
#
# TWO mechanisms keep the edge alive — the scheduler's adaptee() walk and
# the SDK base's modelComponent() propagation — so the mutant must remove
# BOTH to prove the gate watches the property (removing either alone is
# covered by the other; that redundancy is deliberate defence for
# third-party adapters the propagation contract does not reach).
set -u
cd "$(dirname "$0")/.."

SCHED=src/hydrocouplesdk/component/timesteppedworkflow.cpp
BASE=src/hydrocouplesdk/data/abstractadaptedoutput.cpp
BUILD=build/darwin
GATE='TimeSteppedWorkflowTest.TheScheduleFollowsAnAdaptedConnectionToItsRootProvider'

pass=0; fail=0
note() { printf '%s\n' "$*"; }
rebuild() {
    # Stale-relink hardening: force the mutated TUs to recompile.
    rm -f "$BUILD/CMakeFiles/HydroCoupleSDK.dir/src/hydrocouplesdk/component/timesteppedworkflow.cpp.o" \
          "$BUILD/CMakeFiles/HydroCoupleSDK.dir/src/hydrocouplesdk/data/abstractadaptedoutput.cpp.o"
    cmake --build "$BUILD" --target HydroCoupleSDKTests >/dev/null 2>&1 || note "REBUILD FAILED"
}
gate_ok() { "$BUILD/tests/HydroCoupleSDKTests" --gtest_filter="$GATE" >/dev/null 2>&1; }
check() {
    local label=$1 expect=$2 rc=$3
    if [ "$expect" = red ] && [ "$rc" -ne 0 ]; then note "CAUGHT   $label"; pass=$((pass+1))
    elif [ "$expect" = green ] && [ "$rc" -eq 0 ]; then note "GREEN    $label"; pass=$((pass+1))
    else note "SURVIVED $label"; fail=$((fail+1)); fi
}
restore() { for f in "$SCHED" "$BASE"; do
                [ -f "$f.bak" ] && mv "$f.bak" "$f" && touch "$f"; done; }

rebuild; gate_ok; check "baseline" green $?
trap 'restore; rebuild' EXIT

# M1: BOTH root-resolution mechanisms removed.
cp "$SCHED" "$SCHED.bak"
cp "$BASE" "$BASE.bak"
perl -0pi -e 's/                while \(auto \*adapted =\n                           dynamic_cast<HydroCouple::IAdaptedOutput \*>\(\n                               provider\)\)\n                    provider = adapted->adaptee\(\);\n\n                if \(!provider\)\n                    continue;\n\n//s' "$SCHED"
perl -0pi -e 's/                                adaptee \? adaptee->modelComponent\(\) : nullptr\)/                                nullptr)/s' "$BASE"
touch "$SCHED" "$BASE"
rebuild
gate_ok; check "M1 both root resolutions removed" red $?
restore

# M2: propagation alone removed — the scheduler's walk must carry the edge.
cp "$BASE" "$BASE.bak"
perl -0pi -e 's/                                adaptee \? adaptee->modelComponent\(\) : nullptr\)/                                nullptr)/s' "$BASE"
touch "$BASE"
rebuild
gate_ok; check "M2 walk alone carries the edge" green $?
restore

trap - EXIT
rebuild; gate_ok; check "restored" green $?

note "----"; note "pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
