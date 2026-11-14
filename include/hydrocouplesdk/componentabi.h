/*!
 * \file   componentabi.h
 * \author Caleb Buahin
 * \brief  The convention by which a shared library publishes a HydroCouple
 *         component, and the ABI stamp that makes loading it safe.
 *
 * HydroCouple v2 components are plain C++ objects — there is no QObject and
 * therefore no Qt plugin system to lean on, and the SDK deliberately ships no
 * loader (`ModelInitializer` takes a caller-supplied resolver). A host that
 * wants to load components from disk needs a contract; this header is that
 * contract — authored in HydroCoupleComposer, upstreamed here unchanged so
 * component libraries need no host's headers to publish themselves.
 *
 * A component library exports exactly two C functions:
 *
 *   const char          *hydrocouple_component_abi_v1(void);
 *   HydroCouple::IComponentInfo *hydrocouple_component_info_v1(void);
 *
 * `HYDROCOUPLE_DECLARE_COMPONENT(InfoType)` emits both.
 *
 * \par Why an ABI stamp
 * Passing C++ objects across a shared-library boundary is only defined when
 * both sides were built with the same toolchain and standard library. There
 * is no portable way to *recover* from a mismatch once a mismatched C++
 * function has been called, so the mismatch has to be detected before that
 * happens. `hydrocouple_component_abi_v1` is pure C returning a string, which
 * is safe to call across any mismatch; the host compares it against its own
 * `HYDROCOUPLE_COMPONENT_ABI_STAMP` and refuses the library on disagreement.
 * The stamp is therefore the *only* symbol that may be called before the
 * check succeeds.
 *
 * \par Ownership
 * The `IComponentInfo` returned by `hydrocouple_component_info_v1` is owned by
 * the library (a function-local static) and must not be deleted by the host;
 * it stays valid until the library is unloaded. Component *instances* created
 * through `IModelComponentInfo::createComponentInstance()` are owned by the
 * host, and — as a direct consequence — must be destroyed before the library
 * that allocated them is unloaded.
 */

#ifndef HYDROCOUPLESDK_COMPONENTABI_H
#define HYDROCOUPLESDK_COMPONENTABI_H

#include "hydrocouple.h"

// ── Export/visibility ───────────────────────────────────────────────────────
#if defined(_WIN32)
#  define HYDROCOUPLE_COMPONENT_EXPORT __declspec(dllexport)
#else
#  define HYDROCOUPLE_COMPONENT_EXPORT __attribute__((visibility("default")))
#endif

// ── Stamp construction ──────────────────────────────────────────────────────
#define HYDROCOUPLE_ABI_STRINGIFY_(x) #x
#define HYDROCOUPLE_ABI_STRINGIFY(x)  HYDROCOUPLE_ABI_STRINGIFY_(x)

//! Revision of this loading convention itself (not of the interfaces).
#define HYDROCOUPLE_COMPONENT_ABI_REVISION "1"

/*!
 * \brief Interface major version this convention targets.
 *
 * Stated here rather than read from the interface's version.h: that header
 * exports only generically-named macros (PROJECT_VERSION_MAJOR) which any
 * other project may also define, and hydrocouple.h does not include it. Bump
 * deliberately when the interfaces break ABI.
 */
#define HYDROCOUPLE_COMPONENT_IFACE_VERSION "2"

#if defined(__clang__)
#  define HYDROCOUPLE_ABI_COMPILER "clang-" HYDROCOUPLE_ABI_STRINGIFY(__clang_major__)
#elif defined(__GNUC__)
#  define HYDROCOUPLE_ABI_COMPILER "gcc-" HYDROCOUPLE_ABI_STRINGIFY(__GNUC__)
#elif defined(_MSC_VER)
#  define HYDROCOUPLE_ABI_COMPILER "msvc-" HYDROCOUPLE_ABI_STRINGIFY(_MSC_VER)
#else
#  define HYDROCOUPLE_ABI_COMPILER "unknown"
#endif

#if defined(_LIBCPP_VERSION)
#  define HYDROCOUPLE_ABI_STDLIB "libc++"
#elif defined(__GLIBCXX__)
#  define HYDROCOUPLE_ABI_STDLIB "libstdc++"
#elif defined(_MSVC_STL_VERSION)
#  define HYDROCOUPLE_ABI_STDLIB "msvcstl"
#else
#  define HYDROCOUPLE_ABI_STDLIB "unknown"
#endif

/*!
 * \brief The toolchain fingerprint both sides must agree on.
 *
 * Deliberately coarse: compiler family and major version, standard library,
 * pointer width, and the interface major version. Finer granularity would
 * reject libraries that are in fact compatible; coarser would admit ones that
 * are not.
 */
#define HYDROCOUPLE_COMPONENT_ABI_STAMP                                        \
    "hc-abi/" HYDROCOUPLE_COMPONENT_ABI_REVISION                               \
    ";iface=" HYDROCOUPLE_COMPONENT_IFACE_VERSION                              \
    ";cxx=" HYDROCOUPLE_ABI_COMPILER                                           \
    ";stdlib=" HYDROCOUPLE_ABI_STDLIB                                          \
    ";bits=" HYDROCOUPLE_ABI_STRINGIFY(__SIZEOF_POINTER__)

// ── Entry-point names (kept as string literals for the loader's dlsym) ──────
#define HYDROCOUPLE_COMPONENT_ABI_SYMBOL  "hydrocouple_component_abi_v1"
#define HYDROCOUPLE_COMPONENT_INFO_SYMBOL "hydrocouple_component_info_v1"

/*!
 * \brief The pre-existing, unstamped factory symbol.
 *
 * HydroCouple's Python bindings (`hydrocouple.loader.load`) already load
 * components through an `extern "C"` factory named `CreateComponentInfo`
 * returning `IModelComponentInfo *`, with no ABI stamp. Components written
 * against that convention predate this header, and refusing them would split
 * the component ecosystem in two — Python-loadable versus Composer-loadable.
 *
 * Composer therefore accepts both: the stamped entry points when present, and
 * this legacy factory otherwise. Legacy libraries are loaded on a best-effort
 * basis and reported as unstamped, because there is genuinely no way to
 * verify their toolchain before calling into them.
 */
#define HYDROCOUPLE_COMPONENT_LEGACY_INFO_SYMBOL "CreateComponentInfo"

//! Reported as the stamp of a library that carries no stamp of its own.
#define HYDROCOUPLE_COMPONENT_UNSTAMPED "unstamped(legacy CreateComponentInfo)"

extern "C"
{
  //! Signature of the ABI stamp entry point. Safe to call across a mismatch.
  typedef const char *(*HydroCoupleComponentAbiFn)(void);

  //! Signature of the component-info entry point. Only safe once the stamp matches.
  typedef HydroCouple::IComponentInfo *(*HydroCoupleComponentInfoFn)(void);

  //! Signature of the legacy factory, which returns the narrower model-info type.
  typedef HydroCouple::IModelComponentInfo *(*HydroCoupleLegacyComponentInfoFn)(void);
}

/*!
 * \brief Emits both entry points for a component library.
 * \param InfoType A default-constructible HydroCouple::IComponentInfo subclass.
 *
 * Place once in exactly one translation unit of the component library:
 * \code
 *   HYDROCOUPLE_DECLARE_COMPONENT(MyModelComponentInfo)
 * \endcode
 */
#define HYDROCOUPLE_DECLARE_COMPONENT(InfoType)                                \
  extern "C" HYDROCOUPLE_COMPONENT_EXPORT const char *                         \
  hydrocouple_component_abi_v1(void)                                           \
  {                                                                            \
    return HYDROCOUPLE_COMPONENT_ABI_STAMP;                                    \
  }                                                                            \
                                                                               \
  extern "C" HYDROCOUPLE_COMPONENT_EXPORT HydroCouple::IComponentInfo *        \
  hydrocouple_component_info_v1(void)                                          \
  {                                                                            \
    static InfoType s_info;                                                    \
    return &s_info;                                                            \
  }

#endif // HYDROCOUPLESDK_COMPONENTABI_H
