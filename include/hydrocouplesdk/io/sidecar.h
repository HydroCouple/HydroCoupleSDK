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
 * \brief Parsed form of an \c \@ref sidecar reference object.
 */
struct SidecarRef
{
    std::filesystem::path path;               //!< Sidecar file (may be relative).
    DataKind              kind = DataKind::Unknown; //!< Element type.
    std::vector<int64_t>  shape;              //!< Row-major extents.

    /*!
     * \brief Number of elements described by \c shape.
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
 * \brief True when \p payload is (or wraps under "values") an \c \@ref
 *        reference object.
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
 * \param[in]  payload Reference JSON.
 * \param[out] ref     Parsed reference.
 * \param[out] message Failure description when returning false.
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
 *
 * \param[in]  item        Source item (fixed-width numeric kind).
 * \param[in]  sidecarPath Destination file; parents created.
 * \param[out] refOut      The reference object to embed in the document.
 * \param[out] message     Failure description when returning false.
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
 * \param[in]  ref     Parsed reference (path must already be resolved).
 * \param[out] bytes   The file contents.
 * \param[out] message Failure description when returning false.
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
