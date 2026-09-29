#!/usr/bin/env bash
# Phase G2.1 falsifiers: the SDK reverse engine (DifferentiableWorkflow) and
# the linear-transform adapter's adjoint.
#
# Each row plants one fault and requires the named gtest gate to FAIL.
# SURVIVED = the gate is too weak or the code has nothing to say.
# NOT APPLIED = the code moved; a failure of this script.
# NO SUCH GATE = the filter matches no test (gtest exits 0 on an empty
# filter, which would read as a survivor); a failure of this script.
# A mutant that does not build counts as CAUGHT only if the clean tree
# still builds; otherwise the script stops with BROKEN.
#
# Interrupting it (Ctrl-C, SIGTERM) puts the planted fault back before the
# final rebuild. SIGKILL cannot be caught: after a `kill -9`, run
# `git status` and `git checkout -- src` before trusting the tree.
#
# Usage: BUILD=<cmake build dir> verification/g2/falsify.sh   (default: build)
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
cd "$ROOT"
BUILD="${BUILD:-build}"
PY="${PYTHON:-python3}"
TESTS="$BUILD/tests/HydroCoupleSDKTests"
BACKUP="$(mktemp -d)"
MUTATED=""   # the file currently carrying a planted fault, if any
caught=0; failures=0

rebuild() { cmake --build "$BUILD" --target HydroCoupleSDKTests -j 8 >"$BACKUP/build.log" 2>&1; }

matched() { "$TESTS" --gtest_list_tests --gtest_filter="$1" 2>/dev/null | grep -c '^  '; }

mutation() {  # LABEL FILE OLD NEW FILTER
  local label="$1" file="$2" old="$3" new="$4" filter="$5"
  if [ "$(matched "$filter")" -eq 0 ]; then
    echo "NO SUCH GATE $label ($filter)"; failures=$((failures + 1)); return
  fi
  cp "$file" "$BACKUP/orig"
  MUTATED="$file"
  if ! $PY "$HERE/mutate.py" "$file" "$old" "$new"; then
    MUTATED=""
    echo "NOT APPLIED $label"; failures=$((failures + 1)); return
  fi
  if ! rebuild; then
    cp "$BACKUP/orig" "$file"; MUTATED=""
    if ! rebuild; then echo "BROKEN      baseline no longer builds"; exit 2; fi
    echo "CAUGHT      $label (does not build)"; caught=$((caught + 1)); return
  fi
  if "$TESTS" --gtest_filter="$filter" >/dev/null 2>&1; then
    echo "SURVIVED    $label"; failures=$((failures + 1))
  else
    echo "CAUGHT      $label"; caught=$((caught + 1))
  fi
  cp "$BACKUP/orig" "$file"; MUTATED=""
}

# On any exit -- including Ctrl-C mid-row -- put the planted fault back
# before rebuilding, so an interrupted run cannot leave a bug in src/.
restore() {
  if [ -n "$MUTATED" ] && [ -f "$BACKUP/orig" ]; then
    cp "$BACKUP/orig" "$MUTATED"
    echo "restored $MUTATED after an interrupted row" >&2
  fi
}
trap 'restore; rebuild' EXIT
trap 'exit 130' INT TERM

rebuild || { echo "baseline does not build"; exit 1; }
"$TESTS" --gtest_filter='Differentiation*' >/dev/null 2>&1 || { echo "baseline gates are not green"; exit 1; }

E=src/hydrocouplesdk/differentiation/differentiableworkflow.cpp
L=src/hydrocouplesdk/data/lineartransformadaptedoutput.cpp
G='DifferentiationTest.EveryGradientMatchesFiniteDifferences'

echo "== the reverse engine"
mutation "E1 fan-out overwrites instead of adding" $E \
  'accumulate(outBar[provider], cotangent);' 'outBar[provider] = cotangent;' "$G"
mutation "E2 state cotangent not carried to the previous step" $E \
  'stateCarry = std::move(nextCarry);' 'stateCarry.clear();' "$G"
mutation "E3 superseded steps never replayed" $E \
  '    if (m_linearizedAt == step)
        return true;' '    if (true)
        return true;' "$G"
mutation "E4 adapter bypassed on the way back" $E \
  'cotangent = aResults.values(adapteeAt);' '(void)adapteeAt;' "$G"
mutation "E5 adapter arguments not summed" $E \
  'argumentSums.add(adapted, aArguments[a], aArguments[a]->shape(),' \
  'if (false) argumentSums.add(adapted, aArguments[a], aArguments[a]->shape(),' \
  'DifferentiationTest.ZeroIsNotAGradient'
mutation "E6 composition not restored after backward" $E \
  '        if (!restoreAll(finalTokens, message))
            return false;
        m_linearizedAt = -1;
    }
    message.clear();' '        m_linearizedAt = -1;
    }
    message.clear();' 'DifferentiationTest.BackwardLeavesTheCompositionWhereItFoundIt'
mutation "E7 feedback loops not refused" $E \
  '        if (group.size() < 2)
            continue;' '        if (true)
            continue;' 'DifferentiationTest.AFeedbackLoopIsRefused'
mutation "E8 stateful adapter not refused" $E \
  'if (!dynamic_cast<HydroCouple::IDifferentiableAdaptedOutput *>(adapted))' \
  'if (false)' 'DifferentiationTest.AStatefulAdapterOnTheConnectionIsRefusedByName'
mutation "E9 undifferentiated provider output not refused" $E \
  'if (std::find(outs.begin(), outs.end(), provider) == outs.end())' \
  'if (false)' 'DifferentiationTest.AnOutputItsProviderDoesNotDifferentiateIsRefused'
mutation "E10 missing checkpointing found only mid-pass" $E \
  'if (m_records.size() > 1 && !checkpointable(component))' 'if (false)' \
  'DifferentiationTest.WithoutCheckpointingOnlyOneStepDifferentiates'
mutation "E11 early finish not refused" $E \
  'if (!m_records[step].updated[i])' 'if (false)' \
  'DifferentiationTest.AComponentThatFinishedEarlyIsRefused'
mutation "E12 finished-after-restore not refused" $E \
  'if (record.updated[i] && finishedStatus(order[i]->status()))' 'if (false)' \
  'DifferentiationTest.ARestoreThatLeavesAComponentFinishedIsRefused'
mutation "E13 seed length unchecked" $E \
  'if (elementCount(shape) != static_cast<int64_t>(cotangent.size()))' 'if (false)' \
  'DifferentiationTest.ASeedOfTheWrongLengthIsRefused'

echo "== the linear-transform adjoint"
mutation "L1 dx drops the multiplier" $L \
  'xbar[i] = m_m * ybar[i];' 'xbar[i] = ybar[i];' \
  'DifferentiationTest.TheLinearTransformAdjointMatchesFiniteDifferences'
mutation "L2 dm linearized at the output, not the input" $L \
  'mbar += ybar[i] * m_source[i];' 'mbar += ybar[i] * m_values[i];' \
  'DifferentiationTest.TheLinearTransformAdjointMatchesFiniteDifferences'
mutation "L3 jvp drops the offset" $L \
  'ydot[i] = m_m * xdot[i] + mdot[0] * m_source[i] + bdot[0];' \
  'ydot[i] = m_m * xdot[i] + mdot[0] * m_source[i];' \
  'DifferentiationTest.TheLinearTransformAdjointMatchesFiniteDifferences'

echo
echo "caught=$caught failures=$failures"
[ "$failures" -eq 0 ]
