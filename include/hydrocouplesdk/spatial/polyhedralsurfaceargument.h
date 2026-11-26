// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   polyhedralsurfaceargument.h
 * \author Caleb Buahin
 * \version 2.1.0
 * \brief  PolyhedralSurfaceArgument — a mesh as a component argument.
 *
 * The argument class the provider pipeline turns on: a solver's mesh can be
 * a UGRID file today and a mesh generator's live output tomorrow, and the
 * component holding this argument cannot tell the difference — both arrive
 * as a MeshDefinition, the SDK's structure-of-arrays lingua franca that the
 * writers, readers and adapters already speak.
 *
 * Serialization honors the interface's bulk mandate: a mesh is never
 * inlined into a document. serialize() emits the \c \@uri reference the
 * mesh was read from; a mesh that arrived from a component output has no
 * file to point at, and the composition document keeps the \c \@from
 * binding instead (ModelInitializer re-emits bindings itself, so asking
 * this argument to serialize one is already a misuse worth refusing).
 */

#ifndef HYDROCOUPLESDK_SPATIAL_POLYHEDRALSURFACEARGUMENT_H
#define HYDROCOUPLESDK_SPATIAL_POLYHEDRALSURFACEARGUMENT_H

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/abstractargument.h"
#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouplesdk/io/meshpackage.h"
#include <optional>
#include "meshadapters.h"

#include <memory>
#include <string>
#include <vector>

namespace HydroCouple::SDK::Spatial {

/*!
 * \brief A mesh-valued argument.
 *
 * The data plane exposes the node coordinates as a rank-2 Float64 item of
 * shape { nodeCount, 3 } (x, y, z per node; z is 0 where the mesh carries
 * none) — enough for a generic host to preview the geometry. The
 * authoritative access for a consuming component is mesh(): connectivity
 * does not survive a coordinate table, and a component that meshes its
 * water column needs the faces, not a point cloud.
 *
 * A mesh reaches the argument in one of four ways:
 *
 * | Route | Call | Source reference kept |
 * | --- | --- | --- |
 * | UGRID NetCDF file | initialize(path, File) | the path |
 * | Mesh package manifest (`*.mesh.json`) | initialize(path, File) | the path, plus meshPackage() |
 * | JSON object whose \@uri member names a local file | initialize(text, JSON) | the URI |
 * | Another data item (a generator's output, another mesh argument) | initialize(item) | none (or the other argument's) |
 *
 * plus setMesh() for a program that already holds a MeshDefinition. Reading
 * a UGRID file or a mesh package needs a library built with NetCDF; without
 * it the readers fail with "This build has no NetCDF support, so UGRID files
 * cannot be read." The argument registers the file filters
 * "UGRID NetCDF (*.nc)" and "Mesh packages (*.mesh.json)" and accepts, as
 * data-item sources, any IPolyhedralSurfaceComponentDataItem and any
 * PolyhedralSurfaceArgument. Its value definition is a unitless quantity
 * named "Coordinate" that the argument owns.
 *
 * Through the standard the argument also answers as an
 * IPolyhedralSurfaceComponentDataItem: location() Node, meshDataType()
 * Vector, vectorBasis() Cartesian, and polyhedralSurface() a
 * PolyhedralSurfaceAdapter built on first request from the mesh, with its
 * connectivity, and no reference system.
 *
 * \threadsafety Not thread-safe; mutate before component initialization.
 * polyhedralSurface() builds and caches the surface inside a const call, so
 * even concurrent reads must be serialized until it has been called once.
 */
class HYDROCOUPLESDK_EXPORT PolyhedralSurfaceArgument
    : public HydroCouple::SDK::AbstractArgument,
      public virtual HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem
{
public:
    /*!
     * \brief Constructs an empty mesh argument.
     *
     * hasMesh() is false and polyhedralSurface() returns nullptr until a
     * mesh is loaded.
     *
     * \param id             Argument identifier.
     * \param modelComponent Owning component.
     */
    PolyhedralSurfaceArgument(std::string_view id,
                              HydroCouple::IModelComponent *modelComponent);

    //! Destroys the argument, its mesh, its cached surface and its quantity.
    ~PolyhedralSurfaceArgument() override = default;

    // ── Data plane ────────────────────────────────────────────────────────

    /*!
     * \brief The data-plane shape (IComponentDataItem::shape()).
     * \returns { nodeCount, 3 }; { 0, 3 } before a mesh is loaded.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override;

    /*!
     * \brief The element kind (IComponentDataItem::dataKind()).
     * \returns DataKind::Float64.
     */
    [[nodiscard]] DataKind dataKind() const override;

    /*!
     * \brief Copies node coordinates out (IComponentDataItem::getValuesInto()).
     *
     * Builds an interleaved { node, x/y/z } table from the mesh on every
     * call (z is 0 when the mesh has no nodeZ) and copies the selection.
     *
     * \param[in]  destination Float64 buffer receiving the block.
     * \param[in]  start       { first node, first coordinate (0 = x) }.
     * \param[in]  count       { nodes, coordinates } to copy.
     * \param[out] message     Receives the selection error on failure; may be
     *                         nullptr.
     * \returns False when the selection or the buffer does not fit.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override;

    /*!
     * \brief Refused: a mesh is loaded through initialize(), never poked
     *        one coordinate at a time.
     * \param[in]  source  Ignored.
     * \param[in]  start   Ignored.
     * \param[in]  count   Ignored.
     * \param[out] message Receives "A mesh argument is loaded through
     *                     initialize(), not written one coordinate at a
     *                     time"; may be nullptr.
     * \returns Always false.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── IArgument ─────────────────────────────────────────────────────────

    /*!
     * \brief A one-line summary (IArgument::toString()).
     * \returns "PolyhedralSurface(nodes=N, faces=F)".
     */
    [[nodiscard]] std::string toString() const override;

    /*!
     * \brief File: reads a UGRID NetCDF mesh. JSON: accepts an \c \@uri
     *        reference to one. Inline payloads are refused — bulk is never
     *        inlined.
     *
     * For File, a path whose extension is ".json" is read as a mesh package
     * manifest (readMeshPackage(), which also checks the companion NetCDF
     * file and the geometry fingerprint) and fills meshPackage(); any other
     * path is read as UGRID with readUGRIDMesh(), taking the file's first
     * mesh. For JSON, an object with a non-empty \@uri member whose scheme is empty
     * or "file" is passed on as a File path, unresolved. On success the mesh
     * replaces the previous one (dropping the cached surface), the path is
     * kept as the source reference for serialize(), and the input type is
     * recorded. On failure nothing changes.
     *
     * \param[in]  value   The path (File) or JSON text (JSON).
     * \param[in]  argType File or JSON; other types are refused.
     * \param[out] message On failure, the reader's message, or a message that
     *                     remote UGRID meshes are not yet supported (a URI with a
     *                     scheme other than file), that a mesh is never inlined
     *                     into a document (JSON without an \@uri member; pass a
     *                     reference or bind the argument with \@from), or that
     *                     mesh arguments accept a UGRID file or an \@uri JSON
     *                     reference (any other input type). Cleared after a
     *                     UGRID read; may keep the package reader's text after
     *                     a package read.
     * \returns True when a mesh was loaded.
     */
    [[nodiscard]] bool initialize(
        const std::string &value,
        HydroCouple::IArgument::ArgumentInputType argType,
        std::string &message) override;

    /*!
     * \brief Adopts a mesh from another data item — an
     *        IPolyhedralSurfaceComponentDataItem (a mesh generator's
     *        output, via its surface's meshView()) or another
     *        PolyhedralSurfaceArgument.
     *
     * From another PolyhedralSurfaceArgument the mesh, the mesh package and
     * the source reference are copied as they are (initializing from itself
     * is a successful no-op). From any other surface item the mesh view's
     * nodes, face-node lists and edge pairs are copied, the edges a
     * MeshViewAdapter derived from face loops included, checked for
     * coherence (matching coordinate lengths; face offsets ending at the
     * face-node count), and the source reference is cleared, so serialize()
     * to JSON then refuses.
     *
     * \param[in]  componentDataItem The source item.
     * \param[out] message           On failure, "Source data item carries no
     *                               polyhedral surface", "Source data item
     *                               names no surface", "Source surface
     *                               exposes no mesh view to copy from", or a
     *                               coherence message; cleared on success.
     * \returns True when a mesh was adopted; false leaves the current mesh.
     */
    [[nodiscard]] bool initialize(
        const HydroCouple::IComponentDataItem &componentDataItem,
        std::string &message) override;

    /*!
     * \brief JSON emits the \c \@uri file reference; String the summary.
     *
     * A mesh with no file behind it cannot serialize — see the file
     * comment.
     *
     * \param[in]  argType String or JSON.
     * \param[out] value   Receives toString() (String), or (JSON) an object
     *                     whose only member, \@uri, is the source reference.
     * \param[out] message On failure, a message that the mesh came from a
     *                     component output and has no file to reference (the
     *                     composition document keeps the \@from binding
     *                     instead), or that mesh arguments serialize as String
     *                     or JSON; cleared on success.
     * \returns True when \p value was written.
     */
    [[nodiscard]] bool serialize(
        HydroCouple::IArgument::ArgumentInputType argType, std::string &value,
        std::string &message) const override;

    /*!
     * \brief Restores the argument from its JSON form
     *        (AbstractArgument::readData()).
     *
     * Reads the base fields, then, when \p j holds a string \@uri member, loads
     * that mesh through initialize(JSON). A load failure is ignored: the
     * argument is left without the mesh and no message is reported.
     *
     * \param[in] j The JSON object written by writeData().
     */
    void readData(const nlohmann::json &j) override;

    /*!
     * \brief Writes the argument's JSON form (AbstractArgument::writeData()).
     * \returns The base fields plus an \@uri member when the mesh has a source
     *          reference; never the mesh itself.
     */
    [[nodiscard]] nlohmann::json writeData() const override;

    // ── IPolyhedralSurfaceComponentDataItem ───────────────────────────────
    //
    // An IArgument *is* an IComponentDataItem, so a mesh argument can say
    // it holds a surface instead of leaving a consumer to infer it from a
    // shape of { nodes, 3 } — which reads as a coordinate table and loses
    // every face.
    //
    // The surface is built from the mesh on demand and dropped whenever
    // the mesh changes. Every assignment goes through adoptMesh() for
    // that reason: a cache invalidated at four call sites is a cache that
    // will be stale at the fifth.

    /*!
     * \brief Where the argument's values sit
     *        (IPolyhedralSurfaceComponentDataItem::location()).
     * \returns MeshLocation::Node: the data plane holds one row per node.
     */
    [[nodiscard]] HydroCouple::Spatial::MeshLocation location()
        const override;

    /*!
     * \brief The structure of each value
     *        (IPolyhedralSurfaceComponentDataItem::meshDataType()).
     * \returns SpatialDataType::Vector: a three-component position per node.
     */
    [[nodiscard]] HydroCouple::Spatial::SpatialDataType meshDataType()
        const override;

    /*!
     * \brief The surface, or nullptr while no mesh has been loaded.
     *
     * Carries the connectivity as well as the patches: the adapter is
     * given the MeshDefinition it was built from, because a surface
     * reporting no faces for a mesh that has them is not a lesser answer
     * but a wrong one (meshadapters.h).
     *
     * \returns A non-owning pointer to the cached PolyhedralSurfaceAdapter,
     *          built on the first call after each mesh change and valid
     *          until the next one; nullptr when hasMesh() is false.
     */
    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *polyhedralSurface()
        const override;

    //! The node axis — the one the argument's { nodes, 3 } shape is indexed by.
    /*!
     * \returns The argument's own "node" dimension (role Entity), never
     *          nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override;

    //! Node positions are Cartesian x, y, z in the surface's reference system.
    /*!
     * \returns VectorBasis::Cartesian.
     */
    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override;

    //! Physical metadata when loaded from a versioned mesh package.
    /*!
     * \returns The package read by initialize() from a ".json" manifest, or
     *          the one copied from another argument; empty after any other
     *          load, including setMesh().
     */
    [[nodiscard]] const std::optional<HydroCouple::SDK::IO::MeshPackage> &meshPackage() const noexcept
    {
        return m_meshPackage;
    }

    // ── Mesh access ───────────────────────────────────────────────────────

    /*!
     * \brief The mesh itself — what a consuming component builds from.
     * \returns The current mesh; empty before one is loaded. The reference
     *          is valid until the next load.
     */
    [[nodiscard]] const HydroCouple::SDK::IO::MeshDefinition &mesh()
        const noexcept
    {
        return m_mesh;
    }

    /*!
     * \brief True once a mesh has been loaded or adopted.
     * \returns True when the mesh has at least one node.
     */
    [[nodiscard]] bool hasMesh() const noexcept
    {
        return !m_mesh.nodeX.empty();
    }

    /*!
     * \brief Adopts \p mesh directly (a test's or generator's shortcut);
     *        refuses a mesh whose coordinate arrays disagree in length.
     *
     * Also refuses a mesh whose face offsets do not end at the face-node
     * count. Face-node indexes are not bounds-checked; use
     * MeshDefinition::validate() for that. On success the source reference
     * and the mesh package are cleared.
     *
     * \param[in]  mesh    The mesh; moved in.
     * \param[out] message On failure, which arrays disagree ("Mesh has N x
     *                     coordinates and M y coordinates", "Mesh has N nodes
     *                     but M z coordinates", "Mesh face offsets end at K
     *                     but there are F face nodes"); cleared on success.
     * \returns True when the mesh was adopted; false leaves the current mesh.
     */
    [[nodiscard]] bool setMesh(HydroCouple::SDK::IO::MeshDefinition mesh,
                               std::string &message);

private:
    /*!
     * \brief Delegation target: the base borrows the value definition, so
     *        it must exist before the base constructs — members do not.
     * \param[in] id             Argument identifier.
     * \param[in] modelComponent Owning component.
     * \param[in] quantity       The value definition, newly allocated by the
     *                           public constructor; the argument takes
     *                           ownership.
     */
    PolyhedralSurfaceArgument(std::string_view id,
                              HydroCouple::IModelComponent *modelComponent,
                              Quantity *quantity);

    /*!
     * \brief Takes \p mesh as the argument's own and drops the surface
     *        built from the last one.
     *
     * The single door. Assigning m_mesh directly anywhere would leave the
     * cached adapter describing a mesh that is no longer there, and the
     * symptom would be a stale surface rather than a crash — which is the
     * kind of bug that survives a long time.
     *
     * Also clears the mesh package; callers that keep one set it again
     * afterward.
     *
     * \param[in] mesh The new mesh; moved in.
     */
    void adoptMesh(HydroCouple::SDK::IO::MeshDefinition mesh);

    HydroCouple::SDK::IO::MeshDefinition m_mesh;                 //!< The current mesh.
    std::optional<HydroCouple::SDK::IO::MeshPackage> m_meshPackage; //!< Set by a mesh package load.
    std::string m_sourceReference; //!< File/URI the mesh was read from.

    //! Built on demand from m_mesh; dropped by adoptMesh().
    mutable std::unique_ptr<PolyhedralSurfaceAdapter> m_surface;

    Dimension m_nodeDimension{"node", "Mesh node"};             //!< Dimension 0, one row per node.
    Dimension m_coordinateDimension{"coordinate", "x, y, z"};   //!< Dimension 1, three coordinates (role Component).
    std::unique_ptr<Quantity> m_quantity;                       //!< The owned "Coordinate" value definition.
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_SPATIAL_POLYHEDRALSURFACEARGUMENT_H
