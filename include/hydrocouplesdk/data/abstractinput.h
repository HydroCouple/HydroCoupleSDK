// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   abstractinput.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete implementations of HydroCouple::IInput and IMultiInput.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTINPUT_H
#define HYDROCOUPLESDK_ABSTRACTINPUT_H

#include "hydrocouplesdk/data/abstractexchangeitem.h"
#include "hydrocouple.h"

#include <unordered_map>
#include <vector>

namespace HydroCouple::SDK {

// ── AbstractInput ─────────────────────────────────────────────────────────

/*!
 * \brief Concrete base class for HydroCouple::IInput.
 *
 * Stores a single provider output item. Subclasses implement
 * canConsume(), retrieveValuesFromProvider(), and applyData().
 *
 * \threadsafety Not thread-safe on its own.
 */
class HYDROCOUPLESDK_EXPORT AbstractInput
    : public AbstractExchangeItem,
      public virtual HydroCouple::IInput
{
public:
    AbstractInput(std::string_view id,
                  const std::vector<Dimension *> &dimensions,
                  ValueDefinition *valueDefinition,
                  HydroCouple::IModelComponent *modelComponent);

    AbstractInput(std::string_view id,
                  std::string_view caption,
                  const std::vector<Dimension *> &dimensions,
                  ValueDefinition *valueDefinition,
                  HydroCouple::IModelComponent *modelComponent);

    ~AbstractInput() override = default;

    // ── IInput ────────────────────────────────────────────────────────────

    /*!
     * \brief Returns the current provider output (may be nullptr).
     */
    [[nodiscard]] HydroCouple::IOutput *provider() const override;

    /*!
     * \brief Sets the provider. Calls canConsume() first; returns false and
     *        leaves the provider unchanged if the check fails.
     */
    bool setProvider(HydroCouple::IOutput *provider) override;

    /*!
     * \brief Returns true if the given provider is compatible with this input.
     *
     * Pure virtual — subclasses apply component-specific compatibility checks.
     *
     * \param[in]  provider  Candidate output.
     * \param[out] message   Explanation when returning false.
     */
    bool canConsume(HydroCouple::IOutput *provider,
                    std::string &message) const override = 0;

    /*!
     * \brief Pulls values from the connected provider.
     *
     * Called by the model's update cycle before applyData(). Pure virtual.
     */
    virtual void retrieveValuesFromProvider() = 0;

    /*!
     * \brief Applies the retrieved values to the component's internal state.
     *
     * Called after retrieveValuesFromProvider(). Pure virtual.
     */
    virtual void applyData() = 0;

private:
    HydroCouple::IOutput *m_provider = nullptr;
};


// ── AbstractMultiInput ────────────────────────────────────────────────────

/*!
 * \brief Concrete base class for HydroCouple::IMultiInput.
 *
 * Stores multiple provider output items keyed by their IIdentity id.
 * Subclasses implement the canConsume() overloads and the data-pull logic.
 *
 * \threadsafety Not thread-safe on its own.
 */
class HYDROCOUPLESDK_EXPORT AbstractMultiInput
    : public AbstractInput,
      public virtual HydroCouple::IMultiInput
{
public:
    using AbstractInput::canConsume;

    AbstractMultiInput(std::string_view id,
                       const std::vector<Dimension *> &dimensions,
                       ValueDefinition *valueDefinition,
                       HydroCouple::IModelComponent *modelComponent);

    AbstractMultiInput(std::string_view id,
                       std::string_view caption,
                       const std::vector<Dimension *> &dimensions,
                       ValueDefinition *valueDefinition,
                       HydroCouple::IModelComponent *modelComponent);

    ~AbstractMultiInput() override = default;

    // ── IMultiInput ───────────────────────────────────────────────────────

    /*!
     * \brief Returns the role-label identities for each required provider slot.
     */
    [[nodiscard]] std::vector<HydroCouple::IIdentity *> providerLabels() const override;

    /*!
     * \brief Returns true if the given label corresponds to a required slot.
     */
    [[nodiscard]] bool isRequiredProvider(
        const HydroCouple::IIdentity *providerLabel) const override;

    /*!
     * \brief Returns the list of currently connected provider outputs.
     */
    [[nodiscard]] std::vector<HydroCouple::IOutput *> providers() const override;

    /*!
     * \brief Compatibility check (multi-provider variant).
     *
     * Pure virtual — subclasses validate against the specific role.
     */
    bool canConsume(HydroCouple::IOutput *provider,
                    std::string &message,
                    const HydroCouple::IIdentity *providerRoleIdentifier = nullptr)
        const override = 0;

    /*!
     * \brief Adds a provider to the given role slot (or any free slot if role is nullptr).
     */
    bool addProvider(HydroCouple::IOutput *provider,
                     const HydroCouple::IIdentity *providerRoleIdentifier = nullptr) override;

    /*!
     * \brief Removes a provider.
     * \return true if the provider was found and removed.
     */
    bool removeProvider(HydroCouple::IOutput *provider) override;

protected:
    std::unordered_map<std::string, HydroCouple::IOutput *> m_providers;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTINPUT_H
