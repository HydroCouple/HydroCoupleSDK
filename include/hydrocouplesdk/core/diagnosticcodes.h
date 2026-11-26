// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   diagnosticcodes.h
 * \brief  Named ErrorEntry::code values the SDK itself reports.
 *
 * ErrorEntry::code is implementation-defined. Codes below 1000 are left to
 * components (the SDK's own ResultsModelComponent uses 1–5, and the base
 * classes queue code 0 for a Fatal entry that accompanies a Failed status
 * and for validation problems); the SDK's framework-level diagnostics live
 * at 1000 and above so a consumer of errors() can recognize them without
 * parsing message text.
 */

#ifndef HYDROCOUPLESDK_DIAGNOSTICCODES_H
#define HYDROCOUPLESDK_DIAGNOSTICCODES_H

#include <cstdint>

/*!
 * \brief ErrorEntry::code values of the SDK's framework diagnostics.
 * \details Lifecycle codes (1000–1099) are queued by AbstractModelComponent
 *          and AbstractWorkflowComponent; connection codes (1100–1199) are
 *          the ConnectionFinding::code values validateConnection() returns,
 *          which AbstractWorkflowComponent::validate() queues on the
 *          workflow's errors().
 */
namespace HydroCouple::SDK::Diagnostic {

// ── Lifecycle (1000–1099) ────────────────────────────────────────────────

//! A component or workflow made a status transition its normative table
//! (Helpers::isValid{Component,Workflow}StatusTransition) rejects. Reported
//! once per distinct (from, to) pair per object, at Severity::Error; the
//! transition is still performed.
inline constexpr int32_t IllegalStatusTransition = 1001;

// ── Connections (1100–1199), from validateConnection() ───────────────────

//! Provider and consumer hold different DataKinds. Always an Error: the
//! data plane converts no element types.
inline constexpr int32_t DataKindMismatch = 1101;

//! One side is a quantity and the other a quality, or the qualities'
//! categories differ (Error); or one side declares no value definition
//! (Warning, except for an adapted output, which is not checked).
inline constexpr int32_t ValueDefinitionMismatch = 1102;

//! Both sides declare a ValueKind and they differ (Error), or one or both
//! sides declare none (Warning).
inline constexpr int32_t ValueKindMismatch = 1103;

//! Units differ in dimension, or share a dimension but not a scale or
//! offset (Error); or a side declares no unit or no dimensions, or a
//! dimensionless unit meets a dimensioned one (Warning).
inline constexpr int32_t UnitMismatch = 1104;

//! TimeKind differs, or both are the same interval kind over different
//! interval lengths (Error); or a side leaves TimeKind Unknown (Warning).
inline constexpr int32_t TimeKindMismatch = 1105;

//! The provider's interpolation or extrapolation is not one the consumer
//! accepts, or both ends leave it undeclared (Error); one end leaves it
//! undeclared (Warning).
inline constexpr int32_t TemporalPolicyMismatch = 1106;

//! Horizontal reference systems differ, both being declared (Error).
inline constexpr int32_t HorizontalReferenceMismatch = 1107;

//! Vertical datums or vertical units differ (Error), or only one side
//! declares a datum (Warning).
inline constexpr int32_t VerticalReferenceMismatch = 1108;

//! Values sit on different mesh/network entities (node vs face, ...).
//! Always an Error.
inline constexpr int32_t MeshLocationMismatch = 1109;

//! Scalar/vector/tensor structure or vector basis differs (Error), or a
//! vector or tensor end declares no basis (Warning).
inline constexpr int32_t VectorBasisMismatch = 1110;

//! Layered items whose vertical coordinates disagree on what a layer is,
//! or on how many layers there are, or a layered item without a vertical
//! coordinate (Error); or an undeclared vertical coordinate kind (Warning).
inline constexpr int32_t LayeringMismatch = 1111;

//! An axis means different things on the two sides (Time vs Entity, ...).
//! Always an Error; axes whose role is Unknown or Other are skipped.
inline constexpr int32_t DimensionRoleMismatch = 1112;

//! Both sides name their quantity and the names differ: concept URIs when both
//! declare one, otherwise CF standard names. Always a warning — one model's
//! outflow is legitimately another's inflow.
inline constexpr int32_t QuantityNameMismatch = 1113;

} // namespace HydroCouple::SDK::Diagnostic

#endif // HYDROCOUPLESDK_DIAGNOSTICCODES_H
