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
 */
class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactoryComponentInfo
    : public ComponentInfo,
      public virtual HydroCouple::IAdaptedOutputFactoryComponentInfo
{
public:
    explicit AbstractAdaptedOutputFactoryComponentInfo(std::string_view id = "");
    ~AbstractAdaptedOutputFactoryComponentInfo() override = default;

    // IAdaptedOutputFactoryComponentInfo — pure virtual. The caller owns the
    // instance and must destroy it before unloading the producing library.
    std::unique_ptr<HydroCouple::IAdaptedOutputFactoryComponent>
    createComponentInstance() override = 0;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H
