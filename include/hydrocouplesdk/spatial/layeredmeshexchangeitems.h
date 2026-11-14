// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   layeredmeshexchangeitems.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  LayeredMeshInput and LayeredMeshOutput — layered mesh values that can
 *         actually be exchanged.
 *
 * **Why these exist separately from LayeredMeshComponentDataItem.** That class
 * derives from `AbstractComponentDataItem`, which is the right base for an item
 * a component publishes internally, and the wrong one for an exchange item:
 * `addInput()` and `addOutput()` take `AbstractInput *` and `AbstractOutput *`,
 * so a layered item built on the plain base cannot be connected to anything. A
 * layered data item that cannot cross a coupling boundary is most of the way to
 * useless, since crossing one is what it is for.
 *
 * These mirror `exchangeitems1d.h` exactly — `AbstractOutput` (or
 * `AbstractInput`) composed with the typed storage mixin, here the 2-D one —
 * rather than wrapping the plain item, which is the same relationship
 * `Output1D` has to `ComponentDataItem1D`.
 *
 * Dimension ordering, the borrowed-and-live vertical coordinate, and the
 * conventions are as documented on `LayeredMeshComponentDataItem`.
 *
 * \threadsafety Not thread-safe on their own.
 */

#ifndef HYDROCOUPLESDK_LAYEREDMESHEXCHANGEITEMS_H
#define HYDROCOUPLESDK_LAYEREDMESHEXCHANGEITEMS_H

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/data/abstractinput.h"
#include "hydrocouplesdk/data/abstractoutput.h"
#include "hydrocouplesdk/data/componentdataitem.h"
#include "hydrocouplespatial.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace HydroCouple::SDK::Spatial {

namespace detail {

//! The parts of a layered mesh item that do not depend on which exchange
//! direction it is, gathered so the input and the output cannot drift apart.
struct LayeredMeshFacets
{
    using MeshLocation = HydroCouple::Spatial::MeshLocation;

    LayeredMeshFacets(HydroCouple::Spatial::IPolyhedralSurface *surface,
                      HydroCouple::Spatial::IVerticalCoordinate *vertical,
                      MeshLocation objectType,
                      HydroCouple::Spatial::SpatialDataType dataType)
        : surface(surface), vertical(vertical), objectType(objectType),
          dataType(dataType),
          entityDimension(entityDimensionId(objectType), "Mesh entity dimension"),
          layerDimension("layers", "Vertical layer dimension")
    {
        entityDimension.setRole(HydroCouple::IDimension::DimensionRole::Entity);
        layerDimension.setRole(HydroCouple::IDimension::DimensionRole::Layer);
    }

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

    HydroCouple::Spatial::IPolyhedralSurface  *surface;
    HydroCouple::Spatial::IVerticalCoordinate *vertical;
    MeshLocation                         objectType;
    HydroCouple::Spatial::SpatialDataType      dataType;
    HydroCouple::Spatial::VectorBasis          vectorBasis =
        HydroCouple::Spatial::VectorBasis::Unknown;
    Dimension                                  entityDimension;
    Dimension                                  layerDimension;
};

} // namespace detail

/*!
 * \brief Layered mesh values a component publishes ({ entity, layer }).
 * \tparam T Element type.
 */
template <typename T>
class LayeredMeshOutput : public AbstractOutput,
                          public ComponentDataItem2D<T>,
                          public virtual HydroCouple::Spatial::ILayeredMeshComponentDataItem
{
    using Store              = ComponentDataItem2D<T>;
    using MeshLocation = HydroCouple::Spatial::MeshLocation;

public:
    /*!
     * \brief Construct over a borrowed surface and vertical coordinate.
     * \see LayeredMeshComponentDataItem for the parameter meanings.
     */
    LayeredMeshOutput(std::string_view id,
                      HydroCouple::Spatial::IPolyhedralSurface *surface,
                      HydroCouple::Spatial::IVerticalCoordinate *vertical,
                      MeshLocation objectType,
                      HydroCouple::Spatial::SpatialDataType dataType,
                      int entityCount, int layerCount,
                      ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
        : AbstractOutput(id, {&m_facets.entityDimension, &m_facets.layerDimension},
                         valueDef, mc),
          Store(entityCount, layerCount),
          m_facets(surface, vertical, objectType, dataType)
    {
    }

    ~LayeredMeshOutput() override = default;

    [[nodiscard]] MeshLocation location() const override
    {
        return m_facets.objectType;
    }

    [[nodiscard]] HydroCouple::Spatial::SpatialDataType meshDataType() const override
    {
        return m_facets.dataType;
    }

    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *
    polyhedralSurface() const override
    {
        return m_facets.surface;
    }

    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        return const_cast<Dimension *>(&m_facets.entityDimension);
    }

    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override
    {
        return m_facets.vectorBasis;
    }

    //! Declares the frame of vector/tensor values (Unknown for scalars).
    void setVectorBasis(HydroCouple::Spatial::VectorBasis basis)
    {
        m_facets.vectorBasis = basis;
    }

    [[nodiscard]] HydroCouple::IDimension *layerDimension() const override
    {
        return const_cast<Dimension *>(&m_facets.layerDimension);
    }

    [[nodiscard]] HydroCouple::Spatial::IVerticalCoordinate *
    verticalCoordinate() const override
    {
        return m_facets.vertical;
    }

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

    /*!
     * \brief No-op, as on Output1D.
     *
     * A layered model writes its own values as it steps, through the typed
     * accessors, rather than regenerating them when a consumer pulls. A
     * component that needs pull-side refresh overrides this.
     */
    void updateValues() override {}

private:
    detail::LayeredMeshFacets m_facets;
};

/*!
 * \brief Layered mesh values a component consumes ({ entity, layer }).
 * \tparam T Element type.
 */
template <typename T>
class LayeredMeshInput : public AbstractInput,
                         public ComponentDataItem2D<T>,
                         public virtual HydroCouple::Spatial::ILayeredMeshComponentDataItem
{
    using Store              = ComponentDataItem2D<T>;
    using MeshLocation = HydroCouple::Spatial::MeshLocation;

public:
    LayeredMeshInput(std::string_view id,
                     HydroCouple::Spatial::IPolyhedralSurface *surface,
                     HydroCouple::Spatial::IVerticalCoordinate *vertical,
                     MeshLocation objectType,
                     HydroCouple::Spatial::SpatialDataType dataType, int entityCount,
                     int layerCount, ValueDefinition *valueDef,
                     HydroCouple::IModelComponent *mc)
        : AbstractInput(id, {&m_facets.entityDimension, &m_facets.layerDimension},
                        valueDef, mc),
          Store(entityCount, layerCount),
          m_facets(surface, vertical, objectType, dataType)
    {
    }

    ~LayeredMeshInput() override = default;

    [[nodiscard]] MeshLocation location() const override
    {
        return m_facets.objectType;
    }

    [[nodiscard]] HydroCouple::Spatial::SpatialDataType meshDataType() const override
    {
        return m_facets.dataType;
    }

    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *
    polyhedralSurface() const override
    {
        return m_facets.surface;
    }

    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        return const_cast<Dimension *>(&m_facets.entityDimension);
    }

    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override
    {
        return m_facets.vectorBasis;
    }

    //! Declares the frame of vector/tensor values (Unknown for scalars).
    void setVectorBasis(HydroCouple::Spatial::VectorBasis basis)
    {
        m_facets.vectorBasis = basis;
    }

    [[nodiscard]] HydroCouple::IDimension *layerDimension() const override
    {
        return const_cast<Dimension *>(&m_facets.layerDimension);
    }

    [[nodiscard]] HydroCouple::Spatial::IVerticalCoordinate *
    verticalCoordinate() const override
    {
        return m_facets.vertical;
    }

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

    /*!
     * \brief Accepts a two-dimensional provider of matching kind and layering.
     *
     * The layer count is checked as well as the rank, because two layered
     * models that disagree about how many layers they have will still match on
     * rank and produce a silently truncated exchange. Refusing here costs one
     * connection error; accepting costs a wrong answer with no error at all.
     *
     * Layer *elevations* are deliberately not compared: a sigma model and a
     * z-level model legitimately place their layers differently, and
     * reconciling that is remapping, which belongs in an adapted output rather
     * than in a compatibility check.
     */
    [[nodiscard]] bool canConsume(HydroCouple::IOutput *provider,
                                  std::string &message) const override
    {
        if (provider == nullptr)
        {
            message = "Provider is null";
            return false;
        }

        if (provider->dataKind() != Store::storageKind())
        {
            message = "Provider data kind mismatch";
            return false;
        }

        const std::vector<int64_t> providerShape = provider->shape();

        if (providerShape.size() != 2)
        {
            message = "Provider is not two-dimensional {entity, layer}";
            return false;
        }

        if (providerShape[1] != static_cast<int64_t>(this->jLength()))
        {
            message = "Provider has " + std::to_string(providerShape[1]) +
                      " layers, this input has " + std::to_string(this->jLength());
            return false;
        }

        return true;
    }

    /*!
     * \brief Pulls the leading entities the provider and this input share.
     */
    void retrieveValuesFromProvider() override
    {
        HydroCouple::IOutput *source = provider();

        if (source == nullptr)
            return;

        const std::vector<int64_t> sourceShape = source->shape();

        if (sourceShape.size() != 2 || source->dataKind() != Store::storageKind() ||
            sourceShape[1] != static_cast<int64_t>(this->jLength()))
            return;

        const int64_t entities =
            std::min<int64_t>(sourceShape[0], static_cast<int64_t>(this->iLength()));

        if (entities <= 0)
            return;

        const std::array<int64_t, 2> shape{entities,
                                           static_cast<int64_t>(this->jLength())};
        const std::array<int64_t, 2> start{0, 0};

        BufferDescriptor destination;
        destination.data  = this->rawData().data();
        destination.kind  = Store::storageKind();
        destination.rank  = 2;
        destination.shape = shape.data();
        destination.space = MemorySpace::Host;

        std::string message;

        if (source->getValuesInto(destination, start, shape, &message))
            emitValueChanged({0, 0}, {shape[0], shape[1]});
    }

    //! Hook for a component to push retrieved values into its own state.
    void applyData() override {}

private:
    detail::LayeredMeshFacets m_facets;
};

using LayeredMeshOutputDouble = LayeredMeshOutput<double>;
using LayeredMeshInputDouble  = LayeredMeshInput<double>;

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_LAYEREDMESHEXCHANGEITEMS_H
