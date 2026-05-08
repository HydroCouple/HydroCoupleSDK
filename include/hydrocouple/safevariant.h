// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   safevariant.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Type-safe variant accessors and conversion utilities.
 */

#ifndef HYDROCOUPLESDK_SAFEVARIANT_H
#define HYDROCOUPLESDK_SAFEVARIANT_H

#include "hydrocouplesdk.h"
#include "datatype.h"
#include "hydrocouple.h"

#include <optional>
#include <string>

namespace HydroCouple {

/*!
 * \brief Safe get from variant; returns empty optional on type mismatch.
 *
 * Never throws std::bad_variant_access. Returns empty optional if the
 * variant doesn't currently hold a value of type T.
 *
 * Usage:
 * \code
 * hydrocouple_variant v = 42;
 * if (auto val = getSafeValue<int>(v)) {
 *     // Use val.value()
 * } else if (auto val = getSafeValue<double>(v)) {
 *     // Try double conversion
 * }
 * \endcode
 *
 * \tparam T The type to extract
 * \param v The variant to read from
 * \return Optional containing the value if present, empty otherwise
 *
 * \threadsafety Thread-safe; no state mutation
 */
template<typename T>
[[nodiscard]] std::optional<T> getSafeValue(const hydrocouple_variant& v)
{
    if (std::holds_alternative<T>(v)) {
        return std::get<T>(v);
    }
    return std::nullopt;
}

/*!
 * \brief Type-aware variant conversion utilities.
 *
 * Converts hydrocouple_variant values between compatible types with
 * intelligent promotion (numeric to numeric, any to string).
 *
 * \threadsafety All methods are thread-safe; stateless
 */
class HYDROCOUPLESDK_EXPORT VariantConverter
{
public:
    /*!
     * \brief Convert variant to target type.
     *
     * Converts a variant value to a specified target type if possible.
     * Supports:
     * - Identity conversion (same types)
     * - Numeric conversions (int ↔ double, float ↔ int, etc.)
     * - Any type to string
     *
     * \param src Source variant
     * \param targetType Target type specification
     * \param strict If true, only allow identical types; if false, allow conversions
     * \return Converted variant in target type, or empty optional if conversion failed
     *
     * Example:
     * \code
     * hydrocouple_variant src = 42;
     * auto result = VariantConverter::convert(src, DataType(DataType::BaseType::Double));
     * if (result) {
     *     double val = std::get<double>(result.value());  // val == 42.0
     * }
     * \endcode
     */
    static std::optional<hydrocouple_variant> convert(
        const hydrocouple_variant& src,
        DataType targetType,
        bool strict = false);

    /*!
     * \brief Find common type that both variants can convert to.
     *
     * Determines a common type suitable for both variants:
     * - If same type → return that type
     * - If both numeric → return floating-point (double)
     * - Otherwise → return string
     *
     * \param a First variant
     * \param b Second variant
     * \return Common type, or Unknown if no common type exists
     *
     * Example:
     * \code
     * hydrocouple_variant a = 42;
     * hydrocouple_variant b = 3.14;
     * DataType common = VariantConverter::commonType(a, b);
     * assert(common.isFloating());  // true (double)
     * \endcode
     */
    [[nodiscard]] static DataType commonType(
        const hydrocouple_variant& a,
        const hydrocouple_variant& b);

    /*!
     * \brief Convert two variants to a common type.
     *
     * Finds a common type and converts both variants to that type.
     * Useful for binary operations (add, compare, etc.) that require
     * type compatibility.
     *
     * \param a First variant
     * \param b Second variant
     * \param[out] aConverted First variant converted to common type
     * \param[out] bConverted Second variant converted to common type
     * \return The common type found, or Unknown if no conversion possible
     *
     * Example:
     * \code
     * hydrocouple_variant a = 10;
     * hydrocouple_variant b = 3.14;
     * hydrocouple_variant aOut, bOut;
     * DataType common = VariantConverter::convertToCommon(a, b, aOut, bOut);
     * if (!common.isUnknown()) {
     *     double av = std::get<double>(aOut);  // 10.0
     *     double bv = std::get<double>(bOut);  // 3.14
     * }
     * \endcode
     */
    [[nodiscard]] static DataType convertToCommon(
        const hydrocouple_variant& a,
        const hydrocouple_variant& b,
        hydrocouple_variant& aConverted,
        hydrocouple_variant& bConverted);

    /*!
     * \brief Convert variant to string representation.
     *
     * Converts any variant value to a string. Handles all 14 types.
     * Numbers are converted via std::to_string (or similar).
     * Booleans are "true" or "false".
     * Already-string variants are returned as-is.
     *
     * \param v Variant to stringify
     * \return String representation, or "<unknown>" for unrecognized types
     */
    [[nodiscard]] static std::string toString(const hydrocouple_variant& v);

    /*!
     * \brief Get human-readable type name of variant.
     *
     * Returns the type name of the value currently held by the variant.
     * Examples: "int", "double", "string", "bool", "unknown"
     *
     * \param v Variant to inspect
     * \return Type name string
     */
    [[nodiscard]] static std::string typeToString(const hydrocouple_variant& v);

private:
    // Helper: convert numeric types with proper casting
    static std::optional<hydrocouple_variant> convertNumeric(
        const hydrocouple_variant& src,
        DataType targetType);
};

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_SAFEVARIANT_H
