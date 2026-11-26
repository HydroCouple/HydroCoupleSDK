// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   abstractadaptedoutputfactorycomponentinfo.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete base implementation of IAdaptedOutputFactoryComponentInfo.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H
#define HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H

#include "hydrocouplesdk/component/componentinfo.h"
#include "hydrocouple.h"

namespace HydroCouple::SDK {

/*!
 * \brief Concrete base class for IAdaptedOutputFactoryComponentInfo.
 *
 * Subclasses implement createComponentInstance() to return new
 * IAdaptedOutputFactoryComponent instances.
 *
 * \details The ComponentInfo base carries the descriptive metadata (caption,
 * vendor, version, documentation). A plugin library publishes one of these
 * the way a model library publishes a model component info.
 */
class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactoryComponentInfo
    : public ComponentInfo,
      public virtual HydroCouple::IAdaptedOutputFactoryComponentInfo
{
public:
    /*!
     * \brief Construct with the info identifier.
     * \param id Identifier of the factory component type; empty generates a
     *           UUID (ComponentInfo).
     */
    explicit AbstractAdaptedOutputFactoryComponentInfo(std::string_view id = "");
    //! Destroys the info; instances it created must already be destroyed.
    ~AbstractAdaptedOutputFactoryComponentInfo() override = default;

    // IAdaptedOutputFactoryComponentInfo — pure virtual. The caller owns the
    // instance and must destroy it before unloading the producing library.
    /*!
     * \brief Creates a new factory component instance.
     * \details Pure virtual. The caller owns the instance and must destroy it
     *          before unloading the library that produced it.
     * \return The new instance.
     */
    std::unique_ptr<HydroCouple::IAdaptedOutputFactoryComponent>
    createComponentInstance() override = 0;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H
