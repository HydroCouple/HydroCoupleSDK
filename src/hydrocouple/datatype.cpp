// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "hydrocouple/datatype.h"
#include "hydrocouple.h"

namespace HydroCouple {

DataType::DataType(BaseType base) : m_baseType(base) {}

bool DataType::isNumeric() const
{
    switch (m_baseType) {
        case BaseType::Char:
        case BaseType::Short:
        case BaseType::Int:
        case BaseType::Long:
        case BaseType::UChar:
        case BaseType::UShort:
        case BaseType::UInt:
        case BaseType::ULong:
        case BaseType::Float:
        case BaseType::Double:
        case BaseType::LongDouble:
            return true;
        default:
            return false;
    }
}

bool DataType::isFloating() const
{
    return m_baseType == BaseType::Float || m_baseType == BaseType::Double ||
           m_baseType == BaseType::LongDouble;
}

bool DataType::isInteger() const
{
    return (m_baseType >= BaseType::Char && m_baseType <= BaseType::Long) ||
           (m_baseType >= BaseType::UChar && m_baseType <= BaseType::ULong);
}

const std::type_info& DataType::typeInfo() const
{
    switch (m_baseType) {
        case BaseType::Bool:
            return typeid(bool);
        case BaseType::Char:
            return typeid(char);
        case BaseType::Short:
            return typeid(short);
        case BaseType::Int:
            return typeid(int);
        case BaseType::Long:
            return typeid(long);
        case BaseType::UChar:
            return typeid(unsigned char);
        case BaseType::UShort:
            return typeid(unsigned short);
        case BaseType::UInt:
            return typeid(unsigned int);
        case BaseType::ULong:
            return typeid(unsigned long);
        case BaseType::Float:
            return typeid(float);
        case BaseType::Double:
            return typeid(double);
        case BaseType::LongDouble:
            return typeid(long double);
        case BaseType::String:
            return typeid(std::string);
        case BaseType::VoidPtr:
            return typeid(void*);
        default:
            return typeid(void);
    }
}

std::string DataType::name() const
{
    switch (m_baseType) {
        case BaseType::Bool:
            return "bool";
        case BaseType::Char:
            return "char";
        case BaseType::Short:
            return "short";
        case BaseType::Int:
            return "int";
        case BaseType::Long:
            return "long";
        case BaseType::UChar:
            return "unsigned char";
        case BaseType::UShort:
            return "unsigned short";
        case BaseType::UInt:
            return "unsigned int";
        case BaseType::ULong:
            return "unsigned long";
        case BaseType::Float:
            return "float";
        case BaseType::Double:
            return "double";
        case BaseType::LongDouble:
            return "long double";
        case BaseType::String:
            return "string";
        case BaseType::VoidPtr:
            return "void*";
        default:
            return "unknown";
    }
}

bool DataType::canConvertTo(const DataType& other) const
{
    // Same type always converts
    if (*this == other)
        return true;

    // Numeric types convert to each other
    if (isNumeric() && other.isNumeric())
        return true;

    // Any type converts to string
    if (other.isString())
        return true;

    // Pointer is flexible (can be used as generic storage)
    if (isPointer() || other.isPointer())
        return true;

    return false;
}

DataType DataType::fromVariant(const hydrocouple_variant& v)
{
    if (std::holds_alternative<bool>(v))
        return DataType(BaseType::Bool);
    if (std::holds_alternative<char>(v))
        return DataType(BaseType::Char);
    if (std::holds_alternative<short>(v))
        return DataType(BaseType::Short);
    if (std::holds_alternative<int>(v))
        return DataType(BaseType::Int);
    if (std::holds_alternative<long>(v))
        return DataType(BaseType::Long);
    if (std::holds_alternative<unsigned char>(v))
        return DataType(BaseType::UChar);
    if (std::holds_alternative<unsigned short>(v))
        return DataType(BaseType::UShort);
    if (std::holds_alternative<unsigned int>(v))
        return DataType(BaseType::UInt);
    if (std::holds_alternative<unsigned long>(v))
        return DataType(BaseType::ULong);
    if (std::holds_alternative<float>(v))
        return DataType(BaseType::Float);
    if (std::holds_alternative<double>(v))
        return DataType(BaseType::Double);
    if (std::holds_alternative<long double>(v))
        return DataType(BaseType::LongDouble);
    if (std::holds_alternative<std::string>(v))
        return DataType(BaseType::String);
    if (std::holds_alternative<void*>(v))
        return DataType(BaseType::VoidPtr);

    return DataType(BaseType::Unknown);
}

DataType DataType::fromTypeInfo(const std::type_info& info)
{
    if (info == typeid(bool))
        return DataType(BaseType::Bool);
    if (info == typeid(char))
        return DataType(BaseType::Char);
    if (info == typeid(short))
        return DataType(BaseType::Short);
    if (info == typeid(int))
        return DataType(BaseType::Int);
    if (info == typeid(long))
        return DataType(BaseType::Long);
    if (info == typeid(unsigned char))
        return DataType(BaseType::UChar);
    if (info == typeid(unsigned short))
        return DataType(BaseType::UShort);
    if (info == typeid(unsigned int))
        return DataType(BaseType::UInt);
    if (info == typeid(unsigned long))
        return DataType(BaseType::ULong);
    if (info == typeid(float))
        return DataType(BaseType::Float);
    if (info == typeid(double))
        return DataType(BaseType::Double);
    if (info == typeid(long double))
        return DataType(BaseType::LongDouble);
    if (info == typeid(std::string))
        return DataType(BaseType::String);
    if (info == typeid(void*))
        return DataType(BaseType::VoidPtr);

    return DataType(BaseType::Unknown);
}

} // namespace HydroCouple
