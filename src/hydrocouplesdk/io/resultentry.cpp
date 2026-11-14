// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/io/resultentry.h"

#include <array>

namespace HydroCouple::SDK::IO {

namespace {

struct KindName
{
    DataKind    kind;
    const char *name;
};

// Fixed-width kinds a results catalog can describe. String and Opaque are
// deliberately absent: the writers reject them at capture time.
constexpr std::array<KindName, 11> kKindNames{{
    {DataKind::Int8, "Int8"},       {DataKind::UInt8, "UInt8"},
    {DataKind::Int16, "Int16"},     {DataKind::UInt16, "UInt16"},
    {DataKind::Int32, "Int32"},     {DataKind::UInt32, "UInt32"},
    {DataKind::Int64, "Int64"},     {DataKind::UInt64, "UInt64"},
    {DataKind::Float32, "Float32"}, {DataKind::Float64, "Float64"},
    {DataKind::Boolean, "Boolean"},
}};

} // namespace

std::string dataKindName(DataKind kind)
{
    for (const KindName &entry : kKindNames)
    {
        if (entry.kind == kind)
            return entry.name;
    }
    return "Unknown";
}

bool dataKindFromName(const std::string &name, DataKind &kind)
{
    for (const KindName &entry : kKindNames)
    {
        if (name == entry.name)
        {
            kind = entry.kind;
            return true;
        }
    }
    return false;
}

std::string meshLocationName(MeshLocation location)
{
    switch (location)
    {
        case MeshLocation::Node: return "node";
        case MeshLocation::Edge: return "edge";
        case MeshLocation::Face: return "face";
    }
    return "node";
}

bool meshLocationFromName(const std::string &name, MeshLocation &location)
{
    if (name == "node") { location = MeshLocation::Node; return true; }
    if (name == "edge") { location = MeshLocation::Edge; return true; }
    if (name == "face") { location = MeshLocation::Face; return true; }
    return false;
}

// ── ResultEntry ───────────────────────────────────────────────────────────

int64_t ResultEntry::elementCount() const noexcept
{
    int64_t count = 1;
    for (int64_t extent : shape)
        count *= extent;
    return count;
}

std::string ResultEntry::variableName() const
{
    return variable.empty() ? itemId : variable;
}

nlohmann::json ResultEntry::toJson() const
{
    nlohmann::json entry;
    if (!componentId.empty())
        entry["component"] = componentId;
    entry["item"]     = itemId;
    entry["artifact"] = artifact;
    entry["format"]   = format;
    if (!variable.empty())
        entry["variable"] = variable;
    entry["kind"]  = dataKindName(kind);
    entry["shape"] = shape;
    if (!dimensions.empty())
        entry["dimensions"] = dimensions;
    if (!units.empty())
        entry["units"] = units;
    if (!description.empty())
        entry["description"] = description;
    if (!mesh.empty())
        entry["mesh"] = mesh;
    if (location.has_value())
        entry["location"] = meshLocationName(*location);
    if (time.has_value())
    {
        entry["time"] = {{"count", time->count},
                         {"start", time->start},
                         {"end", time->end},
                         {"units", time->units}};
    }
    if (!sha256.empty())
        entry["sha256"] = sha256;
    return entry;
}

bool ResultEntry::fromJson(const nlohmann::json &json, ResultEntry &entry,
                           std::string &message)
{
    entry = ResultEntry{};

    if (!json.is_object())
    {
        message = "results[] entry must be an object";
        return false;
    }
    for (const char *required : {"item", "artifact", "format", "kind"})
    {
        if (!json.contains(required) || !json[required].is_string())
        {
            message = std::string("results[] entry needs a string \"") +
                      required + "\"";
            return false;
        }
    }

    entry.componentId = json.value("component", "");
    entry.itemId      = json["item"].get<std::string>();
    entry.artifact    = json["artifact"].get<std::string>();
    entry.format      = json["format"].get<std::string>();
    entry.variable    = json.value("variable", "");

    if (!dataKindFromName(json["kind"].get<std::string>(), entry.kind))
    {
        message = "results[\"" + entry.itemId + "\"].kind \"" +
                  json["kind"].get<std::string>() + "\" is not a fixed-width "
                  "DataKind";
        return false;
    }

    if (!json.contains("shape") || !json["shape"].is_array() ||
        json["shape"].empty())
    {
        message = "results[\"" + entry.itemId +
                  "\"] needs a non-empty \"shape\" array";
        return false;
    }
    for (const auto &extent : json["shape"])
    {
        if (!extent.is_number_integer() || extent.get<int64_t>() < 0)
        {
            message = "results[\"" + entry.itemId +
                      "\"].shape must hold non-negative integers";
            return false;
        }
        entry.shape.push_back(extent.get<int64_t>());
    }

    if (json.contains("dimensions"))
    {
        if (!json["dimensions"].is_array())
        {
            message = "results[\"" + entry.itemId +
                      "\"].dimensions must be an array of names";
            return false;
        }
        for (const auto &name : json["dimensions"])
            entry.dimensions.push_back(name.get<std::string>());
        if (!entry.dimensions.empty() &&
            entry.dimensions.size() != entry.shape.size())
        {
            message = "results[\"" + entry.itemId +
                      "\"].dimensions must be parallel to shape (" +
                      std::to_string(entry.dimensions.size()) + " vs " +
                      std::to_string(entry.shape.size()) + ")";
            return false;
        }
    }

    entry.units       = json.value("units", "");
    entry.description = json.value("description", "");
    entry.mesh        = json.value("mesh", "");
    entry.sha256      = json.value("sha256", "");

    if (json.contains("location"))
    {
        MeshLocation location{};
        if (!meshLocationFromName(json["location"].get<std::string>(),
                                  location))
        {
            message = "results[\"" + entry.itemId +
                      "\"].location must be \"node\", \"edge\" or \"face\"";
            return false;
        }
        entry.location = location;
    }

    if (json.contains("time"))
    {
        if (!json["time"].is_object())
        {
            message = "results[\"" + entry.itemId +
                      "\"].time must be an object";
            return false;
        }
        ResultTimeAxis axis;
        axis.count = json["time"].value("count", int64_t{0});
        axis.start = json["time"].value("start", 0.0);
        axis.end   = json["time"].value("end", 0.0);
        axis.units = json["time"].value("units", std::string("julian_day"));
        entry.time = axis;
    }

    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::IO
