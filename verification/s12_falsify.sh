#!/bin/bash
# S1.2 falsifier — ModelInitializer staged apply + @from resolution.
set -u
cd "$(dirname "$0")/.."
SRC=src/hydrocouplesdk/io/modelinitializer.cpp
BAK=$SRC.s12bak
GATES='ArgumentBindingTest.*:ModelInitializerTest.*'
BUILD="cmake --build build/darwin --target HydroCoupleSDKTests -j 8"
RUN="./build/darwin/tests/HydroCoupleSDKTests --gtest_filter=$GATES"

cp "$SRC" "$BAK"
restore() { cp "$BAK" "$SRC"; }
trap 'restore; rm -f "$BAK"; $BUILD >/dev/null 2>&1' EXIT

verdict() {
    if ! $BUILD >/dev/null 2>&1; then echo "M$1: BUILD FAILED"; return; fi
    if $RUN >/dev/null 2>&1; then echo "M$1: SURVIVED"; else echo "M$1: CAUGHT"; fi
}

mutate() {
    restore
    perl -0pi -e "$1" "$SRC"
    if cmp -s "$SRC" "$BAK"; then return 1; fi
    return 0
}

# M1 staging ignored: everything applies in one document-order stage
mutate 's/if \(!bindingStages\(m_spec, stages, message\)\)\n        return false;/{ std::string ignored; (void)bindingStages(m_spec, stages, ignored); std::vector<std::string> flat; for (const ComponentSpec \&b : m_spec.components) flat.push_back(b.id); stages.clear(); stages.push_back(flat); }/s' \
    && verdict 1 || echo "M1: NOT APPLIED"

# M2 the provider is never actually run
mutate 's/(if \(provider->status\(\) == Status::Finished\)\n        return true;)/$1\n    return true;/s' \
    && verdict 2 || echo "M2: NOT APPLIED"

# M3 a Done provider is never finished
mutate 's/if \(provider->status\(\) == Status::Done\)\n        provider->finish\(\);/\/\/ mutant: no finish/s' \
    && verdict 3 || echo "M3: NOT APPLIED"

# M4 serialize bakes the produced value instead of the reference
mutate 's/if \(previous && previous->arguments\.contains\(argument->id\(\)\) &&\n                isArgumentBinding\(previous->arguments\[argument->id\(\)\]\)\)/if (false)/s' \
    && verdict 4 || echo "M4: NOT APPLIED"

# M5 a Failed provider is reported as fine
mutate 's/if \(provider->status\(\) != Status::Finished\)\n    \{\n        message = context \+ ": provider \\"" \+ providerId \+\n                  "\\" failed while producing/if (false)\n    {\n        message = context + ": provider \\"" + providerId +\n                  "\\" failed while producing/s' \
    && verdict 5 || echo "M5: NOT APPLIED"

# M6 a missing provider output goes unchecked
mutate 's/if \(!output\)\n                    \{\n                        message = context \+ ": provider \\"" \+ providerId \+\n                                  "\\" has no output/if (false)\n                    {\n                        message = context + ": provider \\"" + providerId +\n                                  "\\" has no output/s' \
    && verdict 6 || echo "M6: NOT APPLIED"

# M7 a Finished provider is re-run for every consumer
mutate 's/if \(provider->status\(\) == Status::Finished\)\n        return true;/\/\/ mutant: always re-run/s' \
    && verdict 7 || echo "M7: NOT APPLIED"

# M8 progress never reported
mutate 's/if \(m_progress\)/if (false)/g' \
    && verdict 8 || echo "M8: NOT APPLIED"
