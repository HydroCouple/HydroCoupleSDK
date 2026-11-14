// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "hydrocouplesdk/io/uriresolver.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace HydroCouple::SDK::IO
{

namespace
{
    UriResolver *g_resolver = nullptr;

    //! Reads a local path whole, or says why it could not.
    bool readLocalFile(const std::string &path, std::string &contents,
                       std::string &message)
    {
        std::ifstream file(path, std::ios::binary);

        if (!file)
        {
            message = "Cannot open file: " + path;
            return false;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        contents = buffer.str();
        return true;
    }

    /*!
     * \brief The local path a `file:` URI names.
     *
     * Handles `file:///path`, `file://localhost/path` and the `file:/path`
     * form; a host other than localhost is somebody else's disk and is left
     * for a resolver to refuse.
     */
    std::string filePathFromUri(const std::string &uri)
    {
        std::string rest = uri.substr(5); // past "file:"

        if (rest.rfind("//", 0) == 0)
        {
            const std::size_t slash = rest.find('/', 2);

            if (slash == std::string::npos)
            {
                return {};
            }

            const std::string host = rest.substr(2, slash - 2);

            if (!host.empty() && host != "localhost")
            {
                return {};
            }

            rest = rest.substr(slash);
        }

        // "/C:/data/x.json" is a Windows path wearing a leading slash.
        if (rest.size() > 2 && rest[0] == '/' &&
            std::isalpha(static_cast<unsigned char>(rest[1])) && rest[2] == ':')
        {
            rest = rest.substr(1);
        }

        return rest;
    }
} // namespace

UriResolver::~UriResolver() = default;

void setUriResolver(UriResolver *resolver)
{
    g_resolver = resolver;
}

UriResolver *uriResolver()
{
    return g_resolver;
}

std::string uriScheme(const std::string &reference)
{
    const std::size_t colon = reference.find(':');

    if (colon < 2 || colon == std::string::npos)
    {
        return {};
    }

    if (!std::isalpha(static_cast<unsigned char>(reference[0])))
    {
        return {};
    }

    for (std::size_t i = 1; i < colon; ++i)
    {
        const char c = reference[i];

        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '+' &&
            c != '-' && c != '.')
        {
            return {};
        }
    }

    std::string scheme = reference.substr(0, colon);
    std::transform(scheme.begin(), scheme.end(), scheme.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return scheme;
}

bool readReference(const std::string &reference, std::string &contents,
                   std::string &message)
{
    const std::string scheme = uriScheme(reference);

    if (scheme.empty())
    {
        return readLocalFile(reference, contents, message);
    }

    if (scheme == "file")
    {
        const std::string path = filePathFromUri(reference);

        if (path.empty())
        {
            message = "Not a local file URI: " + reference;
            return false;
        }

        return readLocalFile(path, contents, message);
    }

    UriResolver *resolver = uriResolver();

    if (!resolver || !resolver->handles(scheme))
    {
        message = "No resolver is installed for '" + scheme +
                  "' URIs, so this argument cannot read " + reference;
        return false;
    }

    return resolver->resolve(reference, contents, message);
}

bool jsonUriReference(const std::string &payload, std::string &uri)
{
    const nlohmann::json parsed =
        nlohmann::json::parse(payload, nullptr, false);

    if (!parsed.is_object() || !parsed.contains(kUriRefKey))
    {
        return false;
    }

    const nlohmann::json &reference = parsed[kUriRefKey];

    if (!reference.is_string())
    {
        return false;
    }

    std::string named = reference.get<std::string>();

    if (named.empty())
    {
        return false;
    }

    uri = std::move(named);

    return true;
}

bool isWritableReference(const std::string &reference)
{
    const std::string scheme = uriScheme(reference);

    return scheme.empty() || scheme == "file";
}

} // namespace HydroCouple::SDK::IO
