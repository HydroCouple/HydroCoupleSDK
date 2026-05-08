// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include <gtest/gtest.h>
#include "hydrocouple/datatype.h"

using namespace HydroCouple;

// ────────────────────────────────────────────────────────────────────────────
// Constructor and Basic Properties
// ────────────────────────────────────────────────────────────────────────────

TEST(DataTypeTest, ConstructorDefault)
{
    DataType t;
    EXPECT_EQ(t.baseType(), DataType::BaseType::Unknown);
    EXPECT_FALSE(t.isNumeric());
    EXPECT_FALSE(t.isFloating());
    EXPECT_FALSE(t.isInteger());
    EXPECT_TRUE(t.isUnknown());
}

TEST(DataTypeTest, ConstructorWithType)
{
    DataType t(DataType::BaseType::Double);
    EXPECT_EQ(t.baseType(), DataType::BaseType::Double);
    EXPECT_TRUE(t.isNumeric());
    EXPECT_TRUE(t.isFloating());
    EXPECT_FALSE(t.isInteger());
    EXPECT_FALSE(t.isUnknown());
}

// ────────────────────────────────────────────────────────────────────────────
// Type Classification Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(DataTypeTest, ClassifyBool)
{
    DataType t(DataType::BaseType::Bool);
    EXPECT_TRUE(t.isBool());
    EXPECT_FALSE(t.isNumeric());
    EXPECT_FALSE(t.isString());
}

TEST(DataTypeTest, ClassifyString)
{
    DataType t(DataType::BaseType::String);
    EXPECT_TRUE(t.isString());
    EXPECT_FALSE(t.isNumeric());
    EXPECT_FALSE(t.isBool());
}

TEST(DataTypeTest, ClassifyFloatingPoint)
{
    DataType floatT(DataType::BaseType::Float);
    EXPECT_TRUE(floatT.isFloating());
    EXPECT_TRUE(floatT.isNumeric());
    EXPECT_FALSE(floatT.isInteger());

    DataType doubleT(DataType::BaseType::Double);
    EXPECT_TRUE(doubleT.isFloating());
    EXPECT_TRUE(doubleT.isNumeric());
    EXPECT_FALSE(doubleT.isInteger());

    DataType longDoubleT(DataType::BaseType::LongDouble);
    EXPECT_TRUE(longDoubleT.isFloating());
    EXPECT_TRUE(longDoubleT.isNumeric());
    EXPECT_FALSE(longDoubleT.isInteger());
}

TEST(DataTypeTest, ClassifySignedInteger)
{
    DataType charT(DataType::BaseType::Char);
    EXPECT_TRUE(charT.isInteger());
    EXPECT_TRUE(charT.isNumeric());
    EXPECT_FALSE(charT.isFloating());

    DataType shortT(DataType::BaseType::Short);
    EXPECT_TRUE(shortT.isInteger());
    EXPECT_TRUE(shortT.isNumeric());

    DataType intT(DataType::BaseType::Int);
    EXPECT_TRUE(intT.isInteger());
    EXPECT_TRUE(intT.isNumeric());

    DataType longT(DataType::BaseType::Long);
    EXPECT_TRUE(longT.isInteger());
    EXPECT_TRUE(longT.isNumeric());
}

TEST(DataTypeTest, ClassifyUnsignedInteger)
{
    DataType ucharT(DataType::BaseType::UChar);
    EXPECT_TRUE(ucharT.isInteger());
    EXPECT_TRUE(ucharT.isNumeric());

    DataType ushortT(DataType::BaseType::UShort);
    EXPECT_TRUE(ushortT.isInteger());
    EXPECT_TRUE(ushortT.isNumeric());

    DataType uintT(DataType::BaseType::UInt);
    EXPECT_TRUE(uintT.isInteger());
    EXPECT_TRUE(uintT.isNumeric());

    DataType ulongT(DataType::BaseType::ULong);
    EXPECT_TRUE(ulongT.isInteger());
    EXPECT_TRUE(ulongT.isNumeric());
}

TEST(DataTypeTest, ClassifyPointer)
{
    DataType t(DataType::BaseType::VoidPtr);
    EXPECT_TRUE(t.isPointer());
    EXPECT_FALSE(t.isNumeric());
}

// ────────────────────────────────────────────────────────────────────────────
// Type Names
// ────────────────────────────────────────────────────────────────────────────

TEST(DataTypeTest, Names)
{
    EXPECT_EQ(DataType(DataType::BaseType::Bool).name(), "bool");
    EXPECT_EQ(DataType(DataType::BaseType::Char).name(), "char");
    EXPECT_EQ(DataType(DataType::BaseType::Short).name(), "short");
    EXPECT_EQ(DataType(DataType::BaseType::Int).name(), "int");
    EXPECT_EQ(DataType(DataType::BaseType::Long).name(), "long");
    EXPECT_EQ(DataType(DataType::BaseType::UChar).name(), "unsigned char");
    EXPECT_EQ(DataType(DataType::BaseType::UShort).name(), "unsigned short");
    EXPECT_EQ(DataType(DataType::BaseType::UInt).name(), "unsigned int");
    EXPECT_EQ(DataType(DataType::BaseType::ULong).name(), "unsigned long");
    EXPECT_EQ(DataType(DataType::BaseType::Float).name(), "float");
    EXPECT_EQ(DataType(DataType::BaseType::Double).name(), "double");
    EXPECT_EQ(DataType(DataType::BaseType::LongDouble).name(), "long double");
    EXPECT_EQ(DataType(DataType::BaseType::String).name(), "string");
    EXPECT_EQ(DataType(DataType::BaseType::VoidPtr).name(), "void*");
    EXPECT_EQ(DataType(DataType::BaseType::Unknown).name(), "unknown");
}

// ────────────────────────────────────────────────────────────────────────────
// Type Info
// ────────────────────────────────────────────────────────────────────────────

TEST(DataTypeTest, TypeInfo)
{
    DataType t(DataType::BaseType::Int);
    EXPECT_EQ(t.typeInfo(), typeid(int));

    DataType d(DataType::BaseType::Double);
    EXPECT_EQ(d.typeInfo(), typeid(double));

    DataType s(DataType::BaseType::String);
    EXPECT_EQ(s.typeInfo(), typeid(std::string));
}

// ────────────────────────────────────────────────────────────────────────────
// Conversion Rules
// ────────────────────────────────────────────────────────────────────────────

TEST(DataTypeTest, SameTypeCanConvert)
{
    DataType intType(DataType::BaseType::Int);
    EXPECT_TRUE(intType.canConvertTo(intType));

    DataType stringType(DataType::BaseType::String);
    EXPECT_TRUE(stringType.canConvertTo(stringType));
}

TEST(DataTypeTest, NumericToNumericConversions)
{
    DataType intType(DataType::BaseType::Int);
    DataType doubleType(DataType::BaseType::Double);
    DataType floatType(DataType::BaseType::Float);

    EXPECT_TRUE(intType.canConvertTo(doubleType));
    EXPECT_TRUE(doubleType.canConvertTo(intType));
    EXPECT_TRUE(floatType.canConvertTo(doubleType));
    EXPECT_TRUE(doubleType.canConvertTo(floatType));

    // All numeric types can convert to each other
    DataType longType(DataType::BaseType::Long);
    EXPECT_TRUE(intType.canConvertTo(longType));
    EXPECT_TRUE(longType.canConvertTo(doubleType));
}

TEST(DataTypeTest, AnyTypeToString)
{
    DataType stringType(DataType::BaseType::String);
    DataType intType(DataType::BaseType::Int);
    DataType doubleType(DataType::BaseType::Double);
    DataType boolType(DataType::BaseType::Bool);

    EXPECT_TRUE(intType.canConvertTo(stringType));
    EXPECT_TRUE(doubleType.canConvertTo(stringType));
    EXPECT_TRUE(boolType.canConvertTo(stringType));
}

TEST(DataTypeTest, StringDoesNotConvertToOthers)
{
    DataType stringType(DataType::BaseType::String);
    DataType intType(DataType::BaseType::Int);
    DataType doubleType(DataType::BaseType::Double);

    EXPECT_FALSE(stringType.canConvertTo(intType));
    EXPECT_FALSE(stringType.canConvertTo(doubleType));
}

TEST(DataTypeTest, PointerConversions)
{
    DataType ptrType(DataType::BaseType::VoidPtr);
    DataType intType(DataType::BaseType::Int);
    DataType doubleType(DataType::BaseType::Double);

    // Pointer can convert to any type
    EXPECT_TRUE(ptrType.canConvertTo(intType));
    EXPECT_TRUE(ptrType.canConvertTo(doubleType));

    // Any type can convert to pointer
    EXPECT_TRUE(intType.canConvertTo(ptrType));
    EXPECT_TRUE(doubleType.canConvertTo(ptrType));
}

// ────────────────────────────────────────────────────────────────────────────
// fromVariant Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(DataTypeTest, FromVariantBool)
{
    hydrocouple_variant v = true;
    DataType t = DataType::fromVariant(v);
    EXPECT_EQ(t.baseType(), DataType::BaseType::Bool);
    EXPECT_TRUE(t.isBool());
}

TEST(DataTypeTest, FromVariantInteger)
{
    hydrocouple_variant v = 42;
    DataType t = DataType::fromVariant(v);
    EXPECT_EQ(t.baseType(), DataType::BaseType::Int);
    EXPECT_TRUE(t.isInteger());
    EXPECT_TRUE(t.isNumeric());
}

TEST(DataTypeTest, FromVariantDouble)
{
    hydrocouple_variant v = 3.14;
    DataType t = DataType::fromVariant(v);
    EXPECT_EQ(t.baseType(), DataType::BaseType::Double);
    EXPECT_TRUE(t.isFloating());
    EXPECT_TRUE(t.isNumeric());
}

TEST(DataTypeTest, FromVariantString)
{
    hydrocouple_variant v = std::string("hello");
    DataType t = DataType::fromVariant(v);
    EXPECT_EQ(t.baseType(), DataType::BaseType::String);
    EXPECT_TRUE(t.isString());
}

TEST(DataTypeTest, FromVariantFloat)
{
    hydrocouple_variant v = 2.71f;
    DataType t = DataType::fromVariant(v);
    EXPECT_EQ(t.baseType(), DataType::BaseType::Float);
    EXPECT_TRUE(t.isFloating());
}

TEST(DataTypeTest, FromVariantLong)
{
    hydrocouple_variant v = 100L;
    DataType t = DataType::fromVariant(v);
    EXPECT_EQ(t.baseType(), DataType::BaseType::Long);
    EXPECT_TRUE(t.isInteger());
}

TEST(DataTypeTest, FromVariantChar)
{
    hydrocouple_variant v = 'c';
    DataType t = DataType::fromVariant(v);
    EXPECT_EQ(t.baseType(), DataType::BaseType::Char);
    EXPECT_TRUE(t.isInteger());
}

// ────────────────────────────────────────────────────────────────────────────
// fromTypeInfo Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(DataTypeTest, FromTypeInfoInt)
{
    DataType t = DataType::fromTypeInfo(typeid(int));
    EXPECT_EQ(t.baseType(), DataType::BaseType::Int);
}

TEST(DataTypeTest, FromTypeInfoDouble)
{
    DataType t = DataType::fromTypeInfo(typeid(double));
    EXPECT_EQ(t.baseType(), DataType::BaseType::Double);
}

TEST(DataTypeTest, FromTypeInfoString)
{
    DataType t = DataType::fromTypeInfo(typeid(std::string));
    EXPECT_EQ(t.baseType(), DataType::BaseType::String);
}

TEST(DataTypeTest, FromTypeInfoBool)
{
    DataType t = DataType::fromTypeInfo(typeid(bool));
    EXPECT_EQ(t.baseType(), DataType::BaseType::Bool);
}

TEST(DataTypeTest, FromTypeInfoUnknown)
{
    struct CustomType {};
    DataType t = DataType::fromTypeInfo(typeid(CustomType));
    EXPECT_EQ(t.baseType(), DataType::BaseType::Unknown);
}

// ────────────────────────────────────────────────────────────────────────────
// Equality Operators
// ────────────────────────────────────────────────────────────────────────────

TEST(DataTypeTest, EqualityOperator)
{
    DataType t1(DataType::BaseType::Int);
    DataType t2(DataType::BaseType::Int);
    DataType t3(DataType::BaseType::Double);

    EXPECT_TRUE(t1 == t2);
    EXPECT_FALSE(t1 == t3);
    EXPECT_FALSE(t1 != t2);
    EXPECT_TRUE(t1 != t3);
}

// ────────────────────────────────────────────────────────────────────────────
// Round-trip Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(DataTypeTest, VariantRoundTrip)
{
    // Create variant with int
    hydrocouple_variant v = 42;

    // Detect type
    DataType t = DataType::fromVariant(v);

    // Should be int
    EXPECT_EQ(t.baseType(), DataType::BaseType::Int);
    EXPECT_EQ(t.name(), "int");
    EXPECT_EQ(t.typeInfo(), typeid(int));
}

TEST(DataTypeTest, TypeInfoRoundTrip)
{
    // Start with type info
    const std::type_info& info = typeid(double);

    // Create DataType
    DataType t = DataType::fromTypeInfo(info);

    // Should reconstruct
    EXPECT_EQ(t.baseType(), DataType::BaseType::Double);
    EXPECT_EQ(t.typeInfo(), info);
}

TEST(DataTypeTest, AllTypesRoundTrip)
{
    // Test all 14 types
    std::vector<DataType::BaseType> allTypes = {
        DataType::BaseType::Bool,       DataType::BaseType::Char,      DataType::BaseType::Short,
        DataType::BaseType::Int,        DataType::BaseType::Long,      DataType::BaseType::UChar,
        DataType::BaseType::UShort,     DataType::BaseType::UInt,      DataType::BaseType::ULong,
        DataType::BaseType::Float,      DataType::BaseType::Double,    DataType::BaseType::LongDouble,
        DataType::BaseType::String,     DataType::BaseType::VoidPtr};

    for (auto baseType : allTypes) {
        DataType t(baseType);

        // Should have non-empty name
        EXPECT_FALSE(t.name().empty());

        // Should be able to get type info
        const std::type_info& info = t.typeInfo();
        (void)info; // Use to avoid unused variable warning

        // Should reconstruct from type info
        DataType reconstructed = DataType::fromTypeInfo(t.typeInfo());
        EXPECT_EQ(reconstructed, t);
    }
}
