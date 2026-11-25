#!/usr/bin/env bash
# A1 falsifiers: spec equality must see chain and argument content.
set -u
cd "$(dirname "$0")/.."

HDR=include/hydrocouplesdk/io/compositionspec.h
BUILD=build/darwin
GATE='CompositionSpecTest.ConnectionSpecsCompareByValueIncludingAdapterChains'

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

rebuild; gate_ok; check "baseline" green $?

cp "$HDR" "$HDR.bak"
trap '[ -f "$HDR.bak" ] && mv "$HDR.bak" "$HDR" && touch "$HDR"; rebuild' EXIT

# M1: ConnectionSpec equality forgets the adaptation chain.
perl -0pi -e 's/\[\[nodiscard\]\] bool operator==\(const ConnectionSpec &\) const = default;/[[nodiscard]] bool operator==(const ConnectionSpec \&o) const { return fromComponent==o.fromComponent \&\& output==o.output \&\& toComponent==o.toComponent \&\& input==o.input \&\& role==o.role; }/s' "$HDR"
rebuild
gate_ok; check "M1 == ignores adaptedOutputs" red $?
cp "$HDR.bak" "$HDR"; touch "$HDR"

# M2: ComponentSpec equality forgets the arguments JSON.
perl -0pi -e 's/\[\[nodiscard\]\] bool operator==\(const ComponentSpec &\) const = default;/[[nodiscard]] bool operator==(const ComponentSpec \&o) const { return id==o.id \&\& caption==o.caption \&\& info==o.info \&\& mode==o.mode \&\& resultsManifest==o.resultsManifest; }/s' "$HDR"
rebuild
gate_ok; check "M2 == ignores arguments" red $?

mv "$HDR.bak" "$HDR"; touch "$HDR"
trap - EXIT
rebuild; gate_ok; check "restored" green $?

note "----"; note "pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
