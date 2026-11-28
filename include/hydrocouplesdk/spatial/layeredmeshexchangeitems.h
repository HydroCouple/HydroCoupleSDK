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
 * Only the double instantiations are exported from the library
 * (LayeredMeshOutputDouble, LayeredMeshInputDouble); other element types
 * instantiate from this header in the including library.
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

/*!
 * \brief Implementation details of the layered mesh exchange items; not part
 *        of the public interface.
 */
namespace detail {

//! The parts of a layered mesh item that do not depend on which exchange
//! direction it is, gathered so the input and the output cannot drift apart.
struct LayeredMeshFacets
{
    using MeshLocation = HydroCouple::Spatial::MeshLocation; //!< The standard's entity enumeration.

    /*!
     * \brief Records the geometry associations and builds the two dimensions.
     * \param[in] surface    The plan surface (borrowed; may be nullptr).
     * \param[in] vertical   The vertical coordinate (borrowed; should not be
     *                       null).
     * \param[in] objectType Where the values sit; names the entity dimension.
     * \param[in] dataType   Structure of each value.
     */
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

    /*!
     * \brief The id of the entity dimension for a location.
     * \param[in] objectType The location.
     * \returns "nodes", "edges", "faces" or "volumes"; "entities" for a value
     *          outside the enumeration.
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

    HydroCouple::Spatial::IPolyhedralSurface  *surface;      //!< Borrowed plan surface; may be nullptr.
    HydroCouple::Spatial::IVerticalCoordinate *vertical;     //!< Borrowed, live vertical coordinate.
    MeshLocation                         objectType;         //!< Where the values sit.
    HydroCouple::Spatial::SpatialDataType      dataType;     //!< Structure of each value.
    HydroCouple::Spatial::VectorBasis          vectorBasis =
        HydroCouple::Spatial::VectorBasis::Unknown;          //!< Frame of vector values; Unknown until set.
    Dimension                                  entityDimension; //!< Dimension 0 (role Entity).
    Dimension                                  layerDimension;  //!< Dimension 1, "layers" (role Layer).
};

} // namespace detail

/*!
 * \brief Layered mesh values a component publishes ({ entity, layer }).
 *
 * An AbstractOutput, so a component adds it with addOutput() and consumers
 * (a LayeredMeshInput, or an adapted output) connect to it. Storage, shape,
 * dimensions and accessors are as on LayeredMeshComponentDataItem: values in
 * a ComponentDataItem2D<T> store, entity-major, always rank 2. The component
 * writes values as it steps, with operator()(e, k), rawData() or
 * setValuesFrom(); updateValues() does nothing.
 *
 * \tparam T Element type.
 */
template <typename T>
class LayeredMeshOutput : public AbstractOutput,
                          public ComponentDataItem2D<T>,
                          public virtual HydroCouple::Spatial::ILayeredMeshComponentDataItem
{
    using Store              = ComponentDataItem2D<T>;             //!< The value store.
    using MeshLocation = HydroCouple::Spatial::MeshLocation;      //!< The standard's entity enumeration.

public:
    /*!
     * \brief Construct over a borrowed surface and vertical coordinate.
     * \see LayeredMeshComponentDataItem for the parameter meanings.
     * \param[in] id          Item identifier.
     * \param[in] surface     The plan surface (borrowed; may be nullptr).
     * \param[in] vertical    The vertical coordinate (borrowed; must not be
     *                        null and must outlive the item).
     * \param[in] objectType  Where the values sit.
     * \param[in] dataType    Structure of each value.
     * \param[in] entityCount Entities along dimension 0.
     * \param[in] layerCount  Layers along dimension 1.
     * \param[in] valueDef    Value definition (borrowed); the connection
     *                        validator warns when it is null.
     * \param[in] mc          Owning component (borrowed; may be nullptr).
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

    //! Destroys the output and its values.
    ~LayeredMeshOutput() override = default;

    /*!
     * \brief Which mesh entity the values sit on.
     * \returns The location given at construction.
     */
    [[nodiscard]] MeshLocation location() const override
    {
        return m_facets.objectType;
    }

    /*!
     * \brief The structure of each value.
     * \returns The data type given at construction.
     */
    [[nodiscard]] HydroCouple::Spatial::SpatialDataType meshDataType() const override
    {
        return m_facets.dataType;
    }

    /*!
     * \brief The plan surface.
     * \returns The borrowed surface; nullptr when none was given.
     */
    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *
    polyhedralSurface() const override
    {
        return m_facets.surface;
    }

    /*!
     * \brief The entity axis.
     * \returns Dimension 0, never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        return const_cast<Dimension *>(&m_facets.entityDimension);
    }

    /*!
     * \brief The frame of vector values.
     * \returns The basis set with setVectorBasis(); Unknown by default.
     */
    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override
    {
        return m_facets.vectorBasis;
    }

    //! Declares the frame of vector/tensor values (Unknown for scalars).
    /*!
     * \param[in] basis The basis vectorBasis() reports from now on.
     */
    void setVectorBasis(HydroCouple::Spatial::VectorBasis basis)
    {
        m_facets.vectorBasis = basis;
    }

    /*!
     * \brief The layer axis.
     * \returns Dimension 1 ("layers"), never nullptr; layer 0 is the top.
     */
    [[nodiscard]] HydroCouple::IDimension *layerDimension() const override
    {
        return const_cast<Dimension *>(&m_facets.layerDimension);
    }

    /*!
     * \brief Where the layers are; borrowed and live.
     * \returns The borrowed coordinate given at construction, not checked for
     *          null.
     */
    [[nodiscard]] HydroCouple::Spatial::IVerticalCoordinate *
    verticalCoordinate() const override
    {
        return m_facets.vertical;
    }

    /*!
     * \brief The output's shape.
     * \returns { entityCount, layerCount }.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return Store::storageShape();
    }

    /*!
     * \brief The element kind.
     * \returns The DataKind of T.
     */
    [[nodiscard]] DataKind dataKind() const override { return Store::storageKind(); }

    /*!
     * \brief Copies a block of values out.
     * \param[in]  destination Buffer of dataKind() receiving the block.
     * \param[in]  start       { first entity, first layer }.
     * \param[in]  count       { entities, layers } to copy.
     * \param[out] message     Receives the reason on failure; may be nullptr.
     * \returns False when the selection or the buffer does not fit.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return Store::getSlab(destination, start, count, message);
    }

    /*!
     * \brief Copies a block of values in and signals the change.
     * \param[in]  source  Buffer of dataKind() holding the block.
     * \param[in]  start   { first entity, first layer }.
     * \param[in]  count   { entities, layers } to write.
     * \param[out] message Receives the reason on failure; may be nullptr.
     * \returns False, with nothing written or signaled, when the selection
     *          or the buffer does not fit.
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

    /*!
     * \brief No-op, as on Output1D.
     *
     * A layered model writes its own values as it steps, through the typed
     * accessors, rather than regenerating them when a consumer pulls. A
     * component that needs pull-side refresh overrides this.
     */
    void updateValues() override {}

private:
    detail::LayeredMeshFacets m_facets; //!< Geometry associations and dimensions.
};

/*!
 * \brief Layered mesh values a component consumes ({ entity, layer }).
 *
 * An AbstractInput with the same storage and accessors as LayeredMeshOutput.
 * canConsume() accepts a provider of the same element kind, rank 2, and the
 * same layer count; retrieveValuesFromProvider(), which
 * AbstractModelComponent::applyInputValues() calls before applyData(), copies
 * the provider's values into this input's store.
 *
 * \tparam T Element type.
 */
template <typename T>
class LayeredMeshInput : public AbstractInput,
                         public ComponentDataItem2D<T>,
                         public virtual HydroCouple::Spatial::ILayeredMeshComponentDataItem
{
    using Store              = ComponentDataItem2D<T>;             //!< The value store.
    using MeshLocation = HydroCouple::Spatial::MeshLocation;      //!< The standard's entity enumeration.

public:
    /*!
     * \brief Construct over a borrowed surface and vertical coordinate.
     * \param[in] id          Item identifier.
     * \param[in] surface     The plan surface (borrowed; may be nullptr).
     * \param[in] vertical    The vertical coordinate (borrowed; must not be
     *                        null and must outlive the item).
     * \param[in] objectType  Where the values are expected to sit.
     * \param[in] dataType    Structure of each value.
     * \param[in] entityCount Entities along dimension 0.
     * \param[in] layerCount  Layers along dimension 1; a provider must match
     *                        it.
     * \param[in] valueDef    Value definition (borrowed).
     * \param[in] mc          Owning component (borrowed; may be nullptr).
     */
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

    //! Destroys the input and its values.
    ~LayeredMeshInput() override = default;

    /*!
     * \brief Which mesh entity the values sit on.
     * \returns The location given at construction.
     */
    [[nodiscard]] MeshLocation location() const override
    {
        return m_facets.objectType;
    }

    /*!
     * \brief The structure of each value.
     * \returns The data type given at construction.
     */
    [[nodiscard]] HydroCouple::Spatial::SpatialDataType meshDataType() const override
    {
        return m_facets.dataType;
    }

    /*!
     * \brief The plan surface.
     * \returns The borrowed surface; nullptr when none was given.
     */
    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *
    polyhedralSurface() const override
    {
        return m_facets.surface;
    }

    /*!
     * \brief The entity axis.
     * \returns Dimension 0, never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        return const_cast<Dimension *>(&m_facets.entityDimension);
    }

    /*!
     * \brief The frame of vector values.
     * \returns The basis set with setVectorBasis(); Unknown by default.
     */
    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override
    {
        return m_facets.vectorBasis;
    }

    //! Declares the frame of vector/tensor values (Unknown for scalars).
    /*!
     * \param[in] basis The basis vectorBasis() reports from now on.
     */
    void setVectorBasis(HydroCouple::Spatial::VectorBasis basis)
    {
        m_facets.vectorBasis = basis;
    }

    /*!
     * \brief The layer axis.
     * \returns Dimension 1 ("layers"), never nullptr; layer 0 is the top.
     */
    [[nodiscard]] HydroCouple::IDimension *layerDimension() const override
    {
        return const_cast<Dimension *>(&m_facets.layerDimension);
    }

    /*!
     * \brief Where this input's layers are; borrowed and live.
     * \returns The borrowed coordinate given at construction, not checked for
     *          null.
     */
    [[nodiscard]] HydroCouple::Spatial::IVerticalCoordinate *
    verticalCoordinate() const override
    {
        return m_facets.vertical;
    }

    /*!
     * \brief The input's shape.
     * \returns { entityCount, layerCount }.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return Store::storageShape();
    }

    /*!
     * \brief The element kind.
     * \returns The DataKind of T.
     */
    [[nodiscard]] DataKind dataKind() const override { return Store::storageKind(); }

    /*!
     * \brief Copies a block of values out.
     * \param[in]  destination Buffer of dataKind() receiving the block.
     * \param[in]  start       { first entity, first layer }.
     * \param[in]  count       { entities, layers } to copy.
     * \param[out] message     Receives the reason on failure; may be nullptr.
     * \returns False when the selection or the buffer does not fit.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return Store::getSlab(destination, start, count, message);
    }

    /*!
     * \brief Copies a block of values in and signals the change.
     * \param[in]  source  Buffer of dataKind() holding the block.
     * \param[in]  start   { first entity, first layer }.
     * \param[in]  count   { entities, layers } to write.
     * \param[out] message Receives the reason on failure; may be nullptr.
     * \returns False, with nothing written or signaled, when the selection
     *          or the buffer does not fit.
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
     *
     * The entity count is not compared either: a provider with more or fewer
     * entities is accepted, and retrieveValuesFromProvider() copies the
     * entities the two share. The vertical coordinates' kinds and the
     * reference systems are compared by validateConnection(), not here.
     * AbstractInput::setProvider() calls this and refuses the provider when
     * it returns false.
     *
     * \param[in]  provider The candidate output; may be nullptr.
     * \param[out] message  On refusal, receives "Provider is null",
     *                      "Provider data kind mismatch", "Provider is not
     *                      two-dimensional {entity, layer}" or "Provider has
     *                      P layers, this input has L"; left unchanged on
     *                      acceptance.
     * \returns True when the provider can feed this input.
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
     *
     * Reads min(provider entities, this input's entities) entities, all
     * layers, from the provider straight into this input's store and emits
     * the value-changed signal for that block. Entities beyond the provider's
     * count keep their previous values. Does nothing, silently, when there is
     * no provider, when the provider no longer matches (kind, rank or layer
     * count), when there are no entities to copy, or when the provider's
     * getValuesInto() fails.
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
    /*!
     * Called by AbstractModelComponent::applyInputValues() right after
     * retrieveValuesFromProvider(); does nothing here. Override it, or read
     * the store directly in the component's update, to use the values.
     */
    void applyData() override {}

private:
    detail::LayeredMeshFacets m_facets; //!< Geometry associations and dimensions.
};

//! The exported double-valued layered mesh output.
using LayeredMeshOutputDouble = LayeredMeshOutput<double>;
//! The exported double-valued layered mesh input.
using LayeredMeshInputDouble  = LayeredMeshInput<double>;

// Export the concrete aliases' RTTI/vtables once. Header-only instantiation
// under a plugin's hidden visibility makes cross-library dynamic_cast fail.
extern template class HYDROCOUPLESDK_EXPORT LayeredMeshOutput<double>;
extern template class HYDROCOUPLESDK_EXPORT LayeredMeshInput<double>;

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_LAYEREDMESHEXCHANGEITEMS_H
