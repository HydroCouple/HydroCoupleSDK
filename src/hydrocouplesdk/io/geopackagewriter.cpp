// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file geopackagewriter.cpp
 * \brief GeoPackageWriter implementation over the SQLite C API, following
 *        the openswmm.engine GeoPackage convention (RAII kit, prepared
 *        statements, buffered transactional inserts).
 */

#include "stdafx.h"
#include "hydrocouplesdk/io/geopackagewriter.h"

#include <sqlite3.h>

#include <cstring>
#include <ctime>
#include <map>

namespace HydroCouple::SDK::IO {

namespace {

/*! GeoPackage application id "GPKG" and 1.3 user version. */
constexpr int32_t kApplicationId = 0x47504B47;
constexpr int32_t kUserVersion   = 10300;

// ── RAII kit (engine pattern) ─────────────────────────────────────────────

/*! Owning sqlite3* handle. */
struct DbPtr
{
    sqlite3 *db = nullptr;
    ~DbPtr() { close(); }
    void close()
    {
        if (db)
        {
            sqlite3_close_v2(db);
            db = nullptr;
        }
    }
};

/*! Owning prepared-statement handle. */
struct StmtPtr
{
    sqlite3_stmt *stmt = nullptr;
    ~StmtPtr() { finalizeStmt(); }
    void finalizeStmt()
    {
        if (stmt)
        {
            sqlite3_finalize(stmt);
            stmt = nullptr;
        }
    }
};

/*! Scope transaction with statement-reset rollback on failure. */
struct Transaction
{
    sqlite3 *db     = nullptr;
    bool     active = false;

    bool begin(sqlite3 *database)
    {
        db = database;
        active = sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr) ==
                 SQLITE_OK;
        return active;
    }
    bool commit()
    {
        if (!active)
            return true;
        active = false;
        return sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) ==
               SQLITE_OK;
    }
    ~Transaction()
    {
        if (active)
            sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    }
};

bool exec(sqlite3 *db, const char *sql, std::string &error)
{
    char *sqliteError = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &sqliteError) != SQLITE_OK)
    {
        error = sqliteError ? sqliteError : "sqlite3_exec failed";
        sqlite3_free(sqliteError);
        return false;
    }
    return true;
}

/*!
 * Encodes a point as GeoPackageBinary: "GP" magic, version 0, flags
 * (little-endian, no envelope), srs_id, then standard WKB point.
 */
std::vector<std::byte> gpkgPointBlob(int32_t srsId, double x, double y)
{
    std::vector<std::byte> blob(8 + 21);
    uint8_t *bytes = reinterpret_cast<uint8_t *>(blob.data());
    bytes[0] = 'G';
    bytes[1] = 'P';
    bytes[2] = 0;    // version
    bytes[3] = 0x01; // flags: little-endian, no envelope
    std::memcpy(bytes + 4, &srsId, 4);
    bytes[8] = 0x01; // WKB little-endian
    const uint32_t geometryType = 1; // Point
    std::memcpy(bytes + 9, &geometryType, 4);
    std::memcpy(bytes + 13, &x, 8);
    std::memcpy(bytes + 21, &y, 8);
    return blob;
}

} // namespace

// ── Impl ──────────────────────────────────────────────────────────────────

struct GeoPackageWriter::Impl
{
    std::filesystem::path path;
    std::string           simulationName;
    int                   flushRowCount = 5000;

    MeshDefinition mesh;
    bool           hasMesh = false;

    struct ObservedExtent
    {
        int64_t timeLevels  = 0;
        int64_t objectCount = 0;
        DataKind kind = DataKind::Float64;
    };

    struct FieldMeta
    {
        std::string units;
        std::string description;
        int64_t     variableId = -1;
    };
    std::map<std::string, FieldMeta> fields;

    std::string error;
    DbPtr       db;
    StmtPtr     insertStmt;
    Transaction transaction;
    int64_t     simulationId = -1;
    int         pendingRows  = 0;

    std::vector<std::string>                       itemOrder;
    std::map<std::string, ObservedExtent>          extents;
    bool   haveTimes = false;
    double firstTime = 0.0;
    double lastTime  = 0.0;

    /*! Returns (creating on demand) the variables row id for \p itemId. */
    bool variableId(const std::string &itemId, int64_t &id)
    {
        FieldMeta &meta = fields[itemId]; // auto-registers
        if (meta.variableId >= 0)
        {
            id = meta.variableId;
            return true;
        }
        StmtPtr stmt;
        if (sqlite3_prepare_v2(db.db,
                               "INSERT INTO variables(item_id, units, "
                               "description) VALUES(?, ?, ?)",
                               -1, &stmt.stmt, nullptr) != SQLITE_OK)
        {
            error = sqlite3_errmsg(db.db);
            return false;
        }
        sqlite3_bind_text(stmt.stmt, 1, itemId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.stmt, 2, meta.units.c_str(), -1,
                          SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.stmt, 3, meta.description.c_str(), -1,
                          SQLITE_TRANSIENT);
        if (sqlite3_step(stmt.stmt) != SQLITE_DONE)
        {
            error = sqlite3_errmsg(db.db);
            return false;
        }
        meta.variableId = sqlite3_last_insert_rowid(db.db);
        id = meta.variableId;
        return true;
    }

    /*! Commits the open insert transaction and begins a fresh one. */
    bool rotateTransaction()
    {
        if (!transaction.commit())
        {
            error = sqlite3_errmsg(db.db);
            return false;
        }
        pendingRows = 0;
        if (!transaction.begin(db.db))
        {
            error = sqlite3_errmsg(db.db);
            return false;
        }
        return true;
    }
};

// ── Construction ──────────────────────────────────────────────────────────

GeoPackageWriter::GeoPackageWriter(std::filesystem::path path,
                                   std::string simulationName,
                                   int flushRowCount)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->path           = std::move(path);
    m_impl->simulationName = std::move(simulationName);
    m_impl->flushRowCount  = std::max(1, flushRowCount);
}

GeoPackageWriter::~GeoPackageWriter()
{
    (void)finalize();
}

void GeoPackageWriter::setMesh(MeshDefinition mesh)
{
    m_impl->mesh    = std::move(mesh);
    m_impl->hasMesh = true;
}

void GeoPackageWriter::addField(const std::string &itemId,
                                const std::string &units,
                                const std::string &description)
{
    Impl::FieldMeta &meta = m_impl->fields[itemId];
    meta.units       = units;
    meta.description = description;
}

std::string GeoPackageWriter::name() const
{
    return "geopackage";
}

const std::filesystem::path &GeoPackageWriter::path() const noexcept
{
    return m_impl->path;
}

std::string GeoPackageWriter::errorMessage() const
{
    return m_impl->error;
}

// ── initialize ────────────────────────────────────────────────────────────

int GeoPackageWriter::initialize()
{
    Impl &impl = *m_impl;

    if (impl.hasMesh && !impl.mesh.validate(impl.error))
        return 30;

    std::error_code ec;
    if (impl.path.has_parent_path())
        std::filesystem::create_directories(impl.path.parent_path(), ec);
    std::filesystem::remove(impl.path, ec);

    if (sqlite3_open_v2(impl.path.string().c_str(), &impl.db.db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        nullptr) != SQLITE_OK)
    {
        impl.error = "cannot open '" + impl.path.string() + "'";
        return 31;
    }

    const std::string pragmas =
        "PRAGMA application_id = " + std::to_string(kApplicationId) + ";"
        "PRAGMA user_version = " + std::to_string(kUserVersion) + ";"
        "PRAGMA synchronous = NORMAL;"
        "PRAGMA foreign_keys = ON;";
    if (!exec(impl.db.db, pragmas.c_str(), impl.error))
        return 32;

    // GeoPackage conformance tables + mandatory SRS rows.
    const char *conformance = R"SQL(
CREATE TABLE gpkg_spatial_ref_sys (
  srs_name TEXT NOT NULL, srs_id INTEGER PRIMARY KEY,
  organization TEXT NOT NULL, organization_coordsys_id INTEGER NOT NULL,
  definition TEXT NOT NULL, description TEXT);
INSERT INTO gpkg_spatial_ref_sys VALUES
  ('Undefined cartesian SRS', -1, 'NONE', -1, 'undefined',
   'undefined cartesian coordinate reference system'),
  ('Undefined geographic SRS', 0, 'NONE', 0, 'undefined',
   'undefined geographic coordinate reference system'),
  ('WGS 84 geodetic', 4326, 'EPSG', 4326,
   'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]]',
   'longitude/latitude coordinates in decimal degrees');
CREATE TABLE gpkg_contents (
  table_name TEXT NOT NULL PRIMARY KEY, data_type TEXT NOT NULL,
  identifier TEXT UNIQUE, description TEXT DEFAULT '',
  last_change DATETIME NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
  min_x DOUBLE, min_y DOUBLE, max_x DOUBLE, max_y DOUBLE,
  srs_id INTEGER,
  CONSTRAINT fk_gc_r_srs_id FOREIGN KEY (srs_id)
    REFERENCES gpkg_spatial_ref_sys(srs_id));
CREATE TABLE gpkg_geometry_columns (
  table_name TEXT NOT NULL, column_name TEXT NOT NULL,
  geometry_type_name TEXT NOT NULL, srs_id INTEGER NOT NULL,
  z TINYINT NOT NULL, m TINYINT NOT NULL,
  CONSTRAINT pk_geom_cols PRIMARY KEY (table_name, column_name),
  CONSTRAINT fk_gc_tn FOREIGN KEY (table_name)
    REFERENCES gpkg_contents(table_name),
  CONSTRAINT fk_gc_srs FOREIGN KEY (srs_id)
    REFERENCES gpkg_spatial_ref_sys(srs_id));
)SQL";
    if (!exec(impl.db.db, conformance, impl.error))
        return 33;

    // Result schema (engine convention).
    const char *results = R"SQL(
CREATE TABLE simulations (
  id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL,
  started_at DATETIME NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')));
CREATE TABLE variables (
  id INTEGER PRIMARY KEY AUTOINCREMENT, item_id TEXT NOT NULL UNIQUE,
  units TEXT DEFAULT '', description TEXT DEFAULT '');
CREATE TABLE result_timeseries (
  simulation_id INTEGER NOT NULL REFERENCES simulations(id),
  variable_id INTEGER NOT NULL REFERENCES variables(id),
  object_index INTEGER NOT NULL,
  elapsed_time DOUBLE NOT NULL,
  value DOUBLE NOT NULL);
CREATE INDEX idx_result_series
  ON result_timeseries(simulation_id, variable_id, object_index);
CREATE TABLE result_summary (
  simulation_id INTEGER NOT NULL REFERENCES simulations(id),
  variable_id INTEGER NOT NULL REFERENCES variables(id),
  count INTEGER NOT NULL, min DOUBLE, max DOUBLE, mean DOUBLE);
INSERT INTO gpkg_contents(table_name, data_type, identifier, description)
  VALUES ('result_timeseries', 'attributes', 'result_timeseries',
          'Simulation result time series'),
         ('result_summary', 'attributes', 'result_summary',
          'Per-variable result aggregates');
)SQL";
    if (!exec(impl.db.db, results, impl.error))
        return 34;

    // Simulation row.
    {
        StmtPtr stmt;
        sqlite3_prepare_v2(impl.db.db,
                           "INSERT INTO simulations(name) VALUES(?)", -1,
                           &stmt.stmt, nullptr);
        sqlite3_bind_text(stmt.stmt, 1, impl.simulationName.c_str(), -1,
                          SQLITE_TRANSIENT);
        if (sqlite3_step(stmt.stmt) != SQLITE_DONE)
        {
            impl.error = sqlite3_errmsg(impl.db.db);
            return 35;
        }
        impl.simulationId = sqlite3_last_insert_rowid(impl.db.db);
    }

    // Optional node feature layer.
    if (impl.hasMesh)
    {
        const char *featureTable = R"SQL(
CREATE TABLE mesh_nodes (
  fid INTEGER PRIMARY KEY AUTOINCREMENT,
  geom BLOB, node_index INTEGER NOT NULL);
INSERT INTO gpkg_contents(table_name, data_type, identifier, description,
                          srs_id)
  VALUES ('mesh_nodes', 'features', 'mesh_nodes', 'Mesh node locations', -1);
INSERT INTO gpkg_geometry_columns
  VALUES ('mesh_nodes', 'geom', 'POINT', -1, 0, 0);
)SQL";
        if (!exec(impl.db.db, featureTable, impl.error))
            return 36;

        Transaction transaction;
        transaction.begin(impl.db.db);
        StmtPtr stmt;
        sqlite3_prepare_v2(impl.db.db,
                           "INSERT INTO mesh_nodes(geom, node_index) "
                           "VALUES(?, ?)",
                           -1, &stmt.stmt, nullptr);
        for (int64_t node = 0; node < impl.mesh.nodeCount(); ++node)
        {
            const auto blob = gpkgPointBlob(
                -1, impl.mesh.nodeX[static_cast<size_t>(node)],
                impl.mesh.nodeY[static_cast<size_t>(node)]);
            sqlite3_reset(stmt.stmt);
            sqlite3_bind_blob(stmt.stmt, 1, blob.data(),
                              static_cast<int>(blob.size()),
                              SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt.stmt, 2, node);
            if (sqlite3_step(stmt.stmt) != SQLITE_DONE)
            {
                impl.error = sqlite3_errmsg(impl.db.db);
                return 37;
            }
        }
        if (!transaction.commit())
        {
            impl.error = sqlite3_errmsg(impl.db.db);
            return 38;
        }
    }

    // Prepared insert + first buffered transaction.
    if (sqlite3_prepare_v2(impl.db.db,
                           "INSERT INTO result_timeseries(simulation_id, "
                           "variable_id, object_index, elapsed_time, value) "
                           "VALUES(?, ?, ?, ?, ?)",
                           -1, &impl.insertStmt.stmt, nullptr) != SQLITE_OK)
    {
        impl.error = sqlite3_errmsg(impl.db.db);
        return 39;
    }
    if (!impl.transaction.begin(impl.db.db))
    {
        impl.error = sqlite3_errmsg(impl.db.db);
        return 39;
    }
    impl.error.clear();
    return 0;
}

// ── write ─────────────────────────────────────────────────────────────────

int GeoPackageWriter::write(const Snapshot &snapshot)
{
    Impl &impl = *m_impl;

    if (!impl.haveTimes)
    {
        impl.firstTime = snapshot.time();
        impl.haveTimes = true;
    }
    impl.lastTime = snapshot.time();

    for (const FieldSlice &field : snapshot.fields())
    {
        int64_t variableId = -1;
        if (!impl.variableId(field.itemId, variableId))
            return 40;

        const int64_t count = field.elementCount();

        // Observed extents back the catalog: a long-format table states
        // neither the time count nor the object count.
        Impl::ObservedExtent &extent = impl.extents[field.itemId];
        if (extent.timeLevels == 0)
        {
            impl.itemOrder.push_back(field.itemId);
            extent.objectCount = count;
            extent.kind        = field.kind;
        }
        ++extent.timeLevels;
        for (int64_t i = 0; i < count; ++i)
        {
            double value = 0.0;
            switch (field.kind)
            {
            case DataKind::Float64: value = field.values<double>()[i];   break;
            case DataKind::Float32: value = field.values<float>()[i];    break;
            case DataKind::Int64:   value = static_cast<double>(field.values<int64_t>()[i]);  break;
            case DataKind::Int32:   value = field.values<int32_t>()[i];  break;
            case DataKind::UInt64:  value = static_cast<double>(field.values<uint64_t>()[i]); break;
            case DataKind::UInt32:  value = field.values<uint32_t>()[i]; break;
            case DataKind::Int16:   value = field.values<int16_t>()[i];  break;
            case DataKind::UInt16:  value = field.values<uint16_t>()[i]; break;
            case DataKind::Int8:    value = field.values<int8_t>()[i];   break;
            case DataKind::UInt8:   value = field.values<uint8_t>()[i];  break;
            case DataKind::Boolean: value = field.values<bool>()[i] ? 1.0 : 0.0; break;
            default:
                impl.error = "field '" + field.itemId +
                             "' has an unsupported DataKind";
                return 41;
            }

            sqlite3_reset(impl.insertStmt.stmt);
            sqlite3_bind_int64(impl.insertStmt.stmt, 1, impl.simulationId);
            sqlite3_bind_int64(impl.insertStmt.stmt, 2, variableId);
            sqlite3_bind_int64(impl.insertStmt.stmt, 3, i);
            sqlite3_bind_double(impl.insertStmt.stmt, 4, snapshot.time());
            sqlite3_bind_double(impl.insertStmt.stmt, 5, value);
            if (sqlite3_step(impl.insertStmt.stmt) != SQLITE_DONE)
            {
                impl.error = sqlite3_errmsg(impl.db.db);
                return 42;
            }
            if (++impl.pendingRows >= impl.flushRowCount &&
                !impl.rotateTransaction())
                return 43;
        }
    }
    return 0;
}

// ── finalize ──────────────────────────────────────────────────────────────

int GeoPackageWriter::finalize()
{
    Impl &impl = *m_impl;
    if (!impl.db.db)
        return 0;

    impl.insertStmt.finalizeStmt();
    if (!impl.transaction.commit())
    {
        impl.error = sqlite3_errmsg(impl.db.db);
        impl.db.close();
        return 44;
    }

    // Aggregates.
    const char *summary = R"SQL(
INSERT INTO result_summary(simulation_id, variable_id, count, min, max, mean)
  SELECT simulation_id, variable_id, COUNT(*), MIN(value), MAX(value),
         AVG(value)
  FROM result_timeseries GROUP BY simulation_id, variable_id;
)SQL";
    const bool summarized = exec(impl.db.db, summary, impl.error);
    impl.db.close();
    return summarized ? 0 : 45;
}

std::vector<ResultEntry> GeoPackageWriter::catalog() const
{
    const Impl &impl = *m_impl;

    std::vector<ResultEntry> entries;
    entries.reserve(impl.itemOrder.size());

    for (const std::string &itemId : impl.itemOrder)
    {
        const Impl::ObservedExtent &extent = impl.extents.at(itemId);

        ResultEntry entry;
        entry.itemId     = itemId;
        entry.artifact   = impl.path.filename().string();
        entry.format     = "geopackage";
        entry.variable   = itemId;
        // result_timeseries stores REAL regardless of the source kind.
        entry.kind       = DataKind::Float64;
        entry.shape      = {extent.timeLevels, extent.objectCount};
        entry.dimensions = {"time", "object"};

        const auto meta = impl.fields.find(itemId);
        if (meta != impl.fields.end())
        {
            entry.units       = meta->second.units;
            entry.description = meta->second.description;
        }
        if (!impl.mesh.meshName.empty())
            entry.mesh = impl.mesh.meshName;

        ResultTimeAxis axis;
        axis.count = extent.timeLevels;
        axis.start = impl.firstTime;
        axis.end   = impl.lastTime;
        entry.time = axis;

        entries.push_back(std::move(entry));
    }
    return entries;
}

} // namespace HydroCouple::SDK::IO
