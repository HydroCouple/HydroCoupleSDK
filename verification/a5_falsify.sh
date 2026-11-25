#!/usr/bin/env bash
# A5 falsifiers: serialize() must reproduce chains, roles, live arguments.
set -u
cd "$(dirname "$0")/.."

SRC=src/hydrocouplesdk/io/modelinitializer.cpp
BUILD=build/darwin
GATE='AdaptedChainTest.SerializeReproducesTheAdaptedChainAndMultiInputRoles'

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

# M1: the adaptee walk is gone — the root never gets past the chain tail.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/                while \(auto \*adapted =\n                           dynamic_cast<HydroCouple::IAdaptedOutput \*>\(root\)\)/                while (auto *adapted =\n                           static_cast<HydroCouple::IAdaptedOutput *>(nullptr))/s' "$SRC"
rebuild
gate_ok; check "M1 adaptee walk removed" red $?
restore

# M2: the role never reaches the document.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/                if \(authored && !authored->role\.empty\(\)\)\n                    to\["role"\] = authored->role;\n//s' "$SRC"
rebuild
gate_ok; check "M2 role recovery dropped" red $?
restore

# M3: authored arguments emitted instead of the live adapter's.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/                        if \(!arguments\.empty\(\)\)\n                            step\["arguments"\] = std::move\(arguments\);/                        if (authoredStep \&\& !authoredStep->arguments.empty())\n                            step["arguments"] = authoredStep->arguments;\n                        else if (!arguments.empty())\n                            step["arguments"] = std::move(arguments);/s' "$SRC"
rebuild
gate_ok; check "M3 stale authored arguments win" red $?
restore

trap - EXIT
rebuild; gate_ok; check "restored" green $?

note "----"; note "pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
