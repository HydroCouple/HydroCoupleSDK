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
 * \threadsafety See AbstractWorkflowComponent.
 */

#ifndef HYDROCOUPLESDK_PULLDRIVENWORKFLOW_H
#define HYDROCOUPLESDK_PULLDRIVENWORKFLOW_H

#include "hydrocouplesdk/component/abstractworkflowcomponent.h"

#include <unordered_set>

namespace HydroCouple::SDK {

/*!
 * \brief Request-reply (pull) workflow driven from an explicit trigger input.
 */
class HYDROCOUPLESDK_EXPORT PullDrivenWorkflow
    : public AbstractWorkflowComponent
{
public:
    /*!
     * \brief Construct with an id and optional component-info pointer.
     */
    explicit PullDrivenWorkflow(
        std::string_view id,
        HydroCouple::IWorkflowComponentInfo *info = nullptr);

    ~PullDrivenWorkflow() override = default;

    /*!
     * \brief Declares the most-downstream trigger input that starts every
     *        request chain. Must be set before validate().
     */
    void setTrigger(HydroCouple::IInput *trigger);

    /*!
     * \brief The declared trigger input (nullptr until setTrigger()).
     */
    [[nodiscard]] HydroCouple::IInput *trigger() const noexcept;

protected:
    /*!
     * \brief Requires a trigger with a connected provider.
     */
    void validateComposition(std::vector<std::string> &problems) override;

    /*!
     * \brief Services one request-reply chain from the trigger.
     */
    bool performStep(std::string &message) override;

private:
    /*!
     * \brief Post-order DFS: updates every transitive provider component of
     *        \p component exactly once per step.
     * \return False when a serviced component fails.
     */
    bool serviceComponent(
        HydroCouple::IModelComponent *component,
        std::unordered_set<HydroCouple::IModelComponent *> &visited,
        std::string &message);

    HydroCouple::IInput *m_trigger = nullptr;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_PULLDRIVENWORKFLOW_H
