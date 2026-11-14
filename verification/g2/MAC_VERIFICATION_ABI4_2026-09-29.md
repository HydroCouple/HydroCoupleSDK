# Mac verification of the SDK port to ABI 4 — 2026-09-29

**Answers:** `plans/hydrocouple/ABI4_MAC_VERIFICATION_HANDOFF_2026-09-29.md`,
checks S1–S6 and C1 · **Tree:** HydroCoupleSDK `removing_qt` at `f16a307`
plus the 53 port files (the 48 of the port, and five carrying the four
off-machine fixes), against HydroCouple's ABI 4 working tree ·
**Machine:** Darwin 25.6.0, Apple silicon, Apple clang, GNU Make 3.81
(`macos-full` preset, Unix Makefiles, `arm64-osx-dynamic`).

**Interface headers used by both builds:**
`HydroCoupleSDK/../HydroCouple/include` (`HYDROCOUPLE_INCLUDE_DIR`, checked
before and after the builds). No interface package was found, and the
stale ABI 3 install in `HydroCouple/install/Darwin` is on no search path.

## Results

| # | Check | Expected | This Mac |
|---|---|---|---|
| S1 | `build-macos-full`, full `HydroCoupleSDKTests` | 358 + 1 skip | **358 passed, 1 skipped, 0 failed**. 100 translation units recompiled against ABI 4. |
| S2 | `Differentiation*` | 29 | **29 passed** |
| S3 | `falsify.sh`, lean build | 32/32, T rows SKIPPED | **32/32 CAUGHT**, none by build failure (after the harness fix below; first run 29/32) |
| S4 | `build-macos-torch`, `HydroCoupleSDKTorchTests` | 20 | **20 passed**; that build's `HydroCoupleSDKTests` is also 358 + 1 skip |
| S5 | the core SDK links no LibTorch | no torch | **none** among `libHydroCoupleSDK.dylib`'s dependencies (see the note) |
| S6 | `falsify.sh`, Torch build | 46/46 | **46/46 CAUGHT**, none by build failure (6 min 13 s) |
| C1 | Composer references (report only) | — | see below |

Logs: `falsify_abi4_lean_mac_2026-09-29.log` (S3),
`falsify_abi4_lean_mac_stale_2026-09-29.log` (S3's failed first run, kept
as evidence) and `falsify_abi4_torch_mac_2026-09-29.log` (S6).

**S5 note.** The hand-off's command, `otool -L … | grep -i torch`, always
prints one line here: the path header `build-macos-torch/libHydroCoupleSDK.dylib:`
matches because of the directory name. None of the dependency lines
mention torch. Only `libHydroCoupleSDKTorch.dylib` links `libtorch`,
`libtorch_cpu` and `libc10`.

`build-macos-torch` links the LibTorch of the `openswmm313` env's torch
wheel (`Torch_DIR`), which is the same 2.14.0 as the `hydrocouple` env's
but a different copy. That does not matter for these checks. It will
matter for plan §5.1 ("one LibTorch per process") once Python and the SDK
Torch layer share a process.

## Harness fix: `verification/g2/falsify.sh`, same-second rebuilds

**Symptom.** The first S3 run gave 29/32, with E10, X2 and W1 reported as
SURVIVED. Applied by hand, E10 is caught: its gate
`WithoutCheckpointingOnlyOneStepDifferentiates` fails. The lean build has
one target, so the parallel-make race fixed in `b84bca7` could not explain
it.

**Cause.** Apple's GNU Make 3.81 compares timestamps to the whole second.
A scratch Makefile proved it. A source written 0.8 ms after its output,
within the same second, read as `'out' is up to date`, and the output kept
the old content. The harness restores one mutant and plants the next
within milliseconds. When a row's rebuild, test and restore fit in one
second, the next mutant is never compiled, and its gate runs the
**previous row's binary**. The exit trap's final rebuild can be skipped
the same way, leaving a mutant binary in the build directory after a run.

**Fix.** `settle()` (a 1 s sleep) runs before every source write: the
plant, the restore after a failed build, the restore after the test, and
the interrupt restore. Each write therefore lands in a later second than
the last build output.

**Effect.** S3 gives 32/32 and S6 gives 46/46, neither with a build-failure
catch. A run costs about 2 s more per row. Ninja builds were never
affected (Ninja records subsecond timestamps).

**Consequence for earlier Mac records.** `MAC_VERIFICATION_G2_3_2026-09-29.md`
put T7's false SURVIVED down to the parallel-make race alone. The race was
real, but this timestamp effect produces the same symptom and was probably
also involved. Every row reported CAUGHT in those runs remains trustworthy,
because the stale-binary effect can only hide a catch, never invent one.

## C1: Composer references to renamed or removed names (not ported)

- `src/layers/dataitemlayer.cpp` is **production code, not only test
  stubs**. It has five uses of `Spatial::MeshDataObjectType` (lines 332,
  334, 445, 448, 455) and one of `patchDimension()` (line 337), so the
  Composer application itself will not build against ABI 4.
- `tests/gui/test_dataitemlayers.cpp` and `tests/gui/spatialstubs.h` are
  known.
- There are no references to `HydroCouple::SDK::Device`,
  `IValueSemantics` or `ITemporalSemantics`. No Composer file imports both
  namespaces behind the `DeviceBackend` ambiguity (P5).

## Pre-existing warnings (not from the port)

`-Wunused-result` in `src/hydrocouplesdk/spatial/raster.cpp:76` (a GDAL
`RasterIO` return) and `tests/hydrocouplesdk/test_ugridreader.cpp:72,104,127`
(`writer.finalize()`). The port touches neither file.
