// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   abstractadaptedoutputfactory.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete base classes for IAdaptedOutputFactory and
 *         IAdaptedOutputFactoryComponent.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORY_H
#define HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORY_H

#include "hydrocouplesdk/core/identity.h"
#include "hydrocouple.h"

namespace HydroCouple::SDK {

class AbstractAdaptedOutputFactoryComponentInfo;

// ── AbstractAdaptedOutputFactory ─────────────────────────────────────────

/*!
 * \brief Concrete base class for HydroCouple::IAdaptedOutputFactory.
 *
 * Subclasses implement getAvailableAdaptedOutputIds() and
 * createAdaptedOutput() to produce transformation chains between
 * an IOutput and an IInput.
 */
class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactory
    : public Identity,
      public virtual HydroCouple::IAdaptedOutputFactory
{
public:
    explicit AbstractAdaptedOutputFactory(std::string_view id);
    ~AbstractAdaptedOutputFactory() override = default;

    //! Not copyable: owns non-copyable state (unique_ptr / mutex /
    //! atomic). Declared rather than left implicit because the class is
    //! exported, and __declspec(dllexport) instantiates every member --
    //! including the implicit copy assignment, which cannot compile
    //! against those members. Other toolchains only instantiate it on use.
    AbstractAdaptedOutputFactory(const AbstractAdaptedOutputFactory &)            = delete;
    AbstractAdaptedOutputFactory &operator=(const AbstractAdaptedOutputFactory &) = delete;

    // IAdaptedOutputFactory — pure virtual, subclasses implement
    std::vector<HydroCouple::IIdentity *>
    getAvailableAdaptedOutputIds(const HydroCouple::IOutput *provider,
                                  const HydroCouple::IInput  *consumer = nullptr) override = 0;

    std::unique_ptr<HydroCouple::IAdaptedOutput>
    createAdaptedOutput(HydroCouple::IIdentity *adaptedProviderId,
                         HydroCouple::IOutput   *provider,
                         HydroCouple::IInput    *consumer = nullptr) override = 0;
};


// ── AbstractAdaptedOutputFactoryComponent ────────────────────────────────

/*!
 * \brief Concrete base class for HydroCouple::IAdaptedOutputFactoryComponent.
 *
 * Extends AbstractAdaptedOutputFactory with a back-pointer to its
 * IAdaptedOutputFactoryComponentInfo.
 */
class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactoryComponent
    : public AbstractAdaptedOutputFactory,
      public virtual HydroCouple::IAdaptedOutputFactoryComponent
{
public:
    AbstractAdaptedOutputFactoryComponent(std::string_view id,
                                           AbstractAdaptedOutputFactoryComponentInfo *info);
    ~AbstractAdaptedOutputFactoryComponent() override = default;

    [[nodiscard]] HydroCouple::IAdaptedOutputFactoryComponentInfo *componentInfo() const override;

protected:
    void setComponentInfo(AbstractAdaptedOutputFactoryComponentInfo *info);

private:
    AbstractAdaptedOutputFactoryComponentInfo *m_componentInfo;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUTFACTORY_H
