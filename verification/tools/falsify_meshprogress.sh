#!/bin/bash
# Falsification: watching a meshing run, and stopping one.
#
# Every mutation still reports something and still returns a mesh for a run
# nobody stopped. What they break is whether "stop" is obeyed, whether a
# stopped run leaves anything behind, and whether the phases are reported in
# the order they happen.

set -u
cd "$(dirname "$0")/../.." || exit 1

BUILD=build/darwin
TEST=$BUILD/tests/HydroCoupleSDKTests
FILTER='MeshProgressTest*:TriangulatorTest*:QuadMesherTest*'
LOG=verification/tools/falsify_meshprogress.log
: > "$LOG"

FILES=(src/hydrocoupletools/triangulator.cpp src/hydrocoupletools/quadmesher.cpp)

backup() { for f in "${FILES[@]}"; do cp "$f" "$f.orig"; done; }
restore() { for f in "${FILES[@]}"; do cp "$f.orig" "$f"; touch "$f"; done; }
cleanup() { restore; for f in "${FILES[@]}"; do rm -f "$f.orig"; done; }
trap cleanup EXIT

backup

run_mutation() {
  local name="$1"; shift
  restore
  "$@" || { echo "MUTATION $name: could not be applied" | tee -a "$LOG"; return; }

  local changed=0
  for f in "${FILES[@]}"; do cmp -s "$f" "$f.orig" || changed=1; done
  if [ "$changed" -eq 0 ]; then
    echo "MUTATION $name: NOT APPLIED (stale pattern) -- inconclusive" | tee -a "$LOG"; return
  fi

  for f in "${FILES[@]}"; do touch "$f"; done

  if ! cmake --build $BUILD --target HydroCoupleSDKTests -j 8 >> "$LOG" 2>&1; then
    echo "MUTATION $name: BUILD FAILED (inconclusive)" | tee -a "$LOG"; return
  fi

  local out_file="$LOG.run"
  $TEST --gtest_filter="$FILTER" > "$out_file" 2>&1 &
  local pid=$!; local waited=0
  while kill -0 $pid 2>/dev/null && [ $waited -lt 240 ]; do sleep 1; waited=$((waited+1)); done
  if kill -0 $pid 2>/dev/null; then
    kill -9 $pid 2>/dev/null; wait $pid 2>/dev/null
    echo "MUTATION $name: CAUGHT (hung)" | tee -a "$LOG"; return
  fi
  wait $pid; local status=$?
  local out; out=$(cat "$out_file"); rm -f "$out_file"
  echo "=== $name ===" >> "$LOG"; echo "$out" | grep '^\[  FAILED  \]' >> "$LOG"

  if echo "$out" | grep -q "\[  FAILED  \]"; then
    echo "MUTATION $name: CAUGHT -- $(echo "$out" | grep '^\[  FAILED  \] ' | head -2 | sed 's/\[  FAILED  \] //' | tr '\n' ' ')" | tee -a "$LOG"
  elif [ "$status" -ne 0 ]; then
    echo "MUTATION $name: CAUGHT (the suite died, status $status)" | tee -a "$LOG"
  else
    echo "MUTATION $name: *** SURVIVED ***" | tee -a "$LOG"
  fi
}

# 1. The answer is asked for and thrown away, so Cancel reports progress and
#    changes nothing.
m_answer_ignored_triangulator() {
  perl -0pi -e 's/    return !progress \|\| progress\(fraction, stage\);\n\}\n\n\/\/! What a cancelled run/    if (progress) progress(fraction, stage);\n    return true;\n}\n\n\/\/! What a cancelled run/' src/hydrocoupletools/triangulator.cpp
}

# 2. A stopped run keeps whatever it had built, so a caller that reads the
#    mesh after cancelling gets geometry from a run the user abandoned.
m_cancel_keeps_the_mesh() {
  perl -0pi -e 's/    out     = IO::MeshDefinition\{\};\n    message = "cancelled";/    message = "cancelled";/' src/hydrocoupletools/triangulator.cpp
}

# 3. The run never reports that it finished, so a bar driven by this stops
#    short of the end on every successful run.
m_no_final_report() {
  perl -0pi -e 's/    carryOn\(progress, 1\.0, "done"\);\n\n    message\.clear\(\);\n    return true;\n\}\n\n\} \/\/ namespace HydroCouple::SDK::Tools/    message.clear();\n    return true;\n}\n\n} \/\/ namespace HydroCouple::SDK::Tools/' src/hydrocoupletools/triangulator.cpp
}

# 4. Two phases report fractions in the wrong order, so the bar goes
#    backwards halfway through.
m_fractions_out_of_order() {
  perl -0pi -e 's/        if \(!carryOn\(progress, 0\.20, "inserting vertices"\)\)/        if (!carryOn(progress, 0.85, "inserting vertices"))/' src/hydrocoupletools/triangulator.cpp
}

# 5. A phase is not announced, so the caller shows the previous stage's name
#    through the whole of it.
m_phase_not_announced() {
  perl -0pi -e 's/        if \(!carryOn\(progress, 0\.80, "removing holes"\)\)\n            return cancelled\(out, message\);\n//' src/hydrocoupletools/triangulator.cpp
}

# 6. The merge asks but does not listen, so a quad-dominant run cannot be
#    stopped once the triangulation is done.
m_answer_ignored_quadmesher() {
  perl -0pi -e 's/    return !progress \|\| progress\(fraction, stage\);/    if (progress) progress(fraction, stage);\n    return true;/' src/hydrocoupletools/quadmesher.cpp
}

# 7. A stopped merge keeps its partial output.
m_cancelled_merge_keeps_output() {
  perl -0pi -e 's/    out     = IO::MeshDefinition\{\};\n    message = "cancelled";/    message = "cancelled";/' src/hydrocoupletools/quadmesher.cpp
}

run_mutation "1-cancel-ignored-triangulator"  m_answer_ignored_triangulator
run_mutation "2-cancel-keeps-the-mesh"        m_cancel_keeps_the_mesh
run_mutation "3-no-final-report"              m_no_final_report
run_mutation "4-fractions-out-of-order"       m_fractions_out_of_order
run_mutation "5-phase-not-announced"          m_phase_not_announced
run_mutation "6-cancel-ignored-quadmesher"    m_answer_ignored_quadmesher
run_mutation "7-cancelled-merge-keeps-output" m_cancelled_merge_keeps_output

restore
touch "${FILES[@]}"
cmake --build $BUILD --target HydroCoupleSDKTests -j 8 >> "$LOG" 2>&1
echo
echo "--- restored, clean run ---"
$TEST --gtest_filter="$FILTER" 2>&1 | tail -3
