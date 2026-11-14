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
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class LayeredMeshComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::ComponentDataItem2D<T>,
      public virtual HydroCouple::Spatial::ILayeredMeshComponentDataItem
{
    using Store              = HydroCouple::SDK::ComponentDataItem2D<T>;
    using MeshLocation = HydroCouple::Spatial::MeshLocation;

public:
    /*!
     * \brief Construct over a borrowed surface and vertical coordinate.
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

    ~LayeredMeshComponentDataItem() override = default;

    // ── IPolyhedralSurfaceComponentDataItem ───────────────────────────────

    [[nodiscard]] MeshLocation location() const override
    {
        return m_objectType;
    }

    [[nodiscard]] HydroCouple::Spatial::SpatialDataType meshDataType() const override
    {
        return m_dataType;
    }

    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *
    polyhedralSurface() const override
    {
        return m_surface;
    }

    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        return const_cast<Dimension *>(&m_entityDimension);
    }

    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override
    {
        return m_vectorBasis;
    }

    //! Declares the frame of vector/tensor values (Unknown for scalars).
    void setVectorBasis(HydroCouple::Spatial::VectorBasis basis)
    {
        m_vectorBasis = basis;
    }

    // ── ILayering ─────────────────────────────────────────────────────────

    [[nodiscard]] HydroCouple::IDimension *layerDimension() const override
    {
        return const_cast<Dimension *>(&m_layerDimension);
    }

    /*!
     * \brief Where the layers are. Borrowed, and live.
     *
     * Returned rather than copied from so the caller reads the coordinate the
     * model currently has. See the file comment.
     */
    [[nodiscard]] HydroCouple::Spatial::IVerticalCoordinate *
    verticalCoordinate() const override
    {
        return m_vertical;
    }

    // ── v2 typed hyperslab data plane ─────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return Store::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override { return Store::storageKind(); }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return Store::getSlab(destination, start, count, message);
    }

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

    HydroCouple::Spatial::IPolyhedralSurface  *m_surface;
    HydroCouple::Spatial::IVerticalCoordinate *m_vertical;
    MeshLocation                         m_objectType;
    HydroCouple::Spatial::SpatialDataType      m_dataType;
    HydroCouple::Spatial::VectorBasis          m_vectorBasis = HydroCouple::Spatial::VectorBasis::Unknown;
    Dimension m_entityDimension;
    Dimension m_layerDimension;
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_LAYEREDMESHCOMPONENTDATAITEM_H
