// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin <>

/*!
 * \file   abstractadaptedoutput.h
 * \author Caleb Buahin <>
 * \version 1.0.0
 * \brief  Concrete base implementation of HydroCouple::IAdaptedOutput.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUT_H
#define HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUT_H

#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouple.h"

#include <unordered_map>
#include <vector>

namespace HydroCouple::SDK {

class AbstractArgument;

/*!
 * \brief Concrete base class for HydroCouple::IAdaptedOutput.
 *
 * An adapted output wraps an existing IOutput (the adaptee) and applies a
 * transformation (e.g. temporal interpolation, unit conversion, spatial
 * aggregation) before delivering values to its consumers.
 *
 * This class re-implements the AbstractComponentDataItem interface directly
 * (rather than inheriting from AbstractExchangeItem) so it can hold its own
 * dimensions and value definition independently of the adaptee.
 *
 * Argument management
 * -------------------
 * Subclasses register IArgument instances via addArgument() in their
 * constructors. initialize() is called by the owning output once after all
 * arguments have been set. refresh() is called whenever the adaptee
 * produces new values.
 *
 * \details Who calls what, in the SDK: the chain builder
 * (IO::ModelInitializer) creates an adapter through a factory, applies its
 * arguments as JSON and calls initialize(); AbstractOutput::
 * refreshAdaptedOutputs() and the time-stepped workflow call refresh() on
 * the first adapter of each chain after the provider updates, and every SDK
 * adapter's refresh() ends by refreshing its own children. Nothing calls
 * refresh() because a value-changed event fired; the push is explicit.
 *
 * Ownership: whoever holds the \c std::unique_ptr returned by
 * IAdaptedOutputFactory::createAdaptedOutput() owns the adapter. The adaptee,
 * the factory and the consumers are borrowed. The adapter's
 * modelComponent() is the adaptee's component, captured at construction, so
 * an error during addConsumer() is queued on the provider's component.
 *
 * The constructor does not register the adapter with its adaptee; the
 * factory does (SdkAdaptedOutputFactory), or the code that builds an adapter
 * by hand calls adaptee->addAdaptedOutput(adapter). The destructor
 * deregisters it.
 *
 * \threadsafety Not thread-safe on its own.
 */
class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutput
    : public AbstractComponentDataItem,
      public virtual HydroCouple::IAdaptedOutput
{
public:
    /*!
     * \brief Construct with identifier, dimensions, value definition,
     *        adaptee output, and creating factory.
     * \param id                   Unique identifier.
     * \param dimensions           Ordered dimensions (borrowed).
     * \param valueDefinition      Value definition (borrowed).
     * \param adaptee              The IOutput being adapted (borrowed).
     * \param adaptedOutputFactory The factory that created this adapted output.
     * \details \p id must be unique among the adapters registered on one
     *          adaptee, because AbstractOutput keys its registry by id. The
     *          SDK's concrete adapters pass an empty dimension list and a
     *          null value definition. \p adaptee may be nullptr (the adapter
     *          then has no component and serves nothing);
     *          \p adaptedOutputFactory may be nullptr for an adapter built by
     *          hand.
     */
    AbstractAdaptedOutput(std::string_view id,
                           const std::vector<Dimension *> &dimensions,
                           ValueDefinition *valueDefinition,
                           HydroCouple::IOutput *adaptee,
                           HydroCouple::IAdaptedOutputFactory *adaptedOutputFactory);

    /*!
     * \brief Deregisters from the adaptee (IOutput::addAdaptedOutput contract:
     *        registration is non-owning, so the adapter must remove itself).
     * \details First detaches every SDK adapter chained on this one, so a
     *          chain can be destroyed from either end. Consumers are not
     *          disconnected; disconnect them before destroying the adapter.
     */
    ~AbstractAdaptedOutput() override;

    /*!
     * \brief Forgets the adaptee. Called by AbstractOutput's destructor when the
     *        adaptee dies first; after this, adaptee() is nullptr and refresh()
     *        is a no-op.
     */
    void detachFromAdaptee() noexcept { m_adaptee = nullptr; }

    // ── IOutput ───────────────────────────────────────────────────────────

    /*!
     * \brief The inputs connected to this adapter.
     * \return The consumers in connection order (non-owning).
     */
    [[nodiscard]] std::vector<HydroCouple::IInput *>         consumers()      const override;
    /*!
     * \brief Connects a consumer to this adapter, exactly as
     *        AbstractOutput::addConsumer() does.
     * \details The chain builder wires the consumer to the chain's tail
     *          through this call. On refusal nothing is recorded, an error is
     *          queued on modelComponent() when it is an AbstractModelComponent,
     *          and std::invalid_argument is thrown with the message
     *          `Adapted output "<adapter id>" cannot be consumed by input "<input id>": <reason>`.
     * \param[in] consumer     Input to connect (borrowed); nullptr or an
     *                         input already connected is ignored.
     * \param[in] providerRole Role for a multi-input consumer, or nullptr.
     * \exception std::invalid_argument when the consumer refuses the adapter.
     */
    void addConsumer(HydroCouple::IInput *consumer,
                     const HydroCouple::IIdentity *providerRole = nullptr)          override;
    /*!
     * \brief Disconnects a consumer, as AbstractOutput::removeConsumer() does.
     * \param[in] consumer Input to disconnect; nullptr returns false.
     * \return true if the consumer was found and removed.
     */
    bool removeConsumer(HydroCouple::IInput *consumer)                              override;

    /*!
     * \brief The adapters chained on this one.
     * \return Registered adapters (non-owning), in hash order.
     */
    [[nodiscard]] std::vector<HydroCouple::IAdaptedOutput *> adaptedOutputs() const override;
    /*!
     * \brief Registers an adapter whose adaptee is this adapter.
     * \details Keyed by id(), replacing an adapter with the same id; nullptr
     *          is ignored. Fires propertyChanged("AdaptedOutputs").
     * \param[in] adaptedOutput Adapter to chain (borrowed).
     */
    void addAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput)                override;
    /*!
     * \brief Deregisters a chained adapter.
     * \param[in] adaptedOutput Adapter to remove; removed only when it is the
     *                          object registered under its id.
     * \return true if the adapter was found and removed.
     */
    bool removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput)             override;

    /*!
     * \brief Calls the pure-virtual updateValues() then refreshes adapted outputs.
     * \details Records \p querySpecifier as input() first. The adaptee is not
     *          asked to update: the adapter recomputes from the adaptee's
     *          current values. The SDK's concrete adapters implement
     *          updateValues() as refresh(), which already refreshes their
     *          children, so a call here refreshes each child twice; a
     *          stateful child (RelaxationAdaptedOutput) then blends twice.
     * \param[in] querySpecifier The input asking for values (borrowed; may be nullptr).
     */
    void updateValues(const HydroCouple::IInput *querySpecifier) override;

    /*!
     * \brief Computes fresh adapted values.
     *
     * Pure virtual — subclasses apply their transformation here.
     */
    virtual void updateValues() = 0;

    // ── IAdaptedOutput ────────────────────────────────────────────────────

    /*!
     * \brief Returns the factory that created this adapted output.
     * \return The factory given at construction (non-owning; may be nullptr).
     */
    [[nodiscard]] HydroCouple::IAdaptedOutputFactory *adaptedOutputFactory() const override;

    /*!
     * \brief Returns the list of configuration arguments for this adapted output.
     * \return The arguments registered with addArgument() (non-owning), in
     *         hash order. A host sets them, through IArgument::initialize(),
     *         before initialize().
     */
    [[nodiscard]] std::vector<HydroCouple::IArgument *> arguments() const override;

    /*!
     * \brief Initializes the adapted output from current argument values.
     *
     * Called once by the owning output after all arguments are set.
     * Pure virtual — subclasses parse arguments and prepare internal state.
     */
    void initialize() override = 0;

    /*!
     * \brief Returns the output being adapted.
     * \return The adaptee (non-owning), or nullptr after detachFromAdaptee().
     */
    [[nodiscard]] HydroCouple::IOutput *adaptee() const override;

    /*!
     * \brief Requests the adapted output to recompute its values from the adaptee.
     *
     * Pure virtual — called after the adaptee fires a value-changed event.
     *
     * \details In the SDK the caller is the adaptee's refreshAdaptedOutputs(),
     *          the time-stepped workflow, or the host; implementations end by
     *          calling refreshAdaptedOutputs() so the chain below follows.
     */
    void refresh() override = 0;

    /*!
     * \brief The state carried from one refresh() to the next: none here.
     *
     * Right for an adapter whose values depend only on its adaptee's current
     * values and its arguments. A subclass whose refresh() depends on its
     * past (RelaxationAdaptedOutput, TemporalInterpolationAdaptedOutput)
     * overrides it, so an orchestrator returning the composition to an
     * earlier step knows the adapter must be returned too.
     *
     * \return An empty vector.
     */
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> states() const override;

    // ── Convenience accessors ─────────────────────────────────────────────

    /*!
     * \brief Returns the input that requested the most recent updateValues().
     * \return The last query input (non-owning), or nullptr before any. None
     *         of the SDK's adapters reads it.
     */
    [[nodiscard]] HydroCouple::IInput *input() const;

    /*!
     * \brief Sets the requesting input (called internally by updateValues).
     * \param[in] input Input to record (borrowed; may be nullptr).
     */
    void setInput(HydroCouple::IInput *input);

    /*!
     * \brief Calls refresh() on all chained adapted outputs.
     * \details Only the direct children; each SDK child refreshes its own.
     */
    virtual void refreshAdaptedOutputs();

    /*!
     * \brief Calls initialize() on all chained adapted outputs.
     * \details Only the direct children, in hash order.
     */
    virtual void initializeAdaptedOutputs();

protected:
    /*!
     * \brief Registers a configuration argument.
     * \details Keyed by the argument's id(); an argument with the id of one
     *          already registered replaces it. nullptr is ignored. Fires
     *          propertyChanged("Arguments").
     * \param argument  Non-owning pointer; lifetime managed by the subclass.
     */
    void addArgument(AbstractArgument *argument);

    /*!
     * \brief Removes a previously registered argument.
     * \details Fires propertyChanged("Arguments") when found.
     * \param[in] argument Argument to remove; removed only when it is the
     *                     object registered under its id. nullptr returns false.
     * \return true if found and removed.
     */
    bool removeArgument(AbstractArgument *argument);

    /*!
     * \brief Removes all arguments.
     * \details The argument objects are not destroyed. Fires
     *          propertyChanged("Arguments").
     */
    void clearArguments();

    /*!
     * \brief Returns the internal argument map (keyed by argument id).
     * \return A copy of the map; the pointers are non-owning.
     */
    [[nodiscard]] std::unordered_map<std::string, AbstractArgument *>
    argumentsInternal() const;

private:
    HydroCouple::IOutput                 *m_adaptee;                //!< Output being adapted (borrowed), or nullptr once detached.
    HydroCouple::IAdaptedOutputFactory   *m_adaptedOutputFactory;   //!< Creating factory (borrowed; may be nullptr).
    HydroCouple::IInput                  *m_input = nullptr;        //!< Input of the most recent updateValues() query.

    //! In connection order, identified by object: an input's id() is unique
    //! only within its own component, so two components' "inflow" inputs can
    //! both consume this output.
    std::vector<HydroCouple::IInput *>                             m_consumers;
    //! Chained adapters keyed by their id() (borrowed).
    std::unordered_map<std::string, HydroCouple::IAdaptedOutput *> m_adaptedOutputs;
    //! Configuration arguments keyed by their id() (borrowed; owned by the subclass).
    std::unordered_map<std::string, AbstractArgument *>            m_arguments;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUT_H
