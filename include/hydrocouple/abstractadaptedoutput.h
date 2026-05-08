// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin <calebgh@gmail.com>

/*!
 * \file   abstractadaptedoutput.h
 * \author Caleb Buahin <calebgh@gmail.com>
 * \version 1.0.0
 * \brief  Concrete base implementation of HydroCouple::IAdaptedOutput.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUT_H
#define HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUT_H

#include "abstractcomponentdataitem.h"
#include "hydrocouple.h"

#include <unordered_map>
#include <vector>

namespace HydroCouple {

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
     */
    AbstractAdaptedOutput(std::string_view id,
                           const std::vector<Dimension *> &dimensions,
                           ValueDefinition *valueDefinition,
                           HydroCouple::IOutput *adaptee,
                           HydroCouple::IAdaptedOutputFactory *adaptedOutputFactory);

    ~AbstractAdaptedOutput() override = default;

    // ── IOutput ───────────────────────────────────────────────────────────

    [[nodiscard]] std::vector<HydroCouple::IInput *>         consumers()      const override;
    void addConsumer(HydroCouple::IInput *consumer)                                 override;
    bool removeConsumer(HydroCouple::IInput *consumer)                              override;

    [[nodiscard]] std::vector<HydroCouple::IAdaptedOutput *> adaptedOutputs() const override;
    void addAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput)                override;
    bool removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput)             override;

    /*!
     * \brief Calls the pure-virtual updateValues() then refreshes adapted outputs.
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
     */
    [[nodiscard]] HydroCouple::IAdaptedOutputFactory *adaptedOutputFactory() const override;

    /*!
     * \brief Returns the list of configuration arguments for this adapted output.
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
     */
    [[nodiscard]] HydroCouple::IOutput *adaptee() const override;

    /*!
     * \brief Requests the adapted output to recompute its values from the adaptee.
     *
     * Pure virtual — called after the adaptee fires a value-changed event.
     */
    void refresh() override = 0;

    // ── Convenience accessors ─────────────────────────────────────────────

    /*!
     * \brief Returns the input that requested the most recent updateValues().
     */
    [[nodiscard]] HydroCouple::IInput *input() const;

    /*!
     * \brief Sets the requesting input (called internally by updateValues).
     */
    void setInput(HydroCouple::IInput *input);

    /*!
     * \brief Calls refresh() on all chained adapted outputs.
     */
    virtual void refreshAdaptedOutputs();

    /*!
     * \brief Calls initialize() on all chained adapted outputs.
     */
    virtual void initializeAdaptedOutputs();

protected:
    /*!
     * \brief Registers a configuration argument.
     * \param argument  Non-owning pointer; lifetime managed by the subclass.
     */
    void addArgument(AbstractArgument *argument);

    /*!
     * \brief Removes a previously registered argument.
     * \return true if found and removed.
     */
    bool removeArgument(AbstractArgument *argument);

    /*!
     * \brief Removes all arguments.
     */
    void clearArguments();

    /*!
     * \brief Returns the internal argument map (keyed by argument id).
     */
    [[nodiscard]] std::unordered_map<std::string, AbstractArgument *>
    argumentsInternal() const;

private:
    HydroCouple::IOutput                 *m_adaptee;
    HydroCouple::IAdaptedOutputFactory   *m_adaptedOutputFactory;
    HydroCouple::IInput                  *m_input = nullptr;

    std::unordered_map<std::string, HydroCouple::IInput *>         m_consumers;
    std::unordered_map<std::string, HydroCouple::IAdaptedOutput *> m_adaptedOutputs;
    std::unordered_map<std::string, AbstractArgument *>            m_arguments;
};

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_ABSTRACTADAPTEDOUTPUT_H
