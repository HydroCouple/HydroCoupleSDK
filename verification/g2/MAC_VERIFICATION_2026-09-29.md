# Mac verification of G2.1 — 2026-09-29

**Answers:** the G2.1 hand-off (reverse-mode differentiation of a
composition in the SDK, without LibTorch) · **Trees:** HydroCouple `dev`
at `99eab6f`; HydroCoupleSDK `removing_qt` at `67cb7b3` plus one test-build
fix (below) · **Machine:** Darwin 25.6.0, Apple silicon, Apple clang,
CMake 4.3.2, vcpkg triplet `arm64-osx-dynamic` (`macos-full` preset).

## Results

| Suite | Claimed (Linux, lean build) | This Mac (`macos-full`) |
|---|---|---|
| HydroCouple `hydrocouple_tests` | 145 passed | **145 passed** |
| HydroCouple Python `pytest` | 181 passed, 3 skipped | **182 passed, 3 skipped** (+1 = an untracked generated `include/version.h` in the include-allowlist glob; same as G0/G1) |
| SDK `--gtest_filter='Differentiation*'` | 16 passed | **16 passed** |
| SDK `HydroCoupleSDKTests` | 280 passed, 1 skipped, 4 failed (lean) | **345 passed, 1 skipped, 0 failed** |
| SDK `verification/g2/falsify.sh` | 16/16 caught | **16/16 CAUGHT, none by build failure** |

The count is 346 rather than the hand-off's "284 + skips" because the full
build compiles the NetCDF, HDF5, GeoPackage, GDAL and tools suites the lean
build omits. The four `ResultsReopenTest` Geometry/Mesh/Surface rows that
failed in the lean build pass here, as the hand-off predicted. The one
skip is `ModelInitializerYaml.YamlFileWithoutSupportFailsClearly` (moot
when yaml-cpp is present).

The SDK configured against the sibling checkout's headers
(`../HydroCouple/include`, which carry `IDifferentiableAdaptedOutput`); no
interface package was found, so no stale install was in play.

Log: `falsify_mac_2026-09-29.log`.

## The fix the `macos-full` build needed (pre-existing, not G2.1)

On `macos-full`, `HydroCoupleSDKTests` **did not link**: undefined
`_H5*` and `_nc_*` symbols from `test_netcdf.cpp` and
`test_ugridreader.cpp`. `tests/CMakeLists.txt` (since `47a4e32`,
2026-08-29) gives the test executable netCDF/HDF5 *headers only*,
relying on `libHydroCoupleSDK` to export their symbols. That holds for
vcpkg's **static** archives, which are linked into the SDK, and linking
them again would put a second copy of HDF5's globals in the process. The
`macos` presets use the **dynamic** triplet, however: netCDF and HDF5 are
their own dylibs, the SDK exports none of their symbols, and the rule
leaves the executable unresolvable. So the hand-off's "`macos-full`
should pass" had never been exercised on this preset.

The fix (`tests/CMakeLists.txt`, `hydrocouplesdk_link_if_shared`) also
links a netCDF/HDF5 target whose `TYPE` is `SHARED_LIBRARY`. A shared
library is one instance however many images link it, so the
duplicate-globals hazard cannot occur. For static archives and INTERFACE
targets the function does nothing, so the static-triplet behaviour the
original comment protects is unchanged. The link produced no
duplicate-library warning.

## Review notes

- `backward` matches the hand-off. Steps and the schedule are walked
  last-first. Each input cotangent travels back through every
  `IDifferentiableAdaptedOutput::vjp` to the root output. Argument
  cotangents are summed over steps, and state cotangents are carried to
  the previous step. Every failure path restores the composition from the
  tokens saved before the pass.
- A component with no seeds at a step is skipped. Its carried state
  cotangent is then absent (zero) for the previous step, which is correct.
- Harness fragility: `falsify.sh`'s `EXIT` trap only rebuilds and does not
  restore a mutated source, so an **interrupted** run leaves a fault
  planted in `src/`. A completed run restores each file itself. Check
  `git status` after any aborted run.
