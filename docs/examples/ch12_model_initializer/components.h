// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Two small components for the chapter 12 initializer example. They exist
// only to receive arguments and connections from a composition document:
//
//   Producer  argument "scale" (any length), output "series" (3 cells).
//             Each update() sets series[i] = scale[i] * step; Done after
//             three steps.
//   Consumer  arguments "initial" and "weights", input "inflow" (3 cells).

#ifndef DOCS_EXAMPLE_CH12_COMPONENTS_H
#define DOCS_EXAMPLE_CH12_COMPONENTS_H

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"

#include <memory>
#include <string>
#include <vector>

namespace example {

using HydroCouple::IModelComponent;
using Status = IModelComponent::ComponentStatus;
using namespace HydroCouple::SDK;

constexpr int kCells = 3;
constexpr int kSteps = 3;

class Producer : public AbstractModelComponent
{
public:
    explicit Producer(std::string_view id) : AbstractModelComponent(id, "Producer") {}

    std::vector<std::string> validate() override
    {
        setStatus(Status::Validating);
        setStatus(Status::Valid);
        return {};
    }
    void prepare() override
    {
        setStatus(Status::Preparing);
        m_step = 0;
        setStatus(Status::Updated);
    }
    void update(const std::vector<HydroCouple::IOutput *> & = {}) override
    {
        setStatus(Status::Updating);
        ++m_step;
        for (int cell = 0; cell < kCells; ++cell)
            (*series)[cell] = (cell < scale->length() ? (*scale)[cell] : 0.0) * m_step;
        setStatus(m_step >= kSteps ? Status::Done : Status::Updated);
    }
    void finish() override
    {
        setStatus(Status::Finishing);
        setStatus(Status::Finished);
    }

    Argument1DDouble *scale = nullptr;
    Output1DDouble   *series = nullptr;

protected:
    void createArguments() override
    {
        m_scale = std::make_unique<Argument1DDouble>("scale", &m_cells, 0, m_quantity.get(), this);
        scale = m_scale.get();
        addArgument(scale);
    }
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override {}
    void createOutputs() override
    {
        m_series = std::make_unique<Output1DDouble>("series", &m_cells, kCells, m_quantity.get(), this);
        series = m_series.get();
        addOutput(series);
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension                         m_cells{"cell", "Cell"};
    std::unique_ptr<Quantity>         m_quantity{Quantity::unitLess("Level")};
    std::unique_ptr<Argument1DDouble> m_scale;
    std::unique_ptr<Output1DDouble>   m_series;
    int                               m_step = 0;
};

class Consumer : public AbstractModelComponent
{
public:
    explicit Consumer(std::string_view id) : AbstractModelComponent(id, "Consumer") {}

    std::vector<std::string> validate() override { return {}; }
    void prepare() override {}
    void update(const std::vector<HydroCouple::IOutput *> & = {}) override {}
    void finish() override {}

    Argument1DDouble *initial = nullptr;
    Argument1DDouble *weights = nullptr;
    Input1DDouble    *inflow  = nullptr;

protected:
    void createArguments() override
    {
        m_initial = std::make_unique<Argument1DDouble>("initial", &m_cells, 0, m_quantity.get(), this);
        m_weights = std::make_unique<Argument1DDouble>("weights", &m_cells, 0, m_quantity.get(), this);
        initial = m_initial.get();
        weights = m_weights.get();
        addArgument(initial);
        addArgument(weights);
    }
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        m_inflow = std::make_unique<Input1DDouble>("inflow", &m_cells, kCells, m_quantity.get(), this);
        inflow = m_inflow.get();
        addInput(inflow);
    }
    void createOutputs() override {}
    void initializeFailureCleanUp() override {}

private:
    Dimension                         m_cells{"cell", "Cell"};
    std::unique_ptr<Quantity>         m_quantity{Quantity::unitLess("Level")};
    std::unique_ptr<Argument1DDouble> m_initial;
    std::unique_ptr<Argument1DDouble> m_weights;
    std::unique_ptr<Input1DDouble>    m_inflow;
};

} // namespace example

#endif
