// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   pulldrivenworkflow.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  PullDrivenWorkflow — the OpenMI-style request-reply execution
 *         mode expressed as a workflow strategy.
 *
 * The default OpenMI/HydroCouple execution mode is a pull chain: a trigger
 * input at the most downstream point of the composition requests data, and
 * the request propagates upstream — each output asks its component to
 * update, which pulls that component's own inputs, and so on. Historically
 * the trigger was pure convention; this class makes it declared, inspectable
 * configuration and unifies the pull mode with the orchestrated one: the
 * request-reply chain becomes just one workflow implementation.
 *
 * One update() services one chain from the trigger: a post-order depth-first
 * walk over the provider graph updates every upstream component exactly once
 * (a visited set breaks feedback loops — time-lagged, matching OpenMI's
 * implicit behavior), each provider output is refreshed with
 * IOutput::updateValues(consumer), and finally the trigger input retrieves
 * and applies the values. The workflow is Done when every managed component
 * reports Done or Finished.
 *
 * Details of one step, from the code:
 * - A component already Done or Finished is not updated, and its own
 *   inputs are not serviced; its outputs are still refreshed with
 *   updateValues() for the consumer that asked.
 * - Before a component is updated, each of its inputs is serviced: every
 *   provider's component first, then provider->updateValues(input), then
 *   the input's retrieveValuesFromProvider() and applyData() (for SDK
 *   inputs). The component's update() is called with an empty list.
 * - The walk follows IOutput::modelComponent() and does not check that a
 *   provider's component is managed by the workflow: an unmanaged provider
 *   upstream of the trigger is updated too.
 * - The trigger's own component, if it has one, is not updated by the
 *   step; the trigger only retrieves. A trigger that belongs to no managed
 *   component (a probe input owned by the host) lets the workflow reach
 *   Done when the upstream components do.
 *
 * \threadsafety See AbstractWorkflowComponent.
 */

#ifndef HYDROCOUPLESDK_PULLDRIVENWORKFLOW_H
#define HYDROCOUPLESDK_PULLDRIVENWORKFLOW_H

#include "hydrocouplesdk/component/abstractworkflowcomponent.h"

#include <unordered_set>

namespace HydroCouple::SDK {

/*!
 * \brief Request-reply (pull) workflow driven from an explicit trigger input.
 * \details Typical use: add the components, initialize(), wire the links,
 *          setTrigger() with the most downstream input, then validate(),
 *          prepare(), and call update() until the status is Done.
 */
class HYDROCOUPLESDK_EXPORT PullDrivenWorkflow
    : public AbstractWorkflowComponent
{
public:
    /*!
     * \brief Construct with an id and optional component-info pointer.
     * \param[in] id    Workflow identifier; copied.
     * \param[in] info  Metadata object; borrowed, may be nullptr.
     */
    explicit PullDrivenWorkflow(
        std::string_view id,
        HydroCouple::IWorkflowComponentInfo *info = nullptr);

    //! Destroys the workflow (see AbstractWorkflowComponent::~AbstractWorkflowComponent()).
    ~PullDrivenWorkflow() override = default;

    /*!
     * \brief Declares the most-downstream trigger input that starts every
     *        request chain. Must be set before validate().
     * \details The input is borrowed and may belong to no managed component.
     *          It must stay valid, and non-null, while update() runs:
     *          performStep() does not check it again.
     * \param[in] trigger  The trigger input; nullptr clears the declaration
     *                     (validate() then reports it).
     */
    void setTrigger(HydroCouple::IInput *trigger);

    /*!
     * \brief The declared trigger input (nullptr until setTrigger()).
     * \return The borrowed trigger, or nullptr.
     */
    [[nodiscard]] HydroCouple::IInput *trigger() const noexcept;

protected:
    /*!
     * \brief Requires a trigger with a connected provider.
     * \details Appends "no trigger input declared (see setTrigger())" or
     *          "trigger input 'id' has no connected provider".
     * \param[in,out] problems  Problems found so far.
     */
    void validateComposition(std::vector<std::string> &problems) override;

    /*!
     * \brief Services one request-reply chain from the trigger.
     * \details For each provider of the trigger: services its component
     *          (serviceComponent()), then calls updateValues(trigger) on it;
     *          finally the trigger retrieves and applies the values when it
     *          is an AbstractInput.
     * \param[out] message  Set when a serviced component fails.
     * \return \c false when a serviced component ends its update() Failed.
     */
    bool performStep(std::string &message) override;

private:
    /*!
     * \brief Post-order DFS: updates every transitive provider component of
     *        \p component exactly once per step.
     * \details A component already in \p visited, or Done or Finished, is
     *          not updated. A feedback loop is cut at the first component
     *          visited twice, so that component reads its partners' values
     *          from the previous step.
     * \param[in]     component  Component to service; nullptr is ignored.
     * \param[in,out] visited    Components serviced in this step.
     * \param[out]    message    "component 'id' failed during update" on
     *                           failure.
     * \return False when a serviced component fails.
     */
    bool serviceComponent(
        HydroCouple::IModelComponent *component,
        std::unordered_set<HydroCouple::IModelComponent *> &visited,
        std::string &message);

    HydroCouple::IInput *m_trigger = nullptr; //!< Borrowed trigger input.
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_PULLDRIVENWORKFLOW_H
