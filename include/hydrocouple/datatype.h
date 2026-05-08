// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   datatype.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Type information for hydrocouple_variant values.
 */

#ifndef HYDROCOUPLESDK_DATATYPE_H
#define HYDROCOUPLESDK_DATATYPE_H

#include "hydrocouplesdk.h"
#include <string>
#include <typeinfo>
#include "hydrocouple.h"

namespace HydroCouple {

/*!
 * \brief Type information for hydrocouple_variant values.
 *
 * Provides safe type introspection and conversion capabilities.
 * Mirrors the 14 primitive types in hydrocouple_variant.
 *
 * \threadsafety Immutable; safe to share across threads.
 */
class HYDROCOUPLESDK_EXPORT DataType
{
public:
    /*!
     * \brief Base type enumeration matching hydrocouple_variant alternatives.
     */
    enum class BaseType
    {
        Bool,       ///< bool
        Char,       ///< char
        Short,      ///< short
        Int,        ///< int
        Long,       ///< long
        UChar,      ///< unsigned char
        UShort,     ///< unsigned short
        UInt,       ///< unsigned int
        ULong,      ///< unsigned long
        Float,      ///< float
        Double,     ///< double
        LongDouble, ///< long double
        String,     ///< std::string
        VoidPtr,    ///< void*
        Unknown     ///< Unknown or invalid type
    };

    /*!
     * \brief Construct from a BaseType enum.
     * \param base Type enumeration (default: Unknown)
     */
    explicit DataType(BaseType base = BaseType::Unknown);

    /*!
     * \brief Get the base type.
     */
    [[nodiscard]] BaseType baseType() const { return m_baseType; }

    /*!
     * \brief Check if type is numeric (int, float, etc.).
     */
    [[nodiscard]] bool isNumeric() const;

    /*!
     * \brief Check if type is floating-point (float, double, long double).
     */
    [[nodiscard]] bool isFloating() const;

    /*!
     * \brief Check if type is integer (char, short, int, long, unsigned variants).
     */
    [[nodiscard]] bool isInteger() const;

    /*!
     * \brief Check if type is boolean.
     */
    [[nodiscard]] bool isBool() const { return m_baseType == BaseType::Bool; }

    /*!
     * \brief Check if type is string.
     */
    [[nodiscard]] bool isString() const { return m_baseType == BaseType::String; }

    /*!
     * \brief Check if type is pointer.
     */
    [[nodiscard]] bool isPointer() const { return m_baseType == BaseType::VoidPtr; }

    /*!
     * \brief Check if type is unknown.
     */
    [[nodiscard]] bool isUnknown() const { return m_baseType == BaseType::Unknown; }

    /*!
     * \brief Get std::type_info for this type.
     */
    [[nodiscard]] const std::type_info& typeInfo() const;

    /*!
     * \brief Get human-readable name (e.g., "int", "double", "string").
     */
    [[nodiscard]] std::string name() const;

    /*!
     * \brief Check if this type can be converted to another type.
     *
     * Conversion rules:
     * - Numeric types convert to each other
     * - Any type converts to string
     * - Same types always convert
     * - Pointer is special case (true for most types)
     *
     * \param other Target type for conversion
     * \return true if this type can be converted to other
     */
    [[nodiscard]] bool canConvertTo(const DataType& other) const;

    /*!
     * \brief Detect type of a variant value.
     *
     * \param v Variant to inspect
     * \return DataType corresponding to variant's current alternative
     */
    static DataType fromVariant(const hydrocouple_variant& v);

    /*!
     * \brief Create from std::type_info.
     *
     * \param info Type information object
     * \return DataType matching the type, or Unknown
     */
    static DataType fromTypeInfo(const std::type_info& info);

    /*!
     * \brief Equality comparison.
     */
    [[nodiscard]] bool operator==(const DataType& other) const
    {
        return m_baseType == other.m_baseType;
    }

    /*!
     * \brief Inequality comparison.
     */
    [[nodiscard]] bool operator!=(const DataType& other) const
    {
        return !(*this == other);
    }

private:
    BaseType m_baseType;
};

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_DATATYPE_H
