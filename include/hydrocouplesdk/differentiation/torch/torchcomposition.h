// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
/*!
 * \file   torchcomposition.h
 * \brief  A whole recorded composition run as one differentiable LibTorch
 *         operation (Phase G2.3).
 *
 * TorchComposition lets C++ code that already lives in LibTorch -- a
 * network whose output is a model parameter, a loss over simulated
 * trajectories, a torch::optim optimizer -- treat an SDK composition as a
 * function of tensors:
 *
 *     trajectories = composition({k, m, S0, ...});   // forward: run N steps
 *     loss(trajectories).backward();                  // reverse: the SDK engine
 *
 * Forward rewinds the workflow to the start of its last run (the first call
 * needs a freshly prepared workflow instead), writes every bound parameter
 * tensor into its data item, runs N steps, and returns each observed
 * output's values at every step, stacked as [N, item shape...]. Backward
 * hands the incoming gradients to DifferentiableWorkflow::backward() as
 * per-step seeds and returns the argument and initial-state gradients it
 * reports.
 *
 * The SDK's reverse engine stays the tape; Torch sees the composition as a
 * single node. So the components in it keep whatever differentiation they
 * already have -- hand-written vjp()s, SDK adapter adjoints, and
 * AtenModelComponent's autograd -- and checkpointing stays the workflow's
 * policy (EveryStep or Logarithmic).
 *
 * Rules the caller must keep:
 *  - A bound argument must be read by its owner at every update (not
 *    latched at initialize()); an adapted output's arguments are, so after
 *    writing them TorchComposition calls the adapter's initialize() again.
 *  - A bound initial state is written after the rewind, before step 1;
 *    outputs computed from the state before step 1 are not recomputed.
 *  - Each forward call rewinds the workflow, so only the graph of the most
 *    recent call can be differentiated; backward() through an older one is
 *    refused.
 *  - Float64, host memory. Parameters are converted (dtype, device, layout)
 *    on the way in; gradients come back as Float64 CPU tensors.
 */
#ifndef HYDROCOUPLESDK_TORCHCOMPOSITION_H
#define HYDROCOUPLESDK_TORCHCOMPOSITION_H

#include "hydrocouplesdk/differentiation/differentiableworkflow.h"
#include "hydrocouplesdk/differentiation/torch/torchexport.h"
#include "hydrocouple.h"

#include <torch/torch.h>

#include <vector>

namespace HydroCouple::SDK::Differentiation::Torch {

class HYDROCOUPLESDKTORCH_EXPORT TorchComposition
{
public:
    //! \p workflow must outlive this object and every graph it builds.
    TorchComposition(DifferentiableWorkflow &workflow, int steps);

    /*!
     * \brief Makes \p argument of \p owner (a model component or an adapted
     *        output) a parameter. \returns its position among the tensors
     *        operator() takes.
     */
    int bindArgument(const HydroCouple::IIdentity *owner, HydroCouple::IArgument *argument);

    //! Makes the value of \p state before step 1 a parameter.
    int bindInitialState(HydroCouple::IModelComponent *component,
                         HydroCouple::IComponentDataItem *state);

    //! Returns \p output's trajectory. \returns its position among the
    //! tensors operator() returns.
    int observe(HydroCouple::IOutput *output);

    /*!
     * \brief Runs the composition. Differentiable with respect to every
     *        parameter. Throws c10::Error (TORCH_CHECK) on any failure.
     */
    std::vector<torch::Tensor> operator()(const std::vector<torch::Tensor> &parameters);

    //! Forward runs so far.
    [[nodiscard]] int runs() const noexcept { return m_generation; }

    //! The gradients of the most recent backward, as the engine reported them.
    [[nodiscard]] const Gradients &lastGradients() const noexcept { return m_gradients; }

private:
    friend struct CompositionFunction;

    struct Binding
    {
        const HydroCouple::IIdentity *owner = nullptr;           // arguments
        HydroCouple::IArgument *argument = nullptr;
        HydroCouple::IModelComponent *component = nullptr;       // initial states
        HydroCouple::IComponentDataItem *state = nullptr;
        [[nodiscard]] HydroCouple::IComponentDataItem *item() const;
    };

    std::vector<torch::Tensor> runForward(at::TensorList parameters);
    std::vector<torch::Tensor> runBackward(int generation,
                                           const std::vector<torch::Tensor> &gradOutputs);

    DifferentiableWorkflow &m_workflow;
    int m_steps;
    std::vector<Binding> m_bindings;
    std::vector<HydroCouple::IOutput *> m_observed;
    int m_generation = 0;
    Gradients m_gradients;
};

} // namespace HydroCouple::SDK::Differentiation::Torch

#endif // HYDROCOUPLESDK_TORCHCOMPOSITION_H
