// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include <gtest/gtest.h>
#include "hydrocouple/safevariant.h"

using namespace HydroCouple;

// ────────────────────────────────────────────────────────────────────────────
// getSafeValue Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(SafeVariantTest, GetSafeValueMatch_Int)
{
    hydrocouple_variant v = 42;
    auto result = getSafeValue<int>(v);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), 42);
}

TEST(SafeVariantTest, GetSafeValueMatch_Double)
{
    hydrocouple_variant v = 3.14;
    auto result = getSafeValue<double>(v);
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result.value(), 3.14);
}

TEST(SafeVariantTest, GetSafeValueMatch_String)
{
    hydrocouple_variant v = std::string("hello");
    auto result = getSafeValue<std::string>(v);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "hello");
}

TEST(SafeVariantTest, GetSafeValueMismatch_NoThrow)
{
    hydrocouple_variant v = 42;

    // Should NOT throw, should return empty optional
    EXPECT_NO_THROW({
        auto result = getSafeValue<double>(v);
        EXPECT_FALSE(result);
    });
}

TEST(SafeVariantTest, GetSafeValueMismatch_IntToString)
{
    hydrocouple_variant v = 42;
    auto result = getSafeValue<std::string>(v);
    EXPECT_FALSE(result);  // No automatic conversion in getSafeValue
}

// ────────────────────────────────────────────────────────────────────────────
// VariantConverter::convert Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(VariantConverterTest, ConvertIdenticalInt)
{
    hydrocouple_variant v = 42;
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::Int));
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<int>(result.value()), 42);
}

TEST(VariantConverterTest, ConvertIdenticalDouble)
{
    hydrocouple_variant v = 3.14;
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::Double));
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(std::get<double>(result.value()), 3.14);
}

TEST(VariantConverterTest, ConvertIntToDouble)
{
    hydrocouple_variant v = 42;
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::Double));
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(std::get<double>(result.value()), 42.0);
}

TEST(VariantConverterTest, ConvertDoubleToInt)
{
    hydrocouple_variant v = 3.14;
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::Int));
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<int>(result.value()), 3);  // Truncated
}

TEST(VariantConverterTest, ConvertFloatToDouble)
{
    hydrocouple_variant v = 2.71f;
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::Double));
    ASSERT_TRUE(result);
    // Float precision loss expected
    EXPECT_NEAR(std::get<double>(result.value()), 2.71, 0.01);
}

TEST(VariantConverterTest, ConvertIntToString)
{
    hydrocouple_variant v = 42;
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::String));
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<std::string>(result.value()), "42");
}

TEST(VariantConverterTest, ConvertDoubleToString)
{
    hydrocouple_variant v = 3.14;
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::String));
    ASSERT_TRUE(result);
    std::string str = std::get<std::string>(result.value());
    EXPECT_EQ(str.substr(0, 4), "3.14");  // Allow precision variation
}

TEST(VariantConverterTest, ConvertStringToString)
{
    hydrocouple_variant v = std::string("hello");
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::String));
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<std::string>(result.value()), "hello");
}

TEST(VariantConverterTest, ConvertStringToIntFails)
{
    hydrocouple_variant v = std::string("hello");
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::Int));
    EXPECT_FALSE(result);  // Can't convert string to int
}

TEST(VariantConverterTest, ConvertStrictMode)
{
    hydrocouple_variant v = 42;

    // Same type: OK even in strict mode
    auto result1 = VariantConverter::convert(v, DataType(DataType::BaseType::Int), true);
    EXPECT_TRUE(result1);

    // Different type: fails in strict mode
    auto result2 = VariantConverter::convert(v, DataType(DataType::BaseType::Double), true);
    EXPECT_FALSE(result2);
}

// ────────────────────────────────────────────────────────────────────────────
// VariantConverter::commonType Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(VariantConverterTest, CommonTypeSameType)
{
    hydrocouple_variant a = 42;
    hydrocouple_variant b = 99;
    DataType common = VariantConverter::commonType(a, b);
    EXPECT_EQ(common.baseType(), DataType::BaseType::Int);
}

TEST(VariantConverterTest, CommonTypeIntAndDouble)
{
    hydrocouple_variant a = 42;
    hydrocouple_variant b = 3.14;
    DataType common = VariantConverter::commonType(a, b);
    EXPECT_TRUE(common.isFloating());
    EXPECT_EQ(common.baseType(), DataType::BaseType::Double);
}

TEST(VariantConverterTest, CommonTypeIntAndFloat)
{
    hydrocouple_variant a = 42;
    hydrocouple_variant b = 2.71f;
    DataType common = VariantConverter::commonType(a, b);
    EXPECT_TRUE(common.isFloating());
}

TEST(VariantConverterTest, CommonTypeIntAndString)
{
    hydrocouple_variant a = 42;
    hydrocouple_variant b = std::string("hello");
    DataType common = VariantConverter::commonType(a, b);
    EXPECT_TRUE(common.isString());
}

TEST(VariantConverterTest, CommonTypeDoubleAndString)
{
    hydrocouple_variant a = 3.14;
    hydrocouple_variant b = std::string("world");
    DataType common = VariantConverter::commonType(a, b);
    EXPECT_TRUE(common.isString());
}

TEST(VariantConverterTest, CommonTypeIntAndLong)
{
    hydrocouple_variant a = 42;
    hydrocouple_variant b = 100L;
    DataType common = VariantConverter::commonType(a, b);
    EXPECT_TRUE(common.isNumeric());
    // Promotion to long
    EXPECT_TRUE(common.isInteger());
}

// ────────────────────────────────────────────────────────────────────────────
// VariantConverter::convertToCommon Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(VariantConverterTest, ConvertToCommonIntAndDouble)
{
    hydrocouple_variant a = 10;
    hydrocouple_variant b = 3.14;
    hydrocouple_variant aOut, bOut;

    DataType common = VariantConverter::convertToCommon(a, b, aOut, bOut);

    EXPECT_TRUE(common.isFloating());
    EXPECT_DOUBLE_EQ(std::get<double>(aOut), 10.0);
    EXPECT_DOUBLE_EQ(std::get<double>(bOut), 3.14);
}

TEST(VariantConverterTest, ConvertToCommonIntAndInt)
{
    hydrocouple_variant a = 10;
    hydrocouple_variant b = 20;
    hydrocouple_variant aOut, bOut;

    DataType common = VariantConverter::convertToCommon(a, b, aOut, bOut);

    EXPECT_EQ(common.baseType(), DataType::BaseType::Int);
    EXPECT_EQ(std::get<int>(aOut), 10);
    EXPECT_EQ(std::get<int>(bOut), 20);
}

TEST(VariantConverterTest, ConvertToCommonIntAndString)
{
    hydrocouple_variant a = 42;
    hydrocouple_variant b = std::string("hello");
    hydrocouple_variant aOut, bOut;

    DataType common = VariantConverter::convertToCommon(a, b, aOut, bOut);

    EXPECT_TRUE(common.isString());
    EXPECT_EQ(std::get<std::string>(aOut), "42");
    EXPECT_EQ(std::get<std::string>(bOut), "hello");
}

// ────────────────────────────────────────────────────────────────────────────
// VariantConverter::toString Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(VariantConverterTest, ToStringInt)
{
    hydrocouple_variant v = 42;
    EXPECT_EQ(VariantConverter::toString(v), "42");
}

TEST(VariantConverterTest, ToStringDouble)
{
    hydrocouple_variant v = 3.14;
    std::string str = VariantConverter::toString(v);
    EXPECT_EQ(str.substr(0, 4), "3.14");
}

TEST(VariantConverterTest, ToStringString)
{
    hydrocouple_variant v = std::string("hello");
    EXPECT_EQ(VariantConverter::toString(v), "hello");
}

TEST(VariantConverterTest, ToStringBool)
{
    hydrocouple_variant v = true;
    EXPECT_EQ(VariantConverter::toString(v), "true");

    v = false;
    EXPECT_EQ(VariantConverter::toString(v), "false");
}

TEST(VariantConverterTest, ToStringFloat)
{
    hydrocouple_variant v = 2.71f;
    std::string str = VariantConverter::toString(v);
    EXPECT_EQ(str.substr(0, 4), "2.71");
}

TEST(VariantConverterTest, ToStringChar)
{
    hydrocouple_variant v = 'c';
    EXPECT_EQ(VariantConverter::toString(v), "c");
}

TEST(VariantConverterTest, ToStringLong)
{
    hydrocouple_variant v = 123456L;
    EXPECT_EQ(VariantConverter::toString(v), "123456");
}

// ────────────────────────────────────────────────────────────────────────────
// VariantConverter::typeToString Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(VariantConverterTest, TypeToStringInt)
{
    hydrocouple_variant v = 42;
    EXPECT_EQ(VariantConverter::typeToString(v), "int");
}

TEST(VariantConverterTest, TypeToStringDouble)
{
    hydrocouple_variant v = 3.14;
    EXPECT_EQ(VariantConverter::typeToString(v), "double");
}

TEST(VariantConverterTest, TypeToStringString)
{
    hydrocouple_variant v = std::string("hello");
    EXPECT_EQ(VariantConverter::typeToString(v), "string");
}

TEST(VariantConverterTest, TypeToStringBool)
{
    hydrocouple_variant v = true;
    EXPECT_EQ(VariantConverter::typeToString(v), "bool");
}

// ────────────────────────────────────────────────────────────────────────────
// Real-world Usage Patterns
// ────────────────────────────────────────────────────────────────────────────

TEST(VariantConverterTest, RealWorldPattern_AdapterTypeChecking)
{
    // Simulates an adapter checking type compatibility
    hydrocouple_variant adapteeOutput = 3.14;  // double output
    DataType expectedInputType = DataType(DataType::BaseType::Int);

    DataType adapteeType = DataType::fromVariant(adapteeOutput);
    EXPECT_TRUE(adapteeType.canConvertTo(expectedInputType));

    // Try conversion
    auto converted = VariantConverter::convert(adapteeOutput, expectedInputType);
    ASSERT_TRUE(converted);
    EXPECT_EQ(std::get<int>(converted.value()), 3);
}

TEST(VariantConverterTest, RealWorldPattern_BinaryOperation)
{
    // Simulates a binary operation (e.g., adding two values)
    hydrocouple_variant left = 10;
    hydrocouple_variant right = 3.5;

    hydrocouple_variant leftConverted, rightConverted;
    DataType common = VariantConverter::convertToCommon(left, right, leftConverted, rightConverted);

    EXPECT_TRUE(common.isFloating());

    // Now we can safely perform operation
    double leftVal = std::get<double>(leftConverted);
    double rightVal = std::get<double>(rightConverted);
    double result = leftVal + rightVal;

    EXPECT_DOUBLE_EQ(result, 13.5);
}

TEST(VariantConverterTest, RealWorldPattern_DataImport)
{
    // Simulates importing data and converting to expected type
    std::vector<hydrocouple_variant> rawData = {
        hydrocouple_variant(1),
        hydrocouple_variant(2.5),
        hydrocouple_variant(std::string("3")),
    };

    DataType targetType = DataType(DataType::BaseType::String);
    std::vector<std::string> convertedData;

    for (const auto& value : rawData) {
        auto converted = VariantConverter::convert(value, targetType);
        ASSERT_TRUE(converted);
        convertedData.push_back(std::get<std::string>(converted.value()));
    }

    EXPECT_EQ(convertedData[0], "1");
    EXPECT_EQ(convertedData[1].substr(0, 3), "2.5");
    EXPECT_EQ(convertedData[2], "3");
}

// ────────────────────────────────────────────────────────────────────────────
// Edge Cases and Error Conditions
// ────────────────────────────────────────────────────────────────────────────

TEST(VariantConverterTest, ConvertFromEmpty)
{
    // Variant in unknown state (shouldn't happen in practice)
    hydrocouple_variant v;

    // Basic creation puts it in a valid state; depends on variant implementation
    // This test ensures no crash when converting various variants
    EXPECT_NO_THROW({
        VariantConverter::toString(v);
        VariantConverter::typeToString(v);
    });
}

TEST(VariantConverterTest, LargeNumberConversion)
{
    hydrocouple_variant v = 9999999999999L;
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::Double));
    ASSERT_TRUE(result);
    EXPECT_NEAR(std::get<double>(result.value()), 9999999999999.0, 1.0);
}

TEST(VariantConverterTest, VerySmallNumberConversion)
{
    hydrocouple_variant v = 0.000000001;
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::Int));
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<int>(result.value()), 0);  // Truncates to 0
}

TEST(VariantConverterTest, NegativeNumberConversion)
{
    hydrocouple_variant v = -42;
    auto result = VariantConverter::convert(v, DataType(DataType::BaseType::Double));
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(std::get<double>(result.value()), -42.0);
}

TEST(VariantConverterTest, EmptyStringToString)
{
    hydrocouple_variant v = std::string("");
    EXPECT_EQ(VariantConverter::toString(v), "");
}

TEST(VariantConverterTest, StringWithSpecialChars)
{
    hydrocouple_variant v = std::string("hello\nworld\t!");
    EXPECT_EQ(VariantConverter::toString(v), "hello\nworld\t!");
}
