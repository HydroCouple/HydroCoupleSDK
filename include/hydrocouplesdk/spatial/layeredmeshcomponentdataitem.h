// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   layeredmeshcomponentdataitem.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  LayeredMeshComponentDataItem — mesh-entity values resolved into
 *         vertical layers.
 *
 * Implements the standard's `ILayeredMeshComponentDataItem`: the mesh entity
 * selected by `location()` is dimension 0 and the layer is dimension
 * 1, so `shape() = { entityCount, layerCount }`. That ordering is not a free
 * choice — `IPolyhedralSurfaceComponentDataItem` fixes the entity as dimension
 * 0 with additional dimensions following, and this item has to remain readable
 * through that base by a partner that knows nothing about layers.
 *
 * **What makes this different from a two-dimensional array with a nice name.**
 * The item borrows an `IVerticalCoordinate` and hands it out. Values indexed
 * `{entity, layer}` are uninterpretable without one: under a terrain-following
 * coordinate, layer 7's elevation moves every step as the free surface rises.
 * The coordinate is borrowed rather than copied precisely so that it keeps
 * moving — a snapshot taken at construction would answer confidently and
 * wrongly for the rest of the run.
 *
 * The surface and the vertical coordinate are both borrowed and must outlive
 * the item; they normally belong to the owning component.
 *
 * This class derives from AbstractComponentDataItem, so it cannot be added to
 * a component as an input or output; LayeredMeshInput and LayeredMeshOutput
 * (layeredmeshexchangeitems.h) are the exchangeable forms.
 *
 * \threadsafety Not thread-safe on its own.
 */

#ifndef HYDROCOUPLESDK_LAYEREDMESHCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_LAYEREDMESHCOMPONENTDATAITEM_H

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouplesdk/data/componentdataitem.h"
#include "hydrocouplespatial.h"

#include <string>
#include <vector>

namespace HydroCouple::SDK::Spatial {

/*!
 * \brief Typed layered-mesh data item ({ mesh-entity, layer } storage).
 *
 * Values live in a ComponentDataItem2D<T> store of entityCount × layerCount
 * elements, entity-major: value (e, k) is at e·layerCount + k, so one
 * entity's whole column is contiguous. The element access operator(e, k),
 * rawData() and the default-value helpers come from that store. The item
 * owns its two dimensions: the entity dimension, whose id names the location
 * ("nodes", "edges", "faces" or "volumes") and whose role is Entity, and the
 * "layers" dimension with role Layer.
 *
 * The item is always rank 2: it does not add the trailing Component
 * dimension the standard calls for when meshDataType() is Vector, Tensor or
 * MultiScalar, so such values must be packed by the caller. Neither
 * entityCount nor layerCount is checked against the surface or the vertical
 * coordinate; keep layerCount equal to the coordinate's layerCount().
 *
 * The template is header-only and not exported from the library.
 *
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class LayeredMeshComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::ComponentDataItem2D<T>,
      public virtual HydroCouple::Spatial::ILayeredMeshComponentDataItem
{
    using Store              = HydroCouple::SDK::ComponentDataItem2D<T>; //!< The value store.
    using MeshLocation = HydroCouple::Spatial::MeshLocation;            //!< The standard's entity enumeration.

public:
    /*!
     * \brief Construct over a borrowed surface and vertical coordinate.
     *
     * Every value starts as T{}.
     *
     * \param[in] id          Item identifier.
     * \param[in] surface     The polyhedral surface (borrowed; may be nullptr
     *                        when topology lives elsewhere).
     * \param[in] vertical    Where the layers are (borrowed; must not be null —
     *                        see \ref verticalCoordinate()).
     * \param[in] objectType  Mesh entity the values attach to.
     * \param[in] dataType    Nature of each value (Scalar, Vector, …).
     * \param[in] entityCount Entities along dimension 0.
     * \param[in] layerCount  Layers along dimension 1.
     * \param[in] valueDef    Value definition (borrowed).
     * \param[in] mc          Owning component (borrowed).
     */
    LayeredMeshComponentDataItem(
        std::string_view id, HydroCouple::Spatial::IPolyhedralSurface *surface,
        HydroCouple::Spatial::IVerticalCoordinate *vertical,
        MeshLocation objectType,
        HydroCouple::Spatial::SpatialDataType dataType, int entityCount,
        int layerCount, ValueDefinition *valueDef = nullptr,
        HydroCouple::IModelComponent *mc = nullptr)
        : AbstractComponentDataItem(id, {&m_entityDimension, &m_layerDimension},
                                    valueDef, mc),
          Store(entityCount, layerCount),
          m_surface(surface),
          m_vertical(vertical),
          m_objectType(objectType),
          m_dataType(dataType),
          m_entityDimension(entityDimensionId(objectType), "Mesh entity dimension"),
          m_layerDimension("layers", "Vertical layer dimension")
    {
        m_entityDimension.setRole(HydroCouple::IDimension::DimensionRole::Entity);
        m_layerDimension.setRole(HydroCouple::IDimension::DimensionRole::Layer);
    }

    //! Destroys the item and its values; the surface and coordinate are not owned.
    ~LayeredMeshComponentDataItem() override = default;

    // ── IPolyhedralSurfaceComponentDataItem ───────────────────────────────

    /*!
     * \brief Which mesh entity the values sit on
     *        (IPolyhedralSurfaceComponentDataItem::location()).
     * \returns The location given at construction: Volume for values in the
     *          layer cells, Face for values on the layer interfaces of each
     *          face, by the standard's convention.
     */
    [[nodiscard]] MeshLocation location() const override
    {
        return m_objectType;
    }

    /*!
     * \brief The structure of each value
     *        (IPolyhedralSurfaceComponentDataItem::meshDataType()).
     * \returns The data type given at construction.
     */
    [[nodiscard]] HydroCouple::Spatial::SpatialDataType meshDataType() const override
    {
        return m_dataType;
    }

    /*!
     * \brief The plan surface
     *        (IPolyhedralSurfaceComponentDataItem::polyhedralSurface()).
     * \returns The borrowed surface; nullptr when none was given. The
     *          connection validator reads the item's reference system from
     *          it.
     */
    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *
    polyhedralSurface() const override
    {
        return m_surface;
    }

    /*!
     * \brief The entity axis
     *        (IPolyhedralSurfaceComponentDataItem::entityDimension()).
     * \returns The item's own dimension 0, never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        return const_cast<Dimension *>(&m_entityDimension);
    }

    /*!
     * \brief The frame of vector values
     *        (IPolyhedralSurfaceComponentDataItem::vectorBasis()).
     * \returns The basis set with setVectorBasis(); VectorBasis::Unknown by
     *          default, which the standard allows only for scalar data.
     */
    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override
    {
        return m_vectorBasis;
    }

    //! Declares the frame of vector/tensor values (Unknown for scalars).
    /*!
     * \param[in] basis The basis vectorBasis() reports from now on.
     */
    void setVectorBasis(HydroCouple::Spatial::VectorBasis basis)
    {
        m_vectorBasis = basis;
    }

    // ── ILayering ─────────────────────────────────────────────────────────

    /*!
     * \brief The layer axis (ILayering::layerDimension()).
     * \returns The item's own dimension 1 ("layers", role Layer), never
     *          nullptr. Layer 0 is the top layer.
     */
    [[nodiscard]] HydroCouple::IDimension *layerDimension() const override
    {
        return const_cast<Dimension *>(&m_layerDimension);
    }

    /*!
     * \brief Where the layers are. Borrowed, and live.
     *
     * Returned rather than copied from so the caller reads the coordinate the
     * model currently has. See the file comment.
     *
     * \returns The borrowed coordinate given at construction. It is not
     *          checked for null; a null coordinate makes the connection
     *          validator report a LayeringMismatch error.
     */
    [[nodiscard]] HydroCouple::Spatial::IVerticalCoordinate *
    verticalCoordinate() const override
    {
        return m_vertical;
    }

    // ── v2 typed hyperslab data plane ─────────────────────────────────────

    /*!
     * \brief The item's shape (IComponentDataItem::shape()).
     * \returns { entityCount, layerCount }.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return Store::storageShape();
    }

    /*!
     * \brief The element kind (IComponentDataItem::dataKind()).
     * \returns The DataKind of T (Float64 for double).
     */
    [[nodiscard]] DataKind dataKind() const override { return Store::storageKind(); }

    /*!
     * \brief Copies a block of values out (IComponentDataItem::getValuesInto()).
     * \param[in]  destination Buffer of dataKind() receiving the block.
     * \param[in]  start       { first entity, first layer }.
     * \param[in]  count       { entities, layers } to copy.
     * \param[out] message     Receives the reason on failure; may be nullptr.
     * \returns False when the selection or the buffer does not fit the item.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return Store::getSlab(destination, start, count, message);
    }

    /*!
     * \brief Copies a block of values in (IComponentDataItem::setValuesFrom()).
     *
     * On success emits the value-changed signal for the block written.
     *
     * \param[in]  source  Buffer of dataKind() holding the block.
     * \param[in]  start   { first entity, first layer }.
     * \param[in]  count   { entities, layers } to write.
     * \param[out] message Receives the reason on failure; may be nullptr.
     * \returns False, with nothing written or signaled, when the selection or
     *          the buffer does not fit the item.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!Store::setSlab(source, start, count, message))
            return false;
        emitValueChanged({start.begin(), start.end()}, {count.begin(), count.end()});
        return true;
    }

private:
    /*!
     * \brief The id of the entity dimension for a location.
     * \param[in] objectType The location.
     * \returns "nodes", "edges", "faces" or "volumes"; "entities" for a value
     *          outside the enumeration. Each maps to DimensionRole::Entity.
     */
    [[nodiscard]] static const char *entityDimensionId(MeshLocation objectType)
    {
        switch (objectType)
        {
            case MeshLocation::Node:   return "nodes";
            case MeshLocation::Edge:   return "edges";
            case MeshLocation::Face:   return "faces";
            case MeshLocation::Volume: return "volumes";
        }
        return "entities";
    }

    HydroCouple::Spatial::IPolyhedralSurface  *m_surface;    //!< Borrowed plan surface; may be nullptr.
    HydroCouple::Spatial::IVerticalCoordinate *m_vertical;   //!< Borrowed, live vertical coordinate.
    MeshLocation                         m_objectType;       //!< Where the values sit.
    HydroCouple::Spatial::SpatialDataType      m_dataType;   //!< Structure of each value.
    HydroCouple::Spatial::VectorBasis          m_vectorBasis = HydroCouple::Spatial::VectorBasis::Unknown; //!< Frame of vector values.
    Dimension m_entityDimension; //!< Dimension 0, the entities.
    Dimension m_layerDimension;  //!< Dimension 1, the layers.
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_LAYEREDMESHCOMPONENTDATAITEM_H
