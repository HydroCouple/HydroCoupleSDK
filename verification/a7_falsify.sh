#!/usr/bin/env bash
# A7 falsifiers: TemporalInterpolationAdaptedOutput.
set -u
cd "$(dirname "$0")/.."

SRC=src/hydrocouplesdk/temporal/temporalinterpolationadaptedoutput.cpp
BUILD=build/darwin
GATE='AdaptedOutputTest.TemporalInterpolationServesTheValueAtTheQueriedTime'

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

# M1: the same-time dedupe is dropped.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/    if \(m_series\.count\(\) > 0 &&\n        times\[static_cast<size_t>\(last\)\] <= m_series\.times\(\)\.back\(\)\)\n        return;\n//s' "$SRC"
rebuild
gate_ok; check "M1 same-time dedupe dropped" red $?
restore

# M2: initialize() stops seeding the adaptee's history.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/    for \(int64_t index = 0; index < timeSeries->timeCount\(\); \+\+index\)\n        appendSample\(index\);/    (void)timeSeries;/s' "$SRC"
rebuild
gate_ok; check "M2 history seeding dropped" red $?
restore

# M3: "previous" quietly maps to linear.
cp "$SRC" "$SRC.bak"
perl -0pi -e 's/    else if \(method == "previous"\)\n        m_series\.setInterpolationMethod\(TimeSeries::InterpolationMethod::Constant\);/    else if (method == "previous")\n        m_series.setInterpolationMethod(TimeSeries::InterpolationMethod::Linear);/s' "$SRC"
rebuild
gate_ok; check "M3 previous maps to linear" red $?
restore

trap - EXIT
rebuild; gate_ok; check "restored" green $?

note "----"; note "pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
