// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the concrete typed data items: Argument1D/2D, IdBasedArgument,
// Input1D/Output1D — v2 hyperslab data plane, JSON round trips, signals.

#include <gtest/gtest.h>

#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/data/argument2d.h"
#include "hydrocouplesdk/data/idbasedargument.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/data/exchangeitems2d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/unit.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>

using namespace HydroCouple;
using namespace HydroCouple::SDK;

#ifndef TEST_ARTIFACTS_DIR
#define TEST_ARTIFACTS_DIR "."
#endif

namespace {

using AT = HydroCouple::IArgument::ArgumentInputType;

std::filesystem::path artifactPath(const std::string &name)
{
    const std::filesystem::path dir{TEST_ARTIFACTS_DIR};
    std::filesystem::create_directories(dir);
    return dir / name;
}

} // namespace

// ── Argument1D ────────────────────────────────────────────────────────────

class Argument1DTest : public ::testing::Test
{
protected:
    Dimension m_dim{"dim0", "Index"};
    std::unique_ptr<Quantity> m_quantity{Quantity::unitLess("TestQuantity")};
};

TEST_F(Argument1DTest, ShapeAndKind)
{
    Argument1DDouble arg("arg", &m_dim, 5, m_quantity.get(), nullptr);
    EXPECT_EQ(arg.shape(), (std::vector<int64_t>{5}));
    EXPECT_EQ(arg.dataKind(), DataKind::Float64);
}

TEST_F(Argument1DTest, InitializeFromJsonString)
{
    Argument1DDouble arg("arg", &m_dim, 0, m_quantity.get(), nullptr);
    std::string msg;
    ASSERT_TRUE(arg.initialize("[1.5, 2.5, 3.5]", AT::String, msg)) << msg;
    EXPECT_EQ(arg.length(), 3);
    EXPECT_DOUBLE_EQ(arg[1], 2.5);
    EXPECT_EQ(arg.currentArgumentInputType(), AT::String);
}

TEST_F(Argument1DTest, InitializeRejectsMalformedJson)
{
    Argument1DDouble arg("arg", &m_dim, 0, m_quantity.get(), nullptr);
    std::string msg;
    EXPECT_FALSE(arg.initialize("{not json", AT::String, msg));
    EXPECT_FALSE(msg.empty());
}

TEST_F(Argument1DTest, HyperslabRoundTrip)
{
    Argument1DDouble arg("arg", &m_dim, 4, m_quantity.get(), nullptr);

    const std::array<double, 4> in{10, 20, 30, 40};
    const std::array<int64_t, 1> shape{4}, zero{0};
    const auto src = detail::hostDescriptor(
        const_cast<double *>(in.data()), DataKind::Float64, shape);
    std::string msg;
    ASSERT_TRUE(arg.setValuesFrom(src, zero, shape, &msg)) << msg;

    std::array<double, 2> out{};
    const std::array<int64_t, 1> outShape{2}, start{1}, count{2};
    const auto dst = detail::hostDescriptor(out.data(), DataKind::Float64, outShape);
    ASSERT_TRUE(arg.getValuesInto(dst, start, count, &msg)) << msg;
    EXPECT_EQ(out, (std::array<double, 2>{20, 30}));
}

TEST_F(Argument1DTest, ToStringAndSerializeJson)
{
    Argument1DInt arg("arg", &m_dim, 0, m_quantity.get(), nullptr);
    std::string msg;
    ASSERT_TRUE(arg.initialize("[1, 2, 3]", AT::String, msg)) << msg;

    EXPECT_EQ(nlohmann::json::parse(arg.toString()),
              nlohmann::json::parse("[1,2,3]"));

    std::string serialized;
    ASSERT_TRUE(arg.serialize(AT::JSON, serialized, msg)) << msg;
    const auto j = nlohmann::json::parse(serialized);
    EXPECT_EQ(j["values"], nlohmann::json::parse("[1,2,3]"));
}

TEST_F(Argument1DTest, JsonWriteReadRoundTrip)
{
    Argument1DString arg("arg", &m_dim, 0, m_quantity.get(), nullptr);
    std::string msg;
    ASSERT_TRUE(arg.initialize(R"(["alpha","beta"])", AT::String, msg)) << msg;

    const auto j = arg.writeData();
    Argument1DString restored("arg2", &m_dim, 0, m_quantity.get(), nullptr);
    restored.readData(j);
    EXPECT_EQ(restored.length(), 2);
    EXPECT_EQ(restored[0], "alpha");
    EXPECT_EQ(restored[1], "beta");
}

TEST_F(Argument1DTest, InitializeFromFile)
{
    const auto path = artifactPath("argument1d_values.json");
    {
        std::ofstream f(path);
        f << "[4, 5, 6]";
    }
    Argument1DInt arg("arg", &m_dim, 0, m_quantity.get(), nullptr);
    std::string msg;
    ASSERT_TRUE(arg.initialize(path.string(), AT::File, msg)) << msg;
    EXPECT_EQ(arg.length(), 3);
    EXPECT_EQ(arg[2], 6);

    // saveData writes back to the same file.
    arg[2] = 60;
    arg.saveData();
    std::ifstream f(path);
    nlohmann::json j;
    f >> j;
    EXPECT_EQ(j[2].get<int>(), 60);
}

TEST_F(Argument1DTest, InitializeFromComponentDataItem)
{
    Output1DDouble source("src", &m_dim, 3, m_quantity.get(), nullptr);
    source[0] = 1.0; source[1] = 2.0; source[2] = 3.0;

    Argument1DDouble arg("arg", &m_dim, 0, m_quantity.get(), nullptr);
    std::string msg;
    ASSERT_TRUE(arg.initialize(source, msg)) << msg;
    EXPECT_EQ(arg.length(), 3);
    EXPECT_DOUBLE_EQ(arg[2], 3.0);
}

TEST_F(Argument1DTest, ValueChangedSignalCarriesSlab)
{
    Argument1DDouble arg("arg", &m_dim, 4, m_quantity.get(), nullptr);

    std::vector<int64_t> gotStart, gotCount;
    using EventPtr = const std::shared_ptr<IComponentDataItemValueChanged> &;
    auto slot = std::make_shared<FunctionSlot<EventPtr>>(
        [&](const ISignal<EventPtr> &, EventPtr args) {
            gotStart = args->start();
            gotCount = args->count();
        });
    arg.connect(slot);

    const std::array<double, 2> in{7, 8};
    const std::array<int64_t, 1> inShape{2}, start{1}, count{2};
    const auto src = detail::hostDescriptor(
        const_cast<double *>(in.data()), DataKind::Float64, inShape);
    ASSERT_TRUE(arg.setValuesFrom(src, start, count, nullptr));

    EXPECT_EQ(gotStart, (std::vector<int64_t>{1}));
    EXPECT_EQ(gotCount, (std::vector<int64_t>{2}));
}

// ── Argument2D ────────────────────────────────────────────────────────────

class Argument2DTest : public ::testing::Test
{
protected:
    Dimension m_dimX{"x", "Rows"};
    Dimension m_dimY{"y", "Cols"};
    std::unique_ptr<Quantity> m_quantity{Quantity::unitLess("TestQuantity")};
};

TEST_F(Argument2DTest, InitializeFromNestedJson)
{
    Argument2DDouble arg("arg", &m_dimX, &m_dimY, 0, 0, m_quantity.get(), nullptr);
    std::string msg;
    ASSERT_TRUE(arg.initialize("[[1,2,3],[4,5,6]]", AT::String, msg)) << msg;
    EXPECT_EQ(arg.shape(), (std::vector<int64_t>{2, 3}));
    EXPECT_DOUBLE_EQ(arg(1, 2), 6.0);
}

TEST_F(Argument2DTest, RejectsRaggedRows)
{
    Argument2DDouble arg("arg", &m_dimX, &m_dimY, 0, 0, m_quantity.get(), nullptr);
    std::string msg;
    EXPECT_FALSE(arg.initialize("[[1,2],[3]]", AT::String, msg));
    EXPECT_FALSE(msg.empty());
}

TEST_F(Argument2DTest, HyperslabInteriorBlock)
{
    Argument2DInt arg("arg", &m_dimX, &m_dimY, 3, 4, m_quantity.get(), nullptr);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            arg(i, j) = i * 10 + j;

    std::array<int, 4> out{};
    const std::array<int64_t, 2> outShape{2, 2}, start{1, 1}, count{2, 2};
    const auto dst = detail::hostDescriptor(out.data(), DataKind::Int32, outShape);
    std::string msg;
    ASSERT_TRUE(arg.getValuesInto(dst, start, count, &msg)) << msg;
    EXPECT_EQ(out, (std::array<int, 4>{11, 12, 21, 22}));
}

TEST_F(Argument2DTest, JsonRoundTripPreservesShape)
{
    Argument2DString arg("arg", &m_dimX, &m_dimY, 0, 0, m_quantity.get(), nullptr);
    std::string msg;
    ASSERT_TRUE(arg.initialize(R"([["a","b"],["c","d"]])", AT::String, msg)) << msg;

    const auto j = arg.writeData();
    Argument2DString restored("arg2", &m_dimX, &m_dimY, 0, 0, m_quantity.get(), nullptr);
    restored.readData(j);
    EXPECT_EQ(restored.shape(), (std::vector<int64_t>{2, 2}));
    EXPECT_EQ(restored(1, 0), "c");
}

// ── IdBasedArgument ───────────────────────────────────────────────────────

class IdBasedArgumentTest : public ::testing::Test
{
protected:
    Dimension m_idDim{"ids", "Identifiers"};
    std::unique_ptr<Quantity> m_quantity{Quantity::unitLess("TestQuantity")};
};

TEST_F(IdBasedArgumentTest, IdentifiersAndShape)
{
    IdBasedArgumentDouble arg("arg", {"nodeA", "nodeB"}, &m_idDim,
                              m_quantity.get(), nullptr);
    EXPECT_EQ(arg.identifiers(), (std::vector<std::string>{"nodeA", "nodeB"}));
    EXPECT_EQ(arg.shape(), (std::vector<int64_t>{2}));
    EXPECT_EQ(arg.identifierDimension(), &m_idDim);
}

TEST_F(IdBasedArgumentTest, InitializeFromIdValueObject)
{
    IdBasedArgumentDouble arg("arg", {}, &m_idDim, m_quantity.get(), nullptr);
    std::string msg;
    ASSERT_TRUE(arg.initialize(R"({"nodeA": 1.5, "nodeB": 2.5})", AT::String, msg))
        << msg;
    EXPECT_EQ(arg.shape(), (std::vector<int64_t>{2}));
    EXPECT_DOUBLE_EQ(arg.valueAt("nodeA"), 1.5);
    EXPECT_DOUBLE_EQ(arg.valueAt("nodeB"), 2.5);
}

TEST_F(IdBasedArgumentTest, InitializeFromParallelArrays)
{
    IdBasedArgumentInt arg("arg", {}, &m_idDim, m_quantity.get(), nullptr);
    std::string msg;
    ASSERT_TRUE(arg.initialize(
        R"({"identifiers": ["a", "b", "c"], "values": [1, 2, 3]})", AT::String,
        msg)) << msg;
    EXPECT_EQ(arg.valueAt("c"), 3);
}

TEST_F(IdBasedArgumentTest, HyperslabReadByIdIndex)
{
    IdBasedArgumentDouble arg("arg", {"a", "b", "c"}, &m_idDim,
                              m_quantity.get(), nullptr);
    arg.valueAt("a") = 1; arg.valueAt("b") = 2; arg.valueAt("c") = 3;

    std::array<double, 2> out{};
    const std::array<int64_t, 1> outShape{2}, start{1}, count{2};
    const auto dst = detail::hostDescriptor(out.data(), DataKind::Float64, outShape);
    std::string msg;
    ASSERT_TRUE(arg.getValuesInto(dst, start, count, &msg)) << msg;
    EXPECT_EQ(out, (std::array<double, 2>{2, 3}));
}

TEST_F(IdBasedArgumentTest, InitializeFromAnotherIdBasedItem)
{
    IdBasedArgumentDouble src("src", {"x", "y"}, &m_idDim, m_quantity.get(),
                              nullptr);
    src.valueAt("x") = 5; src.valueAt("y") = 6;

    IdBasedArgumentDouble arg("arg", {}, &m_idDim, m_quantity.get(), nullptr);
    std::string msg;
    ASSERT_TRUE(arg.initialize(src, msg)) << msg;
    EXPECT_EQ(arg.identifiers(), (std::vector<std::string>{"x", "y"}));
    EXPECT_DOUBLE_EQ(arg.valueAt("y"), 6);
}

// ── Exchange items ────────────────────────────────────────────────────────

class ExchangeItemTest : public ::testing::Test
{
protected:
    Dimension m_dim{"dim0", "Index"};
    std::unique_ptr<Quantity> m_quantity{Quantity::unitLess("TestQuantity")};
};

TEST_F(ExchangeItemTest, InputCanConsumeMatchingProvider)
{
    Input1DDouble input("in", &m_dim, 4, m_quantity.get(), nullptr);
    Output1DDouble output("out", &m_dim, 4, m_quantity.get(), nullptr);
    std::string msg;
    EXPECT_TRUE(input.canConsume(&output, msg)) << msg;
}

TEST_F(ExchangeItemTest, InputRejectsKindMismatch)
{
    Input1DDouble input("in", &m_dim, 4, m_quantity.get(), nullptr);
    Output1DInt output("out", &m_dim, 4, m_quantity.get(), nullptr);
    std::string msg;
    EXPECT_FALSE(input.canConsume(&output, msg));
    EXPECT_EQ(msg, "Provider data kind mismatch");
}

TEST_F(ExchangeItemTest, InputPullsValuesFromProvider)
{
    Input1DDouble input("in", &m_dim, 3, m_quantity.get(), nullptr);
    Output1DDouble output("out", &m_dim, 3, m_quantity.get(), nullptr);
    output[0] = 1.0; output[1] = 2.0; output[2] = 3.0;

    ASSERT_TRUE(input.setProvider(&output));
    input.retrieveValuesFromProvider();
    EXPECT_DOUBLE_EQ(input[0], 1.0);
    EXPECT_DOUBLE_EQ(input[2], 3.0);
}

TEST_F(ExchangeItemTest, Input2DPullsOverlappingBlock)
{
    Dimension dimY{"y", "Cols"};
    Input2DDouble input("in", &m_dim, &dimY, 2, 2, m_quantity.get(), nullptr);
    Output2DDouble output("out", &m_dim, &dimY, 3, 3, m_quantity.get(), nullptr);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            output(i, j) = i * 3.0 + j;

    ASSERT_TRUE(input.setProvider(&output));
    input.retrieveValuesFromProvider();
    EXPECT_DOUBLE_EQ(input(0, 0), 0.0);
    EXPECT_DOUBLE_EQ(input(1, 1), 4.0);
}

TEST_F(ExchangeItemTest, OutputJsonRoundTrip)
{
    Output1DInt output("out", &m_dim, 3, m_quantity.get(), nullptr);
    output[0] = 9; output[1] = 8; output[2] = 7;

    const auto j = output.writeData();
    Output1DInt restored("out2", &m_dim, 0, m_quantity.get(), nullptr);
    restored.readData(j);
    EXPECT_EQ(restored.length(), 3);
    EXPECT_EQ(restored[1], 8);
}

// ── Vocabulary binding (ABI 5) ────────────────────────────────────────────

// A quantity's standard name and concept URI, and a unit's UDUNITS/UCUM codes,
// survive a copy and a JSON round trip; the stock units declare their codes.
TEST(VocabularyBindingTest, NamesAndCodesSurviveCopiesAndJson)
{
    // Quantity borrows its unit, so the test owns each one.
    std::unique_ptr<Unit> cfs(Unit::flowInCFS());
    Quantity flow("Flow", typeid(double), cfs.get());
    flow.setStandardName("water_volume_transport_in_river_channel");
    flow.setConceptURI("http://qudt.org/vocab/quantitykind/VolumeFlowRate");
    EXPECT_EQ(flow.unit()->symbol(), "ft3 s-1");
    EXPECT_EQ(flow.unit()->ucumCode(), "[ft_i]3/s");

    std::unique_ptr<Quantity> copied(Quantity::copy(&flow));
    std::unique_ptr<HydroCouple::IUnit> copiedUnit(copied->unit());
    EXPECT_EQ(copied->standardName(), flow.standardName());
    EXPECT_EQ(copied->conceptURI(), flow.conceptURI());
    EXPECT_EQ(copied->unit()->symbol(), "ft3 s-1");
    EXPECT_EQ(copied->unit()->ucumCode(), "[ft_i]3/s");

    const nlohmann::json written = flow.toJson();
    EXPECT_EQ(written["unit"]["symbol"], "ft3 s-1");
    std::unique_ptr<Unit> readUnit(Unit::flowInCFS());
    Quantity read("", typeid(double), readUnit.get());
    read.fromJson(written);
    EXPECT_EQ(read.standardName(), "water_volume_transport_in_river_channel");
    EXPECT_EQ(read.conceptURI(), "http://qudt.org/vocab/quantitykind/VolumeFlowRate");

    // Undeclared names stay out of the document, which reads as before.
    std::unique_ptr<Unit> cms(Unit::flowInCMS());
    Quantity plain("Plain", typeid(double), cms.get());
    EXPECT_FALSE(plain.toJson().contains("standardName"));
    EXPECT_FALSE(plain.toJson().contains("conceptURI"));
}

// The ready-made quantities declare their QUDT quantity kind, which is what a
// generic "Flow (m³/s)" honestly knows; CMS and CFS flows therefore match.
TEST(VocabularyBindingTest, FactoryQuantitiesDeclareTheirQuantityKind)
{
    const std::string qudt = "http://qudt.org/vocab/quantitykind/";
    const std::pair<Quantity *(*)(), const char *> factories[] = {
        {&Quantity::flowInCMS, "VolumeFlowRate"},  {&Quantity::flowInCFS, "VolumeFlowRate"},
        {&Quantity::lengthInMeters, "Length"},     {&Quantity::lengthInFeet, "Length"},
        {&Quantity::areaInSquareMeters, "Area"},   {&Quantity::volumeInCubicMeters, "Volume"},
        {&Quantity::timeInDays, "Time"},           {&Quantity::timeInSeconds, "Time"},
    };
    for (const auto &[make, kind] : factories)
    {
        std::unique_ptr<Quantity> quantity(make());
        std::unique_ptr<HydroCouple::IUnit> unit(quantity->unit()); // borrowed by the quantity
        EXPECT_EQ(quantity->conceptURI(), qudt + kind) << quantity->caption();
        EXPECT_TRUE(quantity->standardName().empty()) << quantity->caption();
        EXPECT_FALSE(unit->symbol().empty()) << quantity->caption();
        EXPECT_FALSE(unit->ucumCode().empty()) << quantity->caption();
    }
}

// ── Output consumers ──────────────────────────────────────────────────────

// An input's id() is unique only within its component, so the output must
// tell two components' "inflow" inputs apart. Keyed by id(), the second
// silently replaced the first.
TEST(OutputConsumersTest, InputsSharingAnIdFromDifferentComponentsBothConsume)
{
    std::unique_ptr<Unit> unit(Unit::flowInCMS());
    Quantity quantity("q", typeid(double), unit.get());
    Dimension dimension("i", "Index");
    Output1DDouble out("out", &dimension, 3, &quantity, nullptr);
    Input1DDouble first("inflow", &dimension, 3, &quantity, nullptr);
    Input1DDouble second("inflow", &dimension, 3, &quantity, nullptr);

    out.addConsumer(&first);
    out.addConsumer(&second);
    out.addConsumer(&first); // already connected: no duplicate
    const auto consumers = out.consumers();
    ASSERT_EQ(consumers.size(), 2u);
    EXPECT_EQ(consumers[0], static_cast<HydroCouple::IInput *>(&first));
    EXPECT_EQ(consumers[1], static_cast<HydroCouple::IInput *>(&second));

    EXPECT_TRUE(out.removeConsumer(&first));
    EXPECT_EQ(first.provider(), nullptr);
    EXPECT_EQ(second.provider(), static_cast<HydroCouple::IOutput *>(&out));
    ASSERT_EQ(out.consumers().size(), 1u);
    EXPECT_EQ(out.consumers()[0], static_cast<HydroCouple::IInput *>(&second));
}

// ── UnitDimensions ────────────────────────────────────────────────────────

// Every fundamental dimension the standard names has its own slot: set, read
// back, copied, and untouched by its neighbors. PlaneAngle was added to the
// standard after the storage was sized, and for a while read and wrote one
// element past the end of it.
TEST(UnitDimensionsTest, HoldEveryFundamentalDimension)
{
    using D = HydroCouple::IUnitDimensions::FundamentalUnitDimension;
    const D all[] = {D::Length,           D::Mass,
                     D::Time,             D::ElectricCurrent,
                     D::Temperature,      D::AmountOfSubstance,
                     D::LuminousIntensity, D::Currency,
                     D::Unitless,         D::PlaneAngle};

    UnitDimensions fresh("fresh");
    for (D axis : all)
        EXPECT_EQ(fresh.power(axis), 0.0) << static_cast<int>(axis);

    UnitDimensions dims("all");
    for (size_t i = 0; i < std::size(all); ++i)
        dims.setPower(all[i], static_cast<double>(i + 1));

    std::unique_ptr<UnitDimensions> copied(UnitDimensions::copy(&dims));
    for (size_t i = 0; i < std::size(all); ++i)
    {
        EXPECT_EQ(dims.power(all[i]), static_cast<double>(i + 1)) << i;
        EXPECT_EQ(copied->power(all[i]), static_cast<double>(i + 1)) << i;
    }
}
