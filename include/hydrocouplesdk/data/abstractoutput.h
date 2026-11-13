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
 * \threadsafety Not thread-safe on its own.
 */
class HYDROCOUPLESDK_EXPORT AbstractOutput
    : public AbstractExchangeItem,
      public virtual HydroCouple::IOutput
{
public:
    AbstractOutput(std::string_view id,
                   const std::vector<Dimension *> &dimensions,
                   ValueDefinition *valueDefinition,
                   HydroCouple::IModelComponent *modelComponent);

    AbstractOutput(std::string_view id,
                   std::string_view caption,
                   const std::vector<Dimension *> &dimensions,
                   ValueDefinition *valueDefinition,
                   HydroCouple::IModelComponent *modelComponent);

    /*!
     * \brief Orphans any SDK adapters still registered, so an adapter that
     *        outlives its adaptee does not deregister through a dangling
     *        pointer in its own destructor.
     */
    ~AbstractOutput() override;

    // ── IOutput ───────────────────────────────────────────────────────────

    /*!
     * \brief Returns the list of registered consumer inputs.
     */
    [[nodiscard]] std::vector<HydroCouple::IInput *> consumers() const override;

    /*!
     * \brief Adds a consumer and registers this output as the consumer's provider
     * (IMultiInput::addProvider() with \p providerRole for a multi-input,
     * IInput::setProvider() otherwise).
     */
    void addConsumer(HydroCouple::IInput *consumer,
                     const HydroCouple::IIdentity *providerRole = nullptr) override;

    /*!
     * \brief Removes a consumer (IMultiInput::removeProvider() for a multi-input,
     * IInput::setProvider(nullptr) otherwise).
     * \return true if the consumer was found and removed.
     */
    bool removeConsumer(HydroCouple::IInput *consumer) override;

    /*!
     * \brief Returns the list of registered adapted outputs.
     */
    [[nodiscard]] std::vector<HydroCouple::IAdaptedOutput *> adaptedOutputs() const override;

    /*!
     * \brief Registers an adapted output against this output as its adaptee.
     */
    void addAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput) override;

    /*!
     * \brief Removes an adapted output.
     * \return true if the adapted output was found and removed.
     */
    bool removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput) override;

    /*!
     * \brief Provides fresh values to the query specifier.
     *
     * Default implementation calls the pure-virtual updateValues() and then
     * calls refresh() on all adapted outputs.
     *
     * \param querySpecifier  The input requesting the values.
     */
    void updateValues(const HydroCouple::IInput *querySpecifier) override;

    /*!
     * \brief Computes and stores fresh values.
     *
     * Pure virtual — subclasses retrieve data from the model state.
     */
    virtual void updateValues() = 0;

    /*!
     * \brief Calls refresh() on all registered adapted outputs.
     */
    virtual void refreshAdaptedOutputs();

    /*!
     * \brief Calls initialize() on all registered adapted outputs.
     */
    virtual void initializeAdaptedOutputs();

private:
    //! In connection order, identified by object: an input's id() is unique
    //! only within its own component, so two components' "inflow" inputs can
    //! both consume this output.
    std::vector<HydroCouple::IInput *>                             m_consumers;
    std::unordered_map<std::string, HydroCouple::IAdaptedOutput *> m_adaptedOutputs;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTOUTPUT_H
