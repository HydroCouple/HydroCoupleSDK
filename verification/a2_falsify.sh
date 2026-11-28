#!/usr/bin/env bash
# A2 falsifiers: concrete adapters + SdkAdaptedOutputFactory.
set -u
cd "$(dirname "$0")/.."

FACTORY=src/hydrocouplesdk/data/sdkadaptedoutputfactory.cpp
LINEAR=src/hydrocouplesdk/data/lineartransformadaptedoutput.cpp
RELAX=src/hydrocouplesdk/data/relaxationadaptedoutput.cpp
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
restore() { for f in "$FACTORY" "$LINEAR" "$RELAX"; do
                [ -f "$f.bak" ] && mv "$f.bak" "$f" && touch "$f"; done; }

rebuild; gate_ok 'AdaptedOutputTest.*'; check "baseline" green $?
trap 'restore; rebuild' EXIT

# M1: the factory forgets to register the adapter with its provider.
cp "$FACTORY" "$FACTORY.bak"
perl -0pi -e 's/if \(created\)\n        provider->addAdaptedOutput\(created\.get\(\)\);/(void)provider;/s' "$FACTORY"
rebuild
gate_ok 'AdaptedOutputTest.FactoryRegistersTheCreatedAdapterWithItsProvider'
check "M1 registration dropped" red $?
restore

# M2: multiplier and offset swapped.
cp "$LINEAR" "$LINEAR.bak"
perl -0pi -e 's/m_values\[i\] = m_m \* source\[i\] \+ m_b;/m_values[i] = m_b * source[i] + m_m;/s' "$LINEAR"
rebuild
gate_ok 'AdaptedOutputTest.LinearTransformAppliesMultiplierAndOffsetOnRefresh'
check "M2 coefficients swapped" red $?
restore

# M3: the rank-1 Double availability filter is dropped.
cp "$FACTORY" "$FACTORY.bak"
perl -0pi -e 's/if \(!isRankOneDouble\(provider\)\)\n        return \{\};/if (!provider) return {};/s' "$FACTORY"
rebuild
gate_ok 'AdaptedOutputTest.FactoryOffersLinearTransformOnlyForOneDimensionalDoubleOutputs'
check "M3 availability filter dropped" red $?
restore

# M4: the blend is inverted (history and adaptee swapped).
cp "$RELAX" "$RELAX.bak"
perl -0pi -e 's/m_values\[i\] = \(1\.0 - m_lambda\) \* m_values\[i\] \+ m_lambda \* source\[i\];/m_values[i] = (1.0 - m_lambda) * source[i] + m_lambda * m_values[i];/s' "$RELAX"
rebuild
gate_ok 'AdaptedOutputTest.RelaxationBlendsSuccessiveRefreshesByItsFactor'
check "M4 blend inverted" red $?
restore

trap - EXIT
rebuild; gate_ok 'AdaptedOutputTest.*'; check "restored" green $?

note "----"; note "pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
