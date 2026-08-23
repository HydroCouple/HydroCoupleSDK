// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file hdf5ugridwriter.cpp
 * \brief HDF5UGRIDWriter implementation over the HDF5 C API.
 *
 * File layout (CF-1.11 / UGRID-1.0):
 *
 *   /                     Conventions, title, source attributes
 *   /<mesh>               int scalar dummy — cf_role=mesh_topology,
 *                         topology_dimension, node_coordinates,
 *                         face_node_connectivity, edge_node_connectivity
 *   /<mesh>_node_x|y|z    [nNodes] double
 *   /<mesh>_face_nodes    [nFaces, maxNodesPerFace] int64, _FillValue=-1,
 *                         start_index=0
 *   /<mesh>_edge_nodes    [nEdges, 2] int64, start_index=0
 *   /time                 [unlimited] double, chunked+deflate
 *   /<field>              [unlimited, count(location)] double,
 *                         chunked+deflate, mesh=<mesh>, location=...
 */

#include "stdafx.h"
#include "hydrocouplesdk/io/hdf5ugridwriter.h"

#include <hdf5.h>

#include <cstring>
#include <map>

namespace HydroCouple::SDK::IO {

// ── Impl ──────────────────────────────────────────────────────────────────

namespace {

constexpr int64_t  kFillValue      = -1;
constexpr hsize_t  kTimeChunk      = 256;
constexpr unsigned kDeflateLevel   = 4;

/*!
 * Writes a string attribute on an HDF5 object.
 */
bool writeStringAttribute(hid_t object, const char *attrName,
                          const std::string &value)
{
    const hid_t type = H5Tcopy(H5T_C_S1);
    H5Tset_size(type, std::max<size_t>(1, value.size()));
    H5Tset_strpad(type, H5T_STR_NULLTERM);
    const hid_t space = H5Screate(H5S_SCALAR);
    const hid_t attr =
        H5Acreate2(object, attrName, type, space, H5P_DEFAULT, H5P_DEFAULT);
    bool ok = attr >= 0 &&
              H5Awrite(attr, type, value.empty() ? "" : value.c_str()) >= 0;
    if (attr >= 0) H5Aclose(attr);
    H5Sclose(space);
    H5Tclose(type);
    return ok;
}

/*!
 * Writes an int32 scalar attribute on an HDF5 object.
 */
bool writeIntAttribute(hid_t object, const char *attrName, int32_t value)
{
    const hid_t space = H5Screate(H5S_SCALAR);
    const hid_t attr  = H5Acreate2(object, attrName, H5T_NATIVE_INT32, space,
                                   H5P_DEFAULT, H5P_DEFAULT);
    bool ok = attr >= 0 && H5Awrite(attr, H5T_NATIVE_INT32, &value) >= 0;
    if (attr >= 0) H5Aclose(attr);
    H5Sclose(space);
    return ok;
}

/*!
 * Creates and fully writes a fixed 1-D double dataset.
 */
hid_t writeStaticDouble1D(hid_t file, const std::string &datasetName,
                          const std::vector<double> &values)
{
    const hsize_t dims[1] = {values.size()};
    const hid_t space = H5Screate_simple(1, dims, nullptr);
    const hid_t dataset =
        H5Dcreate2(file, datasetName.c_str(), H5T_NATIVE_DOUBLE, space,
                   H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dataset >= 0)
        H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                 values.data());
    H5Sclose(space);
    return dataset;
}

} // namespace

struct HDF5UGRIDWriter::Impl
{
    struct Field
    {
        std::string  itemId;
        MeshLocation location = MeshLocation::Node;
        std::string  units;
        std::string  standardName;
        hid_t        dataset = -1;
        int64_t      count   = 0; //!< Values per time level.
    };

    std::filesystem::path path;
    MeshDefinition        mesh;
    std::vector<Field>    fields;
    std::string           error;

    hid_t file        = -1;
    hid_t timeDataset = -1;
    hsize_t timeLevels = 0;

    double firstTime = 0.0; //!< Time axis bounds for catalog().
    double lastTime  = 0.0;

    std::vector<double> staging; //!< Reused conversion buffer.

    // ── helpers ──────────────────────────────────────────────────────────

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

    /*!
     * Creates an unlimited [time, count] double dataset with chunking and
     * deflate compression.
     */
    hid_t createTimeVaryingDataset(const std::string &datasetName,
                                   int64_t count)
    {
        const int rank = count > 0 ? 2 : 1;
        const hsize_t dims[2]    = {0, static_cast<hsize_t>(count)};
        const hsize_t maxDims[2] = {H5S_UNLIMITED, static_cast<hsize_t>(count)};
        const hsize_t chunk[2]   = {
            kTimeChunk,
            static_cast<hsize_t>(std::max<int64_t>(1, count))};

        const hid_t space = H5Screate_simple(rank, dims, maxDims);
        const hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_chunk(plist, rank, chunk);
        H5Pset_deflate(plist, kDeflateLevel);
        const hid_t dataset =
            H5Dcreate2(file, datasetName.c_str(), H5T_NATIVE_DOUBLE, space,
                       H5P_DEFAULT, plist, H5P_DEFAULT);
        H5Pclose(plist);
        H5Sclose(space);
        return dataset;
    }

    /*!
     * Extends a time-varying dataset to \p levels rows and writes \p row
     * into the last row (the engine's extend-and-write hyperslab append).
     */
    bool extendAndWrite(hid_t dataset, hsize_t levels, const double *row,
                        int64_t count)
    {
        const int rank = count > 0 ? 2 : 1;
        hsize_t newSize[2] = {levels, static_cast<hsize_t>(count)};
        if (H5Dset_extent(dataset, newSize) < 0)
            return false;

        const hid_t fileSpace = H5Dget_space(dataset);
        const hsize_t start[2] = {levels - 1, 0};
        const hsize_t sel[2]   = {1, static_cast<hsize_t>(count)};
        H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, nullptr, sel,
                            nullptr);
        const hsize_t memDims[2] = {1, static_cast<hsize_t>(count)};
        const hid_t memSpace = H5Screate_simple(rank, memDims, nullptr);
        const bool ok = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, memSpace,
                                 fileSpace, H5P_DEFAULT, row) >= 0;
        H5Sclose(memSpace);
        H5Sclose(fileSpace);
        return ok;
    }
};

// ── Construction ──────────────────────────────────────────────────────────

HDF5UGRIDWriter::HDF5UGRIDWriter(std::filesystem::path path,
                                 MeshDefinition mesh)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->path = std::move(path);
    m_impl->mesh = std::move(mesh);
}

HDF5UGRIDWriter::~HDF5UGRIDWriter()
{
    (void)finalize();
}

void HDF5UGRIDWriter::addField(const std::string &itemId,
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

std::string HDF5UGRIDWriter::name() const
{
    return "hdf5-ugrid";
}

const std::filesystem::path &HDF5UGRIDWriter::path() const noexcept
{
    return m_impl->path;
}

std::string HDF5UGRIDWriter::errorMessage() const
{
    return m_impl->error;
}

// ── initialize ────────────────────────────────────────────────────────────

int HDF5UGRIDWriter::initialize()
{
    Impl &impl = *m_impl;
    const MeshDefinition &mesh = impl.mesh;

    if (!mesh.validate(impl.error))
        return 10;

    std::error_code ec;
    if (impl.path.has_parent_path())
        std::filesystem::create_directories(impl.path.parent_path(), ec);

    impl.file = H5Fcreate(impl.path.string().c_str(), H5F_ACC_TRUNC,
                          H5P_DEFAULT, H5P_DEFAULT);
    if (impl.file < 0)
    {
        impl.error = "H5Fcreate failed for '" + impl.path.string() + "'";
        return 11;
    }

    // Root conventions.
    writeStringAttribute(impl.file, "Conventions", "CF-1.11 UGRID-1.0");
    writeStringAttribute(impl.file, "title", "HydroCoupleSDK output");
    writeStringAttribute(impl.file, "source",
                         "HydroCoupleSDK HDF5UGRIDWriter");

    // Mesh-topology dummy dataset with UGRID attributes.
    {
        const hid_t space = H5Screate(H5S_SCALAR);
        const hid_t topology =
            H5Dcreate2(impl.file, mesh.meshName.c_str(), H5T_NATIVE_INT32,
                       space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Sclose(space);
        if (topology < 0)
        {
            impl.error = "failed to create mesh topology dataset '" +
                         mesh.meshName + "'";
            return 12;
        }
        const int32_t marker = 0;
        H5Dwrite(topology, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                 &marker);

        writeStringAttribute(topology, "cf_role", "mesh_topology");
        writeIntAttribute(topology, "topology_dimension",
                          mesh.faceCount() > 0 ? 2 : 1);
        writeStringAttribute(topology, "node_coordinates",
                             mesh.meshName + "_node_x " + mesh.meshName +
                                 "_node_y");
        if (mesh.faceCount() > 0)
            writeStringAttribute(topology, "face_node_connectivity",
                                 mesh.meshName + "_face_nodes");
        if (mesh.edgeCount() > 0)
            writeStringAttribute(topology, "edge_node_connectivity",
                                 mesh.meshName + "_edge_nodes");
        H5Dclose(topology);
    }

    // Node coordinates.
    for (const auto &[suffix, values] :
         std::initializer_list<std::pair<const char *,
                                         const std::vector<double> *>>{
             {"_node_x", &mesh.nodeX},
             {"_node_y", &mesh.nodeY},
             {"_node_z", &mesh.nodeZ}})
    {
        if (values->empty())
            continue;
        const hid_t dataset =
            writeStaticDouble1D(impl.file, mesh.meshName + suffix, *values);
        if (dataset < 0)
        {
            impl.error = std::string("failed to write coordinate dataset ") +
                         suffix;
            return 13;
        }
        writeStringAttribute(dataset, "standard_name",
                             std::strcmp(suffix, "_node_x") == 0
                                 ? "projection_x_coordinate"
                                 : (std::strcmp(suffix, "_node_y") == 0
                                        ? "projection_y_coordinate"
                                        : "altitude"));
        H5Dclose(dataset);
    }

    // Face→node connectivity, padded to maxNodesPerFace with the fill value.
    if (mesh.faceCount() > 0)
    {
        const int64_t faces = mesh.faceCount();
        const int64_t width = mesh.maxNodesPerFace();
        std::vector<int64_t> padded(static_cast<size_t>(faces * width),
                                    kFillValue);
        for (int64_t f = 0; f < faces; ++f)
        {
            const int64_t begin = mesh.faceNodeOffsets[static_cast<size_t>(f)];
            const int64_t end = mesh.faceNodeOffsets[static_cast<size_t>(f) + 1];
            for (int64_t k = begin; k < end; ++k)
                padded[static_cast<size_t>(f * width + (k - begin))] =
                    mesh.faceNodes[static_cast<size_t>(k)];
        }
        const hsize_t dims[2] = {static_cast<hsize_t>(faces),
                                 static_cast<hsize_t>(width)};
        const hid_t space = H5Screate_simple(2, dims, nullptr);
        const hid_t dataset = H5Dcreate2(
            impl.file, (mesh.meshName + "_face_nodes").c_str(),
            H5T_NATIVE_INT64, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Sclose(space);
        if (dataset < 0)
        {
            impl.error = "failed to create face_node_connectivity dataset";
            return 14;
        }
        H5Dwrite(dataset, H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                 padded.data());
        writeStringAttribute(dataset, "cf_role", "face_node_connectivity");
        writeIntAttribute(dataset, "start_index", 0);
        {
            const hid_t space2 = H5Screate(H5S_SCALAR);
            const hid_t attr =
                H5Acreate2(dataset, "_FillValue", H5T_NATIVE_INT64, space2,
                           H5P_DEFAULT, H5P_DEFAULT);
            if (attr >= 0)
            {
                H5Awrite(attr, H5T_NATIVE_INT64, &kFillValue);
                H5Aclose(attr);
            }
            H5Sclose(space2);
        }
        H5Dclose(dataset);
    }

    // Edge endpoints.
    if (mesh.edgeCount() > 0)
    {
        const hsize_t dims[2] = {static_cast<hsize_t>(mesh.edgeCount()), 2};
        const hid_t space = H5Screate_simple(2, dims, nullptr);
        const hid_t dataset = H5Dcreate2(
            impl.file, (mesh.meshName + "_edge_nodes").c_str(),
            H5T_NATIVE_INT64, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Sclose(space);
        if (dataset >= 0)
        {
            H5Dwrite(dataset, H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                     mesh.edgeNodes.data());
            writeStringAttribute(dataset, "cf_role", "edge_node_connectivity");
            writeIntAttribute(dataset, "start_index", 0);
            H5Dclose(dataset);
        }
    }

    // Unlimited time coordinate.
    impl.timeDataset = impl.createTimeVaryingDataset("time", 0);
    if (impl.timeDataset < 0)
    {
        impl.error = "failed to create time dataset";
        return 15;
    }
    writeStringAttribute(impl.timeDataset, "standard_name", "time");
    writeStringAttribute(impl.timeDataset, "long_name", "time");
    writeStringAttribute(impl.timeDataset, "units", "days since -4713-01-01");
    writeStringAttribute(impl.timeDataset, "calendar", "julian");

    // Registered time-varying fields.
    for (Impl::Field &field : impl.fields)
    {
        field.count = impl.locationCount(field.location);
        if (field.count <= 0)
        {
            impl.error = "field '" + field.itemId + "' registered on a " +
                         std::string(Impl::locationName(field.location)) +
                         " location, but the mesh has none";
            return 16;
        }
        field.dataset =
            impl.createTimeVaryingDataset(field.itemId, field.count);
        if (field.dataset < 0)
        {
            impl.error = "failed to create field dataset '" + field.itemId +
                         "'";
            return 17;
        }
        writeStringAttribute(field.dataset, "mesh", impl.mesh.meshName);
        writeStringAttribute(field.dataset, "location",
                             Impl::locationName(field.location));
        writeStringAttribute(field.dataset, "coordinates", "time");
        if (!field.units.empty())
            writeStringAttribute(field.dataset, "units", field.units);
        if (!field.standardName.empty())
            writeStringAttribute(field.dataset, "standard_name",
                                 field.standardName);
    }

    impl.error.clear();
    return 0;
}

// ── write ─────────────────────────────────────────────────────────────────

int HDF5UGRIDWriter::write(const Snapshot &snapshot)
{
    Impl &impl = *m_impl;
    ++impl.timeLevels;

    const double time = snapshot.time();
    if (impl.timeLevels == 1)
        impl.firstTime = time;
    impl.lastTime = time;
    if (!impl.extendAndWrite(impl.timeDataset, impl.timeLevels, &time, 0))
    {
        impl.error = "failed to append to /time";
        return 20;
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
            impl.error = "snapshot at t=" + std::to_string(time) +
                         " is missing registered field '" + field.itemId + "'";
            return 21;
        }
        if (slice->elementCount() != field.count)
        {
            impl.error = "field '" + field.itemId + "' has " +
                         std::to_string(slice->elementCount()) +
                         " elements; the mesh " +
                         Impl::locationName(field.location) + " count is " +
                         std::to_string(field.count);
            return 22;
        }

        // Convert any fixed-width numeric kind to double via the reused
        // staging buffer (no allocation after the first snapshot).
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
                return 23;
            }
            impl.staging[static_cast<size_t>(i)] = value;
        }

        if (!impl.extendAndWrite(field.dataset, impl.timeLevels,
                                 impl.staging.data(), field.count))
        {
            impl.error = "failed to append to field '" + field.itemId + "'";
            return 24;
        }
    }
    return 0;
}

// ── finalize ──────────────────────────────────────────────────────────────

int HDF5UGRIDWriter::finalize()
{
    Impl &impl = *m_impl;
    for (Impl::Field &field : impl.fields)
        if (field.dataset >= 0)
        {
            H5Dclose(field.dataset);
            field.dataset = -1;
        }
    if (impl.timeDataset >= 0)
    {
        H5Dclose(impl.timeDataset);
        impl.timeDataset = -1;
    }
    if (impl.file >= 0)
    {
        H5Fflush(impl.file, H5F_SCOPE_GLOBAL);
        H5Fclose(impl.file);
        impl.file = -1;
    }
    return 0;
}

std::vector<ResultEntry> HDF5UGRIDWriter::catalog() const
{
    std::vector<ResultEntry> entries;
    entries.reserve(m_impl->fields.size());

    for (const Impl::Field &field : m_impl->fields)
    {
        ResultEntry entry;
        entry.itemId      = field.itemId;
        entry.artifact    = m_impl->path.filename().string();
        entry.format      = "hdf5_ugrid";
        entry.variable    = field.itemId;
        entry.kind        = DataKind::Float64; // UGRID fields store doubles
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
