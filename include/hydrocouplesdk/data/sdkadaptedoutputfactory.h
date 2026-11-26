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
 * \brief Factory offering the SDK's built-in adapted outputs:
 *        linear_transform and relaxation over rank-1 Double outputs, and
 *        temporal_interpolation over single-column Double time-series
 *        outputs.
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
 *
 * \details The code is less strict than the last sentence: every adapter
 * this factory creates is an AbstractAdaptedOutput, whose destructor removes
 * it from its adaptee, so destroying the instance is enough. Unregistering
 * first is harmless.
 *
 * Offerings (getAvailableAdaptedOutputIds()):
 * | Offering id | Offered when the provider is | Creates |
 * | --- | --- | --- |
 * | `linear_transform` | Float64 with a rank-1 shape | LinearTransformAdaptedOutput |
 * | `relaxation` | Float64 with a rank-1 shape | RelaxationAdaptedOutput |
 * | `temporal_interpolation` | Float64, an ITimeSeriesComponentDataItem, shape {times, 1} | Temporal::TemporalInterpolationAdaptedOutput |
 *
 * When a consumer is named and its dataKind() is not Float64, nothing is
 * offered.
 *
 * \threadsafety Not thread-safe: createAdaptedOutput() increments a counter,
 * and instance() is shared by the whole process. Build chains from one thread.
 */
class HYDROCOUPLESDK_EXPORT SdkAdaptedOutputFactory final
    : public AbstractAdaptedOutputFactory
{
public:
    //! Factory id, which composition documents name in a chain step's "factory".
    static constexpr const char *kId                = "hydrocouple.sdk.adapters";
    //! Offering id of the element-wise linear transform (LinearTransformAdaptedOutput).
    static constexpr const char *kLinearTransformId = "linear_transform";
    //! Offering id of the under-relaxation adapter (RelaxationAdaptedOutput).
    static constexpr const char *kRelaxationId      = "relaxation";
    //! Offering id of the temporal interpolation adapter
    //! (Temporal::TemporalInterpolationAdaptedOutput).
    static constexpr const char *kTemporalInterpolationId =
        "temporal_interpolation";

    /*!
     * \brief Builds a factory with the three offerings and id kId.
     * \details Hosts normally use instance(); a separate object behaves the
     *          same but numbers its instances independently.
     */
    SdkAdaptedOutputFactory();
    //! Destroys the factory and its offering identities.
    ~SdkAdaptedOutputFactory() override = default;

    //! Process-wide instance, for resolvers and component-info opt-in.
    //! \return A function-local static that lives until the process exits.
    [[nodiscard]] static SdkAdaptedOutputFactory *instance();

    /*!
     * \brief The offerings that fit \p provider (see the class table).
     * \param[in] provider Output to adapt; nullptr gets no offerings.
     * \param[in] consumer Input the result feeds, or nullptr. Only its
     *                     dataKind() is checked (Float64 required); its
     *                     shape is not compared with the provider's.
     * \return Offering identities owned by the factory, in the order
     *         linear_transform, relaxation, temporal_interpolation.
     */
    std::vector<HydroCouple::IIdentity *> getAvailableAdaptedOutputIds(
        const HydroCouple::IOutput *provider,
        const HydroCouple::IInput *consumer = nullptr) override;

    /*!
     * \brief Creates the adapter for an offering and registers it with
     *        \p provider.
     * \details The offering is chosen by \p adaptedProviderId's id(), so any
     *          identity with a matching id works. The instance id is the
     *          offering id followed by "#" and a per-factory ordinal
     *          ("linear_transform#3"), so several adapters of one offering on
     *          one provider do not replace each other. The provider's fitness
     *          is not checked again here: an adapter created over a provider
     *          it was not offered for serves nothing (its reads fail or it
     *          holds no samples). No error is queued on failure.
     * \param[in] adaptedProviderId Offering identity; nullptr returns nullptr.
     * \param[in] provider          Output to adapt (borrowed); nullptr
     *                              returns nullptr.
     * \param[in] consumer          Ignored.
     * \return The new adapter, already registered with \p provider, or
     *         nullptr for an unknown offering id.
     */
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
