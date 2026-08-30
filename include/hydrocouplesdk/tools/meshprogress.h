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
 *        equal in cost and CDT's insertion reports nothing from inside, so
 *        a caller drawing a bar from this should show the stage beside it
 *        rather than let the bar imply a measurement.
 * \param stage The phase under way, as a short lower-case English phrase
 *        ("inserting edges"). Never null, and valid only for the duration
 *        of the call.
 * \returns True to carry on. **Returning false abandons the run**: the tool
 *          returns false, sets its message to say it was cancelled, and
 *          leaves its output empty — a cancelled run hands back nothing
 *          rather than half a mesh.
 *
 * \par What can be cancelled
 * Cancellation is offered where the tools' own loops run — densification,
 * candidate scoring, merging, extraction — and at every phase boundary.
 * **The CDT insertion phases cannot be interrupted**: they are one call into
 * the library, which offers no hook, so a cancellation asked for while one
 * is running takes effect when it returns. On a large domain that is the
 * longest wait a user can be made to sit through, and a caller must not
 * promise a cancel that lands sooner than it does.
 */
using MeshProgress = std::function<bool(double fraction, const char *stage)>;

} // namespace HydroCouple::SDK::Tools

#endif // HYDROCOUPLESDK_TOOLS_MESHPROGRESS_H
