// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/sdkadaptedoutputfactory.h"
#include "hydrocouplesdk/data/lineartransformadaptedoutput.h"
#include "hydrocouplesdk/data/relaxationadaptedoutput.h"

namespace HydroCouple::SDK {

namespace {

bool isRankOneDouble(const HydroCouple::IComponentDataItem *item)
{
    return item && item->dataKind() == HydroCouple::DataKind::Float64 &&
           item->shape().size() == 1;
}

} // namespace

SdkAdaptedOutputFactory::SdkAdaptedOutputFactory()
    : AbstractAdaptedOutputFactory(kId)
{
    m_ids.push_back(std::make_unique<Identity>(
        kLinearTransformId, "Linear transform (multiplier, offset)"));
    m_ids.push_back(std::make_unique<Identity>(
        kRelaxationId, "Under-relaxation (relaxation_factor)"));
}

SdkAdaptedOutputFactory *SdkAdaptedOutputFactory::instance()
{
    static SdkAdaptedOutputFactory s_instance;
    return &s_instance;
}

std::vector<HydroCouple::IIdentity *>
SdkAdaptedOutputFactory::getAvailableAdaptedOutputIds(
    const HydroCouple::IOutput *provider, const HydroCouple::IInput *consumer)
{
    // Every offering transforms a rank-1 Double slab element-wise, so the
    // provider gate is the offering gate. A consumer, when named, must be
    // able to take doubles; its shape is the run pipeline's problem.
    if (!isRankOneDouble(provider))
        return {};
    if (consumer && consumer->dataKind() != HydroCouple::DataKind::Float64)
        return {};

    std::vector<HydroCouple::IIdentity *> available;
    available.reserve(m_ids.size());
    for (const std::unique_ptr<Identity> &identity : m_ids)
        available.push_back(identity.get());
    return available;
}

std::unique_ptr<HydroCouple::IAdaptedOutput>
SdkAdaptedOutputFactory::createAdaptedOutput(
    HydroCouple::IIdentity *adaptedProviderId, HydroCouple::IOutput *provider,
    HydroCouple::IInput * /*consumer*/)
{
    if (!adaptedProviderId || !provider)
        return nullptr;

    // Instance ids get a per-factory ordinal: the provider's adapted-output
    // registry keys by id, and one output may carry several instances of
    // the same offering (one per consuming connection).
    const std::string instanceId =
        adaptedProviderId->id() + "#" + std::to_string(++m_created);

    std::unique_ptr<HydroCouple::IAdaptedOutput> created;

    if (adaptedProviderId->id() == kLinearTransformId)
    {
        created = std::make_unique<LinearTransformAdaptedOutput>(
            instanceId, provider, this);
    }
    else if (adaptedProviderId->id() == kRelaxationId)
    {
        created = std::make_unique<RelaxationAdaptedOutput>(
            instanceId, provider, this);
    }

    if (created)
        provider->addAdaptedOutput(created.get());

    return created;
}

} // namespace HydroCouple::SDK
