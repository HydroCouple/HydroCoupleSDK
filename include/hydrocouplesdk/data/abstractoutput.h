// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   abstractoutput.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete base implementation of HydroCouple::IOutput.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTOUTPUT_H
#define HYDROCOUPLESDK_ABSTRACTOUTPUT_H

#include "hydrocouplesdk/data/abstractexchangeitem.h"
#include "hydrocouple.h"

#include <unordered_map>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief Concrete base class for HydroCouple::IOutput.
 *
 * Manages the lists of consumers (IInput) and adapted outputs (IAdaptedOutput).
 * Subclasses implement updateValues() which pushes fresh data to consumers and
 * triggers adapted-output refresh.
 *
 * Consumer/adapted-output maps are keyed by the item's id() for O(1) lookup.
 * Pointers are non-owning — lifetimes are managed by the respective components.
 *
 * \details What the implementation keeps: consumers in a vector, in
 * connection order and identified by object (so two components' inputs with
 * the same id both connect); adapted outputs in a map keyed by the adapter's
 * id(), so registering a second adapter with an id already present replaces
 * the first. SdkAdaptedOutputFactory gives each instance a distinct id for
 * that reason. adaptedOutputs() is in hash order.
 *
 * \threadsafety Not thread-safe on its own.
 */
class HYDROCOUPLESDK_EXPORT AbstractOutput
    : public AbstractExchangeItem,
      public virtual HydroCouple::IOutput
{
public:
    /*!
     * \brief Construct with identifier, dimensions, value definition and
     *        owning component; the output starts with no consumers.
     * \param id              Output identifier, unique within the component;
     *                        composition documents name the output by it.
     * \param dimensions      Axes in storage order (borrowed; list copied).
     * \param valueDefinition Value definition (borrowed; may be nullptr).
     * \param modelComponent  Producing component (borrowed; may be nullptr).
     *                        addConsumer() reports failures to it when it is
     *                        an AbstractModelComponent.
     */
    AbstractOutput(std::string_view id,
                   const std::vector<Dimension *> &dimensions,
                   ValueDefinition *valueDefinition,
                   HydroCouple::IModelComponent *modelComponent);

    /*!
     * \brief Construct with identifier, caption, dimensions, value definition
     *        and owning component; the output starts with no consumers.
     * \param id              Output identifier, unique within the component.
     * \param caption         Human-readable caption.
     * \param dimensions      Axes in storage order (borrowed; list copied).
     * \param valueDefinition Value definition (borrowed; may be nullptr).
     * \param modelComponent  Producing component (borrowed; may be nullptr).
     */
    AbstractOutput(std::string_view id,
                   std::string_view caption,
                   const std::vector<Dimension *> &dimensions,
                   ValueDefinition *valueDefinition,
                   HydroCouple::IModelComponent *modelComponent);

    /*!
     * \brief Orphans any SDK adapters still registered, so an adapter that
     *        outlives its adaptee does not deregister through a dangling
     *        pointer in its own destructor.
     * \details Calls AbstractAdaptedOutput::detachFromAdaptee() on every
     *          registered adapter that is an SDK adapter. Consumers are not
     *          told: an input still connected keeps a dangling provider
     *          pointer, so disconnect consumers (removeConsumer()) before
     *          destroying an output.
     */
    ~AbstractOutput() override;

    // ── IOutput ───────────────────────────────────────────────────────────

    /*!
     * \brief Returns the list of registered consumer inputs.
     * \return The consumers in connection order (non-owning), multi-inputs
     *         included.
     */
    [[nodiscard]] std::vector<HydroCouple::IInput *> consumers() const override;

    /*!
     * \brief Adds a consumer and registers this output as the consumer's provider
     * (IMultiInput::addProvider() with \p providerRole for a multi-input,
     * IInput::setProvider() otherwise).
     * \details The single wiring entry point. A null consumer, or one already
     *          connected, is ignored. For a multi-input the output asks
     *          canConsume(this, why, providerRole) and then addProvider();
     *          for any other input it refuses a non-null role and otherwise
     *          asks canConsume(this, why) and then setProvider(this). When
     *          either step fails nothing is recorded, a Severity::Error entry
     *          is queued on the owning component (when it is an
     *          AbstractModelComponent), and std::invalid_argument is thrown
     *          with the message
     *          `Output "<output id>" cannot be consumed by input "<input id>": <reason>`.
     *          On success fires
     *          propertyChanged("Consumers").
     * \param[in] consumer     Input to connect (borrowed; its component owns it).
     * \param[in] providerRole Role this output fills for a multi-input (one
     *                         of its providerLabels()), or nullptr.
     * \exception std::invalid_argument when the consumer refuses the output.
     */
    void addConsumer(HydroCouple::IInput *consumer,
                     const HydroCouple::IIdentity *providerRole = nullptr) override;

    /*!
     * \brief Removes a consumer (IMultiInput::removeProvider() for a multi-input,
     * IInput::setProvider(nullptr) otherwise).
     * \details Fires propertyChanged("Consumers") when the consumer was found.
     * \param[in] consumer Input to disconnect; nullptr returns false.
     * \return true if the consumer was found and removed.
     */
    bool removeConsumer(HydroCouple::IInput *consumer) override;

    /*!
     * \brief Returns the list of registered adapted outputs.
     * \return The adapters registered with addAdaptedOutput() (non-owning),
     *         in hash order.
     */
    [[nodiscard]] std::vector<HydroCouple::IAdaptedOutput *> adaptedOutputs() const override;

    /*!
     * \brief Registers an adapted output against this output as its adaptee.
     * \details Non-owning. Keyed by the adapter's id(): an adapter with the id
     *          of one already registered replaces it. Fires
     *          propertyChanged("AdaptedOutputs"). nullptr is ignored.
     *          SdkAdaptedOutputFactory::createAdaptedOutput() calls this for
     *          the adapters it creates.
     * \param[in] adaptedOutput Adapter whose adaptee() is this output.
     */
    void addAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput) override;

    /*!
     * \brief Removes an adapted output.
     * \details Removes the entry only when the adapter registered under its id
     *          is this same object; fires propertyChanged("AdaptedOutputs").
     *          An SDK adapter calls it from its own destructor.
     * \param[in] adaptedOutput Adapter to deregister; nullptr returns false.
     * \return true if the adapted output was found and removed.
     */
    bool removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput) override;

    /*!
     * \brief Provides fresh values to the query specifier.
     *
     * Default implementation calls the pure-virtual updateValues() and then
     * calls refresh() on all adapted outputs.
     *
     * \details The query is not inspected and the owning component is not
     *          advanced: the values served are whatever updateValues() leaves.
     * \param querySpecifier  The input requesting the values.
     */
    void updateValues(const HydroCouple::IInput *querySpecifier) override;

    /*!
     * \brief Computes and stores fresh values.
     *
     * Pure virtual — subclasses retrieve data from the model state.
     *
     * \details The ready typed outputs (Output1D, Output2D, IdBasedOutput,
     *          TimeSeriesOutput) implement it as a no-op because their
     *          component writes the storage directly in its update().
     *          AbstractModelComponent::updateOutputValues() calls it on each
     *          output; it does not refresh adapted outputs.
     */
    virtual void updateValues() = 0;

    /*!
     * \brief Calls refresh() on all registered adapted outputs.
     * \details Each SDK adapter refreshes its own children in turn, so one
     *          call reaches a whole chain.
     */
    virtual void refreshAdaptedOutputs();

    /*!
     * \brief Calls initialize() on all registered adapted outputs.
     * \details AbstractModelComponent::initialize() calls it for every
     *          output at the end of initialization. Only the first tier is
     *          initialized here; an adapter's own children are not.
     */
    virtual void initializeAdaptedOutputs();

private:
    //! In connection order, identified by object: an input's id() is unique
    //! only within its own component, so two components' "inflow" inputs can
    //! both consume this output.
    std::vector<HydroCouple::IInput *>                             m_consumers;
    //! Registered adapters keyed by their id() (borrowed pointers).
    std::unordered_map<std::string, HydroCouple::IAdaptedOutput *> m_adaptedOutputs;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTOUTPUT_H
