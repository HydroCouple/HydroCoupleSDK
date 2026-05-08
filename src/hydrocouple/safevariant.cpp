// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "hydrocouple/safevariant.h"
#include <sstream>
#include <iomanip>

namespace HydroCouple {

std::optional<hydrocouple_variant> VariantConverter::convert(
    const hydrocouple_variant& src,
    DataType targetType,
    bool strict)
{
    DataType srcType = DataType::fromVariant(src);

    // Identical types: always OK
    if (srcType == targetType) {
        return src;
    }

    // Strict mode: only identical types
    if (strict) {
        return std::nullopt;
    }

    // Numeric conversions
    if (srcType.isNumeric() && targetType.isNumeric()) {
        return convertNumeric(src, targetType);
    }

    // Any type to string
    if (targetType.isString()) {
        return hydrocouple_variant(toString(src));
    }

    return std::nullopt;
}

DataType VariantConverter::commonType(
    const hydrocouple_variant& a,
    const hydrocouple_variant& b)
{
    DataType aType = DataType::fromVariant(a);
    DataType bType = DataType::fromVariant(b);

    // Same type is always common
    if (aType == bType) {
        return aType;
    }

    // Both numeric: promote to floating-point
    if (aType.isNumeric() && bType.isNumeric()) {
        if (aType.isFloating() || bType.isFloating()) {
            return DataType(DataType::BaseType::Double);
        }
        // Both integers: promote to larger integer
        return DataType(DataType::BaseType::Long);
    }

    // Any type to string is always possible
    return DataType(DataType::BaseType::String);
}

DataType VariantConverter::convertToCommon(
    const hydrocouple_variant& a,
    const hydrocouple_variant& b,
    hydrocouple_variant& aConverted,
    hydrocouple_variant& bConverted)
{
    DataType common = commonType(a, b);

    if (common.isUnknown()) {
        return common;
    }

    auto aConv = convert(a, common);
    auto bConv = convert(b, common);

    if (!aConv || !bConv) {
        return DataType(DataType::BaseType::Unknown);
    }

    aConverted = aConv.value();
    bConverted = bConv.value();
    return common;
}

std::optional<hydrocouple_variant> VariantConverter::convertNumeric(
    const hydrocouple_variant& src,
    DataType targetType)
{
    try {
        // Try int target
        if (targetType.baseType() == DataType::BaseType::Int) {
            if (auto val = getSafeValue<int>(src)) {
                return hydrocouple_variant(val.value());
            }
            if (auto val = getSafeValue<double>(src)) {
                return hydrocouple_variant(static_cast<int>(val.value()));
            }
            if (auto val = getSafeValue<float>(src)) {
                return hydrocouple_variant(static_cast<int>(val.value()));
            }
            if (auto val = getSafeValue<long>(src)) {
                return hydrocouple_variant(static_cast<int>(val.value()));
            }
            if (auto val = getSafeValue<short>(src)) {
                return hydrocouple_variant(static_cast<int>(val.value()));
            }
            if (auto val = getSafeValue<char>(src)) {
                return hydrocouple_variant(static_cast<int>(val.value()));
            }
        }

        // Try double target
        if (targetType.baseType() == DataType::BaseType::Double) {
            if (auto val = getSafeValue<double>(src)) {
                return hydrocouple_variant(val.value());
            }
            if (auto val = getSafeValue<int>(src)) {
                return hydrocouple_variant(static_cast<double>(val.value()));
            }
            if (auto val = getSafeValue<float>(src)) {
                return hydrocouple_variant(static_cast<double>(val.value()));
            }
            if (auto val = getSafeValue<long>(src)) {
                return hydrocouple_variant(static_cast<double>(val.value()));
            }
            if (auto val = getSafeValue<short>(src)) {
                return hydrocouple_variant(static_cast<double>(val.value()));
            }
            if (auto val = getSafeValue<char>(src)) {
                return hydrocouple_variant(static_cast<double>(val.value()));
            }
        }

        // Try float target
        if (targetType.baseType() == DataType::BaseType::Float) {
            if (auto val = getSafeValue<float>(src)) {
                return hydrocouple_variant(val.value());
            }
            if (auto val = getSafeValue<double>(src)) {
                return hydrocouple_variant(static_cast<float>(val.value()));
            }
            if (auto val = getSafeValue<int>(src)) {
                return hydrocouple_variant(static_cast<float>(val.value()));
            }
        }

        // Try long target
        if (targetType.baseType() == DataType::BaseType::Long) {
            if (auto val = getSafeValue<long>(src)) {
                return hydrocouple_variant(val.value());
            }
            if (auto val = getSafeValue<int>(src)) {
                return hydrocouple_variant(static_cast<long>(val.value()));
            }
            if (auto val = getSafeValue<double>(src)) {
                return hydrocouple_variant(static_cast<long>(val.value()));
            }
        }

        // Add other numeric type conversions as needed
    } catch (...) {
        return std::nullopt;
    }

    return std::nullopt;
}

std::string VariantConverter::toString(const hydrocouple_variant& v)
{
    if (auto val = getSafeValue<bool>(v)) {
        return val.value() ? "true" : "false";
    }
    if (auto val = getSafeValue<int>(v)) {
        return std::to_string(val.value());
    }
    if (auto val = getSafeValue<long>(v)) {
        return std::to_string(val.value());
    }
    if (auto val = getSafeValue<double>(v)) {
        std::ostringstream oss;
        oss << std::setprecision(15) << val.value();
        return oss.str();
    }
    if (auto val = getSafeValue<float>(v)) {
        std::ostringstream oss;
        oss << std::setprecision(7) << val.value();
        return oss.str();
    }
    if (auto val = getSafeValue<std::string>(v)) {
        return val.value();
    }
    if (auto val = getSafeValue<char>(v)) {
        return std::string(1, val.value());
    }
    if (auto val = getSafeValue<short>(v)) {
        return std::to_string(val.value());
    }
    if (auto val = getSafeValue<unsigned int>(v)) {
        return std::to_string(val.value());
    }
    if (auto val = getSafeValue<unsigned long>(v)) {
        return std::to_string(val.value());
    }
    if (auto val = getSafeValue<unsigned char>(v)) {
        return std::to_string(val.value());
    }
    if (auto val = getSafeValue<unsigned short>(v)) {
        return std::to_string(val.value());
    }
    if (auto val = getSafeValue<long double>(v)) {
        std::ostringstream oss;
        oss << std::setprecision(20) << val.value();
        return oss.str();
    }
    if (auto val = getSafeValue<void*>(v)) {
        std::ostringstream oss;
        oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(val.value());
        return oss.str();
    }

    return "<unknown>";
}

std::string VariantConverter::typeToString(const hydrocouple_variant& v)
{
    return DataType::fromVariant(v).name();
}

} // namespace HydroCouple
