// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 7: 1-D and 2-D exchange items, bulk reads with
 *         BufferDescriptor (contiguous, strided, any rank), the refusals,
 *         value-changed events, and a provider feeding a consumer.
 */

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/signal.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/data/exchangeitems2d.h"

#include <hydrocouplehelpers.h>

#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;

namespace {

int failures = 0;

void check(bool ok, const std::string &what)
{
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok)
        ++failures;
}

std::string join(const double *values, size_t n)
{
    std::string text;
    for (size_t i = 0; i < n; ++i)
    {
        char buffer[32];
        std::snprintf(buffer, sizeof buffer, "%s%g", i ? " " : "", values[i]);
        text += buffer;
    }
    return text;
}

} // namespace

int main()
{
    //! [setup]
    // Dimensions and value definitions are borrowed by the items, so they are
    // declared first and outlive them. A Quantity does not delete its Unit:
    // the unique_ptr on unit() below does.
    Dimension reaches("reaches", "Reaches"); // id "reaches" is not a known role
    std::unique_ptr<Quantity> flow(Quantity::flowInCMS());
    std::unique_ptr<IUnit> flowUnit(flow->unit());

    Output1DDouble runoff("runoff", &reaches, 5, flow.get(), nullptr);
    for (int i = 0; i < runoff.length(); ++i)
        runoff[i] = 10.0 * (i + 1); // direct storage access: no event
    std::printf("runoff: rank %zu, extent %lld, kind Float64 %s\n",
                runoff.shape().size(),
                static_cast<long long>(runoff.shape()[0]),
                runoff.dataKind() == DataKind::Float64 ? "yes" : "no");
    //! [setup]
    check(runoff.shape() == std::vector<int64_t>{5}, "Output1D shape is {5}");

    //! [bulk_read]
    // Values 1..3 (count 3 from start 1) into a C-contiguous host buffer.
    std::array<double, 3> slice{};
    const std::array<int64_t, 1> sliceShape{3};
    BufferDescriptor destination;
    destination.data  = slice.data();
    destination.kind  = DataKind::Float64;
    destination.rank  = 1;
    destination.shape = sliceShape.data(); // stridesBytes null: contiguous
    const std::array<int64_t, 1> start{1}, count{3};
    std::string message;
    const bool read = runoff.getValuesInto(destination, start, count, &message);
    std::printf("read values 1..3: %s\n", join(slice.data(), slice.size()).c_str());
    //! [bulk_read]
    check(read && slice == std::array<double, 3>{20, 30, 40}, "contiguous slice read");

    //! [strided_read]
    // The same three values into every other slot of a six-element array:
    // the destination says so with byte strides.
    std::array<double, 6> spread{};
    const int64_t stride = 2 * static_cast<int64_t>(sizeof(double));
    BufferDescriptor strided = destination;
    strided.data         = spread.data();
    strided.stridesBytes = &stride;
    const bool readStrided = runoff.getValuesInto(strided, start, count, &message);
    std::printf("strided into 6 slots: %s\n", join(spread.data(), spread.size()).c_str());
    //! [strided_read]
    check(readStrided && spread == std::array<double, 6>{20, 0, 30, 0, 40, 0},
          "strided destination read");

    //! [typed_helpers]
    // hydrocouplehelpers.h wraps the same call for flat spans and scalars.
    double third = 0.0;
    const std::array<int64_t, 1> index{2};
    const bool scalar = Helpers::getValue(runoff, third, index);
    std::vector<double> all(5);
    const std::array<int64_t, 1> zero{0}, five{5};
    const bool whole = Helpers::getValues<double>(runoff, all, zero, five);
    std::printf("Helpers::getValue at 2: %g; Helpers::getValues: %s\n", third,
                join(all.data(), all.size()).c_str());
    //! [typed_helpers]
    check(scalar && third == 30.0 && whole && all[4] == 50.0, "typed helpers");

    //! [two_d]
    // A 3 x 4 field, row-major: element (i, j) is i * 10 + j.
    Dimension rows("rows", "Rows"), columns("columns", "Columns");
    std::unique_ptr<Quantity> depth(Quantity::lengthInMeters());
    std::unique_ptr<IUnit> depthUnit(depth->unit());
    Output2DDouble field("depth", &rows, &columns, 3, 4, depth.get(), nullptr);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            field(i, j) = i * 10.0 + j;

    // A 2 x 2 interior block into a rank-2 buffer...
    std::array<double, 4> block{};
    const std::array<int64_t, 2> blockShape{2, 2}, blockStart{1, 1}, blockCount{2, 2};
    const bool readBlock = field.getValuesInto(
        Helpers::makeContiguous(block.data(), DataKind::Float64, blockShape),
        blockStart, blockCount, &message);
    std::printf("block rows 1-2, columns 1-2: %s\n", join(block.data(), 4).c_str());

    // ...and column 3 into a rank-1 buffer: only the element count must match.
    std::array<double, 3> column{};
    const std::array<int64_t, 1> columnShape{3};
    const std::array<int64_t, 2> columnStart{0, 3}, columnCount{3, 1};
    const bool readColumn = field.getValuesInto(
        Helpers::makeContiguous(column.data(), DataKind::Float64, columnShape),
        columnStart, columnCount, &message);
    std::printf("column 3: %s\n", join(column.data(), 3).c_str());
    //! [two_d]
    check(readBlock && block == std::array<double, 4>{11, 12, 21, 22}, "2-D block read");
    check(readColumn && column == std::array<double, 3>{3, 13, 23}, "2-D column read");

    //! [refusals]
    // Each refusal returns false, leaves the buffer alone and says why.
    std::array<float, 3> wrongKind{};
    std::string kindMessage, boundsMessage, countMessage;
    const bool a = runoff.getValuesInto(
        Helpers::makeContiguous(wrongKind.data(), DataKind::Float32, sliceShape),
        start, count, &kindMessage);
    const std::array<int64_t, 1> late{4};
    const bool b = runoff.getValuesInto(destination, late, count, &boundsMessage);
    const std::array<int64_t, 1> two{2};
    const bool c = runoff.getValuesInto(destination, start, two, &countMessage);
    std::printf("Float32 buffer: %s\n", kindMessage.c_str());
    std::printf("start 4, count 3: %s\n", boundsMessage.c_str());
    std::printf("count 2 into 3 slots: %s\n", countMessage.c_str());
    //! [refusals]
    check(!a && kindMessage == "kind mismatch", "kind refusal");
    check(!b && boundsMessage == "selection out of bounds", "bounds refusal");
    check(!c && countMessage == "buffer element count mismatch", "count refusal");

    //! [events]
    // A slot is held weakly: keep the shared_ptr alive while listening.
    using Event = const std::shared_ptr<IComponentDataItemValueChanged> &;
    std::vector<int64_t> changedStart, changedCount;
    auto slot = std::make_shared<FunctionSlot<Event>>(
        [&](const ISignal<Event> &, Event event) {
            changedStart = event->start();
            changedCount = event->count();
        });
    runoff.connect(slot);

    const std::array<double, 2> update{99.0, 98.0};
    const std::array<int64_t, 1> updateShape{2}, updateStart{2};
    const bool wrote = runoff.setValuesFrom(
        Helpers::makeContiguous(const_cast<double *>(update.data()),
                                DataKind::Float64, updateShape),
        updateStart, updateShape, &message);
    std::printf("setValuesFrom fired: start %lld, count %lld\n",
                static_cast<long long>(changedStart.at(0)),
                static_cast<long long>(changedCount.at(0)));
    runoff.disconnect(slot);
    //! [events]
    check(wrote && changedStart == std::vector<int64_t>{2} &&
              changedCount == std::vector<int64_t>{2},
          "value-changed event carries the written box");

    //! [exchange]
    // The provider wires the connection; the consumer then pulls.
    Input1DDouble inflow("inflow", &reaches, 5, flow.get(), nullptr);
    runoff.addConsumer(&inflow); // canConsume() then setProvider(&runoff)
    inflow.retrieveValuesFromProvider();
    std::printf("inflow after retrieve: %s\n",
                join(inflow.rawData().data(), inflow.rawData().size()).c_str());

    // A consumer that refuses the output: addConsumer() throws.
    Input1DInt counts("counts", &reaches, 5, flow.get(), nullptr);
    std::string refusal;
    try
    {
        runoff.addConsumer(&counts);
    }
    catch (const std::invalid_argument &error)
    {
        refusal = error.what();
    }
    std::printf("refused: %s\n", refusal.c_str());

    const bool removed = runoff.removeConsumer(&inflow); // setProvider(nullptr)
    //! [exchange]
    check(inflow.rawData() == std::vector<double>{10, 20, 99, 98, 50},
          "consumer pulled the provider's values");
    check(refusal == "Output \"runoff\" cannot be consumed by input \"counts\": "
                     "Provider data kind mismatch",
          "kind mismatch refused at wiring time");
    check(removed && inflow.provider() == nullptr, "disconnect clears the provider");

    std::printf("%s\n", failures ? "ch07_exchange_items FAILED" : "ch07_exchange_items passed");
    return failures ? 1 : 0;
}
