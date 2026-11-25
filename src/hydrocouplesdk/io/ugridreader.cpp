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
#include <cmath>
#include <map>
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

bool readVerticalCoordinate(const std::filesystem::path &, const std::string &,
                            size_t, VerticalCoordinate &, std::string &message)
{
    message = "This build has no NetCDF support, so UGRID files cannot be "
              "read.";
    return false;
}

bool hasVerticalCoordinate(const std::filesystem::path &, const std::string &)
{
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
                      int64_t &fillValue, std::string &message,
                      const std::string &rowDimension = "")
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

    if (startIndex != 0 && startIndex != 1) {
        message = name + " start_index must be zero or one.";
        return false;
    }
    if (!rowDimension.empty()) {
        int rowDim = -1;
        if (nc_inq_dimid(ncid, rowDimension.c_str(), &rowDim) != NC_NOERR ||
            (dims[0] != rowDim && dims[1] != rowDim)) {
            message = name + " does not contain declared entity dimension " + rowDimension;
            return false;
        }
        if (dims[1] == rowDim && dims[0] != rowDim) {
            std::vector<int64_t> transposed(values.size());
            for (size_t row = 0; row < rows; ++row)
                for (size_t column = 0; column < columns; ++column)
                    transposed[column * rows + row] = values[row * columns + column];
            values = std::move(transposed);
            std::swap(rows, columns);
        }
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

    // Explicitly declared elevations must be readable, including older
    // node_z/node_elevation attributes used by reservoir files.
    auto elevation = textAttribute(ncid, topologyVar, "node_z");
    if (elevation.empty()) elevation = textAttribute(ncid, topologyVar, "node_elevation");
    if (elevation.empty() && coordinates.size() > 2) elevation = coordinates[2];
    if (!elevation.empty())
    {
        if (!readDoubles(ncid, elevation, mesh.nodeZ, message))
            return false;
        if (mesh.nodeZ.size() != mesh.nodeX.size()) {
            message = "Declared node elevations do not match coordinate count.";
            return false;
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
                              startIndex, fillValue, message,
                              textAttribute(ncid, topologyVar, "face_dimension")))
        {
            return false;
        }

        mesh.faceNodeOffsets.push_back(0);

        for (size_t face = 0; face < rows; ++face)
        {
            bool padding = false;
            for (size_t slot = 0; slot < columns; ++slot)
            {
                const int64_t entry = raw[face * columns + slot];

                // Padding: a rectangular array gives every face the width of
                // the widest, and the short ones are filled to the end.
                if (entry == fillValue)
                {
                    padding = true;
                    continue;
                }
                if (padding || entry < startIndex || entry - startIndex >= mesh.nodeCount()) {
                    message = "Invalid or non-trailing connectivity padding in face " + std::to_string(face);
                    return false;
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
                              startIndex, fillValue, message,
                              textAttribute(ncid, topologyVar, "edge_dimension")))
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

    return mesh.validate(message);
}


namespace {

//! Splits "sigma: a eta: b depth: c" into its key/value pairs.
std::map<std::string, std::string> parseFormulaTerms(const std::string &value)
{
    std::map<std::string, std::string> terms;
    const std::vector<std::string> words = splitNames(value);

    for (size_t index = 0; index + 1 < words.size(); index += 2)
    {
        std::string key = words[index];

        // The conventions write "key: value"; the colon belongs to the key.
        if (!key.empty() && key.back() == ':')
        {
            key.pop_back();
            terms[key] = words[index + 1];
        }
    }

    return terms;
}

//! The sigma coordinate variable of a mesh, or -1.
int findSigmaVariable(int ncid, std::string &formulaTerms)
{
    int variables = 0;

    if (nc_inq_nvars(ncid, &variables) != NC_NOERR)
    {
        return -1;
    }

    for (int varId = 0; varId < variables; ++varId)
    {
        // By what it *is*, not by what it is called: the standard name says
        // this is a sigma coordinate, and formula_terms says what it is
        // built from. A file from another model reads the same way.
        const std::string standardName =
            textAttribute(ncid, varId, "standard_name");

        if (standardName != "ocean_sigma_coordinate" &&
            standardName != "ocean_s_coordinate")
        {
            continue;
        }

        formulaTerms = textAttribute(ncid, varId, "formula_terms");

        if (!formulaTerms.empty())
        {
            return varId;
        }
    }

    return -1;
}

//! Reads a rank-1 double variable by id.
bool readDoublesById(int ncid, int varId, std::vector<double> &values)
{
    int rank = 0;

    if (nc_inq_varndims(ncid, varId, &rank) != NC_NOERR || rank != 1)
    {
        return false;
    }

    int dimId = -1;
    nc_inq_vardimid(ncid, varId, &dimId);

    size_t length = 0;
    nc_inq_dimlen(ncid, dimId, &length);

    values.assign(length, 0.0);

    return nc_get_var_double(ncid, varId, values.data()) == NC_NOERR;
}

/*!
 * \brief Finds interface sigma for \p layers layers.
 *
 * A CF \c bounds attribute is the standard answer and is tried first. Failing
 * that, a rank-1 variable one longer than the layers whose values descend
 * from 0 to -1 *is* an interface sigma, whatever it happens to be called —
 * identifying it by that beats matching a name any one writer chose.
 */
bool findInterfaceSigma(int ncid, int sigmaVar, size_t layers,
                        std::vector<double> &interfaces)
{
    const std::string bounds = textAttribute(ncid, sigmaVar, "bounds");

    if (!bounds.empty())
    {
        int boundsVar = -1;

        if (nc_inq_varid(ncid, bounds.c_str(), &boundsVar) == NC_NOERR)
        {
            int rank = 0;
            nc_inq_varndims(ncid, boundsVar, &rank);

            if (rank == 2)
            {
                std::vector<double> raw(layers * 2, 0.0);

                if (nc_get_var_double(ncid, boundsVar, raw.data()) == NC_NOERR)
                {
                    // Bounds are (layer, 2); consecutive layers share an edge.
                    interfaces.assign(layers + 1, 0.0);
                    interfaces.front() = raw.front();

                    for (size_t layer = 0; layer < layers; ++layer)
                    {
                        interfaces[layer + 1] = raw[layer * 2 + 1];
                    }

                    return true;
                }
            }
        }
    }

    int variables = 0;

    if (nc_inq_nvars(ncid, &variables) != NC_NOERR)
    {
        return false;
    }

    for (int varId = 0; varId < variables; ++varId)
    {
        if (varId == sigmaVar)
        {
            continue;
        }

        std::vector<double> candidate;

        if (!readDoublesById(ncid, varId, candidate) ||
            candidate.size() != layers + 1)
        {
            continue;
        }

        const bool spansTheColumn = std::abs(candidate.front()) < 1.0e-9 &&
                                    std::abs(candidate.back() + 1.0) < 1.0e-9;

        if (!spansTheColumn)
        {
            continue;
        }

        bool descends = true;

        for (size_t index = 1; index < candidate.size(); ++index)
        {
            descends = descends && candidate[index] < candidate[index - 1];
        }

        if (descends)
        {
            interfaces = std::move(candidate);
            return true;
        }
    }

    return false;
}

//! One time slice of a (time, face) variable, or the whole of a (face) one.
bool readSurfaceSlice(int ncid, const std::string &name, size_t timeIndex,
                      std::vector<double> &values, size_t &timeCount,
                      std::string &message)
{
    int varId = -1;

    if (nc_inq_varid(ncid, name.c_str(), &varId) != NC_NOERR)
    {
        message = "The vertical coordinate names a surface elevation "
                  "variable the file does not hold: " + name;
        return false;
    }

    int rank = 0;
    nc_inq_varndims(ncid, varId, &rank);

    if (rank == 1)
    {
        timeCount = 0;

        return readDoublesById(ncid, varId, values);
    }

    if (rank != 2)
    {
        message = name + " is neither a per-face nor a per-face-per-time "
                         "variable.";
        return false;
    }

    int dimIds[2] = {-1, -1};
    nc_inq_vardimid(ncid, varId, dimIds);

    size_t times = 0;
    size_t faces = 0;
    nc_inq_dimlen(ncid, dimIds[0], &times);
    nc_inq_dimlen(ncid, dimIds[1], &faces);

    timeCount = times;

    if (times == 0 || faces == 0)
    {
        message = name + " holds no data.";
        return false;
    }

    // Clamped rather than refused: a viewer asking for the latest time of a
    // run still being written should get the latest there is.
    const size_t slice = timeIndex < times ? timeIndex : times - 1;

    const size_t start[2] = {slice, 0};
    const size_t count[2] = {1, faces};

    values.assign(faces, 0.0);

    if (nc_get_vara_double(ncid, varId, start, count, values.data()) !=
        NC_NOERR)
    {
        message = "The surface elevations in " + name + " could not be read.";
        return false;
    }

    return true;
}

} // namespace

bool readVerticalCoordinate(const std::filesystem::path &path,
                            const std::string &meshName, size_t timeIndex,
                            VerticalCoordinate &vertical,
                            std::string &message)
{
    vertical = {};

    int ncid = -1;

    if (nc_open(path.string().c_str(), NC_NOWRITE, &ncid) != NC_NOERR)
    {
        message = "The file could not be opened: " + path.string();
        return false;
    }

    const FileHandle handle(ncid);

    // The mesh has to exist before its vertical coordinate means anything.
    const std::vector<std::pair<int, std::string>> meshes =
        topologyVariables(ncid);

    if (meshes.empty())
    {
        message = "The file holds no UGRID mesh.";
        return false;
    }

    if (!meshName.empty())
    {
        const bool known =
            std::any_of(meshes.begin(), meshes.end(),
                        [&meshName](const std::pair<int, std::string> &entry)
                        { return entry.second == meshName; });

        if (!known)
        {
            message = "The file holds no mesh named " + meshName + ".";
            return false;
        }
    }

    std::string formulaTerms;
    const int sigmaVar = findSigmaVariable(ncid, formulaTerms);

    if (sigmaVar < 0)
    {
        message = "The file has no CF vertical coordinate; its mesh is "
                  "two-dimensional.";
        return false;
    }

    const std::map<std::string, std::string> terms =
        parseFormulaTerms(formulaTerms);

    const auto sigmaTerm = terms.find("sigma");
    const auto etaTerm = terms.find("eta");
    const auto depthTerm = terms.find("depth");

    if (sigmaTerm == terms.end() || etaTerm == terms.end() ||
        depthTerm == terms.end())
    {
        message = "The vertical coordinate's formula_terms is missing one of "
                  "sigma, eta or depth: " + formulaTerms;
        return false;
    }

    if (!readDoubles(ncid, sigmaTerm->second, vertical.layerSigma, message) ||
        !readDoubles(ncid, depthTerm->second, vertical.depth, message))
    {
        return false;
    }

    if (vertical.layerSigma.empty())
    {
        message = "The vertical coordinate has no layers.";
        return false;
    }

    vertical.surfaceVariable = etaTerm->second;

    if (!readSurfaceSlice(ncid, etaTerm->second, timeIndex, vertical.surface,
                          vertical.timeCount, message))
    {
        return false;
    }

    if (vertical.surface.size() != vertical.depth.size())
    {
        message = "The vertical coordinate has " +
                  std::to_string(vertical.depth.size()) + " depths but " +
                  std::to_string(vertical.surface.size()) +
                  " surface elevations.";
        return false;
    }

    if (!findInterfaceSigma(ncid, sigmaVar, vertical.layerSigma.size(),
                            vertical.interfaceSigma))
    {
        // Midpointing the centres: exact for a uniform distribution, an
        // approximation for a stretched one, and flagged either way.
        vertical.interfacesAreDerived = true;
        vertical.interfaceSigma.assign(vertical.layerSigma.size() + 1, 0.0);
        vertical.interfaceSigma.front() = 0.0;
        vertical.interfaceSigma.back() = -1.0;

        for (size_t layer = 1; layer < vertical.layerSigma.size(); ++layer)
        {
            vertical.interfaceSigma[layer] =
                0.5 * (vertical.layerSigma[layer - 1] +
                       vertical.layerSigma[layer]);
        }
    }

    return true;
}

bool hasVerticalCoordinate(const std::filesystem::path &path,
                           const std::string &meshName)
{
    VerticalCoordinate vertical;
    std::string message;

    return readVerticalCoordinate(path, meshName, 0, vertical, message);
}

#endif // HYDROCOUPLESDK_HAVE_NETCDF

} // namespace HydroCouple::SDK::IO
