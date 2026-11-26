// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   abstractmodelcomponentinfo.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete base implementation of HydroCouple::IModelComponentInfo.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTMODELCOMPONENTINFO_H
#define HYDROCOUPLESDK_ABSTRACTMODELCOMPONENTINFO_H

#include "hydrocouplesdk/component/componentinfo.h"
#include "hydrocouple.h"

#include <unordered_map>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief Concrete base class for HydroCouple::IModelComponentInfo.
 *
 * Extends ComponentInfo with an adaptedOutputFactories registry. Plug-in
 * authors subclass this, fill the metadata in the constructor, and
 * override createComponentInstance() (pure virtual in
 * IModelComponentInfo, not implemented here) to return new model component
 * instances:
 *
 * \code
 *     class ReservoirInfo : public AbstractModelComponentInfo
 *     {
 *     public:
 *         ReservoirInfo() : AbstractModelComponentInfo("org.example.reservoir")
 *         {
 *             setCaption("Reservoir");
 *             setVersion("1.0.0");
 *         }
 *         std::unique_ptr<IModelComponent> createComponentInstance() override
 *         {
 *             return std::make_unique<Reservoir>("reservoir", this);
 *         }
 *     };
 *     HYDROCOUPLE_DECLARE_COMPONENT(ReservoirInfo)
 * \endcode
 *
 * The instance is owned by the caller and must be destroyed before the
 * library that created it is unloaded (hydrocouplecomponentabi.h). Passing
 * \c this to the component's constructor makes componentInfo() return this
 * info; the SDK does not set it otherwise.
 *
 * The adapted-output factory map is keyed by factory id for O(1) lookup.
 * Factories are raw non-owning pointers — ownership belongs to the caller
 * or is managed by a plug-in loading framework.
 *
 * \threadsafety Not thread-safe on its own; factory registration should be
 * completed at construction time before any concurrent reads.
 */
class HYDROCOUPLESDK_EXPORT AbstractModelComponentInfo
    : public ComponentInfo,
      public virtual HydroCouple::IModelComponentInfo
{
public:
    /*!
     * \brief Construct with an auto-generated UUID.
     * \details Give production infos a stable id with the other constructor.
     */
    AbstractModelComponentInfo() = default;

    /*!
     * \brief Construct with an explicit identifier.
     * \param[in] id  Unique identifier string of the component type; copied.
     *                Empty is replaced by a generated UUID v4.
     */
    explicit AbstractModelComponentInfo(std::string_view id);

    //! Destroys the info; registered factories are not deleted.
    ~AbstractModelComponentInfo() override = default;

    // ── IModelComponentInfo ───────────────────────────────────────────────

    /*!
     * \brief Returns all registered IAdaptedOutputFactory instances.
     *
     * The returned vector is freshly built on each call from the internal
     * unordered_map, so callers should not cache it across mutations. Its
     * order is the map's iteration order, which is unspecified.
     *
     * \return Vector of non-owning IAdaptedOutputFactory pointers.
     */
    [[nodiscard]] std::vector<HydroCouple::IAdaptedOutputFactory *>
    adaptedOutputFactories() const override final;

protected:
    /*!
     * \brief Returns the internal factory map (keyed by factory id).
     * \return A copy of the map of IAdaptedOutputFactory pointers.
     */
    [[nodiscard]] std::unordered_map<std::string, HydroCouple::IAdaptedOutputFactory *>
    adaptedOutputFactoriesInternal() const;

    /*!
     * \brief Registers a factory. Overwrites any existing entry with the same id.
     * \details Fires propertyChanged("AdaptedOutputFactories").
     * \param[in] factory  Non-owning pointer; must remain valid for the
     *                     lifetime of this info object. nullptr is ignored.
     */
    void addAdaptedOutputFactory(HydroCouple::IAdaptedOutputFactory *factory);

    /*!
     * \brief Removes a factory by pointer identity.
     * \details Looks the factory up by its id and removes the entry only if
     *          it holds this same pointer. Fires
     *          propertyChanged("AdaptedOutputFactories") on removal.
     * \param[in] factory  Pointer previously registered via addAdaptedOutputFactory.
     * \return true if the factory was found and removed; false otherwise,
     *         including for nullptr.
     */
    bool removeAdaptedOutputFactory(HydroCouple::IAdaptedOutputFactory *factory);

    /*!
     * \brief Removes all registered factories.
     * \details Fires propertyChanged("AdaptedOutputFactories"). The
     *          factories are not deleted.
     */
    void clearAdaptedOutputFactories();

private:
    //! Registered factories by id; non-owning.
    std::unordered_map<std::string, HydroCouple::IAdaptedOutputFactory *> m_adaptedOutputFactories;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTMODELCOMPONENTINFO_H
