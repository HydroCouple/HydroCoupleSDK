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
 * authors subclass this and override createComponentInstance() to return new
 * model component instances.
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
     */
    AbstractModelComponentInfo() = default;

    /*!
     * \brief Construct with an explicit identifier.
     * \param id  Unique identifier string.
     */
    explicit AbstractModelComponentInfo(std::string_view id);

    ~AbstractModelComponentInfo() override = default;

    // ── IModelComponentInfo ───────────────────────────────────────────────

    /*!
     * \brief Returns all registered IAdaptedOutputFactory instances.
     *
     * The returned vector is freshly built on each call from the internal
     * unordered_map, so callers should not cache it across mutations.
     *
     * \return Vector of non-owning IAdaptedOutputFactory pointers.
     */
    [[nodiscard]] std::vector<HydroCouple::IAdaptedOutputFactory *>
    adaptedOutputFactories() const override final;

protected:
    /*!
     * \brief Returns the internal factory map (keyed by factory id).
     * \return Unordered map of IAdaptedOutputFactory pointers.
     */
    [[nodiscard]] std::unordered_map<std::string, HydroCouple::IAdaptedOutputFactory *>
    adaptedOutputFactoriesInternal() const;

    /*!
     * \brief Registers a factory. Overwrites any existing entry with the same id.
     * \param factory  Non-owning pointer; must remain valid for the lifetime of this info object.
     */
    void addAdaptedOutputFactory(HydroCouple::IAdaptedOutputFactory *factory);

    /*!
     * \brief Removes a factory by pointer identity.
     * \param factory  Pointer previously registered via addAdaptedOutputFactory.
     * \return true if the factory was found and removed; false otherwise.
     */
    bool removeAdaptedOutputFactory(HydroCouple::IAdaptedOutputFactory *factory);

    /*!
     * \brief Removes all registered factories.
     */
    void clearAdaptedOutputFactories();

private:
    std::unordered_map<std::string, HydroCouple::IAdaptedOutputFactory *> m_adaptedOutputFactories;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTMODELCOMPONENTINFO_H
