# Mac verification of G2.3 — 2026-09-29

**Answers:** `plans/sdk/G2_3_HANDOFF_2026-09-29.md` (the optional LibTorch
layer) · **Tree:** HydroCoupleSDK `removing_qt` at `5003ec2`, against
HydroCouple `dev` at `7e6d142` (ABI 3) · **Machine:** Darwin 25.6.0, Apple
silicon, Apple clang, GNU Make 3.81, torch 2.14.0 (pip wheel, CPU/MPS) in
the `openswmm313` conda env.

## Results

| Check | Claimed (Linux) | This Mac |
|---|---|---|
| Lean build (`build-macos-full`): `Differentiation*` | 29 | **29 passed** |
| Lean build: full `HydroCoupleSDKTests` | 358 + 1 skip | **358 passed, 1 skipped, 0 failed** |
| Lean build: `falsify.sh` | 32, T rows SKIPPED | **32/32 CAUGHT**, T rows SKIPPED |
| Torch build (`build-macos-torch`): `HydroCoupleSDKTorchTests` | 20 passed | **20 passed** (about 1.2 s) |
| Torch build: full `HydroCoupleSDKTests` | — | **358 passed, 1 skipped, 0 failed**; `Differentiation*` 29 |
| LibTorch isolation (`otool -L`) | none in the core | **none** in `libHydroCoupleSDK.dylib` or `HydroCoupleSDKTests`; `libtorch`, `libtorch_cpu`, `libc10` only in `libHydroCoupleSDKTorch.dylib` and `HydroCoupleSDKTorchTests` |
| Torch build: `falsify.sh` | 46/46 | **46/46 CAUGHT, none by build failure**, after the harness fix below (first run: 45/46) |

Logs: `falsify_g23_torch_mac_2026-09-29.log` (the 46/46 run) and
`falsify_g23_torch_mac_racy_2026-09-29.log` (the first run, kept as
evidence).

## The hand-off's questions

- **Does plain `find_package(Torch)` work on the Mac?** Yes. It found the
  wheel's `torch/lib/libtorch.dylib` through `torch.utils.cmake_prefix_path`,
  and the `HYDROCOUPLESDK_TORCH_ROOT` fallback was not needed. The only
  message is TorchConfig's `library kineto not found`, which every pip
  wheel produces (Kineto is the profiler) and which is harmless.
- **Does `--preset … -B` take the override?** Yes. The build went to
  `build-macos-torch`, and vcpkg's binary cache made that configure
  take 32 s.
- **Does the vcpkg toolchain interfere with `find_package(Torch)`?** No.
  The call stack shows vcpkg wrapping `find_package`, and the call passes
  through unchanged.
- **Is the Torch falsifier's wall time acceptable?** 4 min 44 s for 46 rows,
  each rebuilding both test targets.

## The harness race the first run exposed (fixed in `falsify.sh`)

The first Torch-build run reported **45/46**:

- **T3** was CAUGHT only as "does not build".
- **T7** ("a bad token half-applied") SURVIVED.

Neither was reproducible by hand. Applied alone and built one target at a
time, the T3 mutant compiles and its gate
(`AnAtenJvpIsTheTransposeOfItsVjp`) fails. The T7 mutant also compiles,
and `ABadTokenChangesNothing` fails on all three storage cells. A
diagnostic print (added and removed) showed that the gate is sound: the
truncation point (character 92 of 185) falls after the storage block
(ending at 66), and the extra `update()` really changes the storage.

The cause was `rebuild()`: `cmake --build … --target HydroCoupleSDKTests
HydroCoupleSDKTorchTests -j 8`. Under the Unix Makefiles generator (the
`macos` presets), that is one parallel `make` whose two sub-makes rebuild
the shared libraries at the same time. That is a known race: it can fail
a good build (T3) or leave a stale binary for the gate to run against
(T7). The Linux run most likely used Ninja, which schedules both targets
in one graph, so it never raced.

**Fix:** `rebuild()` builds each target in its own invocation. For the lean
build, which has one target, nothing changes. With the fix, the Torch run
gave 46/46 with no build-failure catches, and ran faster (284 s against
326 s).

## Review note

The design point the hand-off raises is a single Torch autograd node over
G2.1's engine, instead of one `Function` per component step. It holds up
on this evidence. The all-ATen and mixed compositions give the same 17
gradients as the hand-written adjoints and FD. The ladder gives the same
gradients as EveryStep under Torch. The T10–T14 rows cover the
`TorchComposition` seams (mirrored-step seeds, adapter re-latching, stale
graphs, initial-state gradients, parameter shapes).

## Still open (from the hand-off)

Install/export of `HydroCoupleSDKTorch`, device tensors (the CUDA rows and
the stream question), stateful adapter adjoints, G2b, and G3 proper.
