// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   uriresolver.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  UriResolver — how an argument reads something that is not a path.
 *
 * \par Why this is a seam and not a client
 * `IArgument::ArgumentInputType::URL` has existed since v1, and
 * `AbstractArgument::isValidArgType()` has always answered `true` for it
 * whenever an argument declares file filters. Nothing ever fetched: every
 * typed argument treated `URL` exactly as `File` and opened a local
 * `std::ifstream`, so an argument advertised a capability it did not have and
 * an `https:` URL failed as "Cannot open file".
 *
 * The SDK cannot close that gap itself. It has no HTTP client and deliberately
 * no Qt, and adding either would put a transport stack underneath every model
 * that links it. So the SDK defines what resolving a URI means and leaves the
 * doing to whoever hosts the model — HydroCoupleComposer installs one backed
 * by HydroCoupleOgc's Qt fetch tier; a command-line runner may install none at
 * all and get a clear refusal instead of a misleading file error.
 *
 * A resolver is installed once, at start-up, before any component runs. It is
 * process-wide because an argument reached through `initialize()` has no other
 * way to be handed one.
 */

#ifndef HYDROCOUPLESDK_IO_URIRESOLVER_H
#define HYDROCOUPLESDK_IO_URIRESOLVER_H

#include "hydrocouplesdk/export.h"

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

#include <nlohmann/json.hpp>

namespace HydroCouple::SDK::IO
{

/*!
 * \brief Turns a URI into the bytes it names.
 */
class HYDROCOUPLESDK_EXPORT UriResolver
{
public:
    virtual ~UriResolver();

    /*!
     * \brief Whether this resolver handles \p scheme.
     * \param scheme Lower-case scheme without the colon, e.g. "https".
     */
    [[nodiscard]] virtual bool handles(const std::string &scheme) const = 0;

    /*!
     * \brief Reads \p uri, blocking until it has an answer.
     * \param[in]  uri      The URI to read.
     * \param[out] contents The bytes read.
     * \param[out] message  Why it could not be read.
     * \return true when \p contents holds the resource.
     */
    [[nodiscard]] virtual bool resolve(const std::string &uri,
                                       std::string &contents,
                                       std::string &message) = 0;
};

/*!
 * \brief Installs the process-wide resolver; null removes it.
 *
 * The caller keeps ownership and must outlive every component that may read
 * a URI. Call once, before components are initialized.
 */
HYDROCOUPLESDK_EXPORT void setUriResolver(UriResolver *resolver);

/*!
 * \brief The installed resolver, or null when the host installed none.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT UriResolver *uriResolver();

/*!
 * \brief The scheme of \p reference, or empty when it is a plain path.
 *
 * A scheme must be at least two characters, because otherwise every Windows
 * path beginning `C:\` is a URI in the `c` scheme. RFC 3986 does permit a
 * one-letter scheme; no one uses one, and drive letters are everywhere.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string uriScheme(
    const std::string &reference);

/*!
 * \brief Reads \p reference — a local path, a `file:` URI, or a remote URI.
 *
 * Local paths and `file:` URIs are read directly, so a host that installs no
 * resolver still has everything the SDK could do before. Any other scheme
 * needs the installed resolver, and says so when there is none.
 *
 * \param[in]  reference Path or URI.
 * \param[out] contents  The bytes read.
 * \param[out] message   Why it could not be read.
 * \return true when \p contents holds the resource.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool readReference(
    const std::string &reference, std::string &contents, std::string &message);

/*!
 * \brief Whether \p reference names something that can be written back to.
 *
 * A file read from disk can be saved to; a coverage read from a service
 * cannot, and remembering its URI as an output path would write a file named
 * after a URL.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool isWritableReference(
    const std::string &reference);

/*!
 * \brief The key a serialized argument records its source URI under.
 *
 * Sibling of the `@ref` sidecar form in sidecar.h: that one points at bulk
 * bytes beside the document, this one at a resource somewhere else entirely.
 */
inline constexpr const char *kUriRefKey = "@uri";

//! Rebase only an explicit relative @uri against the owning document.
[[nodiscard]] HYDROCOUPLESDK_EXPORT nlohmann::json resolvedArgument(
    const nlohmann::json &payload, const std::filesystem::path &directory);

/*!
 * \brief The URI a payload names, when it names one instead of holding values.
 *
 * \param[in]  payload A serialized argument payload, as JSON text.
 * \param[out] uri     The URI it names.
 * \returns true when \p payload is an object carrying a non-empty `@uri`.
 *          Text that is not JSON at all is not an error here -- the caller
 *          is about to parse it and will say so far better than this can.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool jsonUriReference(
    const std::string &payload, std::string &uri);

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_URIRESOLVER_H
