// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
/*!
 * \file   atenmodelcomponent.h
 * \brief  A model component whose step is written in ATen ops and is
 *         differentiable by construction (Phase G2.3, the LibTorch layer).
 *
 * Built only with HYDROCOUPLESDK_WITH_TORCH, into the separate
 * HydroCoupleSDKTorch library: the core SDK never sees a Torch header.
 *
 * A subclass writes its physics once, as a function of tensors:
 *
 *     step(states, inputs, arguments) -> { states', outputs }
 *
 * in the orders of differentiableStates(), differentiableInputs(),
 * differentiableArguments() and differentiableOutputs(). The base does
 * everything else the interface asks of a differentiable, checkpointable
 * component:
 *
 *  - update() pulls the inputs, gathers every differentiable item into a
 *    Float64 tensor through the data plane (getValuesInto), runs step()
 *    without recording a graph, and writes the new states and outputs back
 *    (setValuesFrom). It keeps the tensors it stepped from: the
 *    linearization point of the most recent step.
 *  - vjp() re-runs step() from that point with autograd on and returns
 *    torch::autograd::grad of the seeded outputs and states -- no adjoint
 *    is written by hand. jvp() is the transpose of that, by the
 *    double-backward identity (the vjp is linear in its seed, so
 *    differentiating it with respect to the seed gives the jvp).
 *  - saveState()/restoreState() serialize the step count, every state item
 *    and every Float64 output into the token itself, so releaseState() has
 *    nothing to reclaim.
 *
 * Items must be Float64 and live in host memory (G2.3 is CPU only). Things
 * the step depends on that are not tensors -- a boundary time series, say
 * -- are the subclass's own business; stepIndex() tells it which step is
 * being taken, and the default checkpoint restores that count, so a
 * forcing indexed by it replays exactly.
 */
#ifndef HYDROCOUPLESDK_ATENMODELCOMPONENT_H
#define HYDROCOUPLESDK_ATENMODELCOMPONENT_H

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/differentiation/torch/torchexport.h"
#include "hydrocouple.h"

#include <torch/torch.h>

#include <set>
#include <string>
#include <vector>

namespace HydroCouple::SDK::Differentiation::Torch {

//! A whole data item as a new Float64 CPU tensor of the item's shape.
HYDROCOUPLESDKTORCH_EXPORT torch::Tensor
readItem(const HydroCouple::IComponentDataItem *item, std::string *message = nullptr);

//! Writes \p values (any dtype/layout; converted to contiguous Float64 CPU)
//! into the whole of \p item. False with a message on a shape mismatch.
HYDROCOUPLESDKTORCH_EXPORT bool
writeItem(HydroCouple::IComponentDataItem *item, const torch::Tensor &values,
          std::string *message = nullptr);

class HYDROCOUPLESDKTORCH_EXPORT AtenModelComponent
    : public AbstractModelComponent,
      public virtual HydroCouple::IDifferentiableModelComponent,
      public virtual HydroCouple::ICheckpointableModelComponent
{
public:
    //! What one step produces, in differentiableStates() and
    //! differentiableOutputs() order.
    struct StepResult
    {
        std::vector<torch::Tensor> states;
        std::vector<torch::Tensor> outputs;
    };

    explicit AtenModelComponent(std::string_view id);
    ~AtenModelComponent() override = default;

    // -- IModelComponent -----------------------------------------------------
    //! Valid by default; override to check configuration.
    std::vector<std::string> validate() override;
    //! Updated by default; override to compute initial outputs.
    void prepare() override;
    //! Gathers, steps, writes back; see the file comment. Final: the physics
    //! belongs in step().
    void update(const std::vector<HydroCouple::IOutput *> &requiredOutputs = {}) final;
    void finish() override;
    [[nodiscard]] std::set<HydroCouple::Capability> capabilities() const override;
    //! The prognostic state: differentiableStates() (which the interface
    //! requires to be a subset). Override to add state step() does not see.
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> states() const override;

    // -- ICheckpointableModelComponent ---------------------------------------
    bool saveState(std::string &token, std::string &message) override;
    bool restoreState(const std::string &token, std::string &message) override;
    //! The token is the state: nothing to reclaim.
    bool releaseState(const std::string &token, std::string &message) override;

    // -- IDifferentiableModelComponent ---------------------------------------
    bool vjp(HydroCouple::DifferentialSet seeds, HydroCouple::DifferentialSet results,
             std::string *message = nullptr) override;
    bool jvp(HydroCouple::DifferentialSet seeds, HydroCouple::DifferentialSet results,
             std::string *message = nullptr) override;

    //! Steps completed since prepare() (restored by restoreState()).
    [[nodiscard]] int completedSteps() const noexcept;

protected:
    /*!
     * \brief The physics of one step, in ATen ops.
     * \details Must be a pure function of its arguments (and of stepIndex()):
     *          vjp() and jvp() call it again at the stored linearization
     *          point and differentiate what it returns.
     */
    virtual StepResult step(const std::vector<torch::Tensor> &states,
                            const std::vector<torch::Tensor> &inputs,
                            const std::vector<torch::Tensor> &arguments) = 0;

    //! Called after update() has written a step back -- for outputs the
    //! component does not differentiate. Values are detached.
    virtual void afterStep(const StepResult &result);

    //! Whether the component is Done after the step just taken.
    [[nodiscard]] virtual bool doneAfterStep() const;

    //! The 1-based index of the step being taken (inside step()), or of the
    //! step last taken (elsewhere).
    [[nodiscard]] int stepIndex() const noexcept;

private:
    struct Point
    {
        std::vector<torch::Tensor> states, inputs, arguments;
        int step = 0;
    };

    bool gather(Point &point, std::string &message) const;
    bool checkResult(const StepResult &result, std::string &message) const;
    //! step() at the linearization point with fresh leaves requiring grad.
    bool relinearize(std::vector<torch::Tensor> &leaves, StepResult &result,
                     std::string &message);

    Point m_point;
    bool m_haveStep = false;
    int m_steps = 0;
    int m_stepIndex = 0;
};

} // namespace HydroCouple::SDK::Differentiation::Torch

#endif // HYDROCOUPLESDK_ATENMODELCOMPONENT_H
