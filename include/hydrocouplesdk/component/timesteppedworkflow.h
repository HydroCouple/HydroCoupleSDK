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
 * Details, from the code:
 * - Edges come from the inputs of managed components; a provider that is
 *   an adapted output is walked back through adaptee() to its root output,
 *   and the edge goes from the root's component. Providers whose component
 *   is not managed, and a component's links to itself, add no edge.
 * - Members of a cyclic group are ordered by insertion. Groups with no path
 *   between them come out in reverse insertion order (the order Tarjan's
 *   algorithm emits them, reversed): three unconnected components added as
 *   a, b, c run c, b, a.
 * - The workflow does not move values into inputs: each component's
 *   update() pulls its own (AbstractModelComponent::applyInputValues()).
 *   After each update() the workflow refreshes the first tier of adapted
 *   outputs on that component's outputs.
 * - A sweep calls update() on every member of the group again. There is no
 *   convergence test: a group always runs iterationsPerGroup() sweeps, and
 *   a component whose update() advances its clock advances it once per
 *   sweep. Use more than one sweep only with components whose update()
 *   re-solves the same interval when called again.
 * - A component that is Done or Finished is skipped, and its adapted
 *   outputs are not refreshed.
 *
 * \threadsafety See AbstractWorkflowComponent.
 */

#ifndef HYDROCOUPLESDK_TIMESTEPPEDWORKFLOW_H
#define HYDROCOUPLESDK_TIMESTEPPEDWORKFLOW_H

#include "hydrocouplesdk/component/abstractworkflowcomponent.h"

namespace HydroCouple::SDK {

/*!
 * \brief Schedule-driven orchestration workflow.
 * \details Typical use: add the components, initialize(), wire the links,
 *          validate(), prepare() (which builds the schedule), then call
 *          update() until the status is Done, and finish().
 */
class HYDROCOUPLESDK_EXPORT TimeSteppedWorkflow
    : public AbstractWorkflowComponent
{
public:
    /*!
     * \brief Construct with an id and optional component-info pointer.
     * \param[in] id    Workflow identifier; copied.
     * \param[in] info  Metadata object; borrowed, may be nullptr.
     */
    explicit TimeSteppedWorkflow(
        std::string_view id,
        HydroCouple::IWorkflowComponentInfo *info = nullptr);

    //! Destroys the workflow (see AbstractWorkflowComponent::~AbstractWorkflowComponent()).
    ~TimeSteppedWorkflow() override = default;

    /*!
     * \brief Update sweeps per cyclic group each step (≥ 1; default 1 =
     *        time-lagged). Groups of one component always update once.
     * \return The number of sweeps.
     */
    [[nodiscard]] int iterationsPerGroup() const noexcept;

    /*!
     * \brief Sets the sweeps per cyclic group (values < 1 are ignored).
     * \details May be called in any status; the next update() uses it. A
     *          value below 1 is dropped without a message. The composition
     *          document's \c iterations_per_group is not applied by this
     *          class: the host passes it here.
     * \param[in] iterations  Sweeps per cyclic group, at least 1.
     */
    void setIterationsPerGroup(int iterations);

    /*!
     * \brief The execution schedule built at prepare(): groups in
     *        topological order; a group with more than one member is a
     *        feedback loop. Empty before prepare().
     * \return Reference to the schedule, valid while the workflow lives;
     *         the component pointers are not owned.
     */
    [[nodiscard]] const std::vector<
        std::vector<HydroCouple::IModelComponent *>> &schedule() const noexcept;

protected:
    /*!
     * \brief Builds the DAG-of-SCCs schedule from the connection graph.
     * \details Called by prepare() after every component is prepared.
     *          Always succeeds; a composition without links gives one group
     *          per component.
     * \param[out] message  Cleared.
     * \return \c true.
     */
    bool onPrepare(std::string &message) override;

    /*!
     * \brief Advances every non-Done component once through the schedule.
     * \details Visits the groups in order; a group with more than one
     *          member runs iterationsPerGroup() sweeps. Each visited
     *          component that is neither Done nor Finished gets update({}),
     *          then the adapted outputs on its outputs are refreshed.
     * \param[out] message  "component 'id' failed during update" on failure.
     * \return \c false as soon as a component ends its update() Failed; the
     *         rest of the schedule is not run.
     */
    bool performStep(std::string &message) override;

private:
    int m_iterationsPerGroup = 1; //!< Sweeps per cyclic group.

    std::vector<std::vector<HydroCouple::IModelComponent *>> m_schedule; //!< Groups in run order.
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_TIMESTEPPEDWORKFLOW_H
