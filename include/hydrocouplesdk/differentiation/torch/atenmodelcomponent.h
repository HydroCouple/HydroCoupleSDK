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

/*!
 * \brief A whole data item as a new Float64 CPU tensor of the item's shape.
 * \param[in]  item    Item to read (Float64; host-readable through
 *                     getValuesInto()).
 * \param[out] message Optional: "null data item", "data item \"ID\" is not
 *                     Float64", or the item's getValuesInto() message.
 * \return The tensor (owned by the caller), or an undefined tensor on
 *         failure.
 */
HYDROCOUPLESDKTORCH_EXPORT torch::Tensor
readItem(const HydroCouple::IComponentDataItem *item, std::string *message = nullptr);

/*!
 * \brief Writes \p values (any dtype/layout; converted to contiguous Float64
 *        CPU) into the whole of \p item. False with a message on a shape
 *        mismatch.
 * \param[in]  item    Item to write (Float64; accepts setValuesFrom()).
 * \param[in]  values  Tensor whose sizes equal item->shape(); detached first,
 *                     so no graph is recorded.
 * \param[out] message Optional: "null data item", "data item \"ID\" is not
 *                     Float64", "a tensor of shape [..] cannot be written to
 *                     data item \"ID\" of shape [..]", or the item's
 *                     setValuesFrom() message.
 * \return True when the item holds the values.
 */
HYDROCOUPLESDKTORCH_EXPORT bool
writeItem(HydroCouple::IComponentDataItem *item, const torch::Tensor &values,
          std::string *message = nullptr);

/*!
 * \brief A differentiable, checkpointable model component whose step is a
 *        function of tensors written in ATen ops.
 *
 * Subclass it, create the items in the AbstractModelComponent hooks, list
 * them in differentiableStates(), differentiableInputs(),
 * differentiableArguments() and differentiableOutputs(), and implement
 * step(). The base implements update(), vjp(), jvp() and the checkpoint
 * methods as the file comment describes, and advertises
 * Capability::Checkpointing and Capability::Differentiable. It is part of
 * the optional HydroCoupleSDKTorch library (HYDROCOUPLESDK_WITH_TORCH=ON).
 *
 * \threadsafety Drive from one thread, like any model component; vjp() and
 * jvp() use torch autograd on the calling thread.
 */
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
        std::vector<torch::Tensor> states;  //!< New state per differentiableStates() entry, each of the item's shape.
        std::vector<torch::Tensor> outputs; //!< Value per differentiableOutputs() entry, each of the item's shape.
    };

    /*!
     * \brief Creates the component in the Created status.
     * \param[in] id Component identifier.
     */
    explicit AtenModelComponent(std::string_view id);
    //! Destroys the component and the tensors of its linearization point.
    ~AtenModelComponent() override = default;

    // -- IModelComponent -----------------------------------------------------
    //! Valid by default; override to check configuration.
    //! \return An empty list (Validating, then Valid).
    std::vector<std::string> validate() override;
    //! Updated by default; override to compute initial outputs.
    //! Resets the step count and forgets the linearization point.
    void prepare() override;
    /*!
     * \brief Gathers, steps, writes back; see the file comment. Final: the
     *        physics belongs in step().
     *
     * Fails the component (Failed, with the message) when an item cannot be
     * read or written, step() throws ("step() threw: ..."), or step()
     * returns the wrong number or shape of tensors. Otherwise the status is
     * Done when doneAfterStep() is true, else Updated.
     *
     * \param[in] requiredOutputs Ignored: every output step() returns is
     *                            written.
     */
    void update(const std::vector<HydroCouple::IOutput *> &requiredOutputs = {}) final;
    //! Forgets the linearization point; Finishing, then Finished.
    void finish() override;
    /*!
     * \brief The base capabilities plus Checkpointing and Differentiable.
     * \return The capability set.
     */
    [[nodiscard]] std::set<HydroCouple::Capability> capabilities() const override;
    //! The prognostic state: differentiableStates() (which the interface
    //! requires to be a subset). Override to add state step() does not see.
    //! \return differentiableStates().
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> states() const override;

    // -- ICheckpointableModelComponent ---------------------------------------
    /*!
     * \brief Serializes the step count, every differentiable state and every
     *        Float64 output into \p token, as text ("aten1" followed by
     *        numbers printed with 17 significant digits).
     * \param[out] token   The checkpoint; it is the state itself.
     * \param[out] message The readItem() failure, if any.
     * \return False when an item cannot be read.
     */
    bool saveState(std::string &token, std::string &message) override;
    /*!
     * \brief Parses \p token and writes the states and Float64 outputs back.
     *
     * Parses everything before writing anything. On success restores the
     * step count, forgets the linearization point (vjp() and jvp() then
     * fail until the next update()), and sets the status to Updated.
     *
     * \param[in]  token   A token from saveState() of a component with the
     *                     same items and shapes.
     * \param[out] message "not a checkpoint of an AtenModelComponent", "bad
     *                     checkpoint token", "bad checkpoint token for
     *                     \"ID\"", or a writeItem() failure; cleared on
     *                     success.
     * \return True when restored.
     */
    bool restoreState(const std::string &token, std::string &message) override;
    //! The token is the state: nothing to reclaim.
    //! \param[in]  token   Ignored.
    //! \param[out] message Cleared.
    //! \return Always true.
    bool releaseState(const std::string &token, std::string &message) override;

    // -- IDifferentiableModelComponent ---------------------------------------
    /*!
     * \brief Vector-Jacobian product of the most recent step, by
     *        torch::autograd::grad.
     *
     * Re-runs step() at the stored linearization point with fresh leaves
     * that require grad, then takes the gradient of the seeded results. A
     * result no seed reaches is written as zeros.
     *
     * \param[in]  seeds   Cotangents of Output and StateAfter entries.
     * \param[in]  results Buffers to overwrite with cotangents of Input,
     *                     Argument and StateBefore entries.
     * \param[out] message Optional: "no step to differentiate: component
     *                     \"ID\" has not updated since it was prepared or
     *                     restored", "vjp(): \"ID\" is not a differentiable
     *                     output or state of \"ID\"" (or input, argument or
     *                     state for a result), or an autograd failure.
     * \return True on success.
     */
    bool vjp(HydroCouple::DifferentialSet seeds, HydroCouple::DifferentialSet results,
             std::string *message = nullptr) override;
    /*!
     * \brief Jacobian-vector product of the most recent step, by double
     *        backward through the vjp.
     *
     * With \f$ g(w) = J^\top w \f$ linear in \f$ w \f$,
     * \f$ \partial \langle g(w), t \rangle / \partial w = J t \f$: the base
     * differentiates the vjp with respect to a zero dummy seed. Several seeds
     * on one leaf are summed.
     *
     * \param[in]  seeds   Tangents of Input, Argument and StateBefore
     *                     entries.
     * \param[in]  results Buffers to overwrite with tangents of Output and
     *                     StateAfter entries.
     * \param[out] message Optional; messages as for vjp() with "jvp(): ".
     * \return True on success.
     */
    bool jvp(HydroCouple::DifferentialSet seeds, HydroCouple::DifferentialSet results,
             std::string *message = nullptr) override;

    //! Steps completed since prepare() (restored by restoreState()).
    //! \return The step count.
    [[nodiscard]] int completedSteps() const noexcept;

protected:
    /*!
     * \brief The physics of one step, in ATen ops.
     * \details Must be a pure function of its arguments (and of stepIndex()):
     *          vjp() and jvp() call it again at the stored linearization
     *          point and differentiate what it returns.
     * \param[in] states    One Float64 CPU tensor per differentiableStates()
     *                      entry, the state before the step.
     * \param[in] inputs    One tensor per differentiableInputs() entry.
     * \param[in] arguments One tensor per differentiableArguments() entry.
     * \return The new states and the outputs, one tensor per item, each of
     *         the item's shape. May throw; update() reports the exception.
     */
    virtual StepResult step(const std::vector<torch::Tensor> &states,
                            const std::vector<torch::Tensor> &inputs,
                            const std::vector<torch::Tensor> &arguments) = 0;

    //! Called after update() has written a step back -- for outputs the
    //! component does not differentiate. Values are detached.
    //! \param[in] result What step() returned (computed without a graph).
    virtual void afterStep(const StepResult &result);

    //! Whether the component is Done after the step just taken.
    //! \return False by default; override to stop after a step count.
    [[nodiscard]] virtual bool doneAfterStep() const;

    //! The 1-based index of the step being taken (inside step()), or of the
    //! step last taken (elsewhere).
    //! \return The index; restoreState() sets it to the restored step count.
    [[nodiscard]] int stepIndex() const noexcept;

private:
    /*!
     * \brief A step's linearization point: the tensors it stepped from.
     */
    struct Point
    {
        std::vector<torch::Tensor> states, inputs, arguments; //!< Argument values the step read, in differentiableArguments() order.
        int step = 0; //!< The step's 1-based index.
    };
    /*!
     * \var HydroCouple::SDK::Differentiation::Torch::AtenModelComponent::Point::states
     * \brief State values before the step, in differentiableStates() order.
     *
     * \var HydroCouple::SDK::Differentiation::Torch::AtenModelComponent::Point::inputs
     * \brief Input values the step read, in differentiableInputs() order.
     */

    /*!
     * \brief Reads every differentiable state, input and argument into
     *        \p point.
     * \param[out] point   Receives the tensors.
     * \param[out] message readItem()'s failure.
     * \return True when every item was read.
     */
    bool gather(Point &point, std::string &message) const;
    /*!
     * \brief Checks that \p result has one defined tensor of the right shape
     *        per state and output.
     * \param[in]  result  What step() returned.
     * \param[out] message "step() returned N states; differentiableStates()
     *                     lists M" (or outputs), "step() returned no tensor
     *                     for \"ID\"", or "step() returned shape [..] for
     *                     \"ID\" of shape [..]".
     * \return True when the result matches.
     */
    bool checkResult(const StepResult &result, std::string &message) const;
    //! step() at the linearization point with fresh leaves requiring grad.
    //! \param[out] leaves  The leaf tensors (states, inputs, arguments order).
    //! \param[out] result  What step() returned, with a graph to the leaves.
    //! \param[out] message Why no step can be differentiated.
    //! \return False when no step is held, step() throws, or the result
    //!         fails checkResult().
    bool relinearize(std::vector<torch::Tensor> &leaves, StepResult &result,
                     std::string &message);

    Point m_point;           //!< Linearization point of the most recent update().
    bool m_haveStep = false; //!< \c m_point holds a step (false after prepare(), restoreState() and finish()).
    int m_steps = 0;         //!< Steps completed since prepare().
    int m_stepIndex = 0;     //!< Value stepIndex() returns.
};

} // namespace HydroCouple::SDK::Differentiation::Torch

#endif // HYDROCOUPLESDK_ATENMODELCOMPONENT_H
