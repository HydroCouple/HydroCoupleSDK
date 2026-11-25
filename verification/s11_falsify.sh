#!/bin/bash
# S1.1 falsifier — Composition Spec v1.1 argument bindings.
# Each mutation breaks one claimed behaviour of compositionspec.cpp; a gate
# must fail (CAUGHT) or the defence is redundant / the gate is weak.
set -u
cd "$(dirname "$0")/.."
SRC=src/hydrocouplesdk/io/compositionspec.cpp
BAK=$SRC.s11bak
GATES='CompositionSpecTest.*'
BUILD="cmake --build build/darwin --target HydroCoupleSDKTests -j 8"
RUN="./build/darwin/tests/HydroCoupleSDKTests --gtest_filter=$GATES"

cp "$SRC" "$BAK"
restore() { cp "$BAK" "$SRC"; }
trap 'restore; rm -f "$BAK"; $BUILD >/dev/null 2>&1' EXIT

verdict() { # name
    if ! $BUILD >/dev/null 2>&1; then echo "M$1: BUILD FAILED"; return; fi
    if $RUN >/dev/null 2>&1; then echo "M$1: SURVIVED"; else echo "M$1: CAUGHT"; fi
}

mutate() { # perl -0pi expression
    restore
    perl -0pi -e "$1" "$SRC"
    if cmp -s "$SRC" "$BAK"; then echo "NOT APPLIED"; return 1; fi
    return 0
}

# M1 cycle detection never runs in parse
mutate 's/if \(!bindingStages\(spec, stages, message\)\)\n            return false;/(void)bindingStages(spec, stages, message);/s' \
    && verdict 1 || echo "M1: NOT APPLIED"

# M2 self-binding accepted
mutate 's/if \(provider == consumerId\)/if (false \&\& provider == consumerId)/' \
    && verdict 2 || echo "M2: NOT APPLIED"

# M3 undeclared provider accepted
mutate 's/if \(!seen\.count\(provider\)\)/if (false \&\& !seen.count(provider))/' \
    && verdict 3 || echo "M3: NOT APPLIED"

# M4 staging ignores bindings: everything lands in stage 0
mutate 's/\+\+waitingOn\[binding\.component\];/(void)binding;/' \
    && verdict 4 || echo "M4: NOT APPLIED"

# M5 toJson never upgrades a 1.0 claim carrying bindings
mutate 's/\(schemaVersion == "1\.0" && !argumentBindings\(\*this\)\.empty\(\)\)\n            \? "1\.1"\n            : schemaVersion;/schemaVersion;/s' \
    && verdict 5 || echo "M5: NOT APPLIED"

# M6 a selector value is silently accepted
mutate 's/if \(!value\.is_null\(\)\)\n            \{\n                message = context \+ "\.\@from\.selector/if (false)\n            {\n                message = context + ".\@from.selector/s' \
    && verdict 6 || echo "M6: NOT APPLIED"

# M7 a binding may carry extra sibling keys
mutate 's/if \(payload\.size\(\) != 1\)/if (false)/' \
    && verdict 7 || echo "M7: NOT APPLIED"

# M8 argumentBindings sees nothing
mutate 's/(std::vector<ArgumentBindingSpec> bindings;)/$1 return bindings;/' \
    && verdict 8 || echo "M8: NOT APPLIED"
