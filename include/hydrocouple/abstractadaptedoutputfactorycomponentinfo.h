// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   abstractadaptedoutputfactorycomponentinfo.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Concrete base implementation of IAdaptedOutputFactoryComponentInfo.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H
#define HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H

#include "componentinfo.h"
#include "hydrocouple.h"

namespace HydroCouple {

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

    // IAdaptedOutputFactoryComponentInfo — pure virtual
    HydroCouple::IAdaptedOutputFactoryComponent *createComponentInstance() override = 0;
};

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H
