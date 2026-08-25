// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file netcdfugridwriter.cpp
 * \brief NetCDFUGRIDWriter implementation over the NetCDF-4 C API.
 */

#include "stdafx.h"
#include "hydrocouplesdk/io/netcdfugridwriter.h"

#include <netcdf.h>

namespace HydroCouple::SDK::IO {

namespace {

constexpr long long kFillValue    = -1;
constexpr int       kDeflateLevel = 4;

/*! Evaluates a NetCDF call, capturing nc_strerror on failure. */
#define HC_NC_CHECK(call, errorOut, failCode)                                \
    do                                                                       \
    {                                                                        \
        const int hcNcStatus = (call);                                       \
        if (hcNcStatus != NC_NOERR)                                          \
        {                                                                    \
            (errorOut) = std::string(#call) + ": " +                         \
                         nc_strerror(hcNcStatus);                            \
            return (failCode);                                               \
        }                                                                    \
    } while (false)

} // namespace

struct NetCDFUGRIDWriter::Impl
{
    struct Field
    {
        std::string  itemId;
        MeshLocation location = MeshLocation::Node;
        std::string  units;
        std::string  standardName;
        int          varId = -1;
        int64_t      count = 0; //!< Values per time level.
    };

    std::filesystem::path path;
    MeshDefinition        mesh;
    std::vector<Field>    fields;
    std::string           error;

    int    ncid       = -1;
    int    timeDim    = -1;
    int    timeVar    = -1;
    size_t timeLevels = 0;
    bool   open       = false;

    //! CF vertical coordinate, when the file is a layered one.
    std::vector<double> interfaceSigma;
    std::vector<double> depth;
    std::vector<double> surface;

    [[nodiscard]] bool layered() const
    {
        return interfaceSigma.size() >= 2 && !depth.empty();
    }

    double firstTime = 0.0; //!< Time axis bounds for catalog().
    double lastTime  = 0.0;

    std::vector<double> staging; //!< Reused conversion buffer.

    [[nodiscard]] int64_t locationCount(MeshLocation location) const noexcept
    {
        switch (location)
        {
        case MeshLocation::Node: return mesh.nodeCount();
        case MeshLocation::Edge: return mesh.edgeCount();
        case MeshLocation::Face: return mesh.faceCount();
        }
        return 0;
    }

    static const char *locationName(MeshLocation location) noexcept
    {
        switch (location)
        {
        case MeshLocation::Node: return "node";
        case MeshLocation::Edge: return "edge";
        case MeshLocation::Face: return "face";
        }
        return "node";
    }

    /*! Location-count dimension ids, created on demand. */
    int nodeDim = -1, edgeDim = -1, faceDim = -1;

    int locationDim(MeshLocation location) const noexcept
    {
        switch (location)
        {
        case MeshLocation::Node: return nodeDim;
        case MeshLocation::Edge: return edgeDim;
        case MeshLocation::Face: return faceDim;
        }
        return nodeDim;
    }
};

// ── Construction ──────────────────────────────────────────────────────────

NetCDFUGRIDWriter::NetCDFUGRIDWriter(std::filesystem::path path,
                                     MeshDefinition mesh)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->path = std::move(path);
    m_impl->mesh = std::move(mesh);
}

NetCDFUGRIDWriter::~NetCDFUGRIDWriter()
{
    (void)finalize();
}

void NetCDFUGRIDWriter::addField(const std::string &itemId,
                                 MeshLocation location,
                                 const std::string &units,
                                 const std::string &standardName)
{
    Impl::Field field;
    field.itemId       = itemId;
    field.location     = location;
    field.units        = units;
    field.standardName = standardName;
    m_impl->fields.push_back(std::move(field));
}

std::string NetCDFUGRIDWriter::name() const
{
    return "netcdf-ugrid";
}

const std::filesystem::path &NetCDFUGRIDWriter::path() const noexcept
{
    return m_impl->path;
}

std::string NetCDFUGRIDWriter::errorMessage() const
{
    return m_impl->error;
}

void NetCDFUGRIDWriter::setVerticalCoordinate(
    std::vector<double> interfaceSigma, std::vector<double> depth,
    std::vector<double> surface)
{
    m_impl->interfaceSigma = std::move(interfaceSigma);
    m_impl->depth = std::move(depth);
    m_impl->surface = std::move(surface);
}

// ── initialize ────────────────────────────────────────────────────────────

int NetCDFUGRIDWriter::initialize()
{
    Impl &impl = *m_impl;
    const MeshDefinition &mesh = impl.mesh;

    if (!mesh.validate(impl.error))
        return 50;

    std::error_code ec;
    if (impl.path.has_parent_path())
        std::filesystem::create_directories(impl.path.parent_path(), ec);

    HC_NC_CHECK(nc_create(impl.path.string().c_str(),
                          NC_NETCDF4 | NC_CLOBBER, &impl.ncid),
                impl.error, 51);
    impl.open = true;

    const auto putGlobalText = [&](const char *attribute,
                                   const std::string &value) {
        return nc_put_att_text(impl.ncid, NC_GLOBAL, attribute, value.size(),
                               value.c_str());
    };
    HC_NC_CHECK(putGlobalText("Conventions", "CF-1.11 UGRID-1.0"),
                impl.error, 52);
    HC_NC_CHECK(putGlobalText("title", "HydroCoupleSDK output"),
                impl.error, 52);
    HC_NC_CHECK(putGlobalText("source", "HydroCoupleSDK NetCDFUGRIDWriter"),
                impl.error, 52);

    // Dimensions.
    HC_NC_CHECK(nc_def_dim(impl.ncid, "time", NC_UNLIMITED, &impl.timeDim),
                impl.error, 53);
    HC_NC_CHECK(nc_def_dim(impl.ncid, (mesh.meshName + "_nNodes").c_str(),
                           static_cast<size_t>(mesh.nodeCount()),
                           &impl.nodeDim),
                impl.error, 53);
    int maxFaceNodesDim = -1;
    if (mesh.faceCount() > 0)
    {
        HC_NC_CHECK(nc_def_dim(impl.ncid, (mesh.meshName + "_nFaces").c_str(),
                               static_cast<size_t>(mesh.faceCount()),
                               &impl.faceDim),
                    impl.error, 53);
        HC_NC_CHECK(nc_def_dim(impl.ncid,
                               (mesh.meshName + "_maxFaceNodes").c_str(),
                               static_cast<size_t>(mesh.maxNodesPerFace()),
                               &maxFaceNodesDim),
                    impl.error, 53);
    }
    int twoDim = -1;
    if (mesh.edgeCount() > 0)
    {
        HC_NC_CHECK(nc_def_dim(impl.ncid, (mesh.meshName + "_nEdges").c_str(),
                               static_cast<size_t>(mesh.edgeCount()),
                               &impl.edgeDim),
                    impl.error, 53);
        HC_NC_CHECK(nc_def_dim(impl.ncid, "Two", 2, &twoDim), impl.error, 53);
    }

    const auto putVarText = [&](int varId, const char *attribute,
                                const std::string &value) {
        return nc_put_att_text(impl.ncid, varId, attribute, value.size(),
                               value.c_str());
    };

    // Mesh-topology dummy variable.
    int topologyVar = -1;
    HC_NC_CHECK(nc_def_var(impl.ncid, mesh.meshName.c_str(), NC_INT, 0,
                           nullptr, &topologyVar),
                impl.error, 54);
    HC_NC_CHECK(putVarText(topologyVar, "cf_role", "mesh_topology"),
                impl.error, 54);
    {
        const int topologyDimension = mesh.faceCount() > 0 ? 2 : 1;
        HC_NC_CHECK(nc_put_att_int(impl.ncid, topologyVar,
                                   "topology_dimension", NC_INT, 1,
                                   &topologyDimension),
                    impl.error, 54);
    }
    {
        // Every node coordinate variable this file writes is named here.
        // Listing only x and y left the elevation written but undiscoverable:
        // a conventions-driven reader looks for coordinates through this
        // attribute and would never find _node_z.
        std::string nodeCoordinates =
            mesh.meshName + "_node_x " + mesh.meshName + "_node_y";

        if (!mesh.nodeZ.empty())
        {
            nodeCoordinates += " " + mesh.meshName + "_node_z";
        }

        HC_NC_CHECK(putVarText(topologyVar, "node_coordinates",
                               nodeCoordinates),
                    impl.error, 54);
    }
    if (mesh.faceCount() > 0)
        HC_NC_CHECK(putVarText(topologyVar, "face_node_connectivity",
                               mesh.meshName + "_face_nodes"),
                    impl.error, 54);
    if (mesh.edgeCount() > 0)
        HC_NC_CHECK(putVarText(topologyVar, "edge_node_connectivity",
                               mesh.meshName + "_edge_nodes"),
                    impl.error, 54);

    // Node coordinates.
    int nodeXVar = -1, nodeYVar = -1, nodeZVar = -1;
    HC_NC_CHECK(nc_def_var(impl.ncid, (mesh.meshName + "_node_x").c_str(),
                           NC_DOUBLE, 1, &impl.nodeDim, &nodeXVar),
                impl.error, 55);
    HC_NC_CHECK(putVarText(nodeXVar, "standard_name",
                           "projection_x_coordinate"),
                impl.error, 55);
    HC_NC_CHECK(nc_def_var(impl.ncid, (mesh.meshName + "_node_y").c_str(),
                           NC_DOUBLE, 1, &impl.nodeDim, &nodeYVar),
                impl.error, 55);
    HC_NC_CHECK(putVarText(nodeYVar, "standard_name",
                           "projection_y_coordinate"),
                impl.error, 55);
    if (!mesh.nodeZ.empty())
    {
        HC_NC_CHECK(nc_def_var(impl.ncid, (mesh.meshName + "_node_z").c_str(),
                               NC_DOUBLE, 1, &impl.nodeDim, &nodeZVar),
                    impl.error, 55);
        HC_NC_CHECK(putVarText(nodeZVar, "standard_name", "altitude"),
                    impl.error, 55);
    }

    // Connectivity.
    int faceNodesVar = -1;
    if (mesh.faceCount() > 0)
    {
        const int dims[2] = {impl.faceDim, maxFaceNodesDim};
        HC_NC_CHECK(nc_def_var(impl.ncid,
                               (mesh.meshName + "_face_nodes").c_str(),
                               NC_INT64, 2, dims, &faceNodesVar),
                    impl.error, 56);
        HC_NC_CHECK(putVarText(faceNodesVar, "cf_role",
                               "face_node_connectivity"),
                    impl.error, 56);
        {
            const int startIndex = 0;
            HC_NC_CHECK(nc_put_att_int(impl.ncid, faceNodesVar, "start_index",
                                       NC_INT, 1, &startIndex),
                        impl.error, 56);
            HC_NC_CHECK(nc_put_att_longlong(impl.ncid, faceNodesVar,
                                            "_FillValue", NC_INT64, 1,
                                            &kFillValue),
                        impl.error, 56);
        }
    }
    int edgeNodesVar = -1;
    if (mesh.edgeCount() > 0)
    {
        const int dims[2] = {impl.edgeDim, twoDim};
        HC_NC_CHECK(nc_def_var(impl.ncid,
                               (mesh.meshName + "_edge_nodes").c_str(),
                               NC_INT64, 2, dims, &edgeNodesVar),
                    impl.error, 56);
        HC_NC_CHECK(putVarText(edgeNodesVar, "cf_role",
                               "edge_node_connectivity"),
                    impl.error, 56);
    }

    // ── CF vertical coordinate ────────────────────────────────────────
    //
    // The topology stays two-dimensional, as UGRID intends; the water column
    // is a sigma coordinate over it. formula_terms is what makes the file
    // self-describing: it names the three variables an elevation is built
    // from, so a reader needs no agreement with this writer about names.
    int sigmaVar = -1, interfaceVar = -1, depthVar = -1, surfaceVar = -1;

    if (impl.layered())
    {
        const size_t layers = impl.interfaceSigma.size() - 1;

        int layerDim = -1, interfaceDim = -1;
        HC_NC_CHECK(nc_def_dim(impl.ncid,
                               (mesh.meshName + "_nLayers").c_str(), layers,
                               &layerDim),
                    impl.error, 53);
        HC_NC_CHECK(nc_def_dim(impl.ncid,
                               (mesh.meshName + "_nInterfaces").c_str(),
                               layers + 1, &interfaceDim),
                    impl.error, 53);

        const std::string sigmaName = mesh.meshName + "_layer_sigma";
        const std::string interfaceName = mesh.meshName + "_interface_sigma";
        const std::string depthName = mesh.meshName + "_depth";
        const std::string surfaceName = mesh.meshName + "_eta";

        HC_NC_CHECK(nc_def_var(impl.ncid, sigmaName.c_str(), NC_DOUBLE, 1,
                               &layerDim, &sigmaVar),
                    impl.error, 56);
        HC_NC_CHECK(nc_def_var(impl.ncid, interfaceName.c_str(), NC_DOUBLE, 1,
                               &interfaceDim, &interfaceVar),
                    impl.error, 56);
        HC_NC_CHECK(nc_def_var(impl.ncid, depthName.c_str(), NC_DOUBLE, 1,
                               &impl.faceDim, &depthVar),
                    impl.error, 56);

        const int surfaceDims[2] = {impl.timeDim, impl.faceDim};
        HC_NC_CHECK(nc_def_var(impl.ncid, surfaceName.c_str(), NC_DOUBLE, 2,
                               surfaceDims, &surfaceVar),
                    impl.error, 56);

        HC_NC_CHECK(putVarText(sigmaVar, "standard_name",
                               "ocean_sigma_coordinate"),
                    impl.error, 56);
        HC_NC_CHECK(putVarText(sigmaVar, "formula_terms",
                               "sigma: " + sigmaName + " eta: " + surfaceName
                                   + " depth: " + depthName),
                    impl.error, 56);
        HC_NC_CHECK(putVarText(sigmaVar, "positive", "up"), impl.error, 56);
        HC_NC_CHECK(putVarText(sigmaVar, "bounds", interfaceName),
                    impl.error, 56);
        HC_NC_CHECK(putVarText(interfaceVar, "long_name",
                               "sigma at layer interfaces"),
                    impl.error, 56);
        HC_NC_CHECK(putVarText(depthVar, "standard_name",
                               "sea_floor_depth_below_geoid"),
                    impl.error, 56);
        HC_NC_CHECK(putVarText(depthVar, "positive", "down"), impl.error, 56);
        HC_NC_CHECK(putVarText(depthVar, "units", "m"), impl.error, 56);
        HC_NC_CHECK(putVarText(surfaceVar, "units", "m"), impl.error, 56);
        HC_NC_CHECK(putVarText(surfaceVar, "positive", "up"), impl.error, 56);
        HC_NC_CHECK(putVarText(surfaceVar, "mesh", mesh.meshName),
                    impl.error, 56);
        HC_NC_CHECK(putVarText(surfaceVar, "location", "face"),
                    impl.error, 56);
    }

    // Time coordinate.
    HC_NC_CHECK(nc_def_var(impl.ncid, "time", NC_DOUBLE, 1, &impl.timeDim,
                           &impl.timeVar),
                impl.error, 57);
    HC_NC_CHECK(putVarText(impl.timeVar, "standard_name", "time"),
                impl.error, 57);
    HC_NC_CHECK(putVarText(impl.timeVar, "units", "days since -4713-01-01"),
                impl.error, 57);
    HC_NC_CHECK(putVarText(impl.timeVar, "calendar", "julian"),
                impl.error, 57);

    // Registered fields.
    for (Impl::Field &field : impl.fields)
    {
        field.count = impl.locationCount(field.location);
        if (field.count <= 0)
        {
            impl.error = "field '" + field.itemId + "' registered on a " +
                         std::string(Impl::locationName(field.location)) +
                         " location, but the mesh has none";
            return 58;
        }
        const int dims[2] = {impl.timeDim, impl.locationDim(field.location)};
        HC_NC_CHECK(nc_def_var(impl.ncid, field.itemId.c_str(), NC_DOUBLE, 2,
                               dims, &field.varId),
                    impl.error, 58);
        HC_NC_CHECK(nc_def_var_deflate(impl.ncid, field.varId, /*shuffle=*/1,
                                       /*deflate=*/1, kDeflateLevel),
                    impl.error, 58);
        HC_NC_CHECK(putVarText(field.varId, "mesh", mesh.meshName),
                    impl.error, 58);
        HC_NC_CHECK(putVarText(field.varId, "location",
                               Impl::locationName(field.location)),
                    impl.error, 58);
        if (!field.units.empty())
            HC_NC_CHECK(putVarText(field.varId, "units", field.units),
                        impl.error, 58);
        if (!field.standardName.empty())
            HC_NC_CHECK(putVarText(field.varId, "standard_name",
                                   field.standardName),
                        impl.error, 58);
    }

    HC_NC_CHECK(nc_enddef(impl.ncid), impl.error, 59);

    // Static payloads.
    HC_NC_CHECK(nc_put_var_double(impl.ncid, nodeXVar, mesh.nodeX.data()),
                impl.error, 59);
    HC_NC_CHECK(nc_put_var_double(impl.ncid, nodeYVar, mesh.nodeY.data()),
                impl.error, 59);
    if (nodeZVar >= 0)
        HC_NC_CHECK(nc_put_var_double(impl.ncid, nodeZVar, mesh.nodeZ.data()),
                    impl.error, 59);

    if (impl.layered())
    {
        // Layer centres are the midpoints of the interfaces, which is what
        // the bounds attribute written above says they are.
        std::vector<double> centres;
        centres.reserve(impl.interfaceSigma.size() - 1);

        for (size_t layer = 0; layer + 1 < impl.interfaceSigma.size(); ++layer)
        {
            centres.push_back(0.5 * (impl.interfaceSigma[layer]
                                     + impl.interfaceSigma[layer + 1]));
        }

        HC_NC_CHECK(nc_put_var_double(impl.ncid, sigmaVar, centres.data()),
                    impl.error, 59);
        HC_NC_CHECK(nc_put_var_double(impl.ncid, interfaceVar,
                                      impl.interfaceSigma.data()),
                    impl.error, 59);
        HC_NC_CHECK(nc_put_var_double(impl.ncid, depthVar, impl.depth.data()),
                    impl.error, 59);

        // The surface is on the time axis. Whatever whole number of levels
        // was handed over is written: one for a steady mesh, several for a
        // run whose water surface moves — which is what makes sigma layers
        // real elevations at each step rather than only at the first.
        const size_t faces = static_cast<size_t>(mesh.faceCount());

        if (faces > 0 && !impl.surface.empty()
            && impl.surface.size() % faces == 0)
        {
            const size_t levels = impl.surface.size() / faces;
            const size_t start[2] = {0, 0};
            const size_t count[2] = {levels, faces};

            HC_NC_CHECK(nc_put_vara_double(impl.ncid, surfaceVar, start, count,
                                           impl.surface.data()),
                        impl.error, 59);
        }
    }
    if (faceNodesVar >= 0)
    {
        const int64_t faces = mesh.faceCount();
        const int64_t width = mesh.maxNodesPerFace();
        std::vector<long long> padded(static_cast<size_t>(faces * width),
                                      kFillValue);
        for (int64_t f = 0; f < faces; ++f)
        {
            const int64_t begin = mesh.faceNodeOffsets[static_cast<size_t>(f)];
            const int64_t end = mesh.faceNodeOffsets[static_cast<size_t>(f) + 1];
            for (int64_t k = begin; k < end; ++k)
                padded[static_cast<size_t>(f * width + (k - begin))] =
                    mesh.faceNodes[static_cast<size_t>(k)];
        }
        HC_NC_CHECK(nc_put_var_longlong(impl.ncid, faceNodesVar,
                                        padded.data()),
                    impl.error, 59);
    }
    if (edgeNodesVar >= 0)
    {
        std::vector<long long> edges;
        edges.reserve(static_cast<size_t>(mesh.edgeCount()) * 2);
        for (const auto &edge : mesh.edgeNodes)
        {
            edges.push_back(edge[0]);
            edges.push_back(edge[1]);
        }
        HC_NC_CHECK(nc_put_var_longlong(impl.ncid, edgeNodesVar,
                                        edges.data()),
                    impl.error, 59);
    }

    impl.error.clear();
    return 0;
}

// ── write ─────────────────────────────────────────────────────────────────

int NetCDFUGRIDWriter::write(const Snapshot &snapshot)
{
    Impl &impl = *m_impl;
    const size_t level = impl.timeLevels++;

    if (level == 0)
        impl.firstTime = snapshot.time();
    impl.lastTime = snapshot.time();

    {
        const size_t start[1] = {level};
        const size_t count[1] = {1};
        const double time    = snapshot.time();
        HC_NC_CHECK(nc_put_vara_double(impl.ncid, impl.timeVar, start, count,
                                       &time),
                    impl.error, 60);
    }

    for (Impl::Field &field : impl.fields)
    {
        const FieldSlice *slice = nullptr;
        for (const FieldSlice &candidate : snapshot.fields())
            if (candidate.itemId == field.itemId)
            {
                slice = &candidate;
                break;
            }
        if (!slice)
        {
            impl.error = "snapshot at t=" + std::to_string(snapshot.time()) +
                         " is missing registered field '" + field.itemId + "'";
            return 61;
        }
        if (slice->elementCount() != field.count)
        {
            impl.error = "field '" + field.itemId + "' has " +
                         std::to_string(slice->elementCount()) +
                         " elements; the mesh " +
                         Impl::locationName(field.location) + " count is " +
                         std::to_string(field.count);
            return 62;
        }

        impl.staging.resize(static_cast<size_t>(field.count));
        for (int64_t i = 0; i < field.count; ++i)
        {
            double value = 0.0;
            switch (slice->kind)
            {
            case DataKind::Float64: value = slice->values<double>()[i];   break;
            case DataKind::Float32: value = slice->values<float>()[i];    break;
            case DataKind::Int64:   value = static_cast<double>(slice->values<int64_t>()[i]);  break;
            case DataKind::Int32:   value = slice->values<int32_t>()[i];  break;
            case DataKind::UInt64:  value = static_cast<double>(slice->values<uint64_t>()[i]); break;
            case DataKind::UInt32:  value = slice->values<uint32_t>()[i]; break;
            case DataKind::Int16:   value = slice->values<int16_t>()[i];  break;
            case DataKind::UInt16:  value = slice->values<uint16_t>()[i]; break;
            case DataKind::Int8:    value = slice->values<int8_t>()[i];   break;
            case DataKind::UInt8:   value = slice->values<uint8_t>()[i];  break;
            case DataKind::Boolean: value = slice->values<bool>()[i] ? 1.0 : 0.0; break;
            default:
                impl.error = "field '" + field.itemId +
                             "' has an unsupported DataKind";
                return 63;
            }
            impl.staging[static_cast<size_t>(i)] = value;
        }

        const size_t start[2] = {level, 0};
        const size_t count[2] = {1, static_cast<size_t>(field.count)};
        HC_NC_CHECK(nc_put_vara_double(impl.ncid, field.varId, start, count,
                                       impl.staging.data()),
                    impl.error, 64);
    }
    return 0;
}

// ── finalize ──────────────────────────────────────────────────────────────

int NetCDFUGRIDWriter::finalize()
{
    Impl &impl = *m_impl;
    if (!impl.open)
        return 0;
    impl.open = false;

    // EXPERIMENT (remove if it proves inert): nc_close already writes
    // buffered data and releases resources by contract, so this should be
    // redundant. It is here only to rule out an incomplete flush as the
    // cause of the macOS nc_open failures in one CI run, alongside the
    // probe in tests/hydrocouplesdk/test_netcdf.cpp. Deliberately not
    // fatal: nc_sync is invalid in define mode, and a diagnostic aid must
    // not become a new failure path.
    (void)nc_sync(impl.ncid);

    HC_NC_CHECK(nc_close(impl.ncid), impl.error, 65);
    impl.ncid = -1;
    return 0;
}

std::vector<ResultEntry> NetCDFUGRIDWriter::catalog() const
{
    std::vector<ResultEntry> entries;
    entries.reserve(m_impl->fields.size());

    for (const Impl::Field &field : m_impl->fields)
    {
        ResultEntry entry;
        entry.itemId      = field.itemId;
        entry.artifact    = m_impl->path.filename().string();
        entry.format      = "netcdf_ugrid";
        entry.variable    = field.itemId;
        entry.kind        = DataKind::Float64;
        entry.shape       = {static_cast<int64_t>(m_impl->timeLevels),
                             field.count};
        entry.dimensions  = {"time", meshLocationName(field.location)};
        entry.units       = field.units;
        entry.description = field.standardName;
        entry.mesh        = m_impl->mesh.meshName;
        entry.location    = field.location;

        ResultTimeAxis axis;
        axis.count = static_cast<int64_t>(m_impl->timeLevels);
        axis.start = m_impl->firstTime;
        axis.end   = m_impl->lastTime;
        entry.time = axis;

        entries.push_back(std::move(entry));
    }
    return entries;
}

} // namespace HydroCouple::SDK::IO
