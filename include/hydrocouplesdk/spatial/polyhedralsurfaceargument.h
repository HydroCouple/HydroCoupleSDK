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
 * Serialization honours the interface's bulk mandate: a mesh is never
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
 * \threadsafety Not thread-safe; mutate before component initialization.
 */
class HYDROCOUPLESDK_EXPORT PolyhedralSurfaceArgument
    : public HydroCouple::SDK::AbstractArgument,
      public virtual HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem
{
public:
    /*!
     * \brief Constructs an empty mesh argument.
     * \param id             Argument identifier.
     * \param modelComponent Owning component.
     */
    PolyhedralSurfaceArgument(std::string_view id,
                              HydroCouple::IModelComponent *modelComponent);

    ~PolyhedralSurfaceArgument() override = default;

    // ── Data plane ────────────────────────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override;

    [[nodiscard]] DataKind dataKind() const override;

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override;

    /*!
     * \brief Refused: a mesh is loaded through initialize(), never poked
     *        one coordinate at a time.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── IArgument ─────────────────────────────────────────────────────────

    [[nodiscard]] std::string toString() const override;

    /*!
     * \brief File: reads a UGRID NetCDF mesh. JSON: accepts an \c \@uri
     *        reference to one. Inline payloads are refused — bulk is never
     *        inlined.
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
     */
    [[nodiscard]] bool initialize(
        const HydroCouple::IComponentDataItem &componentDataItem,
        std::string &message) override;

    /*!
     * \brief JSON emits the \c \@uri file reference; String the summary.
     *
     * A mesh with no file behind it cannot serialize — see the file
     * comment.
     */
    [[nodiscard]] bool serialize(
        HydroCouple::IArgument::ArgumentInputType argType, std::string &value,
        std::string &message) const override;

    void readData(const nlohmann::json &j) override;

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

    [[nodiscard]] HydroCouple::Spatial::MeshLocation location()
        const override;

    [[nodiscard]] HydroCouple::Spatial::SpatialDataType meshDataType()
        const override;

    /*!
     * \brief The surface, or nullptr while no mesh has been loaded.
     *
     * Carries the connectivity as well as the patches: the adapter is
     * given the MeshDefinition it was built from, because a surface
     * reporting no faces for a mesh that has them is not a lesser answer
     * but a wrong one (meshadapters.h).
     */
    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *polyhedralSurface()
        const override;

    //! The node axis — the one the argument's { nodes, 3 } shape is indexed by.
    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override;

    //! Node positions are Cartesian x, y, z in the surface's reference system.
    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override;

    // ── Mesh access ───────────────────────────────────────────────────────

    /*!
     * \brief The mesh itself — what a consuming component builds from.
     */
    [[nodiscard]] const HydroCouple::SDK::IO::MeshDefinition &mesh()
        const noexcept
    {
        return m_mesh;
    }

    /*!
     * \brief True once a mesh has been loaded or adopted.
     */
    [[nodiscard]] bool hasMesh() const noexcept
    {
        return !m_mesh.nodeX.empty();
    }

    /*!
     * \brief Adopts \p mesh directly (a test's or generator's shortcut);
     *        refuses a mesh whose coordinate arrays disagree in length.
     */
    [[nodiscard]] bool setMesh(HydroCouple::SDK::IO::MeshDefinition mesh,
                               std::string &message);

private:
    /*!
     * \brief Delegation target: the base borrows the value definition, so
     *        it must exist before the base constructs — members do not.
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
     */
    void adoptMesh(HydroCouple::SDK::IO::MeshDefinition mesh);

    HydroCouple::SDK::IO::MeshDefinition m_mesh;
    std::string m_sourceReference; //!< File/URI the mesh was read from.

    //! Built on demand from m_mesh; dropped by adoptMesh().
    mutable std::unique_ptr<PolyhedralSurfaceAdapter> m_surface;

    Dimension m_nodeDimension{"node", "Mesh node"};
    Dimension m_coordinateDimension{"coordinate", "x, y, z"};
    std::unique_ptr<Quantity> m_quantity;
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_SPATIAL_POLYHEDRALSURFACEARGUMENT_H
