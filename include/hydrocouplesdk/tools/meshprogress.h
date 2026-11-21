// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   meshprogress.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Watching a meshing run, and asking it to stop.
 *
 * The meshing tools are static functions that return when they are done,
 * which is all a batch caller needs and not enough for one driving a window:
 * a domain of any size takes long enough that a user will want to know it is
 * still working, and long enough that they will want to abandon it.
 *
 * \threadsafety The callback is invoked on the thread that called the tool.
 *               A caller that runs the tool on a worker must marshal to its
 *               own thread itself; the tools do no marshalling.
 */

#ifndef HYDROCOUPLESDK_TOOLS_MESHPROGRESS_H
#define HYDROCOUPLESDK_TOOLS_MESHPROGRESS_H

#include <functional>

namespace HydroCouple::SDK::Tools {

/*!
 * \brief Reports where a meshing run has got to and asks whether to go on.
 *
 * \param fraction How far through the run's *phases* it is, in [0, 1].
 *        Deliberately not a measure of work remaining: the phases are not
 *        equal in cost, so a caller drawing a bar from this should show the
 *        stage beside it rather than let the bar imply a measurement.
 * \param stage The phase under way, as a short lower-case English phrase
 *        ("inserting edges"). Never null, and valid only for the duration
 *        of the call.
 * \returns True to carry on. **Returning false abandons the run**: the tool
 *          returns false, sets its message to say it was canceled, and
 *          leaves its output empty — a canceled run hands back nothing
 *          rather than half a mesh.
 *
 * \par What can be canceled
 * Every phase: densification, candidate scoring, merging and extraction are
 * the tools' own loops, edge insertion is too, and MeshKit's vertex insertion
 * asks between batches of 4,096 vertices. A cancellation therefore lands
 * within one batch or one loop step of being asked for.
 */
using MeshProgress = std::function<bool(double fraction, const char *stage)>;

} // namespace HydroCouple::SDK::Tools

#endif // HYDROCOUPLESDK_TOOLS_MESHPROGRESS_H
