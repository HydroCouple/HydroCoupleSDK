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
 * \details Wiring goes through the provider: IOutput::addConsumer() checks
 * canConsume() and then calls setProvider(this output), and
 * IOutput::removeConsumer() calls setProvider(nullptr). Orchestrators do not
 * call setProvider() themselves. The provider is borrowed; the connection
 * owner (a host, a ModelInitializer) keeps it alive while connected.
 *
 * Exchange in a time step: the workflow brings the provider up to date
 * (IOutput::updateValues() or the component's own update), and the
 * consuming component's AbstractModelComponent::applyInputValues() calls
 * retrieveValuesFromProvider() and then applyData() on each of its inputs.
 *
 * \threadsafety Not thread-safe on its own.
 */
class HYDROCOUPLESDK_EXPORT AbstractInput
    : public AbstractExchangeItem,
      public virtual HydroCouple::IInput
{
public:
    /*!
     * \brief Construct with identifier, dimensions, value definition and
     *        owning component; the input starts unconnected.
     * \param id              Input identifier, unique within the component;
     *                        composition documents name the input by it.
     * \param dimensions      Axes in storage order (borrowed; list copied).
     * \param valueDefinition Value definition (borrowed; may be nullptr).
     * \param modelComponent  Owning component (borrowed; may be nullptr).
     */
    AbstractInput(std::string_view id,
                  const std::vector<Dimension *> &dimensions,
                  ValueDefinition *valueDefinition,
                  HydroCouple::IModelComponent *modelComponent);

    /*!
     * \brief Construct with identifier, caption, dimensions, value definition
     *        and owning component; the input starts unconnected.
     * \param id              Input identifier, unique within the component.
     * \param caption         Human-readable caption.
     * \param dimensions      Axes in storage order (borrowed; list copied).
     * \param valueDefinition Value definition (borrowed; may be nullptr).
     * \param modelComponent  Owning component (borrowed; may be nullptr).
     */
    AbstractInput(std::string_view id,
                  std::string_view caption,
                  const std::vector<Dimension *> &dimensions,
                  ValueDefinition *valueDefinition,
                  HydroCouple::IModelComponent *modelComponent);

    //! Destroys the input. It does not disconnect itself from its provider.
    ~AbstractInput() override = default;

    // ── IInput ────────────────────────────────────────────────────────────

    /*!
     * \brief Returns the current provider output (may be nullptr).
     * \return The output recorded by setProvider() (non-owning), or nullptr
     *         when unconnected. A multi-input returns nullptr; its providers
     *         are read through providers().
     */
    [[nodiscard]] HydroCouple::IOutput *provider() const override;

    /*!
     * \brief Sets the provider. Calls canConsume() first; returns false and
     *        leaves the provider unchanged if the check fails.
     * \details nullptr disconnects without a check. A non-null provider
     *          replaces the current one, if any, after canConsume() accepts
     *          it. Fires propertyChanged("Provider") on success. Called by
     *          IOutput::addConsumer() and removeConsumer().
     * \param[in] provider Output to record (borrowed), or nullptr.
     * \return true when the provider was recorded.
     */
    bool setProvider(HydroCouple::IOutput *provider) override;

    /*!
     * \brief Returns true if the given provider is compatible with this input.
     *
     * Pure virtual — subclasses apply component-specific compatibility checks.
     *
     * \param[in]  provider  Candidate output.
     * \param[out] message   Explanation when returning false.
     * \return true when this input can take values from \p provider.
     */
    bool canConsume(HydroCouple::IOutput *provider,
                    std::string &message) const override = 0;

    /*!
     * \brief Pulls values from the connected provider.
     *
     * Called by the model's update cycle before applyData(). Pure virtual.
     *
     * \details Reads the provider's current values with getValuesInto(); it
     *          does not ask the provider to update. The SDK's typed inputs
     *          return without changing anything when unconnected or when the
     *          provider's shape or kind does not fit, and report no error.
     */
    virtual void retrieveValuesFromProvider() = 0;

    /*!
     * \brief Applies the retrieved values to the component's internal state.
     *
     * Called after retrieveValuesFromProvider(). Pure virtual.
     *
     * \details The SDK's typed inputs implement it as an empty hook; a
     *          component either overrides it or reads the input's storage in
     *          its own update().
     */
    virtual void applyData() = 0;

private:
    HydroCouple::IOutput *m_provider = nullptr; //!< Connected provider (borrowed), or nullptr.
};


// ── AbstractMultiInput ────────────────────────────────────────────────────

/*!
 * \brief Concrete base class for HydroCouple::IMultiInput.
 *
 * Stores multiple provider output items keyed by their IIdentity id.
 * Subclasses implement the canConsume() overloads and the data-pull logic.
 *
 * \details The key is the role label's id when the provider was added with
 * a role, and the provider's own id otherwise. A second provider added
 * under a key already in use replaces the first in providers() without an
 * error, although both outputs keep the input among their consumers(); give
 * each provider a role, or providers with distinct ids, when several feed
 * one multi-input. providers() is in hash order, not connection order.
 *
 * \threadsafety Not thread-safe on its own.
 */
class HYDROCOUPLESDK_EXPORT AbstractMultiInput
    : public AbstractInput,
      public virtual HydroCouple::IMultiInput
{
public:
    // Keeps the two-argument IInput::canConsume() visible beside the role overload.
    using AbstractInput::canConsume;

    /*!
     * \brief Construct with identifier, dimensions, value definition and
     *        owning component; the input starts with no providers.
     * \param id              Input identifier, unique within the component.
     * \param dimensions      Axes in storage order (borrowed; list copied).
     * \param valueDefinition Value definition (borrowed; may be nullptr).
     * \param modelComponent  Owning component (borrowed; may be nullptr).
     */
    AbstractMultiInput(std::string_view id,
                       const std::vector<Dimension *> &dimensions,
                       ValueDefinition *valueDefinition,
                       HydroCouple::IModelComponent *modelComponent);

    /*!
     * \brief Construct with identifier, caption, dimensions, value definition
     *        and owning component; the input starts with no providers.
     * \param id              Input identifier, unique within the component.
     * \param caption         Human-readable caption.
     * \param dimensions      Axes in storage order (borrowed; list copied).
     * \param valueDefinition Value definition (borrowed; may be nullptr).
     * \param modelComponent  Owning component (borrowed; may be nullptr).
     */
    AbstractMultiInput(std::string_view id,
                       std::string_view caption,
                       const std::vector<Dimension *> &dimensions,
                       ValueDefinition *valueDefinition,
                       HydroCouple::IModelComponent *modelComponent);

    //! Destroys the input. It does not disconnect itself from its providers.
    ~AbstractMultiInput() override = default;

    // ── IMultiInput ───────────────────────────────────────────────────────

    /*!
     * \brief Returns the role-label identities for each required provider slot.
     * \return The base returns an empty list: no named roles, every provider
     *         fills the same slot. A subclass with roles overrides it and
     *         owns the label objects.
     */
    [[nodiscard]] std::vector<HydroCouple::IIdentity *> providerLabels() const override;

    /*!
     * \brief Returns true if the given label corresponds to a required slot.
     * \param[in] providerLabel Role label to query (ignored by the base).
     * \return false in the base: no role is required.
     */
    [[nodiscard]] bool isRequiredProvider(
        const HydroCouple::IIdentity *providerLabel) const override;

    /*!
     * \brief Returns the list of currently connected provider outputs.
     * \return One pointer per key (non-owning), in unspecified order.
     */
    [[nodiscard]] std::vector<HydroCouple::IOutput *> providers() const override;

    /*!
     * \brief Compatibility check (multi-provider variant).
     *
     * Pure virtual — subclasses validate against the specific role.
     *
     * \param[in]  provider               Candidate output.
     * \param[out] message                Explanation when returning false.
     * \param[in]  providerRoleIdentifier Role the provider would fill (one
     *                                    of providerLabels()), or nullptr.
     * \return true when \p provider can fill the role.
     */
    bool canConsume(HydroCouple::IOutput *provider,
                    std::string &message,
                    const HydroCouple::IIdentity *providerRoleIdentifier = nullptr)
        const override = 0;

    /*!
     * \brief Adds a provider to the given role slot (or any free slot if role is nullptr).
     * \details Called by IOutput::addConsumer(), which has already checked
     * canConsume() for the role; orchestrators wire through the output.
     * What the implementation does: it stores \p provider under the role's
     * id, or under the provider's id when the role is nullptr, replacing any
     * provider already under that key. Fires propertyChanged("Providers").
     * \param[in] provider               Output to record (borrowed); nullptr
     *                                   is refused.
     * \param[in] providerRoleIdentifier Role it fills, or nullptr.
     * \return false for a null provider, true otherwise.
     */
    bool addProvider(HydroCouple::IOutput *provider,
                     const HydroCouple::IIdentity *providerRoleIdentifier = nullptr) override;

    /*!
     * \brief Removes a provider.
     * \details Removes the first entry holding \p provider and fires
     *          propertyChanged("Providers"). Called by
     *          IOutput::removeConsumer().
     * \param[in] provider Output to forget; nullptr returns false.
     * \return true if the provider was found and removed.
     */
    bool removeProvider(HydroCouple::IOutput *provider) override;

protected:
    //! Connected providers keyed by role id, or by provider id when added
    //! without a role (borrowed pointers).
    std::unordered_map<std::string, HydroCouple::IOutput *> m_providers;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTINPUT_H
