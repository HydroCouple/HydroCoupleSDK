// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timesteppedworkflow.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  TimeSteppedWorkflow — orchestrated execution over a schedule
 *         derived from the exchange-connection graph.
 *
 * Where PullDrivenWorkflow reacts to a downstream trigger, this strategy
 * drives the composition forward explicitly. At prepare() it builds an
 * execution schedule from the connection graph: components are nodes, every
 * connected provider→consumer link is an edge, strongly connected components
 * (feedback loops) are collapsed with Tarjan's algorithm, and the resulting
 * DAG is ordered topologically. Feedback loops thereby become a diagnosed,
 * configured thing rather than silent time-lagging: each cyclic group is
 * updated iterationsPerGroup() times per step (1 = time-lagged, matching
 * OpenMI's implicit behavior; >1 = fixed-point sweeps).
 *
 * One update() advances every non-Done component once through the schedule —
 * the workflow's synchronization point. Stop/pause requests and Done
 * detection are honored between steps (see AbstractWorkflowComponent).
 *
 * \threadsafety See AbstractWorkflowComponent.
 */

#ifndef HYDROCOUPLESDK_TIMESTEPPEDWORKFLOW_H
#define HYDROCOUPLESDK_TIMESTEPPEDWORKFLOW_H

#include "hydrocouplesdk/component/abstractworkflowcomponent.h"

namespace HydroCouple::SDK {

/*!
 * \brief Schedule-driven orchestration workflow.
 */
class HYDROCOUPLESDK_EXPORT TimeSteppedWorkflow
    : public AbstractWorkflowComponent
{
public:
    /*!
     * \brief Construct with an id and optional component-info pointer.
     */
    explicit TimeSteppedWorkflow(
        std::string_view id,
        HydroCouple::IWorkflowComponentInfo *info = nullptr);

    ~TimeSteppedWorkflow() override = default;

    /*!
     * \brief Update sweeps per cyclic group each step (≥ 1; default 1 =
     *        time-lagged). Groups of one component always update once.
     */
    [[nodiscard]] int iterationsPerGroup() const noexcept;

    /*!
     * \brief Sets the sweeps per cyclic group (values < 1 are ignored).
     */
    void setIterationsPerGroup(int iterations);

    /*!
     * \brief The execution schedule built at prepare(): groups in
     *        topological order; a group with more than one member is a
     *        feedback loop. Empty before prepare().
     */
    [[nodiscard]] const std::vector<
        std::vector<HydroCouple::IModelComponent *>> &schedule() const noexcept;

protected:
    /*!
     * \brief Builds the DAG-of-SCCs schedule from the connection graph.
     */
    bool onPrepare(std::string &message) override;

    /*!
     * \brief Advances every non-Done component once through the schedule.
     */
    bool performStep(std::string &message) override;

private:
    int m_iterationsPerGroup = 1;

    std::vector<std::vector<HydroCouple::IModelComponent *>> m_schedule;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_TIMESTEPPEDWORKFLOW_H
