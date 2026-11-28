// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin
#include "hydrocouplesdk/io/meshpackage.h"
#include "hydrocouplesdk/io/ugridreader.h"
#ifdef HYDROCOUPLESDK_HAVE_NETCDF
#include "hydrocouplesdk/io/netcdfugridwriter.h"
#endif
#ifdef USE_GDAL
#include <ogr_spatialref.h>
#endif
#include <charconv>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace HydroCouple::SDK::IO {
namespace {
using Json = nlohmann::json;

/*!
 * Meters per unit of a length unit spelling.
 *
 * \param[in] unit "m", "ft" or "us-survey-ft".
 * \return 1, 0.3048 or 1200/3937.
 * \throws std::invalid_argument for any other spelling.
 */
double scale(const std::string &unit)
{
    if (unit == "m") return 1.0;
    if (unit == "ft") return 0.3048;
    if (unit == "us-survey-ft") return 1200.0 / 3937.0;
    throw std::invalid_argument("unsupported length unit '" + unit +
                                "'; use m, ft or us-survey-ft");
}

/*!
 * Parses a whole token as a 64-bit integer.
 *
 * \throws std::invalid_argument "expected integer, got '...'" when the
 *         token is not entirely an integer.
 */
int64_t integer(const std::string &text)
{
    int64_t value = 0;
    auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size())
        throw std::invalid_argument("expected integer, got '" + text + "'");
    return value;
}

/*!
 * Parses a whole token as a finite double.
 *
 * \throws std::invalid_argument "expected finite number, got '...'" for
 *         trailing text or a non-finite value; std::stod's own exception
 *         (message "stod") when the token does not start with a number.
 */
double number(const std::string &text)
{
    size_t count = 0;
    double value = std::stod(text, &count);
    if (count != text.size() || !std::isfinite(value))
        throw std::invalid_argument("expected finite number, got '" + text + "'");
    return value;
}

/*!
 * Splits a source line into whitespace-separated tokens.
 *
 * Double quotes group a token (the quotes are dropped); ';' outside quotes
 * ends the line.
 *
 * \throws std::invalid_argument "unterminated quoted token".
 */
std::vector<std::string> words(const std::string &line)
{
    std::vector<std::string> result;
    std::string word;
    bool quoted = false;
    for (char ch : line) {
        if (ch == '"') { quoted = !quoted; continue; }
        if (!quoted && ch == ';') break;
        if (!quoted && std::isspace(static_cast<unsigned char>(ch))) {
            if (!word.empty()) { result.push_back(word); word.clear(); }
        } else word += ch;
    }
    if (quoted) throw std::invalid_argument("unterminated quoted token");
    if (!word.empty()) result.push_back(word);
    return result;
}

// Stable content identity for reproducibility, not a security signature.
/*!
 * FNV-1a 64-bit hash of \p bytes, written "fnv1a64:" plus 16 hexadecimal
 * digits.
 */
std::string fingerprint(const std::string &bytes)
{
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char ch : bytes) { h ^= ch; h *= 1099511628211ULL; }
    std::ostringstream out;
    out << "fnv1a64:" << std::hex << std::setw(16) << std::setfill('0') << h;
    return out.str();
}

/*!
 * The geometry a package fingerprint covers: coordinates, elevations,
 * face connectivity, edges and the mesh name.
 */
Json geometry(const MeshDefinition &mesh)
{
    return Json{{"x", mesh.nodeX}, {"y", mesh.nodeY}, {"z", mesh.nodeZ},
                {"offsets", mesh.faceNodeOffsets}, {"nodes", mesh.faceNodes},
                {"edges", mesh.edgeNodes}, {"name", mesh.meshName}};
}

/*!
 * Checks the declared references of a package.
 *
 * Requires a horizontal CRS, a vertical datum other than "unknown", and an
 * elevation kind of "bed-elevation" or "terrain-elevation". A CRS other than
 * "LOCAL" must, with GDAL, be projected and in meters; without GDAL it is
 * refused.
 *
 * \param[in]  package The package to check.
 * \param[out] message Why the declaration is refused.
 * \return True when the references are acceptable.
 */
bool referenceMetadata(const MeshPackage &package, std::string &message)
{
    if (package.horizontalCrs.empty() || package.verticalDatum.empty() ||
        package.verticalDatum == "unknown" ||
        (package.elevationKind != "bed-elevation" && package.elevationKind != "terrain-elevation")) {
        message = "declare horizontal CRS, vertical datum and elevation kind "
                  "(bed-elevation or terrain-elevation)";
        return false;
    }
    if (package.horizontalCrs != "LOCAL") {
#ifdef USE_GDAL
        OGRSpatialReference reference;
        if (reference.SetFromUserInput(package.horizontalCrs.c_str()) != OGRERR_NONE ||
            !reference.IsProjected()) {
            message = "mesh CRS must be projected; reproject geographic coordinates before import";
            return false;
        }
        if (std::abs(reference.GetLinearUnits() - 1.0) > 1.0e-12) {
            message = "mesh package coordinates are metres; its projected CRS must also use metres";
            return false;
        }
#else
        message = "projected CRS validation requires a GDAL-enabled SDK; use LOCAL "
                  "only for an explicitly local Cartesian domain";
        return false;
#endif
    }
    return true;
}

/*!
 * One cell as read from the source, before triangles and quads are
 * ordered: its id (SMS) and tag, its node ids, and the SWMM roughness and
 * optional depth (in meters).
 */
struct Cell {
    std::string id, tag;
    std::vector<int64_t> nodes;
    double roughness = -1.0, depth = 0.0;
    bool hasDepth = false;
};
} // namespace

bool MeshPackage::validate(std::string &message) const
{
    if (!referenceMetadata(*this, message) || !mesh.validate(message)) return false;
    const auto nodes = static_cast<size_t>(mesh.nodeCount());
    const auto faces = static_cast<size_t>(mesh.faceCount());
    auto fail = [&](const std::string &text) { message = text; return false; };
    if (!nodes || !faces || mesh.nodeZ.size() != nodes)
        return fail("mesh package needs faces and one declared elevation per node");
    if (nodeIds.size() != nodes || faceIds.size() != faces || regionTags.size() != faces)
        return fail("mesh identities/tags do not match node and face counts");
    if ((!roughness.empty() && roughness.size() != faces) ||
        (!initialDepth.empty() && initialDepth.size() != faces))
        return fail("mesh attributes do not match face count");
    for (const auto &ids : {nodeIds, faceIds}) {
        std::set<std::string> unique;
        for (const auto &id : ids)
            if (id.empty() || !unique.insert(id).second)
                return fail("empty or duplicate source mesh identity");
    }
    for (const auto *array : {&mesh.nodeX, &mesh.nodeY, &mesh.nodeZ})
        for (double value : *array)
            if (!std::isfinite(value)) return fail("non-finite mesh coordinate/elevation");
    for (const auto *array : {&roughness, &initialDepth})
        for (double value : *array)
            if (!std::isfinite(value) || value < 0.0)
                return fail("mesh roughness/depth must be finite and nonnegative");
    std::map<std::pair<int64_t, int64_t>, int> edges;
    for (size_t f = 0; f < faces; ++f) {
        const int64_t begin = mesh.faceNodeOffsets[f], end = mesh.faceNodeOffsets[f + 1];
        if (end - begin < 3) return fail("mesh face needs at least three nodes");
        std::set<int64_t> seen;
        for (int64_t i = begin; i < end; ++i) {
            const int64_t a = mesh.faceNodes[static_cast<size_t>(i)];
            const int64_t b = mesh.faceNodes[static_cast<size_t>(i + 1 == end ? begin : i + 1)];
            if (!seen.insert(a).second) return fail("mesh face repeats a node");
            auto key = std::minmax(a, b);
            if (++edges[{key.first, key.second}] > 2) return fail("non-manifold mesh edge");
        }
    }
    std::set<std::pair<int64_t, int64_t>> assigned;
    for (const auto &boundary : boundaries) {
        auto key = std::minmax(boundary.node0, boundary.node1);
        const auto pair = std::make_pair(key.first, key.second);
        if (!edges.contains(pair) || edges[pair] != 1 ||
            !assigned.insert(pair).second || boundary.type.empty())
            return fail("boundary must identify one unique exterior edge by endpoints");
    }
    if (!sourceSections.is_object() || !provenance.is_object())
        return fail("mesh provenance and source sections must be objects");
    message.clear();
    return true;
}

bool importSurfaceMesh(const std::filesystem::path &path,
                       const MeshImportOptions &options, MeshPackage &package,
                       std::string &message)
{
    size_t lineNo = 0;
    try {
        MeshPackage candidate;
        candidate.horizontalCrs = options.horizontalCrs;
        candidate.verticalDatum = options.verticalDatum;
        candidate.elevationKind = options.elevationKind;
        const double xy = scale(options.horizontalUnits), z = scale(options.verticalUnits),
                     depth = scale(options.depthUnits);
#ifdef USE_GDAL
        if (candidate.horizontalCrs != "LOCAL") {
            OGRSpatialReference reference;
            if (reference.SetFromUserInput(options.horizontalCrs.c_str()) != OGRERR_NONE ||
                !reference.IsProjected())
                throw std::invalid_argument("declare a valid projected source CRS");
            // SWMM exports may already be in metres while SOURCE_CRS still
            // names the original feet-based projection. Change the CRS units
            // and false easting/northing consistently with the SI coordinates.
            if (reference.SetLinearUnitsAndUpdateParameters("metre", 1.0) != OGRERR_NONE)
                throw std::invalid_argument("could not normalize projected CRS units");
            char *wkt = nullptr;
            if (reference.exportToWkt(&wkt) != OGRERR_NONE || !wkt)
                throw std::invalid_argument("could not serialize normalized CRS");
            candidate.horizontalCrs = wkt;
            CPLFree(wkt);
        }
#endif
        if (!referenceMetadata(candidate, message)) return false;
        std::ifstream in(path);
        if (!in) { message = "cannot open surface mesh: " + path.string(); return false; }
        std::ostringstream bytes;
        bytes << in.rdbuf();
        if (in.bad()) throw std::runtime_error("failed reading surface mesh");
        const std::string text = bytes.str();
        candidate.mesh.meshName = "mesh2d";
        std::map<int64_t, int64_t> nodeLookup;
        std::vector<Cell> triangles, quads;
        std::string section, line;
        bool swmm = false, sms = false;
        Json headers = Json::object();
        std::istringstream lines(text);
        while (std::getline(lines, line)) {
            ++lineNo;
            const auto comment = line.find(";;");
            if (comment != std::string::npos && line.find_first_not_of(" \t") == comment) {
                const auto colon = line.find(':', comment + 2);
                if (colon != std::string::npos) {
                    auto trim = [](std::string value) {
                        const auto first = value.find_first_not_of(" \t\r");
                        return first == std::string::npos ? std::string{} :
                            value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
                    };
                    auto key = trim(line.substr(comment + 2, colon - comment - 2));
                    std::transform(key.begin(), key.end(), key.begin(),
                        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
                    const auto value = trim(line.substr(colon + 1));
                    if (key == "UNITS" || key == "SOURCE_CRS") {
                        if (headers.contains(key) && headers[key] != value)
                            throw std::invalid_argument("conflicting source metadata headers");
                        headers[key] = value;
                    }
                    if (key == "UNITS") {
                        double declared = 0.0;
                        if (value == "SI (m)" || value == "m" || value == "metre" ||
                            value == "meter" || value == "metres" || value == "meters") declared = 1.0;
                        else if (value == "ft" || value == "feet") declared = 0.3048;
                        else if (value == "us-survey-ft" || value == "US survey foot") declared = 1200.0 / 3937.0;
                        else throw std::invalid_argument("unrecognized source UNITS header: " + value);
                        if (std::abs(declared - xy) > 1e-12 || std::abs(declared - z) > 1e-12)
                            throw std::invalid_argument("source UNITS header disagrees with explicit XY/Z units");
                    }
                    if (key == "SOURCE_CRS" && value != options.horizontalCrs) {
#ifdef USE_GDAL
                        OGRSpatialReference authored, supplied;
                        if (authored.SetFromUserInput(value.c_str()) != OGRERR_NONE ||
                            supplied.SetFromUserInput(options.horizontalCrs.c_str()) != OGRERR_NONE ||
                            !authored.IsSame(&supplied))
#endif
                            throw std::invalid_argument("source CRS header disagrees with explicit CRS");
                    }
                }
            }
            auto row = words(line);
            if (row.empty()) continue;
            if (row[0].front() == '[') {
                if (sms) throw std::invalid_argument("mixed SMS and SWMM grammars");
                section = row[0];
                swmm = true;
                continue;
            }
            if (section.empty()) {
                if (row[0] == "MESH2D") { sms = true; continue; }
                if (row[0] == "ND" || row[0] == "E3T" || row[0] == "E4Q") sms = true;
                else throw std::invalid_argument("unrecognized mesh grammar/card '" + row[0] + "'");
            }
            if (sms && swmm) throw std::invalid_argument("mixed SMS and SWMM grammars");
            if ((sms && row[0] == "ND") || section == "[2D_VERTICES]") {
                const size_t offset = sms ? 2 : 0;
                if (row.size() < offset + 3) throw std::invalid_argument("node needs x y z");
                const int64_t id = sms ? integer(row[1]) : candidate.mesh.nodeCount();
                if (!nodeLookup.emplace(id, candidate.mesh.nodeCount()).second)
                    throw std::invalid_argument("duplicate node id");
                candidate.nodeIds.push_back(std::to_string(id));
                candidate.mesh.nodeX.push_back(number(row[offset]) * xy);
                candidate.mesh.nodeY.push_back(number(row[offset + 1]) * xy);
                candidate.mesh.nodeZ.push_back(number(row[offset + 2]) * z);
            } else if ((sms && (row[0] == "E3T" || row[0] == "E4Q")) ||
                       section == "[2D_TRIANGLES]" || section == "[2D_QUADS]") {
                const int nv = (row[0] == "E4Q" || section == "[2D_QUADS]") ? 4 : 3;
                const size_t offset = sms ? 2 : 0;
                if (row.size() < offset + nv + (sms ? 0 : 1))
                    throw std::invalid_argument("cell needs vertex IDs and SWMM roughness");
                Cell cell;
                cell.id = sms ? row[1] : "";
                for (int k = 0; k < nv; ++k) cell.nodes.push_back(integer(row[offset + k]));
                if (sms) {
                    if (row.size() > offset + nv) cell.tag = row[offset + nv];
                } else {
                    cell.roughness = number(row[nv]);
                    if (row.size() > static_cast<size_t>(nv + 1)) {
                        // A nonnumeric legacy tail is a tag, not a depth.
                        const std::string &tail = row[nv + 1];
                        if (std::isdigit(static_cast<unsigned char>(tail.front())) ||
                            tail.front() == '-' || tail.front() == '+' || tail.front() == '.') {
                            cell.depth = number(tail) * depth;
                            cell.hasDepth = true;
                        } else cell.tag = tail;
                        if (row.size() > static_cast<size_t>(nv + 2)) {
                            if (!cell.tag.empty()) throw std::invalid_argument("ambiguous cell attribute tail");
                            cell.tag = row[nv + 2];
                        }
                    }
                }
                (nv == 3 ? triangles : quads).push_back(std::move(cell));
            } else if (swmm) {
                // Retain all native boundary, infiltration, coupling and option rows.
                if (!candidate.sourceSections.contains(section))
                    candidate.sourceSections[section] = Json::array();
                candidate.sourceSections[section].push_back(row);
            } else {
                throw std::invalid_argument("unsupported SMS card '" + row[0] +
                                            "'; export geometry plus explicit metadata");
            }
        }
        candidate.mesh.faceNodeOffsets = {0};
        bool hasDepth = false;
        for (const auto *cells : {&triangles, &quads}) {
            for (const Cell &cell : *cells) {
                const auto face = candidate.mesh.faceCount();
                candidate.faceIds.push_back(sms ? cell.id : std::to_string(face));
                candidate.regionTags.push_back(cell.tag);
                if (!sms) {
                    if (cell.roughness < 0.0 || cell.depth < 0.0)
                        throw std::invalid_argument("negative roughness or depth");
                    candidate.roughness.push_back(cell.roughness);
                    candidate.initialDepth.push_back(cell.depth);
                    hasDepth = hasDepth || cell.hasDepth;
                }
                for (auto id : cell.nodes) {
                    if (!nodeLookup.contains(id)) throw std::invalid_argument("cell references unknown node id");
                    candidate.mesh.faceNodes.push_back(nodeLookup.at(id));
                }
                candidate.mesh.faceNodeOffsets.push_back(static_cast<int64_t>(candidate.mesh.faceNodes.size()));
            }
        }
        if (!hasDepth) candidate.initialDepth.clear();
        if (candidate.sourceSections.contains("[2D_BOUNDARY_CONDITIONS]")) {
            for (const auto &jsonRow : candidate.sourceSections["[2D_BOUNDARY_CONDITIONS]"]) {
                const auto row = jsonRow.get<std::vector<std::string>>();
                if (row.size() < 3) throw std::invalid_argument("boundary needs cell, edge and type");
                const int64_t face = integer(row[0]), edge = integer(row[1]);
                if (face < 0 || face >= candidate.mesh.faceCount()) throw std::invalid_argument("boundary cell out of range");
                const int64_t begin = candidate.mesh.faceNodeOffsets[static_cast<size_t>(face)];
                const int64_t nv = candidate.mesh.faceNodeOffsets[static_cast<size_t>(face + 1)] - begin;
                if (edge < 0 || edge >= nv) throw std::invalid_argument("boundary local edge out of range");
                MeshBoundary boundary;
                boundary.node0 = candidate.mesh.faceNodes[static_cast<size_t>(begin + (edge + 1) % nv)];
                boundary.node1 = candidate.mesh.faceNodes[static_cast<size_t>(begin + (edge + 2) % nv)];
                boundary.type = row[2];
                for (size_t i = 3; i < row.size() && i < 5; ++i) boundary.parameters.push_back(row[i]);
                if (row.size() > 5 && row[5] != "*") boundary.group = row[5];
                candidate.boundaries.push_back(std::move(boundary));
            }
        }
        candidate.provenance = Json{{"source", path.filename().string()},
            {"source_fingerprint", fingerprint(text)}, {"format", sms ? "SMS-2DM" : "SWMM-2D"},
            {"source_horizontal_units", options.horizontalUnits}, {"source_vertical_units", options.verticalUnits},
            {"source_depth_units", options.depthUnits}, {"quad_conversion", "none"}};
        candidate.provenance["source_horizontal_crs"] = options.horizontalCrs;
        candidate.provenance["source_headers"] = std::move(headers);
        if (!candidate.validate(message)) return false;
        package = std::move(candidate);
        message.clear();
        return true;
    } catch (const std::exception &error) {
        message = "surface mesh line " + std::to_string(lineNo) + ": " + error.what();
        return false;
    }
}

bool writeMeshPackage(const std::filesystem::path &manifest,
                      const MeshPackage &package, std::string &message)
{
#ifndef HYDROCOUPLESDK_HAVE_NETCDF
    (void)manifest; (void)package;
    message = "mesh packages require an SDK built with NetCDF";
    return false;
#else
    if (!package.validate(message)) return false;
    auto nc = manifest;
    nc.replace_extension(".nc");
    bool ownsNc = false, ownsManifest = false;
    try {
        if (std::filesystem::exists(manifest) || std::filesystem::exists(nc))
            throw std::invalid_argument("mesh package outputs already exist; choose a new destination");
        if (!manifest.parent_path().empty()) std::filesystem::create_directories(manifest.parent_path());
        ownsNc = true;
        NetCDFUGRIDWriter writer(nc, package.mesh);
        if (writer.initialize() != 0 || writer.finalize() != 0)
            throw std::runtime_error("writing mesh package UGRID failed");
        Json boundaries = Json::array();
        for (const auto &b : package.boundaries)
            boundaries.push_back({{"nodes", {b.node0, b.node1}}, {"type", b.type},
                                   {"group", b.group}, {"parameters", b.parameters}});
        Json data{{"schema", "hydrocouple-mesh-package"}, {"version", "1.0"},
            {"mesh", nc.filename().string()}, {"mesh_name", package.mesh.meshName},
            {"geometry_fingerprint", fingerprint(geometry(package.mesh).dump())},
            {"horizontal_crs", package.horizontalCrs}, {"vertical_datum", package.verticalDatum},
            {"horizontal_units", "m"}, {"vertical_units", "m"}, {"positive", "up"},
            {"elevation_kind", package.elevationKind}, {"node_ids", package.nodeIds},
            {"face_ids", package.faceIds}, {"region_tags", package.regionTags},
            {"roughness", package.roughness}, {"initial_depth_m", package.initialDepth},
            {"boundaries", boundaries}, {"source_sections", package.sourceSections},
            {"provenance", package.provenance}};
        ownsManifest = true;
        std::ofstream out(manifest);
        out << data.dump(2) << '\n';
        out.close();
        if (!out) throw std::runtime_error("writing mesh package manifest failed");
        message.clear();
        return true;
    } catch (const std::exception &error) {
        std::error_code ignored;
        if (ownsManifest) std::filesystem::remove(manifest, ignored);
        if (ownsNc) std::filesystem::remove(nc, ignored);
        message = error.what();
        return false;
    }
#endif
}

bool readMeshPackage(const std::filesystem::path &manifest,
                     MeshPackage &package, std::string &message)
{
    try {
        std::ifstream in(manifest);
        if (!in) throw std::runtime_error("cannot open mesh package manifest");
        Json data = Json::parse(in);
        if (data.at("schema") != "hydrocouple-mesh-package" || data.at("version") != "1.0")
            throw std::invalid_argument("unsupported mesh package schema/version");
        if (data.at("horizontal_units") != "m" || data.at("vertical_units") != "m" ||
            data.at("positive") != "up") throw std::invalid_argument("package coordinates must be metres, positive up");
        MeshPackage candidate;
        const auto nc = manifest.parent_path() / data.at("mesh").get<std::string>();
        if (!readUGRIDMesh(nc, data.at("mesh_name"), candidate.mesh, message)) return false;
        if (data.at("geometry_fingerprint") != fingerprint(geometry(candidate.mesh).dump()))
            throw std::invalid_argument("mesh geometry fingerprint disagrees with manifest");
        candidate.horizontalCrs = data.at("horizontal_crs");
        candidate.verticalDatum = data.at("vertical_datum");
        candidate.elevationKind = data.at("elevation_kind");
        candidate.nodeIds = data.at("node_ids").get<std::vector<std::string>>();
        candidate.faceIds = data.at("face_ids").get<std::vector<std::string>>();
        candidate.regionTags = data.at("region_tags").get<std::vector<std::string>>();
        candidate.roughness = data.at("roughness").get<std::vector<double>>();
        candidate.initialDepth = data.at("initial_depth_m").get<std::vector<double>>();
        candidate.sourceSections = data.at("source_sections");
        candidate.provenance = data.at("provenance");
        for (const auto &entry : data.at("boundaries")) {
            const auto nodes = entry.at("nodes").get<std::vector<int64_t>>();
            if (nodes.size() != 2) throw std::invalid_argument("boundary needs two endpoint indexes");
            candidate.boundaries.push_back({nodes[0], nodes[1], entry.at("type"), entry.at("group"),
                                            entry.at("parameters").get<std::vector<std::string>>()});
        }
        if (!candidate.validate(message)) return false;
        package = std::move(candidate);
        message.clear();
        return true;
    } catch (const std::exception &error) {
        message = "mesh package: " + std::string(error.what());
        return false;
    }
}
MeshPackageReference::MeshPackageReference(const MeshPackage &package)
    : m_horizontal(package.horizontalCrs)
{
    std::string message;
    if (!referenceMetadata(package, message)) throw std::invalid_argument(message);
    auto quote = [](const std::string &text) {
        std::string escaped;
        for (char c : text) { if (c == '"') escaped += '"'; escaped += c; }
        return escaped;
    };
    if (m_horizontal == "LOCAL")
        m_horizontal = "LOCAL_CS[\"Local Cartesian mesh\",LOCAL_DATUM[\"Local\",0],"
                       "UNIT[\"metre\",1],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH]]";
#ifdef USE_GDAL
    OGRSpatialReference horizontal;
    if (horizontal.SetFromUserInput(m_horizontal.c_str()) != OGRERR_NONE)
        throw std::invalid_argument("invalid mesh package horizontal reference");
    if (const char *authority = horizontal.GetAuthorityName(nullptr)) m_authority = authority;
    if (const char *code = horizontal.GetAuthorityCode(nullptr)) m_code = std::stoi(code);
    char *wkt = nullptr;
    if (horizontal.exportToWkt(&wkt) == OGRERR_NONE && wkt) { m_horizontal = wkt; CPLFree(wkt); }
    OGRSpatialReference vertical;
    if (package.verticalDatum.starts_with("EPSG:") &&
        vertical.SetFromUserInput(package.verticalDatum.c_str()) == OGRERR_NONE && vertical.IsVertical()) {
        if (const char *authority = vertical.GetAuthorityName(nullptr)) m_verticalAuthority = authority;
        if (const char *code = vertical.GetAuthorityCode(nullptr)) m_verticalCode = std::stoi(code);
        if (std::abs(vertical.GetLinearUnits() - 1.0) > 1e-12)
            throw std::invalid_argument("mesh package vertical CRS must use metres");
        wkt = nullptr;
        if (vertical.exportToWkt(&wkt) == OGRERR_NONE && wkt) { m_vertical = wkt; CPLFree(wkt); }
    }
#endif
    if (m_vertical.empty()) {
        const auto name = quote(package.verticalDatum);
        m_vertical = "VERT_CS[\"" + name + "\",VERT_DATUM[\"" + name +
            "\",2005],UNIT[\"metre\",1],AXIS[\"Up\",UP]]";
    }
}

HydroCouple::IUnit::DistanceUnits MeshPackageReference::distanceUnits() const
{ return HydroCouple::IUnit::DistanceUnits::Meters; }
HydroCouple::IUnit::DistanceUnits MeshPackageReference::verticalDistanceUnits() const
{ return HydroCouple::IUnit::DistanceUnits::Meters; }

} // namespace HydroCouple::SDK::IO
