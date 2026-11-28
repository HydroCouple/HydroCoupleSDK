// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   sidecar.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Sidecar binary references — the IO layer's escape hatch that keeps
 *         large numeric payloads out of the JSON/YAML DOM.
 *
 * Per the efficiency contract (plan §4.7), a serialized argument above a
 * size threshold is represented in the composition document as a compact
 * reference object instead of an inline array:
 *
 * \code{.json}
 * { "@ref": "upstream_coeffs.bin", "kind": "Float64",
 *   "shape": [10000000], "byteOrder": "little" }
 * \endcode
 *
 * The bulk bytes live in the referenced sidecar file as raw contiguous
 * native-endian values, written through the typed hyperslab data plane
 * (IComponentDataItem::getValuesInto into one reused staging buffer — the
 * only copy on the path) and read back with a single bulk
 * setValuesFrom(). Parsing therefore never walks a ten-million-element
 * JSON tree, and memory stays flat.
 *
 * All utilities are inline (header-only) so the typed argument templates
 * can resolve references inside their own initialize() paths, where
 * storage resizing is available.
 *
 * ModelInitializer::serialize() writes sidecars when a sidecar directory is
 * set (ModelInitializer::setSidecarDirectory()), and
 * ModelInitializer::initialize() reads them for any payload isSidecarRef()
 * recognizes. The \c byteOrder key is always written as "little" and is
 * not read back: the file holds the writing machine's native byte order,
 * and the reader copies the bytes without conversion, so a sidecar is
 * portable only between machines of the same byte order.
 *
 * \threadsafety Free functions over caller-owned data; no shared state.
 */

#ifndef HYDROCOUPLESDK_IO_SIDECAR_H
#define HYDROCOUPLESDK_IO_SIDECAR_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"
#include "hydrocouplehelpers.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace HydroCouple::SDK::IO {

/*!
 * \brief Parsed form of an \@ref sidecar reference object.
 *
 * Plain value type filled by parseSidecarRef().
 */
struct SidecarRef
{
    std::filesystem::path path;               //!< Sidecar file (may be relative).
    DataKind              kind = DataKind::Unknown; //!< Element type.
    std::vector<int64_t>  shape;              //!< Row-major extents.

    /*!
     * \brief Number of elements described by \c shape.
     *
     * \return The product of the extents; 1 for an empty shape.
     */
    [[nodiscard]] int64_t elementCount() const noexcept
    {
        int64_t count = 1;
        for (int64_t extent : shape)
            count *= extent;
        return count;
    }
};

/*!
 * \brief Canonical DataKind name used in reference objects.
 *
 * \param[in] kind Element kind.
 * \return "Float64", "Float32", "Int64", "UInt64", "Int32", "UInt32",
 *         "Int16", "UInt16", "Int8", "UInt8" or "Boolean"; "Unknown" for
 *         String, Opaque and any other kind.
 */
[[nodiscard]] inline const char *sidecarKindName(DataKind kind) noexcept
{
    switch (kind)
    {
    case DataKind::Float64: return "Float64";
    case DataKind::Float32: return "Float32";
    case DataKind::Int64:   return "Int64";
    case DataKind::UInt64:  return "UInt64";
    case DataKind::Int32:   return "Int32";
    case DataKind::UInt32:  return "UInt32";
    case DataKind::Int16:   return "Int16";
    case DataKind::UInt16:  return "UInt16";
    case DataKind::Int8:    return "Int8";
    case DataKind::UInt8:   return "UInt8";
    case DataKind::Boolean: return "Boolean";
    default:                return "Unknown";
    }
}

/*!
 * \brief Inverse of sidecarKindName().
 *
 * \param[in] name Kind spelling, matched exactly.
 * \return The kind, or DataKind::Unknown for any other text.
 */
[[nodiscard]] inline DataKind sidecarKindFromName(const std::string &name) noexcept
{
    if (name == "Float64") return DataKind::Float64;
    if (name == "Float32") return DataKind::Float32;
    if (name == "Int64")   return DataKind::Int64;
    if (name == "UInt64")  return DataKind::UInt64;
    if (name == "Int32")   return DataKind::Int32;
    if (name == "UInt32")  return DataKind::UInt32;
    if (name == "Int16")   return DataKind::Int16;
    if (name == "UInt16")  return DataKind::UInt16;
    if (name == "Int8")    return DataKind::Int8;
    if (name == "UInt8")   return DataKind::UInt8;
    if (name == "Boolean") return DataKind::Boolean;
    return DataKind::Unknown;
}

/*!
 * \brief True when \p payload is (or wraps under "values") an \@ref
 *        reference object.
 *
 * Checks only for the key; parseSidecarRef() validates the rest.
 *
 * \param[in] payload One argument payload.
 * \return True for an object with an "\@ref" key, or an object whose
 *         "values" member is such an object.
 */
[[nodiscard]] inline bool isSidecarRef(const nlohmann::json &payload) noexcept
{
    if (payload.is_object() && payload.contains("@ref"))
        return true;
    return payload.is_object() && payload.contains("values") &&
           payload["values"].is_object() &&
           payload["values"].contains("@ref");
}

/*!
 * \brief Parses a reference object (unwrapping "values" when present).
 *
 * Call only on a payload isSidecarRef() accepted. A "kind" that is not a
 * string, or a "shape" holding non-integers, makes nlohmann::json throw
 * \c nlohmann::json::type_error. ModelInitializer::initialize() lets it
 * propagate; ModelInitializer::initializeFromJsonText() and
 * ModelInitializer::initializeFromFile() catch it and report it as a
 * "JSON parse error" (or, for a YAML document, "YAML parse error") message.
 *
 * \param[in]  payload Reference JSON.
 * \param[out] ref     Parsed reference; the path is stored as written
 *                     (the caller resolves a relative one).
 * \param[out] message "Sidecar reference must carry \"\@ref\", \"kind\",
 *                     and \"shape\"" or "Sidecar reference has unknown kind
 *                     \"\<kind\>\"" when returning false; cleared on success.
 * \return False when a required key is missing or the kind is unknown.
 */
[[nodiscard]] inline bool parseSidecarRef(const nlohmann::json &payload,
                                          SidecarRef &ref,
                                          std::string &message)
{
    const nlohmann::json &node =
        (payload.contains("@ref")) ? payload : payload["values"];
    if (!node.contains("@ref") || !node["@ref"].is_string() ||
        !node.contains("kind") || !node.contains("shape") ||
        !node["shape"].is_array())
    {
        message = "Sidecar reference must carry \"@ref\", \"kind\", and "
                  "\"shape\"";
        return false;
    }
    ref.path = node["@ref"].get<std::string>();
    ref.kind = sidecarKindFromName(node["kind"].get<std::string>());
    if (ref.kind == DataKind::Unknown)
    {
        message = "Sidecar reference has unknown kind \"" +
                  node["kind"].get<std::string>() + "\"";
        return false;
    }
    ref.shape = node["shape"].get<std::vector<int64_t>>();
    message.clear();
    return true;
}

/*!
 * \brief Writes an item's full value block to \p sidecarPath and returns
 *        the reference object (with the file name only — the caller decides
 *        how paths are stored/resolved).
 *
 * The bytes are pulled through getValuesInto() into one staging buffer and
 * written in a single stream write — bulk data never enters a JSON DOM.
 * An existing file is overwritten.
 *
 * \param[in]  item        Source item (fixed-width numeric kind, host
 *                         values).
 * \param[in]  sidecarPath Destination file; parents created.
 * \param[out] refOut      The reference object to embed in the document:
 *                         "\@ref" (file name), "kind", "shape" and
 *                         "byteOrder" ("little").
 * \param[out] message     Failure description when returning false
 *                         ("Sidecar export requires a fixed-width
 *                         DataKind", the item's read error, or a stream
 *                         error naming the file); cleared on success.
 * \return True when the file was written.
 */
[[nodiscard]] inline bool writeSidecar(
    const HydroCouple::IComponentDataItem &item,
    const std::filesystem::path &sidecarPath, nlohmann::json &refOut,
    std::string &message)
{
    const DataKind kind        = item.dataKind();
    const int64_t  elementSize = HydroCouple::Helpers::dataKindSize(kind);
    if (elementSize <= 0)
    {
        message = "Sidecar export requires a fixed-width DataKind";
        return false;
    }
    const std::vector<int64_t> shape = item.shape();
    int64_t count = 1;
    for (int64_t extent : shape)
        count *= extent;

    std::vector<std::byte> staging(static_cast<size_t>(count * elementSize));
    BufferDescriptor destination;
    destination.data         = staging.data();
    destination.kind         = kind;
    destination.rank         = static_cast<int32_t>(shape.size());
    destination.shape        = shape.data();
    destination.stridesBytes = nullptr;
    destination.space        = MemorySpace::Host;
    const std::vector<int64_t> start(shape.size(), 0);
    if (!item.getValuesInto(destination, start, shape, &message))
        return false;

    std::error_code ec;
    if (sidecarPath.has_parent_path())
        std::filesystem::create_directories(sidecarPath.parent_path(), ec);
    std::ofstream file(sidecarPath, std::ios::binary | std::ios::trunc);
    if (!file)
    {
        message = "Cannot open sidecar '" + sidecarPath.string() +
                  "' for writing";
        return false;
    }
    file.write(reinterpret_cast<const char *>(staging.data()),
               static_cast<std::streamsize>(staging.size()));
    if (!file)
    {
        message = "Stream failure writing sidecar '" + sidecarPath.string() +
                  "'";
        return false;
    }

    refOut = {{"@ref", sidecarPath.filename().string()},
              {"kind", sidecarKindName(kind)},
              {"shape", shape},
              {"byteOrder", "little"}};
    message.clear();
    return true;
}

/*!
 * \brief Reads a sidecar file into an owned byte buffer, validating its
 *        size against the reference's kind and shape.
 *
 * \param[in]  ref     Parsed reference (path must already be resolved).
 * \param[out] bytes   The file contents.
 * \param[out] message Failure description when returning false ("Cannot
 *                     open sidecar '\<path\>'", "Sidecar '\<path\>' holds N
 *                     bytes but kind/shape describe M", or a stream
 *                     error); cleared on success.
 * \return True when the file size equals element size times element count
 *         and the whole file was read.
 */
[[nodiscard]] inline bool readSidecarBytes(const SidecarRef &ref,
                                           std::vector<std::byte> &bytes,
                                           std::string &message)
{
    const int64_t elementSize = HydroCouple::Helpers::dataKindSize(ref.kind);
    const int64_t expected    = ref.elementCount() * elementSize;

    std::ifstream file(ref.path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        message = "Cannot open sidecar '" + ref.path.string() + "'";
        return false;
    }
    const std::streamsize size = file.tellg();
    if (size != expected)
    {
        message = "Sidecar '" + ref.path.string() + "' holds " +
                  std::to_string(size) + " bytes but kind/shape describe " +
                  std::to_string(expected);
        return false;
    }
    bytes.resize(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char *>(bytes.data()), size);
    if (!file)
    {
        message = "Stream failure reading sidecar '" + ref.path.string() + "'";
        return false;
    }
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_SIDECAR_H
