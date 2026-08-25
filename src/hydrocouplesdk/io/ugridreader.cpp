// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   ugridreader.cpp
 * \author Caleb Buahin
 * \brief  NetCDF-UGRID mesh reader.
 */

#include "hydrocouplesdk/io/ugridreader.h"

#ifdef HYDROCOUPLESDK_HAVE_NETCDF
#include <netcdf.h>
#endif

#include <algorithm>
#include <sstream>

namespace HydroCouple::SDK::IO {

bool ugridReadSupported()
{
#ifdef HYDROCOUPLESDK_HAVE_NETCDF
    return true;
#else
    return false;
#endif
}

#ifndef HYDROCOUPLESDK_HAVE_NETCDF

std::vector<std::string> ugridMeshNames(const std::filesystem::path &,
                                        std::string &message)
{
    message = "This build has no NetCDF support, so UGRID files cannot be "
              "read.";
    return {};
}

bool readUGRIDMesh(const std::filesystem::path &, const std::string &,
                   MeshDefinition &, std::string &message)
{
    message = "This build has no NetCDF support, so UGRID files cannot be "
              "read.";
    return false;
}

#else

namespace {

//! Closes the file however the read leaves it.
struct FileHandle
{
    explicit FileHandle(int id) : ncid(id) {}

    ~FileHandle()
    {
        if (ncid >= 0)
        {
            nc_close(ncid);
        }
    }

    FileHandle(const FileHandle &) = delete;
    FileHandle &operator=(const FileHandle &) = delete;

    int ncid = -1;
};

//! Reads a text attribute, or an empty string when it is absent.
std::string textAttribute(int ncid, int varId, const char *name)
{
    size_t length = 0;

    if (nc_inq_attlen(ncid, varId, name, &length) != NC_NOERR || length == 0)
    {
        return {};
    }

    std::string value(length, '\0');

    if (nc_get_att_text(ncid, varId, name, value.data()) != NC_NOERR)
    {
        return {};
    }

    // NetCDF text attributes are not necessarily terminated, and may carry a
    // trailing NUL that would otherwise become part of a variable name.
    while (!value.empty() && value.back() == '\0')
    {
        value.pop_back();
    }

    return value;
}

//! Splits a space-separated attribute, as the conventions write name lists.
std::vector<std::string> splitNames(const std::string &value)
{
    std::vector<std::string> names;
    std::istringstream stream(value);
    std::string name;

    while (stream >> name)
    {
        names.push_back(name);
    }

    return names;
}

//! Every variable carrying cf_role = "mesh_topology".
std::vector<std::pair<int, std::string>> topologyVariables(int ncid)
{
    std::vector<std::pair<int, std::string>> found;

    int variableCount = 0;

    if (nc_inq_nvars(ncid, &variableCount) != NC_NOERR)
    {
        return found;
    }

    for (int varId = 0; varId < variableCount; ++varId)
    {
        if (textAttribute(ncid, varId, "cf_role") != "mesh_topology")
        {
            continue;
        }

        char name[NC_MAX_NAME + 1] = {};

        if (nc_inq_varname(ncid, varId, name) == NC_NOERR)
        {
            found.emplace_back(varId, name);
        }
    }

    return found;
}

//! Reads a whole double variable.
bool readDoubles(int ncid, const std::string &name,
                 std::vector<double> &values, std::string &message)
{
    int varId = -1;

    if (nc_inq_varid(ncid, name.c_str(), &varId) != NC_NOERR)
    {
        message = "The file names a coordinate variable it does not hold: "
                  + name;
        return false;
    }

    int rank = 0;

    if (nc_inq_varndims(ncid, varId, &rank) != NC_NOERR || rank != 1)
    {
        message = name + " is not a one-dimensional coordinate variable.";
        return false;
    }

    int dimId = -1;
    nc_inq_vardimid(ncid, varId, &dimId);

    size_t length = 0;
    nc_inq_dimlen(ncid, dimId, &length);

    values.assign(length, 0.0);

    if (nc_get_var_double(ncid, varId, values.data()) != NC_NOERR)
    {
        message = "The coordinates in " + name + " could not be read.";
        return false;
    }

    return true;
}

//! Reads a rank-2 connectivity variable and its index conventions.
bool readConnectivity(int ncid, const std::string &name,
                      std::vector<int64_t> &values, size_t &rows,
                      size_t &columns, int64_t &startIndex,
                      int64_t &fillValue, std::string &message)
{
    int varId = -1;

    if (nc_inq_varid(ncid, name.c_str(), &varId) != NC_NOERR)
    {
        message = "The file names a connectivity variable it does not hold: "
                  + name;
        return false;
    }

    int rank = 0;

    if (nc_inq_varndims(ncid, varId, &rank) != NC_NOERR || rank != 2)
    {
        message = name + " is not a two-dimensional connectivity variable.";
        return false;
    }

    int dims[2] = {-1, -1};
    nc_inq_vardimid(ncid, varId, dims);

    rows = 0;
    columns = 0;
    nc_inq_dimlen(ncid, dims[0], &rows);
    nc_inq_dimlen(ncid, dims[1], &columns);

    values.assign(rows * columns, 0);

    if (nc_get_var_longlong(
            ncid, varId, reinterpret_cast<long long *>(values.data()))
        != NC_NOERR)
    {
        message = "The connectivity in " + name + " could not be read.";
        return false;
    }

    // A 1-based file is converted here, once, rather than by every caller —
    // an off-by-one in connectivity draws a mesh that is subtly wrong
    // everywhere rather than obviously wrong anywhere.
    startIndex = 0;
    int start = 0;

    if (nc_get_att_int(ncid, varId, "start_index", &start) == NC_NOERR)
    {
        startIndex = start;
    }

    fillValue = -1;
    long long fill = 0;

    if (nc_get_att_longlong(ncid, varId, "_FillValue", &fill) == NC_NOERR)
    {
        fillValue = fill;
    }

    return true;
}

} // namespace

std::vector<std::string> ugridMeshNames(const std::filesystem::path &path,
                                        std::string &message)
{
    int ncid = -1;

    if (nc_open(path.string().c_str(), NC_NOWRITE, &ncid) != NC_NOERR)
    {
        message = "The file could not be opened as NetCDF: " + path.string();
        return {};
    }

    const FileHandle handle(ncid);

    std::vector<std::string> names;

    for (const auto &topology : topologyVariables(ncid))
    {
        names.push_back(topology.second);
    }

    if (names.empty())
    {
        message = "The file holds no UGRID mesh topology variable.";
    }

    return names;
}

bool readUGRIDMesh(const std::filesystem::path &path,
                   const std::string &meshName, MeshDefinition &mesh,
                   std::string &message)
{
    int ncid = -1;

    if (nc_open(path.string().c_str(), NC_NOWRITE, &ncid) != NC_NOERR)
    {
        message = "The file could not be opened as NetCDF: " + path.string();
        return false;
    }

    const FileHandle handle(ncid);

    const std::vector<std::pair<int, std::string>> topologies =
        topologyVariables(ncid);

    if (topologies.empty())
    {
        message = "The file holds no UGRID mesh topology variable.";
        return false;
    }

    auto chosen = topologies.begin();

    if (!meshName.empty())
    {
        chosen = std::find_if(topologies.begin(), topologies.end(),
                              [&meshName](const auto &entry)
                              { return entry.second == meshName; });

        if (chosen == topologies.end())
        {
            message = "The file holds no mesh named " + meshName + ".";
            return false;
        }
    }

    const int topologyVar = chosen->first;

    mesh = MeshDefinition();
    mesh.meshName = chosen->second;

    // ── Node coordinates ──────────────────────────────────────────────────
    const std::vector<std::string> coordinates =
        splitNames(textAttribute(ncid, topologyVar, "node_coordinates"));

    if (coordinates.size() < 2)
    {
        message = "The mesh does not name both node coordinate variables.";
        return false;
    }

    if (!readDoubles(ncid, coordinates[0], mesh.nodeX, message)
        || !readDoubles(ncid, coordinates[1], mesh.nodeY, message))
    {
        return false;
    }

    if (mesh.nodeX.size() != mesh.nodeY.size())
    {
        message = "The mesh's x and y coordinates are of different lengths.";
        return false;
    }

    // A third named coordinate is elevation, which is optional.
    if (coordinates.size() > 2)
    {
        std::string elevationMessage;
        std::vector<double> nodeZ;

        if (readDoubles(ncid, coordinates[2], nodeZ, elevationMessage)
            && nodeZ.size() == mesh.nodeX.size())
        {
            mesh.nodeZ = std::move(nodeZ);
        }
    }

    // ── Faces ─────────────────────────────────────────────────────────────
    const std::string faceVariable =
        textAttribute(ncid, topologyVar, "face_node_connectivity");

    if (!faceVariable.empty())
    {
        std::vector<int64_t> raw;
        size_t rows = 0;
        size_t columns = 0;
        int64_t startIndex = 0;
        int64_t fillValue = -1;

        if (!readConnectivity(ncid, faceVariable, raw, rows, columns,
                              startIndex, fillValue, message))
        {
            return false;
        }

        mesh.faceNodeOffsets.push_back(0);

        for (size_t face = 0; face < rows; ++face)
        {
            for (size_t slot = 0; slot < columns; ++slot)
            {
                const int64_t entry = raw[face * columns + slot];

                // Padding: a rectangular array gives every face the width of
                // the widest, and the short ones are filled to the end.
                if (entry == fillValue)
                {
                    break;
                }

                mesh.faceNodes.push_back(entry - startIndex);
            }

            mesh.faceNodeOffsets.push_back(
                static_cast<int64_t>(mesh.faceNodes.size()));
        }
    }

    // ── Edges ─────────────────────────────────────────────────────────────
    const std::string edgeVariable =
        textAttribute(ncid, topologyVar, "edge_node_connectivity");

    if (!edgeVariable.empty())
    {
        std::vector<int64_t> raw;
        size_t rows = 0;
        size_t columns = 0;
        int64_t startIndex = 0;
        int64_t fillValue = -1;

        if (!readConnectivity(ncid, edgeVariable, raw, rows, columns,
                              startIndex, fillValue, message))
        {
            return false;
        }

        if (columns >= 2)
        {
            for (size_t edge = 0; edge < rows; ++edge)
            {
                mesh.edgeNodes.push_back(
                    {raw[edge * columns] - startIndex,
                     raw[edge * columns + 1] - startIndex});
            }
        }
    }

    if (mesh.nodeCount() == 0)
    {
        message = "The mesh holds no nodes.";
        return false;
    }

    return true;
}

#endif // HYDROCOUPLESDK_HAVE_NETCDF

} // namespace HydroCouple::SDK::IO
