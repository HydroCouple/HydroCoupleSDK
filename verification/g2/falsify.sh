#!/usr/bin/env bash
# Phase G2 falsifiers: the SDK reverse engine (DifferentiableWorkflow), the
# linear-transform adapter's adjoint, and -- when the build has
# HYDROCOUPLESDK_WITH_TORCH -- the LibTorch layer (rows T*; skipped, and
# reported as skipped, in a build without it).
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
TORCH_TESTS="$BUILD/tests/HydroCoupleSDKTorchTests"
TARGETS="HydroCoupleSDKTests"
if grep -q '^HYDROCOUPLESDK_WITH_TORCH:BOOL=ON' "$BUILD/CMakeCache.txt" 2>/dev/null; then
  TARGETS="$TARGETS HydroCoupleSDKTorchTests"
fi
BACKUP="$(mktemp -d)"
MUTATED=""   # the file currently carrying a planted fault, if any
caught=0; failures=0

# One target per invocation: under the Unix Makefiles generator (the macos
# presets), `--target A B -j 8` is one parallel make whose two sub-makes
# rebuild the shared libraries concurrently. That race read as a spurious
# "does not build" (T3) and a stale binary (T7 SURVIVED) on macOS.
rebuild() {
  : >"$BACKUP/build.log"
  for target in $TARGETS; do
    cmake --build "$BUILD" --target "$target" -j 8 >>"$BACKUP/build.log" 2>&1 || return 1
  done
}

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
  '    if (m_linearizedAt == to)
        return true;   // already the latest update(): nothing to recompute' \
  '    if (true)
        return true;   // already the latest update(): nothing to recompute' "$G"
mutation "E4 adapter bypassed on the way back" $E \
  'cotangent = aResults.values(adapteeAt);' '(void)adapteeAt;' "$G"
mutation "E5 adapter arguments not summed" $E \
  'pass.argumentSums.add(adapted, aArguments[a], aArguments[a]->shape(),' \
  'if (false) pass.argumentSums.add(adapted, aArguments[a], aArguments[a]->shape(),' \
  'DifferentiationTest.ZeroIsNotAGradient'
mutation "E6 composition not restored after backward" $E \
  'if (!restoreAll(finalTokens, restoreWhy) && ok)' 'if (false && ok)' \
  'DifferentiationTest.BackwardLeavesTheCompositionWhereItFoundIt'
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

echo "== G2.2: the checkpoint ladder and bisection"
C='DifferentiationTest.CheckpointsGrowAsTheLogarithmOfTheSteps'
X='DifferentiationTest.TheLadderGivesExactlyTheSameGradients'
mutation "R1 ladder never thinned" $E \
  'if (m_policy != CheckpointPolicy::Logarithmic || m_checkpoints.empty())' \
  'if (true)' "$C"
mutation "R2 ladder over-thinned (drops the newest rung)" $E \
  'if ((k % power) == 0)' 'if ((k % (2 * power)) == 0)' "$C"
mutation "R3 bisection temporaries never dropped" $E \
  '    m_checkpoints.erase(mid);
' '' "$C"
mutation "R4 linear recompute instead of bisection" $E \
  'const int mid = lo + (hi - lo) / 2;' 'const int mid = hi - 1;' "$C"
mutation "R5 peak metric forgets the temporaries" $E \
  '    notePeak(1);   // + the final state backward() will return to
' '' "$C"
mutation "R6 lower half reversed before the upper" $E \
  '    const bool upper = reverseRange(mid, hi, pass, message);
    releaseAll(m_checkpoints[mid]);
    m_checkpoints.erase(mid);
    return upper && reverseRange(lo, mid, pass, message);' \
  '    const bool lower = reverseRange(lo, mid, pass, message);
    releaseAll(m_checkpoints[mid]);
    m_checkpoints.erase(mid);
    return lower && reverseRange(mid, hi, pass, message);' "$X"
mutation "R7 recompute skipped (stale linearization)" $E \
  '    if (m_linearizedAt == to)
        return true;   // already the latest update(): nothing to recompute' \
  '    if (true)
        return true;   // already the latest update(): nothing to recompute' "$X"

echo "== releaseState: every dropped checkpoint is released exactly once"
mutation "X1 a dropped rung is not released" $E \
  '        releaseAll(it->second);
        it = m_checkpoints.erase(it);' '        it = m_checkpoints.erase(it);' \
  'DifferentiationTest.EveryCheckpointTheLadderDropsIsReleased'
mutation "X2 a bisection temporary is not released" $E \
  '    releaseAll(m_checkpoints[mid]);
' '' 'DifferentiationTest.BackwardReleasesItsTemporariesAndTheFinalState'
mutation "X3 the final state is not released" $E \
  '            releaseAll(finalTokens);    // back (or not): it cannot be used again
' '' 'DifferentiationTest.BackwardReleasesItsTemporariesAndTheFinalState'
mutation "X4 clearRecording forgets without releasing" $E \
  '    for (auto &entry : m_checkpoints)
        releaseAll(entry.second);
    m_records.clear();' '    m_records.clear();' \
  'DifferentiationTest.ClearingOrFinishingReleasesTheRecord'
mutation "X5 finish() does not release the record" $E \
  '    clearRecording();   // while the components are still unfinished
' '' 'DifferentiationTest.ClearingOrFinishingReleasesTheRecord'
mutation "X6 a temporary released twice" $E \
  '    releaseAll(m_checkpoints[mid]);
' '    releaseAll(m_checkpoints[mid]);
    releaseAll(m_checkpoints[mid]);
' 'DifferentiationTest.NoTokenIsEverReleasedTwice'

mutation "X7 a failed step's checkpoint is not released" $E \
  '        releaseAll(tokens);
        return false;' '        return false;' \
  'DifferentiationTest.AFailedStepReleasesTheCheckpointItTook'

echo "== rewind()"
mutation "W1 rewind forgets without restoring" $E \
  '    if (!restoreAll(first->second, message))
        return false;
    clearRecording();' '    clearRecording();' \
  'DifferentiationTest.RewindRepeatsTheRunExactly'
mutation "W2 rewind restores but keeps the record" $E \
  '    if (!restoreAll(first->second, message))
        return false;
    clearRecording();' '    if (!restoreAll(first->second, message))
        return false;' \
  'DifferentiationTest.RewindRepeatsTheRunExactly'

if [ "${TARGETS#*Torch}" != "$TARGETS" ]; then
  TESTS="$TORCH_TESTS"
  # Rows restore their file but do not rebuild: the binaries still hold the
  # last mutant until the clean tree is built again.
  rebuild || { echo "BROKEN      baseline no longer builds"; exit 2; }
  "$TESTS" >/dev/null 2>&1 || { echo "baseline Torch gates are not green"; exit 1; }
  A=src/hydrocouplesdk/differentiation/torch/atenmodelcomponent.cpp
  T=src/hydrocouplesdk/differentiation/torch/torchcomposition.cpp

  echo "== G2.3: AtenModelComponent (autograd in place of an adjoint)"
  mutation "T1 linearized after the step, not before" $A \
    '    m_point = std::move(point);' '    point.states = result.states;
    m_point = std::move(point);' \
    'TorchDifferentiationTest.AnAtenVjpIsTheHandWrittenAdjoint'
  mutation "T2 argument results read from the input leaf" $A \
    'return i < 0 ? -1 : static_cast<int>(nStates + nInputs) + i;' \
    'return i < 0 ? -1 : static_cast<int>(nStates) + i;' \
    'TorchDifferentiationTest.AnAtenVjpIsTheHandWrittenAdjoint'
  mutation "T3 jvp ignores the state tangent" $A \
    'if (g[l].defined() && g[l].requires_grad() && tangents[l].defined())' \
    'if (l > 0 && g[l].defined() && g[l].requires_grad() && tangents[l].defined())' \
    'TorchDifferentiationTest.AnAtenJvpIsTheTransposeOfItsVjp'
  mutation "T4 an unreached result is left, not zeroed" $A \
    'if (!writeEntry(e, shapeOf(leaves[l]), grads[l], message))' \
    'if (grads[l].defined() && !writeEntry(e, shapeOf(leaves[l]), grads[l], message))' \
    'TorchDifferentiationTest.WithNoSeedsEveryResultIsWrittenAsZero'
  mutation "T5 restore forgets the step count" $A \
    '    m_steps = static_cast<int>(steps);' '    (void)steps;' \
    'TorchDifferentiationTest.AnAtenCheckpointRestoresExactly'
  mutation "T6 restore keeps the stale linearization" $A \
    '    m_haveStep = false;
    m_point = Point{};
    setStatus(CStatus::Updated);' '    setStatus(CStatus::Updated);' \
    'TorchDifferentiationTest.AnAtenCheckpointRestoresExactly'
  mutation "T7 a bad token half-applied" $A \
        '        writes.emplace_back(item, std::move(t));' \
        '        writeItem(item, t, nullptr);
        writes.emplace_back(item, std::move(t));' \
    'TorchDifferentiationTest.ABadTokenChangesNothing'
  mutation "T8 step() result count unchecked" $A \
    'if (result.states.size() != states.size())' 'if (false)' \
    'TorchDifferentiationTest.AStepThatReturnsTheWrongStatesFailsTheComponent'
  mutation "T9 foreign seed ignored" $A \
    '        if (r < 0)
            return fail(message, "vjp(): " + entryName(e) +' \
    '        if (r < 0) continue;
        if (r < 0)
            return fail(message, "vjp(): " + entryName(e) +' \
    'TorchDifferentiationTest.AnEntryThatIsNotTheComponentsIsRefused'

  echo "== G2.3: TorchComposition (the composition as one autograd node)"
  mutation "T10 seeds taken from the mirrored step" $T \
    'torch::Tensor row = cotangents[o][step - 1].contiguous();' \
    'torch::Tensor row = cotangents[o][m_steps - step].contiguous();' \
    'TorchDifferentiationTest.TorchCompositionGradientsAreTheEngines'
  mutation "T11 adapters do not re-latch their arguments" $T \
    '                adapted->initialize();   // adapters latch their arguments here' \
    '                (void)adapted;' \
    'TorchDifferentiationTest.TorchCompositionRunsAreRepeatable'
  mutation "T12 a stale graph is not refused" $T \
    'TORCH_CHECK(generation == m_generation,' 'TORCH_CHECK(true || generation == m_generation,' \
    'TorchDifferentiationTest.OnlyTheMostRecentRunCanBeDifferentiated'
  mutation "T13 initial-state gradient read as zero" $T \
    '        else if (const StateGradient *g = gradients.findState(b.component, b.state))
            values = &g->values;' '' \
    'TorchDifferentiationTest.TorchCompositionGradientsAreTheEngines'
  A2=src/hydrocouplesdk/differentiation/torch/atenmodelcomponent.cpp
  mutation "T14 a wrong-shaped parameter accepted" $A2 \
    '    if (shapeOf(c) != shape)
        return fail(message, "a tensor of shape "' '    if (false)
        return fail(message, "a tensor of shape "' \
    'TorchDifferentiationTest.AParameterOfTheWrongShapeIsRefusedByName'
else
  echo "== G2.3 Torch rows SKIPPED: this build does not have HYDROCOUPLESDK_WITH_TORCH=ON"
fi

echo
echo "caught=$caught failures=$failures"
[ "$failures" -eq 0 ]
