// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// TorchComposition -- see torchcomposition.h.

#include "hydrocouplesdk/differentiation/torch/torchcomposition.h"
#include "hydrocouplesdk/differentiation/torch/atenmodelcomponent.h"

#include <span>
#include <string>

namespace HydroCouple::SDK::Differentiation::Torch {

using torch::autograd::AutogradContext;
using torch::autograd::variable_list;

// ============================================================================
// The autograd node
// ============================================================================

/*!
 * One node for the whole run. Its inputs are the composition (not a
 * tensor: no gradient) and the parameter tensors; its outputs are the
 * trajectories.
 */
struct CompositionFunction : public torch::autograd::Function<CompositionFunction>
{
    /*!
     * \brief Runs the composition and remembers which run built the graph.
     * \param[in,out] ctx        Autograd context; saves the composition
     *                           pointer and the run number.
     * \param[in]     self       The composition (not a tensor: no gradient).
     * \param[in]     parameters The parameter tensors.
     * \return The trajectories.
     */
    static variable_list forward(AutogradContext *ctx, TorchComposition *self,
                                 at::TensorList parameters)
    {
        variable_list trajectories = self->runForward(parameters);
        ctx->saved_data["self"] = reinterpret_cast<int64_t>(self);
        ctx->saved_data["generation"] = static_cast<int64_t>(self->m_generation);
        return trajectories;
    }

    /*!
     * \brief Hands the trajectories' gradients to the SDK reverse engine.
     * \param[in] ctx         Autograd context saved by forward().
     * \param[in] gradOutputs Gradients of the trajectories.
     * \return An undefined gradient for the composition pointer, then one
     *         per parameter.
     */
    static variable_list backward(AutogradContext *ctx, variable_list gradOutputs)
    {
        auto *self = reinterpret_cast<TorchComposition *>(ctx->saved_data["self"].toInt());
        const int generation = static_cast<int>(ctx->saved_data["generation"].toInt());
        variable_list grads = self->runBackward(generation, gradOutputs);
        variable_list result;
        result.reserve(grads.size() + 1);
        result.emplace_back();   // the composition pointer
        for (auto &g : grads)
            result.push_back(std::move(g));
        return result;
    }
};

// ============================================================================
// TorchComposition
// ============================================================================

HydroCouple::IComponentDataItem *TorchComposition::Binding::item() const
{
    return argument ? static_cast<HydroCouple::IComponentDataItem *>(argument) : state;
}

TorchComposition::TorchComposition(DifferentiableWorkflow &workflow, int steps)
    : m_workflow(workflow), m_steps(steps)
{
    TORCH_CHECK(steps > 0, "TorchComposition: steps must be positive");
}

int TorchComposition::bindArgument(const HydroCouple::IIdentity *owner,
                                   HydroCouple::IArgument *argument)
{
    TORCH_CHECK(owner && argument, "TorchComposition::bindArgument: null owner or argument");
    Binding b;
    b.owner = owner;
    b.argument = argument;
    m_bindings.push_back(b);
    return static_cast<int>(m_bindings.size()) - 1;
}

int TorchComposition::bindInitialState(HydroCouple::IModelComponent *component,
                                       HydroCouple::IComponentDataItem *state)
{
    TORCH_CHECK(component && state, "TorchComposition::bindInitialState: null component or state");
    Binding b;
    b.component = component;
    b.state = state;
    m_bindings.push_back(b);
    return static_cast<int>(m_bindings.size()) - 1;
}

int TorchComposition::observe(HydroCouple::IOutput *output)
{
    TORCH_CHECK(output, "TorchComposition::observe: null output");
    m_observed.push_back(output);
    return static_cast<int>(m_observed.size()) - 1;
}

std::vector<torch::Tensor>
TorchComposition::operator()(const std::vector<torch::Tensor> &parameters)
{
    TORCH_CHECK(!m_observed.empty(), "TorchComposition: nothing is observed");
    return CompositionFunction::apply(this, at::TensorList(parameters));
}

std::vector<torch::Tensor> TorchComposition::runForward(at::TensorList parameters)
{
    TORCH_CHECK(parameters.size() == m_bindings.size(), "TorchComposition: ",
                m_bindings.size(), " parameters are bound but ", parameters.size(),
                " tensors were given");
    std::string message;
    if (m_workflow.recordedSteps() > 0)
    {
        const bool rewound = m_workflow.rewind(message);
        TORCH_CHECK(rewound, "TorchComposition: rewind failed: ", message);
    }
    ++m_generation;

    for (size_t i = 0; i < m_bindings.size(); ++i)
    {
        const Binding &b = m_bindings[i];
        // Owners read their arguments at every update or refresh, so the new
        // value applies from step 1. No adapter is initialized again: that
        // would refresh it and everything below it, moving a stateful
        // adapter off the state the rewind just restored (G2.4).
        TORCH_CHECK(writeItem(b.item(), parameters[i], &message),
                    "TorchComposition: parameter ", i, ": ", message);
    }

    std::vector<std::vector<torch::Tensor>> perStep(m_observed.size());
    for (int n = 1; n <= m_steps; ++n)
    {
        m_workflow.update();
        TORCH_CHECK(m_workflow.recordedSteps() == n,
                    "TorchComposition: the composition failed at step ", n,
                    " (a component failed, finished early, or the workflow is not prepared)");
        for (size_t o = 0; o < m_observed.size(); ++o)
        {
            torch::Tensor values = readItem(m_observed[o], &message);
            TORCH_CHECK(values.defined(), "TorchComposition: cannot read \"",
                        m_observed[o]->id(), "\": ", message);
            perStep[o].push_back(std::move(values));
        }
    }

    std::vector<torch::Tensor> trajectories;
    trajectories.reserve(m_observed.size());
    for (auto &steps : perStep)
        trajectories.push_back(torch::stack(steps));
    return trajectories;
}

std::vector<torch::Tensor>
TorchComposition::runBackward(int generation, const std::vector<torch::Tensor> &gradOutputs)
{
    TORCH_CHECK(generation == m_generation,
                "TorchComposition: this graph is from run ", generation,
                " but the composition has since been run again (run ", m_generation,
                "); only the most recent run can be differentiated");
    TORCH_CHECK(gradOutputs.size() == m_observed.size(),
                "TorchComposition: expected ", m_observed.size(), " output gradients");

    std::vector<torch::Tensor> cotangents(gradOutputs.size());
    for (size_t o = 0; o < gradOutputs.size(); ++o)
        if (gradOutputs[o].defined())
            cotangents[o] = gradOutputs[o].detach().to(torch::kCPU, torch::kFloat64).contiguous();

    auto seeds = [&](int step, Seeds &s) {
        for (size_t o = 0; o < cotangents.size(); ++o)
        {
            if (!cotangents[o].defined())
                continue;
            torch::Tensor row = cotangents[o][step - 1].contiguous();
            const std::span<const double> values(row.data_ptr<double>(),
                                                 static_cast<size_t>(row.numel()));
            TORCH_CHECK(s.add(m_observed[o], values),
                        "TorchComposition: the gradient of \"", m_observed[o]->id(),
                        "\" does not match its shape");
        }
    };

    Gradients gradients;
    std::string message;
    TORCH_CHECK(m_workflow.backward(seeds, gradients, message),
                "TorchComposition: backward failed: ", message);

    std::vector<torch::Tensor> result;
    result.reserve(m_bindings.size());
    for (const Binding &b : m_bindings)
    {
        const std::vector<double> *values = nullptr;
        if (b.argument)
        {
            if (const ArgumentGradient *g = gradients.find(b.owner, b.argument))
                values = &g->values;
        }
        else if (const StateGradient *g = gradients.findState(b.component, b.state))
            values = &g->values;

        const std::vector<int64_t> shape = b.item()->shape();
        if (!values)
        {
            result.push_back(torch::zeros(shape, torch::kFloat64));
            continue;
        }
        result.push_back(torch::tensor(*values, torch::kFloat64).reshape(shape));
    }
    m_gradients = std::move(gradients);
    return result;
}

} // namespace HydroCouple::SDK::Differentiation::Torch
