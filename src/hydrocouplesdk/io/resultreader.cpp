// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/io/resultreader.h"
#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/io/ugridreader.h"
#include "hydrocouplehelpers.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

#ifdef HYDROCOUPLESDK_HAVE_HDF5
#include <hdf5.h>
#endif
#ifdef HYDROCOUPLESDK_HAVE_NETCDF
#include <netcdf.h>
#endif
#ifdef HYDROCOUPLESDK_HAVE_GEOPACKAGE
#include <sqlite3.h>
#endif

namespace HydroCouple::SDK::IO {

namespace {

/*!
 * Reads the UGRID mesh an entry names, out of the artifact it was recorded
 * in. Shared by the HDF5 and NetCDF readers: both write the same conventions
 * and the reader that understands them does not care which container it is
 * reading from.
 */
bool readUGRIDMeshFor(const ResultEntry &entry,
                      const std::filesystem::path &path, MeshDefinition &mesh,
                      std::string &message)
{
    if (entry.mesh.empty())
    {
        // No attachment recorded, so there is nothing to look for. Reading
        // "the first mesh in the file" instead would hand back geometry the
        // values may have nothing to do with.
        message = "result \"" + entry.itemId + "\" names no mesh";
        return false;
    }

    return readUGRIDMesh(path, entry.mesh, mesh, message);
}

/*!
 * Decodes a GeoPackageBinary point blob into \p x and \p y.
 *
 * The envelope is skipped by the size its flags declare rather than assumed
 * absent: the SDK's own writer stores none, but a GeoPackage that has been
 * through another tool may carry one, and the header field exists so a
 * reader can step over what it does not need.
 */
bool decodeGeoPackagePoint(const void *blob, int bytes, double &x, double &y)
{
    const auto *data = static_cast<const uint8_t *>(blob);

    if (!data || bytes < 8 || data[0] != 'G' || data[1] != 'P')
        return false;

    // Bits 1-3 of the flags select the envelope: none, xy, xyz, xym, xyzm.
    static constexpr int kEnvelopeDoubles[] = {0, 4, 6, 6, 8};
    const int envelope = (data[3] >> 1) & 0x07;

    if (envelope > 4)
        return false;

    const int header = 8 + kEnvelopeDoubles[envelope] * 8;

    // Byte order, geometry type, then the coordinate pair.
    if (bytes < header + 21)
        return false;

    const uint8_t *wkb = data + header;
    uint32_t       type = 0;
    std::memcpy(&type, wkb + 1, 4);

    if ((type & 0xffff) != 1)
        return false;

    std::memcpy(&x, wkb + 5, 8);
    std::memcpy(&y, wkb + 13, 8);
    return true;
}

/*!
 * Validates a hyperslab request against an entry and the destination.
 */
bool validateRequest(const ResultEntry &entry,
                     std::span<const int64_t> start,
                     std::span<const int64_t> count,
                     const BufferDescriptor &into, int64_t &elements,
                     std::string &message)
{
    const size_t rank = entry.shape.size();
    if (start.size() != rank || count.size() != rank)
    {
        message = "result read of \"" + entry.itemId + "\": start/count rank " +
                  std::to_string(start.size()) + "/" +
                  std::to_string(count.size()) + " does not match the "
                  "recorded rank " + std::to_string(rank);
        return false;
    }
    elements = 1;
    for (size_t d = 0; d < rank; ++d)
    {
        if (start[d] < 0 || count[d] < 0 ||
            start[d] + count[d] > entry.shape[d])
        {
            message = "result read of \"" + entry.itemId +
                      "\": selection out of range on dimension " +
                      std::to_string(d);
            return false;
        }
        elements *= count[d];
    }
    if (into.kind != entry.kind)
    {
        message = "result read of \"" + entry.itemId + "\": destination kind " +
                  dataKindName(into.kind) + " differs from recorded kind " +
                  dataKindName(entry.kind);
        return false;
    }
    int64_t destinationElements = 1;
    for (int32_t d = 0; d < into.rank; ++d)
        destinationElements *= into.shape[d];
    if (destinationElements != elements)
    {
        message = "result read of \"" + entry.itemId + "\": destination holds " +
                  std::to_string(destinationElements) + " elements; the "
                  "selection covers " + std::to_string(elements);
        return false;
    }
    return true;
}

/*!
 * Writes \p values (already in selection order) into a host destination,
 * converting from double to the destination's kind.
 */
bool scatterDoubles(const std::vector<double> &values,
                    const BufferDescriptor &into, std::string &message)
{
    switch (into.kind)
    {
        case DataKind::Float64:
        {
            auto *out = static_cast<double *>(into.data);
            std::copy(values.begin(), values.end(), out);
            return true;
        }
        case DataKind::Float32:
        {
            auto *out = static_cast<float *>(into.data);
            for (size_t i = 0; i < values.size(); ++i)
                out[i] = static_cast<float>(values[i]);
            return true;
        }
        case DataKind::Int32:
        {
            auto *out = static_cast<int32_t *>(into.data);
            for (size_t i = 0; i < values.size(); ++i)
                out[i] = static_cast<int32_t>(values[i]);
            return true;
        }
        case DataKind::Int64:
        {
            auto *out = static_cast<int64_t *>(into.data);
            for (size_t i = 0; i < values.size(); ++i)
                out[i] = static_cast<int64_t>(values[i]);
            return true;
        }
        default:
            message = "unsupported destination DataKind " +
                      dataKindName(into.kind) + " for a result read";
            return false;
    }
}

/*!
 * Row-major flat index of a selection coordinate within the full shape.
 */
int64_t flatIndex(std::span<const int64_t> shape,
                  const std::vector<int64_t> &coordinate)
{
    int64_t index = 0;
    for (size_t d = 0; d < shape.size(); ++d)
        index = index * shape[d] + coordinate[d];
    return index;
}

/*!
 * Iterates the selection in row-major order, invoking \p visit with the
 * flat index into the full variable.
 */
template <typename Visitor>
void forEachSelected(const ResultEntry &entry,
                     std::span<const int64_t> start,
                     std::span<const int64_t> count, Visitor visit)
{
    const size_t rank = entry.shape.size();
    std::vector<int64_t> coordinate(rank, 0);
    int64_t elements = 1;
    for (int64_t extent : count)
        elements *= extent;

    for (int64_t n = 0; n < elements; ++n)
    {
        std::vector<int64_t> absolute(rank);
        for (size_t d = 0; d < rank; ++d)
            absolute[d] = start[d] + coordinate[d];
        visit(n, flatIndex(entry.shape, absolute));

        for (size_t d = rank; d-- > 0;)
        {
            if (++coordinate[d] < count[d])
                break;
            coordinate[d] = 0;
        }
    }
}

// ── CSV ───────────────────────────────────────────────────────────────────

/*!
 * Reads the SDK's long-format CSV (\c time,item,index,value).
 *
 * The whole file is indexed once on open() into per-item, per-time rows —
 * a long-format file cannot answer a slab without a scan, so paying it
 * once beats paying it per query. CSV is the debug/interchange format;
 * HDF5 and NetCDF are the analysis formats.
 */
class CSVResultReader final : public IResultReader
{
public:
    [[nodiscard]] std::string format() const override { return "csv"; }

    [[nodiscard]] bool open(const std::filesystem::path &path,
                            std::string &message) override
    {
        std::ifstream stream(path);
        if (!stream)
        {
            message = "cannot open result artifact \"" + path.string() + "\"";
            return false;
        }

        std::string line;
        if (!std::getline(stream, line)) // header
        {
            message = "result artifact \"" + path.string() + "\" is empty";
            return false;
        }

        while (std::getline(stream, line))
        {
            if (line.empty())
                continue;
            std::istringstream row(line);
            std::string timeText, itemId, indexText, valueText;
            if (!std::getline(row, timeText, ',') ||
                !std::getline(row, itemId, ',') ||
                !std::getline(row, indexText, ',') ||
                !std::getline(row, valueText))
                continue;

            ItemData &item = m_items[itemId];
            const double time = std::stod(timeText);
            if (item.times.empty() || item.times.back() != time)
                item.times.push_back(time);
            item.values.push_back(std::stod(valueText));
        }
        m_path = path;
        message.clear();
        return true;
    }

    [[nodiscard]] bool read(const ResultEntry &entry,
                            std::span<const int64_t> start,
                            std::span<const int64_t> count,
                            const BufferDescriptor &into,
                            std::string &message) override
    {
        int64_t elements = 0;
        if (!validateRequest(entry, start, count, into, elements, message))
            return false;

        const auto found = m_items.find(entry.variableName());
        if (found == m_items.end())
        {
            message = "result artifact \"" + m_path.string() +
                      "\" holds no item \"" + entry.variableName() + "\"";
            return false;
        }
        const ItemData &item = found->second;

        if (static_cast<int64_t>(item.values.size()) < entry.elementCount())
        {
            message = "result artifact \"" + m_path.string() + "\" holds " +
                      std::to_string(item.values.size()) + " values for \"" +
                      entry.variableName() + "\"; the catalog records " +
                      std::to_string(entry.elementCount());
            return false;
        }

        std::vector<double> selected(static_cast<size_t>(elements));
        forEachSelected(entry, start, count,
                        [&](int64_t position, int64_t flat) {
                            selected[static_cast<size_t>(position)] =
                                item.values[static_cast<size_t>(flat)];
                        });
        return scatterDoubles(selected, into, message);
    }

    [[nodiscard]] bool readTimes(const ResultEntry &entry,
                                 std::vector<double> &times,
                                 std::string &message) override
    {
        const auto found = m_items.find(entry.variableName());
        if (found == m_items.end())
        {
            message = "result artifact \"" + m_path.string() +
                      "\" holds no item \"" + entry.variableName() + "\"";
            return false;
        }
        times = found->second.times;
        message.clear();
        return true;
    }

private:
    struct ItemData
    {
        std::vector<double> times;  //!< Distinct time levels, in order.
        std::vector<double> values; //!< Row-major over the recorded shape.
    };

    std::filesystem::path                 m_path;
    std::map<std::string, ItemData>       m_items;
};

#ifdef HYDROCOUPLESDK_HAVE_HDF5

/*!
 * Reads the SDK's HDF5/UGRID artifacts through native hyperslabs.
 */
class HDF5ResultReader final : public IResultReader
{
public:
    ~HDF5ResultReader() override
    {
        if (m_file >= 0)
            H5Fclose(m_file);
    }

    [[nodiscard]] std::string format() const override { return "hdf5_ugrid"; }

    [[nodiscard]] bool open(const std::filesystem::path &path,
                            std::string &message) override
    {
        m_file = H5Fopen(path.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (m_file < 0)
        {
            message = "cannot open HDF5 result artifact \"" + path.string() +
                      "\"";
            return false;
        }
        m_path = path;
        message.clear();
        return true;
    }

    [[nodiscard]] bool read(const ResultEntry &entry,
                            std::span<const int64_t> start,
                            std::span<const int64_t> count,
                            const BufferDescriptor &into,
                            std::string &message) override
    {
        int64_t elements = 0;
        if (!validateRequest(entry, start, count, into, elements, message))
            return false;

        const std::string name = "/" + entry.variableName();
        const hid_t dataset = H5Dopen2(m_file, name.c_str(), H5P_DEFAULT);
        if (dataset < 0)
        {
            message = "HDF5 artifact \"" + m_path.string() +
                      "\" holds no dataset \"" + name + "\"";
            return false;
        }

        std::vector<hsize_t> offset(start.begin(), start.end());
        std::vector<hsize_t> extent(count.begin(), count.end());
        const hid_t space = H5Dget_space(dataset);
        H5Sselect_hyperslab(space, H5S_SELECT_SET, offset.data(), nullptr,
                            extent.data(), nullptr);
        const hsize_t memoryExtent =
            static_cast<hsize_t>(elements);
        const hid_t memory = H5Screate_simple(1, &memoryExtent, nullptr);

        std::vector<double> selected(static_cast<size_t>(elements));
        const herr_t status = H5Dread(dataset, H5T_NATIVE_DOUBLE, memory,
                                      space, H5P_DEFAULT, selected.data());
        H5Sclose(memory);
        H5Sclose(space);
        H5Dclose(dataset);

        if (status < 0)
        {
            message = "HDF5 read of \"" + entry.variableName() + "\" failed";
            return false;
        }
        return scatterDoubles(selected, into, message);
    }

    [[nodiscard]] bool readTimes(const ResultEntry &entry,
                                 std::vector<double> &times,
                                 std::string &message) override
    {
        const hid_t dataset = H5Dopen2(m_file, "/time", H5P_DEFAULT);
        if (dataset < 0)
            return IResultReader::readTimes(entry, times, message);

        const hid_t space = H5Dget_space(dataset);
        hsize_t     extent = 0;
        H5Sget_simple_extent_dims(space, &extent, nullptr);
        times.assign(static_cast<size_t>(extent), 0.0);
        const herr_t status = H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL,
                                      H5S_ALL, H5P_DEFAULT, times.data());
        H5Sclose(space);
        H5Dclose(dataset);
        if (status < 0)
        {
            message = "HDF5 read of /time failed";
            return false;
        }
        message.clear();
        return true;
    }


    [[nodiscard]] bool readMesh(const ResultEntry &entry,
                                MeshDefinition &mesh,
                                std::string &message) override
    {
        return readUGRIDMeshFor(entry, m_path, mesh, message);
    }
private:
    std::filesystem::path m_path;
    hid_t                 m_file = -1;
};

#endif // HYDROCOUPLESDK_HAVE_HDF5

#ifdef HYDROCOUPLESDK_HAVE_NETCDF

/*!
 * Reads the SDK's NetCDF/UGRID artifacts through native hyperslabs.
 */
class NetCDFResultReader final : public IResultReader
{
public:
    ~NetCDFResultReader() override
    {
        if (m_ncid >= 0)
            nc_close(m_ncid);
    }

    [[nodiscard]] std::string format() const override
    {
        return "netcdf_ugrid";
    }

    [[nodiscard]] bool open(const std::filesystem::path &path,
                            std::string &message) override
    {
        if (nc_open(path.string().c_str(), NC_NOWRITE, &m_ncid) != NC_NOERR)
        {
            message = "cannot open NetCDF result artifact \"" +
                      path.string() + "\"";
            return false;
        }
        m_path = path;
        message.clear();
        return true;
    }

    [[nodiscard]] bool read(const ResultEntry &entry,
                            std::span<const int64_t> start,
                            std::span<const int64_t> count,
                            const BufferDescriptor &into,
                            std::string &message) override
    {
        int64_t elements = 0;
        if (!validateRequest(entry, start, count, into, elements, message))
            return false;

        int variable = -1;
        if (nc_inq_varid(m_ncid, entry.variableName().c_str(), &variable) !=
            NC_NOERR)
        {
            message = "NetCDF artifact \"" + m_path.string() +
                      "\" holds no variable \"" + entry.variableName() + "\"";
            return false;
        }

        std::vector<size_t> offset(start.begin(), start.end());
        std::vector<size_t> extent(count.begin(), count.end());
        std::vector<double> selected(static_cast<size_t>(elements));
        if (nc_get_vara_double(m_ncid, variable, offset.data(), extent.data(),
                               selected.data()) != NC_NOERR)
        {
            message = "NetCDF read of \"" + entry.variableName() + "\" failed";
            return false;
        }
        return scatterDoubles(selected, into, message);
    }

    [[nodiscard]] bool readTimes(const ResultEntry &entry,
                                 std::vector<double> &times,
                                 std::string &message) override
    {
        int variable = -1;
        if (nc_inq_varid(m_ncid, "time", &variable) != NC_NOERR)
            return IResultReader::readTimes(entry, times, message);

        int dimension = -1;
        if (nc_inq_vardimid(m_ncid, variable, &dimension) != NC_NOERR)
            return IResultReader::readTimes(entry, times, message);
        size_t length = 0;
        nc_inq_dimlen(m_ncid, dimension, &length);
        times.assign(length, 0.0);
        if (nc_get_var_double(m_ncid, variable, times.data()) != NC_NOERR)
        {
            message = "NetCDF read of \"time\" failed";
            return false;
        }
        message.clear();
        return true;
    }


    [[nodiscard]] bool readMesh(const ResultEntry &entry,
                                MeshDefinition &mesh,
                                std::string &message) override
    {
        return readUGRIDMeshFor(entry, m_path, mesh, message);
    }
private:
    std::filesystem::path m_path;
    int                   m_ncid = -1;
};

#endif // HYDROCOUPLESDK_HAVE_NETCDF

#ifdef HYDROCOUPLESDK_HAVE_GEOPACKAGE

/*!
 * Reads the SDK's GeoPackage result schema.
 *
 * \c result_timeseries stores one row per (variable, object_index,
 * elapsed_time), so the reader reconstructs the { time, object } array
 * from the shape the catalog recorded — it never infers one from the
 * table.
 */
class GeoPackageResultReader final : public IResultReader
{
public:
    ~GeoPackageResultReader() override
    {
        if (m_db)
            sqlite3_close(m_db);
    }

    [[nodiscard]] std::string format() const override { return "geopackage"; }

    [[nodiscard]] bool open(const std::filesystem::path &path,
                            std::string &message) override
    {
        if (sqlite3_open_v2(path.string().c_str(), &m_db, SQLITE_OPEN_READONLY,
                            nullptr) != SQLITE_OK)
        {
            message = "cannot open GeoPackage result artifact \"" +
                      path.string() + "\"";
            return false;
        }
        m_path = path;
        message.clear();
        return true;
    }

    [[nodiscard]] bool read(const ResultEntry &entry,
                            std::span<const int64_t> start,
                            std::span<const int64_t> count,
                            const BufferDescriptor &into,
                            std::string &message) override
    {
        int64_t elements = 0;
        if (!validateRequest(entry, start, count, into, elements, message))
            return false;

        // Load the variable's full array once, ordered by time then object,
        // then serve the slab from it.
        const auto cached = m_cache.find(entry.variableName());
        if (cached == m_cache.end())
        {
            std::vector<double> values;
            if (!loadVariable(entry, values, message))
                return false;
            m_cache.emplace(entry.variableName(), std::move(values));
        }
        const std::vector<double> &values = m_cache.at(entry.variableName());

        std::vector<double> selected(static_cast<size_t>(elements));
        forEachSelected(entry, start, count,
                        [&](int64_t position, int64_t flat) {
                            selected[static_cast<size_t>(position)] =
                                values[static_cast<size_t>(flat)];
                        });
        return scatterDoubles(selected, into, message);
    }

    [[nodiscard]] bool readMesh(const ResultEntry &entry,
                                MeshDefinition &mesh,
                                std::string &message) override
    {
        static constexpr const char *kSql =
            "SELECT geom FROM mesh_nodes ORDER BY node_index";

        sqlite3_stmt *statement = nullptr;
        if (sqlite3_prepare_v2(m_db, kSql, -1, &statement, nullptr) !=
            SQLITE_OK)
        {
            message = "GeoPackage artifact \"" + m_path.string() +
                      "\" holds no mesh_nodes table";
            return false;
        }

        MeshDefinition read;
        read.meshName = entry.mesh.empty() ? "mesh" : entry.mesh;

        while (sqlite3_step(statement) == SQLITE_ROW)
        {
            const void *blob = sqlite3_column_blob(statement, 0);
            const int   bytes = sqlite3_column_bytes(statement, 0);
            double      x = 0.0;
            double      y = 0.0;

            if (!decodeGeoPackagePoint(blob, bytes, x, y))
            {
                sqlite3_finalize(statement);
                message = "GeoPackage artifact \"" + m_path.string() +
                          "\" holds a mesh_nodes row that is not a point";
                return false;
            }

            read.nodeX.push_back(x);
            read.nodeY.push_back(y);
        }
        sqlite3_finalize(statement);

        if (read.nodeX.empty())
        {
            message = "GeoPackage artifact \"" + m_path.string() +
                      "\" records no mesh nodes";
            return false;
        }

        // Nodes and no connectivity, because that is what the writer stores.
        // Faces inferred from a point cloud would be an invention, and a
        // node-only mesh is already a valid one.
        mesh = std::move(read);
        message.clear();
        return true;
    }

private:
    [[nodiscard]] bool loadVariable(const ResultEntry &entry,
                                    std::vector<double> &values,
                                    std::string &message)
    {
        static constexpr const char *kSql =
            "SELECT t.elapsed_time, t.object_index, t.value "
            "FROM result_timeseries t JOIN variables v "
            "ON t.variable_id = v.id WHERE v.item_id = ? "
            "ORDER BY t.elapsed_time, t.object_index";

        sqlite3_stmt *statement = nullptr;
        if (sqlite3_prepare_v2(m_db, kSql, -1, &statement, nullptr) !=
            SQLITE_OK)
        {
            message = "GeoPackage query failed: " +
                      std::string(sqlite3_errmsg(m_db));
            return false;
        }
        sqlite3_bind_text(statement, 1, entry.variableName().c_str(), -1,
                          SQLITE_TRANSIENT);

        values.assign(static_cast<size_t>(entry.elementCount()), 0.0);
        int64_t written = 0;
        while (sqlite3_step(statement) == SQLITE_ROW)
        {
            if (written < static_cast<int64_t>(values.size()))
                values[static_cast<size_t>(written)] =
                    sqlite3_column_double(statement, 2);
            ++written;
        }
        sqlite3_finalize(statement);

        if (written != entry.elementCount())
        {
            message = "GeoPackage artifact \"" + m_path.string() + "\" holds " +
                      std::to_string(written) + " rows for \"" +
                      entry.variableName() + "\"; the catalog records " +
                      std::to_string(entry.elementCount());
            return false;
        }
        message.clear();
        return true;
    }


    std::filesystem::path                     m_path;
    sqlite3                                  *m_db = nullptr;
    std::map<std::string, std::vector<double>> m_cache;
};

#endif // HYDROCOUPLESDK_HAVE_GEOPACKAGE

} // namespace

// ── IResultReader ─────────────────────────────────────────────────────────

bool IResultReader::readTimes(const ResultEntry &entry,
                              std::vector<double> &times,
                              std::string &message)
{
    if (!entry.time.has_value())
    {
        message = "result \"" + entry.itemId + "\" records no time axis";
        return false;
    }
    const ResultTimeAxis &axis = *entry.time;
    times.assign(static_cast<size_t>(axis.count), axis.start);
    if (axis.count > 1)
    {
        const double step = (axis.end - axis.start) /
                            static_cast<double>(axis.count - 1);
        for (int64_t i = 0; i < axis.count; ++i)
            times[static_cast<size_t>(i)] =
                axis.start + step * static_cast<double>(i);
    }
    message.clear();
    return true;
}

bool IResultReader::readMesh(const ResultEntry &entry, MeshDefinition &mesh,
                             std::string &message)
{
    (void)mesh;

    // A reason rather than a failure. An artifact of values only is a
    // complete recording of values, and a caller that treats "no geometry"
    // as an error would refuse to open half the runs there are.
    message = "result format \"" + entry.format +
              "\" records values without geometry";
    return false;
}

// ── Registry ──────────────────────────────────────────────────────────────

std::unique_ptr<IResultReader> createResultReader(const std::string &format,
                                                  std::string &message)
{
    if (format == "csv")
    {
        message.clear();
        return std::make_unique<CSVResultReader>();
    }
#ifdef HYDROCOUPLESDK_HAVE_HDF5
    if (format == "hdf5_ugrid")
    {
        message.clear();
        return std::make_unique<HDF5ResultReader>();
    }
#endif
#ifdef HYDROCOUPLESDK_HAVE_NETCDF
    if (format == "netcdf_ugrid")
    {
        message.clear();
        return std::make_unique<NetCDFResultReader>();
    }
#endif
#ifdef HYDROCOUPLESDK_HAVE_GEOPACKAGE
    if (format == "geopackage")
    {
        message.clear();
        return std::make_unique<GeoPackageResultReader>();
    }
#endif

    if (format == "hdf5_ugrid" || format == "netcdf_ugrid" ||
        format == "geopackage")
    {
        message = "this SDK build has no reader for result format \"" +
                  format + "\" — rebuild with the matching feature enabled";
    }
    else
    {
        message = "unknown result format \"" + format + "\"";
    }
    return nullptr;
}

bool resultFormatSupported(const std::string &format)
{
    std::string message;
    return createResultReader(format, message) != nullptr;
}

} // namespace HydroCouple::SDK::IO
