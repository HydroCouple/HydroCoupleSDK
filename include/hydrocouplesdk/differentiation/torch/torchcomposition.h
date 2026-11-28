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
 *  - A bound argument must be read by its owner at every update or refresh,
 *    not latched at initialize() -- the SDK's components and adapters all
 *    do. TorchComposition never initializes an adapter again: that would
 *    refresh it, and every adapter below it, and so move a stateful one
 *    (the relaxation, say) off the state the rewind restored.
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

/*!
 * \brief A recorded composition run exposed to LibTorch as one
 *        differentiable function of parameter tensors.
 *
 * Bind the parameters and observed outputs once, then call operator() as
 * often as needed (a training loop). Part of the optional
 * HydroCoupleSDKTorch library (HYDROCOUPLESDK_WITH_TORCH=ON).
 *
 * \threadsafety Not thread-safe; it drives the workflow it wraps.
 */
class HYDROCOUPLESDKTORCH_EXPORT TorchComposition
{
public:
    //! \p workflow must outlive this object and every graph it builds.
    //! \param[in] workflow A prepared workflow (borrowed).
    //! \param[in] steps    Steps each forward call runs (> 0; TORCH_CHECK
    //!                     throws c10::Error otherwise).
    TorchComposition(DifferentiableWorkflow &workflow, int steps);

    /*!
     * \brief Makes \p argument of \p owner (a model component or an adapted
     *        output) a parameter. \returns its position among the tensors
     *        operator() takes.
     * \param[in] owner    The component or adapted output the argument
     *                     belongs to (not null); the gradient is looked up
     *                     with this owner.
     * \param[in] argument The argument (not null; Float64).
     * \exception c10::Error when either pointer is null.
     */
    int bindArgument(const HydroCouple::IIdentity *owner, HydroCouple::IArgument *argument);

    //! Makes the value of \p state before step 1 a parameter.
    //! \param[in] component The component that carries the state (not null).
    //! \param[in] state     One of its differentiableStates() (not null).
    //! \return Its position among the tensors operator() takes.
    //! \exception c10::Error when either pointer is null.
    int bindInitialState(HydroCouple::IModelComponent *component,
                         HydroCouple::IComponentDataItem *state);

    //! Returns \p output's trajectory. \returns its position among the
    //! tensors operator() returns.
    //! \param[in] output The output to record at every step (not null;
    //!                   Float64).
    //! \exception c10::Error when \p output is null.
    int observe(HydroCouple::IOutput *output);

    /*!
     * \brief Runs the composition. Differentiable with respect to every
     *        parameter. Throws c10::Error (TORCH_CHECK) on any failure.
     *
     * Rewinds the workflow when it has recorded steps, writes each
     * parameter into its item, runs steps() updates and reads each observed
     * output after every one. Calling backward() on a loss of the results
     * runs DifferentiableWorkflow::backward() with the incoming gradients
     * as per-step seeds; a parameter no cotangent reached gets zeros.
     *
     * \param[in] parameters One tensor per binding, in binding order, each
     *                       of its item's shape (any dtype or device;
     *                       converted to Float64 CPU).
     * \return One tensor per observed output, in observe() order, of shape
     *         [steps, output shape...].
     * \exception c10::Error "nothing is observed", a parameter count
     *            mismatch, "rewind failed: ...", "parameter i: ...", "the
     *            composition failed at step n (...)", "cannot read ...";
     *            during backward also "this graph is from run g but the
     *            composition has since been run again", "backward failed:
     *            ...".
     */
    std::vector<torch::Tensor> operator()(const std::vector<torch::Tensor> &parameters);

    //! Forward runs so far.
    //! \return The number of operator() calls that started a run.
    [[nodiscard]] int runs() const noexcept { return m_generation; }

    //! The gradients of the most recent backward, as the engine reported them.
    //! \return The Gradients of the last successful backward (empty before).
    [[nodiscard]] const Gradients &lastGradients() const noexcept { return m_gradients; }

private:
    friend struct CompositionFunction;

    /*!
     * \brief One parameter: an argument, or an initial state.
     */
    struct Binding
    {
        const HydroCouple::IIdentity *owner = nullptr;           //!< Owner of the bound argument (arguments only).
        HydroCouple::IArgument *argument = nullptr;              //!< The bound argument, or null for a state.
        HydroCouple::IModelComponent *component = nullptr;       //!< Component carrying the bound state (initial states only).
        HydroCouple::IComponentDataItem *state = nullptr;        //!< The bound state, or null for an argument.
        //! The item the parameter is written into.
        //! \return \c argument when set, otherwise \c state.
        [[nodiscard]] HydroCouple::IComponentDataItem *item() const;
    };

    /*!
     * \brief The forward half of the autograd node (see operator()).
     * \param[in] parameters One tensor per binding.
     * \return The stacked trajectories.
     */
    std::vector<torch::Tensor> runForward(at::TensorList parameters);
    /*!
     * \brief The backward half of the autograd node.
     * \param[in] generation  The run the graph was built by; must be the
     *                        latest.
     * \param[in] gradOutputs Gradients of the trajectories (undefined: none).
     * \return One gradient tensor per binding.
     */
    std::vector<torch::Tensor> runBackward(int generation,
                                           const std::vector<torch::Tensor> &gradOutputs);

    DifferentiableWorkflow &m_workflow;           //!< The wrapped workflow (borrowed).
    int m_steps;                                  //!< Steps per forward run.
    std::vector<Binding> m_bindings;              //!< Parameters, in binding order.
    std::vector<HydroCouple::IOutput *> m_observed; //!< Observed outputs, in observe() order.
    int m_generation = 0;                         //!< Forward runs so far.
    Gradients m_gradients;                        //!< Gradients of the last backward.
};

} // namespace HydroCouple::SDK::Differentiation::Torch

#endif // HYDROCOUPLESDK_TORCHCOMPOSITION_H
