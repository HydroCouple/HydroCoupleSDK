// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   connectionvalidator.h
 * \brief  Checks one provider → consumer link against what both ends declare.
 *
 * The interface standard lets each data item say what its numbers *are*:
 * how they regrid (IValueDefinition::valueKind()), what their timestamps
 * mean and what the provider does between and beyond them
 * (ITimeSeriesComponentDataItem::timeKind()/timeInterpolation()/
 * timeExtrapolation()), their units, the horizontal and vertical reference
 * systems they live in, which mesh entity they sit on and in which vector
 * basis, what a layer is, and what each axis of the index space means.
 * Declarations are only worth having if something reads them; this is the
 * reader. AbstractWorkflowComponent::validate() runs it over every wired
 * link, and a host (the Composer, say) can run it when a link is drawn.
 *
 * Severity carries the contract:
 *  - Error   — both ends declare, and the declarations contradict each
 *              other: coupling them as wired produces wrong numbers without
 *              an error (means read as instants, NAVD88 heights read as
 *              NGVD29, face values read as node values, ...).
 *  - Warning — a declaration is missing, so the check could not be made.
 *              Nothing is known to be wrong; nothing is known to be right.
 *
 * The provider compared is the consumer's immediate provider. When an
 * adapted output sits between two components, it is the adapter that is
 * compared, which is correct: an adapter exists to change exactly the things
 * these checks compare (units, grids, time bases). An adapted output that
 * declares no value definition is taken to own its value semantics and is
 * not checked for them.
 */

#ifndef HYDROCOUPLESDK_CONNECTIONVALIDATOR_H
#define HYDROCOUPLESDK_CONNECTIONVALIDATOR_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"

#include <cstdint>
#include <string>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief One finding about a link. Severity is Error or Warning; code is one
 *        of the Diagnostic connection codes (diagnosticcodes.h).
 */
struct ConnectionFinding
{
    HydroCouple::ErrorEntry::Severity severity =
        HydroCouple::ErrorEntry::Severity::Warning;
    int32_t     code = 0;
    std::string message;
};

/*!
 * \brief Checks the link provider → consumer against both ends' declarations.
 * \returns Every finding, Errors and Warnings mixed, in check order. Empty
 *          means every check that could be made passed.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::vector<ConnectionFinding>
validateConnection(const HydroCouple::IOutput &provider,
                   const HydroCouple::IInput &consumer);

/*!
 * \brief Whether any finding is an Error.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool
hasConnectionError(const std::vector<ConnectionFinding> &findings);

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_CONNECTIONVALIDATOR_H
