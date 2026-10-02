# Mac verification of G2.4 and the v2.0.0-alpha.2 release — 2026-10-02

**Answers:** `plans/sdk/G2_4_HANDOFF_2026-10-02.md` (which also covers the
enforcement and bindings-conformance rounds of 2026-10-01) · **Machine:**
macOS (Darwin 27), Apple silicon, Apple clang, macOS SDK 27, Qt 6.9.3;
the `hydrocouple` conda env (Python 3.13.15, numpy 2.5.3, torch 2.14.0,
jax 0.11.2).

Every build was recompiled from clean (`cmake --build --clean-first`), as
the hand-off requires: the ABI stamp cannot tell an old ABI-4 object from a
new one.

| # | Check | This Mac |
|---|---|---|
| 1 | HydroCouple C++ `ctest` | **157/157**; the four named new tests pass |
| 2 | HydroCouple Python | **432 passed, 3 skipped** (CUDA). The hand-off expected 430, but that was a hand-added sum. Without `test_dlpack`/`test_gradients` (the two modules the Linux run skipped) the Mac collects **359, exactly the Linux count**; the two modules add 76. `hydrocouple.__version__` is `2.0.0a2`. |
| 3 | SDK `ctest`, every build dir | `build-macos-full` **415/415**; `build-macos-torch` **437/437**; `build-static-full` **415/415**; `build-asan` (no I/O formats, ASan) **352/356**, where the 4 are the `ResultsReopenTest.*` that fail by design without a format, and ASan reported nothing |
| 4 | `verification/g2/falsify.sh` | `build-macos-full` **53/53**; `build-macos-torch` **67/67**; none by build failure |
| 4b | Enforcement hand-off §2.5 rows | all caught as predicted: transition check disabled (2 of 3 `LifecycleEnforcement` red), connection validation skipped (both `WorkflowConnectionTest` red), elevations copied (the read-only-view test red), `geometry_epoch` deleted (member parity red) |
| 5 | Python `states()` falsifier, and the bindings hand-off's four rows | all caught as predicted (2; 1, 3, 4 and 2 red); every file restored to its original hash |
| 6 | HydroCoupleComposer, rebuilt from clean against fresh installs, `ctest -j1` offscreen | **64/65**. `test_viewhandoff`'s three subtests fail with the same numbers (0.15205…, 0.09217…) as the pre-port baseline. |

Logs: `falsify_g24_lean_mac_2026-10-02.log`, `falsify_g24_torch_mac_2026-10-02.log`.

## Notes

- **vcpkg after the OS update.** The version bump changed `vcpkg.json`, and
  the new compiler hash missed every cached package, so the static triplet
  rebuilt all 48 dependencies. The first attempt **hung for over two hours
  configuring `libjpeg-turbo`** and was killed. A retry went through:
  14 packages left, then the build.
- **The SDK's `build/` (Ninja, lean preset) cannot configure.** Its cache
  turns on NetCDF, HDF5, GDAL, GeoPackage and `BUILD_TOOLS`, but the vcpkg
  manifest features are only `tests`, so SQLite and CDT are never installed.
  It was broken the same way on 2026-09-28, before this round, and it was
  left as found. `build-asan` covers the lean configuration.
- **Composer installs.** The interface was installed from the working tree
  (2.0.0-alpha.2, with `hydrocouplecomponentabi.h`). The SDK was installed
  from `build-static-full`, which is self-contained: its only `@rpath`
  dependency is Tools on the SDK.
