// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   linearreservoir.h
 * \brief  The developer guide's first component (chapter 3): a linear
 *         reservoir, dS/dt = I - k S with outflow Q = k S.
 */

#ifndef DOCS_EXAMPLE_LINEARRESERVOIR_H
#define DOCS_EXAMPLE_LINEARRESERVOIR_H

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/component/abstractmodelcomponentinfo.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/unit.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

//! [class]
/*!
 * \brief A single linear reservoir: storage S (m³) fed by an inflow I (m³/s)
 *        and drained at Q = k S, advanced exactly over each time step for the
 *        inflow held during that step.
 */
class LinearReservoir : public HydroCouple::SDK::AbstractModelComponent
{
public:
    explicit LinearReservoir(std::string_view id,
                             HydroCouple::SDK::AbstractModelComponentInfo *info = nullptr);

    std::vector<std::string> validate() override;
    void prepare() override;
    void update(const std::vector<HydroCouple::IOutput *> &requiredOutputs = {}) override;
    void finish() override;

protected:
    void createArguments() override;
    bool initializeArguments(std::string &message) override;
    void createInputs() override;
    void createOutputs() override;
    void initializeFailureCleanUp() override;

private:
    // Metadata the items point to. It must outlive them, so it is declared
    // first (members are destroyed in reverse order). A Quantity does not take
    // ownership of its Unit, so the component owns that too.
    HydroCouple::SDK::Dimension m_scalar{"scalar", "One value"};
    std::unique_ptr<HydroCouple::SDK::Unit> m_perSecond;
    std::unique_ptr<HydroCouple::SDK::Quantity> m_rate;
    std::unique_ptr<HydroCouple::SDK::Quantity> m_volume;
    std::unique_ptr<HydroCouple::SDK::Quantity> m_flow;
    std::unique_ptr<HydroCouple::SDK::Quantity> m_seconds;

    // The component owns its items. addArgument(), addInput() and addOutput()
    // only register them with the base class, which never deletes them.
    std::unique_ptr<HydroCouple::SDK::Argument1DDouble> m_recessionConstant; // k, 1/s
    std::unique_ptr<HydroCouple::SDK::Argument1DDouble> m_initialStorage;    // S0, m³
    std::unique_ptr<HydroCouple::SDK::Argument1DDouble> m_timeStep;          // dt, s
    std::unique_ptr<HydroCouple::SDK::Argument1DDouble> m_duration;          // s
    std::unique_ptr<HydroCouple::SDK::Input1DDouble>    m_inflow;            // I, m³/s
    std::unique_ptr<HydroCouple::SDK::Output1DDouble>   m_outflow;           // Q, m³/s
    std::unique_ptr<HydroCouple::SDK::Output1DDouble>   m_storage;           // S, m³

    double m_time = 0.0; // s since prepare()
};
//! [class]

//! [info]
/*!
 * \brief What a host sees before it creates a LinearReservoir: its type id,
 *        caption and metadata, and the factory for new instances.
 */
class LinearReservoirInfo : public HydroCouple::SDK::AbstractModelComponentInfo
{
public:
    LinearReservoirInfo();
    std::unique_ptr<HydroCouple::IModelComponent> createComponentInstance() override;
};
//! [info]

#endif // DOCS_EXAMPLE_LINEARRESERVOIR_H
