// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/sdkadaptedoutputfactory.h"
#include "hydrocouplesdk/data/lineartransformadaptedoutput.h"
#include "hydrocouplesdk/data/relaxationadaptedoutput.h"
#include "hydrocouplesdk/temporal/temporalinterpolationadaptedoutput.h"

#include "hydrocoupletemporal.h"

namespace HydroCouple::SDK {

namespace {

//! Whether \p item is a Float64 item of rank 1 (what the element-wise
//! adapters accept).
//! \param[in] item Item to test; nullptr gives false.
//! \return true for a rank-1 Float64 item.
bool isRankOneDouble(const HydroCouple::IComponentDataItem *item)
{
    return item && item->dataKind() == HydroCouple::DataKind::Float64 &&
           item->shape().size() == 1;
}

//! A single-column Double time-series: {timeCount, 1} with a time axis.
//! \param[in] provider Output to test; nullptr gives false.
//! \return true for a Float64 ITimeSeriesComponentDataItem of shape {n, 1}.
bool isSingleColumnTimeSeries(const HydroCouple::IOutput *provider)
{
    if (!provider ||
        provider->dataKind() != HydroCouple::DataKind::Float64 ||
        !dynamic_cast<const HydroCouple::Temporal::ITimeSeriesComponentDataItem *>(
            provider))
        return false;
    const std::vector<int64_t> shape = provider->shape();
    return shape.size() == 2 && shape[1] == 1;
}

} // namespace

SdkAdaptedOutputFactory::SdkAdaptedOutputFactory()
    : AbstractAdaptedOutputFactory(kId)
{
    m_ids.push_back(std::make_unique<Identity>(
        kLinearTransformId, "Linear transform (multiplier, offset)"));
    m_ids.push_back(std::make_unique<Identity>(
        kRelaxationId, "Under-relaxation (relaxation_factor)"));
    m_ids.push_back(std::make_unique<Identity>(
        kTemporalInterpolationId, "Temporal interpolation (method)"));
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
    // A consumer, when named, must be able to take doubles; its shape is
    // the run pipeline's problem.
    if (consumer && consumer->dataKind() != HydroCouple::DataKind::Float64)
        return {};

    std::vector<HydroCouple::IIdentity *> available;
    for (const std::unique_ptr<Identity> &identity : m_ids)
    {
        // The element-wise transforms want a rank-1 Double slab; the
        // temporal interpolation wants a single-column time-series.
        if (identity->id() == kTemporalInterpolationId
                ? isSingleColumnTimeSeries(provider)
                : isRankOneDouble(provider))
            available.push_back(identity.get());
    }
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
    else if (adaptedProviderId->id() == kTemporalInterpolationId)
    {
        created = std::make_unique<Temporal::TemporalInterpolationAdaptedOutput>(
            instanceId, provider, this);
    }

    if (created)
        provider->addAdaptedOutput(created.get());

    return created;
}

} // namespace HydroCouple::SDK
