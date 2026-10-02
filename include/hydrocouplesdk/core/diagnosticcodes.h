// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   diagnosticcodes.h
 * \brief  Named ErrorEntry::code values the SDK itself reports.
 *
 * ErrorEntry::code is implementation-defined. Codes below 1000 are left to
 * components (the SDK's own ResultsModelComponent uses 1–4); the SDK's
 * framework-level diagnostics live at 1000 and above so a consumer of
 * errors() can recognise them without parsing message text.
 */

#ifndef HYDROCOUPLESDK_DIAGNOSTICCODES_H
#define HYDROCOUPLESDK_DIAGNOSTICCODES_H

#include <cstdint>

namespace HydroCouple::SDK::Diagnostic {

// ── Lifecycle (1000–1099) ────────────────────────────────────────────────

//! A component or workflow made a status transition its normative table
//! (Helpers::isValid{Component,Workflow}StatusTransition) rejects. Reported
//! once per distinct (from, to) pair per object.
inline constexpr int32_t IllegalStatusTransition = 1001;

// ── Connections (1100–1199), from validateConnection() ───────────────────

//! Provider and consumer hold different DataKinds.
inline constexpr int32_t DataKindMismatch = 1101;

//! One side is a quantity and the other a quality, or the qualities'
//! categories differ.
inline constexpr int32_t ValueDefinitionMismatch = 1102;

//! Both sides declare a ValueKind and they differ, or one declares none.
inline constexpr int32_t ValueKindMismatch = 1103;

//! Units differ in dimension, or share a dimension but not a scale.
inline constexpr int32_t UnitMismatch = 1104;

//! TimeKind or its interval differs, or is undeclared.
inline constexpr int32_t TimeKindMismatch = 1105;

//! The provider's interpolation or extrapolation is not one the consumer
//! accepts, or both ends leave it undeclared.
inline constexpr int32_t TemporalPolicyMismatch = 1106;

//! Horizontal reference systems differ.
inline constexpr int32_t HorizontalReferenceMismatch = 1107;

//! Vertical datums or vertical units differ, or only one side declares one.
inline constexpr int32_t VerticalReferenceMismatch = 1108;

//! Values sit on different mesh/network entities (node vs face, ...).
inline constexpr int32_t MeshLocationMismatch = 1109;

//! Scalar/vector/tensor structure or vector basis differs.
inline constexpr int32_t VectorBasisMismatch = 1110;

//! Layered items whose vertical coordinates disagree on what a layer is.
inline constexpr int32_t LayeringMismatch = 1111;

//! An axis means different things on the two sides (Time vs Entity, ...).
inline constexpr int32_t DimensionRoleMismatch = 1112;

} // namespace HydroCouple::SDK::Diagnostic

#endif // HYDROCOUPLESDK_DIAGNOSTICCODES_H
