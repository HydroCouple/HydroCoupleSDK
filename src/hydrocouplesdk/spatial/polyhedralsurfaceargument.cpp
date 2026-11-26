// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/spatial/polyhedralsurfaceargument.h"

#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/io/ugridreader.h"
#include "hydrocouplesdk/io/uriresolver.h"
#include "hydrocouplespatial.h"

#include <array>

namespace HydroCouple::SDK::Spatial {

namespace {

//! Short name for the argument input type the IArgument overloads switch on.
using AT = HydroCouple::IArgument::ArgumentInputType;

/*!
 * The coordinate arrays a MeshDefinition must agree on before anything
 * downstream trusts its counts.
 *
 * Checks that nodeY has as many values as nodeX, that nodeZ is empty or as
 * long, and that the face offsets, when present, end at the length of
 * faceNodes. It does not check offset monotonicity or index bounds
 * (MeshDefinition::validate() does).
 *
 * \param[in]  mesh    The mesh to check.
 * \param[out] message Receives which arrays disagree, with their lengths,
 *                     when the check fails; unchanged otherwise.
 * \returns True when the three conditions hold.
 */
bool meshIsCoherent(const HydroCouple::SDK::IO::MeshDefinition &mesh,
                    std::string &message)
{
    if (mesh.nodeX.size() != mesh.nodeY.size())
    {
        message = "Mesh has " + std::to_string(mesh.nodeX.size()) +
                  " x coordinates and " + std::to_string(mesh.nodeY.size()) +
                  " y coordinates";
        return false;
    }
    if (!mesh.nodeZ.empty() && mesh.nodeZ.size() != mesh.nodeX.size())
    {
        message = "Mesh has " + std::to_string(mesh.nodeX.size()) +
                  " nodes but " + std::to_string(mesh.nodeZ.size()) +
                  " z coordinates";
        return false;
    }
    if (!mesh.faceNodeOffsets.empty() &&
        mesh.faceNodeOffsets.back() !=
            static_cast<int64_t>(mesh.faceNodes.size()))
    {
        message = "Mesh face offsets end at " +
                  std::to_string(mesh.faceNodeOffsets.back()) +
                  " but there are " + std::to_string(mesh.faceNodes.size()) +
                  " face nodes";
        return false;
    }
    return true;
}

} // namespace

PolyhedralSurfaceArgument::PolyhedralSurfaceArgument(
    std::string_view id, HydroCouple::IModelComponent *modelComponent)
    : PolyhedralSurfaceArgument(id, modelComponent,
                                Quantity::unitLess("Coordinate"))
{
}

PolyhedralSurfaceArgument::PolyhedralSurfaceArgument(
    std::string_view id, HydroCouple::IModelComponent *modelComponent,
    Quantity *quantity)
    // Member addresses are stable before construction, so borrowing them in
    // the base call is sound; the quantity has to be made first, which is
    // what the delegation is for.
    : AbstractArgument(id, {&m_nodeDimension, &m_coordinateDimension},
                       quantity, modelComponent),
      m_quantity(quantity)
{
    addFileFilter("UGRID NetCDF (*.nc)");
    addFileFilter("Mesh packages (*.mesh.json)");
    addValidComponentDataItemType<
        HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem>();
    addValidComponentDataItemType<PolyhedralSurfaceArgument>();
}

// ── Data plane ────────────────────────────────────────────────────────────

std::vector<int64_t> PolyhedralSurfaceArgument::shape() const
{
    return {static_cast<int64_t>(m_mesh.nodeX.size()), 3};
}

DataKind PolyhedralSurfaceArgument::dataKind() const
{
    return DataKind::Float64;
}

bool PolyhedralSurfaceArgument::getValuesInto(
    const BufferDescriptor &destination, std::span<const int64_t> start,
    std::span<const int64_t> count, std::string *message) const
{
    const int64_t nodes = static_cast<int64_t>(m_mesh.nodeX.size());
    std::vector<double> interleaved(static_cast<size_t>(nodes) * 3, 0.0);
    for (int64_t node = 0; node < nodes; ++node)
    {
        interleaved[static_cast<size_t>(node) * 3 + 0] =
            m_mesh.nodeX[static_cast<size_t>(node)];
        interleaved[static_cast<size_t>(node) * 3 + 1] =
            m_mesh.nodeY[static_cast<size_t>(node)];
        if (!m_mesh.nodeZ.empty())
            interleaved[static_cast<size_t>(node) * 3 + 2] =
                m_mesh.nodeZ[static_cast<size_t>(node)];
    }
    const std::array<int64_t, 2> itemShape{nodes, 3};
    return detail::readHyperslab<double>(interleaved.data(), itemShape,
                                         destination, start, count, message);
}

bool PolyhedralSurfaceArgument::setValuesFrom(const BufferDescriptor &,
                                              std::span<const int64_t>,
                                              std::span<const int64_t>,
                                              std::string *message)
{
    if (message)
        *message = "A mesh argument is loaded through initialize(), not "
                   "written one coordinate at a time";
    return false;
}

// ── IArgument ─────────────────────────────────────────────────────────────

std::string PolyhedralSurfaceArgument::toString() const
{
    return "PolyhedralSurface(nodes=" + std::to_string(m_mesh.nodeX.size()) +
           ", faces=" + std::to_string(m_mesh.faceCount()) + ")";
}

// The definition below spells its parameter type with the local alias AT,
// which Doxygen cannot match to the declaration (it reports "no uniquely
// matching class member"); the contract is documented on the declaration in
// polyhedralsurfaceargument.h, so the definition is kept out of Doxygen.
//! \cond
bool PolyhedralSurfaceArgument::initialize(const std::string &value,
                                           AT argType, std::string &message)
{
    switch (argType)
    {
    case AT::File:
    {
        if (std::filesystem::path(value).extension() == ".json") {
            HydroCouple::SDK::IO::MeshPackage package;
            if (!HydroCouple::SDK::IO::readMeshPackage(value, package, message))
                return false;
            adoptMesh(package.mesh);
            m_meshPackage = std::move(package);
            m_sourceReference = value;
            setCurrentArgumentInputType(AT::File);
            setSourceUri(std::string());
            return true;
        }
        HydroCouple::SDK::IO::MeshDefinition mesh;
        // The reader's own suite owns coherence for what it produces; a
        // second check here would be a defense nothing can falsify.
        if (!HydroCouple::SDK::IO::readUGRIDMesh(value, std::string(), mesh,
                                                 message))
            return false;
        adoptMesh(std::move(mesh));
        m_sourceReference = value;
        setCurrentArgumentInputType(AT::File);
        setSourceUri(std::string());
        message.clear();
        return true;
    }
    case AT::JSON:
    {
        std::string uri;
        if (HydroCouple::SDK::IO::jsonUriReference(value, uri))
        {
            const std::string scheme = HydroCouple::SDK::IO::uriScheme(uri);
            if (scheme.empty() || scheme == "file")
                return initialize(uri, AT::File, message);
            message = "Remote UGRID meshes (\"" + uri +
                      "\") are not yet supported; pass a local file or bind "
                      "the argument to a mesh-producing component";
            return false;
        }
        message = "A mesh is never inlined into a document; pass "
                  "{\"@uri\": \"<mesh file>\"} or bind the argument with "
                  "@from";
        return false;
    }
    default:
        message = "Mesh arguments accept a UGRID file or an @uri JSON "
                  "reference";
        return false;
    }
}
//! \endcond

bool PolyhedralSurfaceArgument::initialize(
    const HydroCouple::IComponentDataItem &componentDataItem,
    std::string &message)
{
    // Another mesh argument: adopt its mesh wholesale — connectivity and
    // all, which the meshView() path below also preserves.
    if (const auto *other =
            dynamic_cast<const PolyhedralSurfaceArgument *>(
                &componentDataItem))
    {
        if (other == this) { message.clear(); return true; }
        adoptMesh(other->mesh());
        m_meshPackage = other->m_meshPackage;
        m_sourceReference = other->m_sourceReference;
        message.clear();
        return true;
    }

    const auto *item =
        dynamic_cast<const HydroCouple::Spatial::
                         IPolyhedralSurfaceComponentDataItem *>(
            &componentDataItem);
    if (!item)
    {
        message = "Source data item carries no polyhedral surface";
        return false;
    }
    const HydroCouple::Spatial::IPolyhedralSurface *surface =
        item->polyhedralSurface();
    if (!surface)
    {
        message = "Source data item names no surface";
        return false;
    }
    const HydroCouple::Spatial::IMeshView *view = surface->meshView();
    if (!view || view->nodeCount() == 0)
    {
        message = "Source surface exposes no mesh view to copy from";
        return false;
    }

    HydroCouple::SDK::IO::MeshDefinition mesh;
    const auto nodeX = view->nodeX();
    const auto nodeY = view->nodeY();
    const auto nodeZ = view->nodeZ();
    mesh.nodeX.assign(nodeX.begin(), nodeX.end());
    mesh.nodeY.assign(nodeY.begin(), nodeY.end());
    mesh.nodeZ.assign(nodeZ.begin(), nodeZ.end());
    const auto offsets = view->faceNodeOffsets();
    const auto faces   = view->faceNodes();
    mesh.faceNodeOffsets.assign(offsets.begin(), offsets.end());
    mesh.faceNodes.assign(faces.begin(), faces.end());
    const auto edges = view->edgeNodes();
    mesh.edgeNodes.clear();
    for (size_t edge = 0; edge + 1 < edges.size(); edge += 2)
        mesh.edgeNodes.push_back({edges[edge], edges[edge + 1]});

    if (!meshIsCoherent(mesh, message))
        return false;
    adoptMesh(std::move(mesh));
    // A live output has no file behind it; the document's job is to keep
    // the binding, not a path that never existed.
    m_sourceReference.clear();
    message.clear();
    return true;
}

bool PolyhedralSurfaceArgument::serialize(AT argType, std::string &value,
                                          std::string &message) const
{
    switch (argType)
    {
    case AT::String:
        value = toString();
        message.clear();
        return true;
    case AT::JSON:
        if (m_sourceReference.empty())
        {
            message = "This mesh came from a component output and has no "
                      "file to reference; the composition document keeps "
                      "the @from binding instead";
            return false;
        }
        value = nlohmann::json{
            {HydroCouple::SDK::IO::kUriRefKey, m_sourceReference}}.dump();
        message.clear();
        return true;
    default:
        message = "Mesh arguments serialize as String or JSON";
        return false;
    }
}

void PolyhedralSurfaceArgument::readData(const nlohmann::json &j)
{
    AbstractArgument::readData(j);
    if (j.contains(HydroCouple::SDK::IO::kUriRefKey) &&
        j[HydroCouple::SDK::IO::kUriRefKey].is_string())
    {
        std::string msg;
        (void)initialize(j.dump(), AT::JSON, msg);
    }
}

nlohmann::json PolyhedralSurfaceArgument::writeData() const
{
    auto j = AbstractArgument::writeData();
    if (!m_sourceReference.empty())
        j[HydroCouple::SDK::IO::kUriRefKey] = m_sourceReference;
    return j;
}

// ── IPolyhedralSurfaceComponentDataItem ──────────────────────────────────

void PolyhedralSurfaceArgument::adoptMesh(
    HydroCouple::SDK::IO::MeshDefinition mesh)
{
    m_mesh = std::move(mesh);
    m_meshPackage.reset();

    // Dropped rather than rebuilt: nothing has asked for the surface yet,
    // and building one for a mesh that may be replaced again before
    // anyone reads it would be work done on the chance it is wanted.
    m_surface.reset();
}

HydroCouple::Spatial::MeshLocation
PolyhedralSurfaceArgument::location() const
{
    // The argument's own values are the node coordinates — shape() says
    // { nodes, 3 } — so what this data item is *about* is vertices. A
    // consumer reading Cell here would look for one value per face and
    // find one per node.
    return HydroCouple::Spatial::MeshLocation::Node;
}

HydroCouple::Spatial::SpatialDataType
PolyhedralSurfaceArgument::meshDataType() const
{
    // Three components per vertex: a position, not a scalar field.
    return HydroCouple::Spatial::SpatialDataType::Vector;
}

HydroCouple::Spatial::IPolyhedralSurface *
PolyhedralSurfaceArgument::polyhedralSurface() const
{
    if (m_mesh.nodeX.empty())
    {
        // No mesh is not an empty surface. A caller handed one of those
        // would report zero faces as a fact about the argument rather
        // than as "nothing loaded yet", and act on it.
        return nullptr;
    }

    if (!m_surface)
    {
        // The mesh is passed alongside the surface, not only converted
        // into it. A PolyhedralSurface records polygons and not which
        // vertices each polygon used, so a surface built without the
        // connectivity reports no faces and no edges — which looks like
        // an empty mesh rather than a missing feature (meshadapters.h).
        m_surface = std::make_unique<PolyhedralSurfaceAdapter>(
            polyhedralSurfaceFromMesh(m_mesh), m_mesh);
    }

    return m_surface.get();
}

HydroCouple::IDimension *PolyhedralSurfaceArgument::entityDimension() const
{
    // The argument's own node axis: shape() is { nodes, 3 } against exactly
    // this dimension, so a separate entity dimension would let the two
    // disagree about how many vertices there are.
    return const_cast<Dimension *>(&m_nodeDimension);
}

HydroCouple::Spatial::VectorBasis PolyhedralSurfaceArgument::vectorBasis() const
{
    return HydroCouple::Spatial::VectorBasis::Cartesian;
}

bool PolyhedralSurfaceArgument::setMesh(
    HydroCouple::SDK::IO::MeshDefinition mesh, std::string &message)
{
    if (!meshIsCoherent(mesh, message))
        return false;
    adoptMesh(std::move(mesh));
    m_sourceReference.clear();
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Spatial
