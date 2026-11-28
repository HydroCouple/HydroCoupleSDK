// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   linearreservoir.cpp
 * \brief  Implementation of the chapter 3 linear reservoir and the two C
 *         entry points that publish it from a component library.
 */

#include "linearreservoir.h"

#include "hydrocouplesdk/componentabi.h"

#include <cmath>

using namespace HydroCouple;
using namespace HydroCouple::SDK;

LinearReservoir::LinearReservoir(std::string_view id, AbstractModelComponentInfo *info)
    : AbstractModelComponent(id, "Linear reservoir", info)
{
}

//! [arguments]
void LinearReservoir::createArguments()
{
    // k has units of 1/s: a Unit with time to the power -1. The library's
    // factories cover the common quantities (m³, m³/s, s).
    m_perSecond = std::make_unique<Unit>("per second", 1.0, 0.0);
    m_perSecond->setSymbol("1/s");
    m_perSecond->dimensionsInternal()->setPower(
        IUnitDimensions::FundamentalUnitDimension::Time, -1.0);
    m_rate    = std::make_unique<Quantity>("Recession constant", typeid(double),
                                           m_perSecond.get());
    m_volume  = std::unique_ptr<Quantity>(Quantity::volumeInCubicMeters());
    m_flow    = std::unique_ptr<Quantity>(Quantity::flowInCMS());
    m_seconds = std::unique_ptr<Quantity>(Quantity::timeInSeconds());

    m_recessionConstant = std::make_unique<Argument1DDouble>(
        "recession_constant", &m_scalar, 1, m_rate.get(), this);
    m_initialStorage = std::make_unique<Argument1DDouble>(
        "initial_storage", &m_scalar, 1, m_volume.get(), this);
    m_timeStep = std::make_unique<Argument1DDouble>(
        "time_step", &m_scalar, 1, m_seconds.get(), this);
    m_duration = std::make_unique<Argument1DDouble>(
        "duration", &m_scalar, 1, m_seconds.get(), this);

    // Defaults a host sees before it sets anything.
    (*m_recessionConstant)[0] = 1.0e-3;
    (*m_initialStorage)[0]    = 0.0;
    (*m_timeStep)[0]          = 600.0;
    (*m_duration)[0]          = 86400.0;

    addArgument(m_recessionConstant.get());
    addArgument(m_initialStorage.get());
    addArgument(m_timeStep.get());
    addArgument(m_duration.get());
}
//! [arguments]

//! [initialize_arguments]
bool LinearReservoir::initializeArguments(std::string &message)
{
    // The values are in place (set by the host before initialize()); check
    // them here, where a failure stops initialization with a message.
    const double k  = (*m_recessionConstant)[0];
    const double s0 = (*m_initialStorage)[0];
    const double dt = (*m_timeStep)[0];
    const double t  = (*m_duration)[0];
    if (!(k > 0.0))
    {
        message = "recession_constant must be positive (1/s), got " + std::to_string(k);
        return false;
    }
    if (!(s0 >= 0.0))
    {
        message = "initial_storage must not be negative (m3), got " + std::to_string(s0);
        return false;
    }
    if (!(dt > 0.0) || !(t >= dt))
    {
        message = "time_step must be positive and no longer than duration";
        return false;
    }
    message.clear();
    return true;
}
//! [initialize_arguments]

//! [ports]
void LinearReservoir::createInputs()
{
    // initialize() empties the base class's lists before calling this again,
    // so assigning a new item here also frees the one from a previous call.
    m_inflow = std::make_unique<Input1DDouble>("inflow", &m_scalar, 1, m_flow.get(), this);
    addInput(m_inflow.get());
}

void LinearReservoir::createOutputs()
{
    m_outflow = std::make_unique<Output1DDouble>("outflow", &m_scalar, 1, m_flow.get(), this);
    m_storage = std::make_unique<Output1DDouble>("storage", &m_scalar, 1, m_volume.get(), this);
    addOutput(m_outflow.get());
    addOutput(m_storage.get());
}

void LinearReservoir::initializeFailureCleanUp()
{
    // Nothing to release: the items and metadata are owned by members.
}
//! [ports]

//! [validate]
std::vector<std::string> LinearReservoir::validate()
{
    setStatus(ComponentStatus::Validating);
    // Every requirement this model has is already checked by
    // initializeArguments(); a model that needs, say, a connected inflow
    // would report that here.
    setStatus(ComponentStatus::Valid);
    return {};
}
//! [validate]

//! [prepare]
void LinearReservoir::prepare()
{
    setStatus(ComponentStatus::Preparing);
    m_time         = 0.0;
    (*m_storage)[0] = (*m_initialStorage)[0];
    (*m_outflow)[0] = (*m_recessionConstant)[0] * (*m_storage)[0];
    setStatus(ComponentStatus::Updated, "prepared");
}
//! [prepare]

//! [update]
void LinearReservoir::update(const std::vector<IOutput *> &)
{
    setStatus(ComponentStatus::Updating);

    // Take the providers' values (a no-op for an unconnected input, which
    // keeps whatever value the host wrote into it).
    applyInputValues();

    const double k  = (*m_recessionConstant)[0];
    const double dt = (*m_timeStep)[0];
    const double inflow = (*m_inflow)[0];
    const double equilibrium = inflow / k;               // m³
    const double storage = equilibrium +
        ((*m_storage)[0] - equilibrium) * std::exp(-k * dt); // exact for constant I

    (*m_storage)[0] = storage;
    (*m_outflow)[0] = k * storage;
    m_time += dt;

    setStatus(m_time + 0.5 * dt >= (*m_duration)[0] ? ComponentStatus::Done
                                                     : ComponentStatus::Updated,
              "advanced to t = " + std::to_string(m_time) + " s");
}
//! [update]

//! [finish]
void LinearReservoir::finish()
{
    setStatus(ComponentStatus::Finishing);
    setStatus(ComponentStatus::Finished);
}
//! [finish]

//! [info_impl]
LinearReservoirInfo::LinearReservoirInfo()
    : AbstractModelComponentInfo("docs.examples.linear_reservoir")
{
    setCaption("Linear reservoir");
    setDescription("A single linear reservoir: dS/dt = I - k S, Q = k S.");
    setDeveloper("HydroCouple SDK developer guide");
    setVersion("1.0.0");
}

std::unique_ptr<IModelComponent> LinearReservoirInfo::createComponentInstance()
{
    return std::make_unique<LinearReservoir>("reservoir", this);
}
//! [info_impl]

//! [declare]
// The two C entry points a host looks up after loading the library:
// hydrocouple_component_abi_v1() and hydrocouple_component_info_v1().
HYDROCOUPLE_DECLARE_COMPONENT(LinearReservoirInfo)
//! [declare]
