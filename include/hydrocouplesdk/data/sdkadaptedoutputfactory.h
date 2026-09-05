// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   sdkadaptedoutputfactory.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  The SDK's own adapted-output factory.
 */

#ifndef HYDROCOUPLESDK_SDKADAPTEDOUTPUTFACTORY_H
#define HYDROCOUPLESDK_SDKADAPTEDOUTPUTFACTORY_H

#include "hydrocouplesdk/data/abstractadaptedoutputfactory.h"

#include <memory>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief Factory offering the SDK's built-in adapted outputs over rank-1
 *        Double outputs: linear_transform and relaxation.
 *
 * Hosts hand instance() to a run pipeline's standalone-factory resolver, and
 * a component info may opt in via addAdaptedOutputFactory(instance()).
 *
 * createAdaptedOutput() registers the created adapted output with the
 * provider before returning — the IAdaptedOutputFactory contract
 * ("the returned IAdaptedOutput will already be registered") is a factory
 * postcondition here, deliberately not an AbstractAdaptedOutput constructor
 * side effect, so hand-wired adapted outputs stay unregistered until their
 * owner says otherwise. The caller owns the instance and must unregister it
 * (IOutput::removeAdaptedOutput) before destroying it: the provider's
 * registry is non-owning.
 */
class HYDROCOUPLESDK_EXPORT SdkAdaptedOutputFactory final
    : public AbstractAdaptedOutputFactory
{
public:
    static constexpr const char *kId                = "hydrocouple.sdk.adapters";
    static constexpr const char *kLinearTransformId = "linear_transform";
    static constexpr const char *kRelaxationId      = "relaxation";

    SdkAdaptedOutputFactory();
    ~SdkAdaptedOutputFactory() override = default;

    //! Process-wide instance, for resolvers and component-info opt-in.
    [[nodiscard]] static SdkAdaptedOutputFactory *instance();

    std::vector<HydroCouple::IIdentity *> getAvailableAdaptedOutputIds(
        const HydroCouple::IOutput *provider,
        const HydroCouple::IInput *consumer = nullptr) override;

    std::unique_ptr<HydroCouple::IAdaptedOutput> createAdaptedOutput(
        HydroCouple::IIdentity *adaptedProviderId,
        HydroCouple::IOutput *provider,
        HydroCouple::IInput *consumer = nullptr) override;

private:
    //! Offering identities handed out by getAvailableAdaptedOutputIds().
    std::vector<std::unique_ptr<Identity>> m_ids;

    //! Distinguishes instance ids when one provider carries several
    //! adapted outputs of the same offering (the registry keys by id).
    int m_created = 0;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_SDKADAPTEDOUTPUTFACTORY_H
