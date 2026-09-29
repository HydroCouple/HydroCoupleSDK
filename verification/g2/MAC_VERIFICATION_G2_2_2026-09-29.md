# Mac verification of G2.2 — 2026-09-29

**Answers:** the G2.2 hand-off (logarithmic checkpointing for the SDK
reverse engine) · **Tree:** HydroCoupleSDK `removing_qt` at `596ac07`,
clean · **Machine:** Darwin 25.6.0, Apple silicon, Apple clang,
`macos-full` preset (`arm64-osx-dynamic`), build dir `build-macos-full`.
No source changes were needed.

## Results

| Suite | Claimed (Linux, lean build) | This Mac (`macos-full`) |
|---|---|---|
| `Differentiation*` | 21 passed | **21 passed** |
| Full `HydroCoupleSDKTests` | 287 passed, 1 skipped, 4 lean-only failures | **350 passed, 1 skipped, 0 failed** (as predicted) |
| `verification/g2/falsify.sh` | 23/23 caught | **23/23 CAUGHT, none by build failure** |

The printed ladder table matches the off-machine numbers exactly:

| N | stored | peak | replays |
|---|---|---|---|
| 10 | 5 | 8 | 15 |
| 100 | 8 | 15 | 344 |
| 1000 | 11 | 21 | 5028 |

Identical counts on a different compiler and standard library are
expected, because the ladder and the bisection are integer schedules with
no floating-point input. The gradients themselves are gated by `==`
against the every-step engine, and those gates pass here too.

Log: `falsify_g22_mac_2026-09-29.log`. After the run the tree was clean
and the rebuilt baseline passed 21/21. The interrupted-run restore added
in `c3fa4cd` was not exercised, because the run was not interrupted.

## Review notes

- `thinLadder`'s claim that a rung dropped now is never needed later
  holds. A rung's distance from the newest rung only grows, so the power
  of two it must be a multiple of only grows. `k = 0` and the newest rung
  always survive, and `backward()` relies on both.
- `reverseRange` erases its temporary midpoint on success and on failure
  of the upper half, so no temporary survives a pass on any path. That is
  the guarantee the removed post-pass sweep used to duplicate.
- The last step is differentiated first, from the newest rung (the state
  after step N − 1). The gaps between rungs are then reversed newest
  first, and a final `nextStep != 0` check reports any step left unswept
  instead of returning a partial gradient.

## Open for review (from the hand-off, not changed here)

`ICheckpointableModelComponent` has no `releaseState(token)`. For a
component whose token names a file, every rung the ladder drops leaves a
file on disk: O(N) disk behind an O(log N) ladder. Adding it is an
additive interface change and the review's decision.
