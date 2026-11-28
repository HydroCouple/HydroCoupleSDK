// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 8: the relaxation adapter, a stateful adapted output. It
 *         blends successive refreshes, reports its state, saves and
 *         restores it, and reports the derivative of its last refresh.
 */

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/data/relaxationadaptedoutput.h"
#include "hydrocouplesdk/data/sdkadaptedoutputfactory.h"
#include "hydrocouplesdk/differentiation/differentials.h"

#include <hydrocouplehelpers.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using AT = IArgument::ArgumentInputType;

namespace {

int failures = 0;

void check(bool ok, const std::string &what)
{
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok)
        ++failures;
}

bool near(double a, double b) { return std::abs(a - b) < 1e-12 * (1.0 + std::abs(b)); }

std::vector<double> readAll(const IComponentDataItem &item)
{
    const std::vector<int64_t> shape = item.shape();
    std::vector<double> values(static_cast<size_t>(shape.at(0)));
    const std::array<int64_t, 1> zero{0};
    if (!Helpers::getValues<double>(item, values, zero, shape))
        return {};
    return values;
}

std::string text(const std::vector<double> &values)
{
    std::string out;
    for (double value : values)
    {
        char buffer[32];
        std::snprintf(buffer, sizeof buffer, "%s%g", out.empty() ? "" : " ", value);
        out += buffer;
    }
    return out;
}

} // namespace

int main()
{
    Dimension cells("cells", "Cells");
    std::unique_ptr<Quantity> stageQuantity(Quantity::lengthInMeters());
    std::unique_ptr<IUnit> stageUnit(stageQuantity->unit());
    Output1DDouble stage("stage", &cells, 2, stageQuantity.get(), nullptr);
    stage[0] = 1.0;
    stage[1] = 2.0;

    //! [create]
    SdkAdaptedOutputFactory *factory = SdkAdaptedOutputFactory::instance();
    IIdentity *offering = nullptr;
    for (IIdentity *candidate : factory->getAvailableAdaptedOutputIds(&stage))
        if (candidate->id() == SdkAdaptedOutputFactory::kRelaxationId)
            offering = candidate;
    std::unique_ptr<IAdaptedOutput> adapter = factory->createAdaptedOutput(offering, &stage);

    std::string message;
    for (IArgument *argument : adapter->arguments())
        if (argument->id() == "relaxation_factor")
            (void)argument->initialize("[0.5]", AT::JSON, message);
    adapter->initialize(); // no history: the first values pass through
    std::printf("after initialize: %s\n", text(readAll(*adapter)).c_str());
    //! [create]
    check(readAll(*adapter) == std::vector<double>{1.0, 2.0}, "first refresh passes through");

    //! [blend]
    // The adaptee jumps to {3, 6} and stays there; each refresh moves half
    // of the remaining way: y = 0.5 y_prev + 0.5 x.
    stage[0] = 3.0;
    stage[1] = 6.0;
    std::vector<std::vector<double>> history;
    for (int k = 0; k < 3; ++k)
    {
        adapter->refresh();
        history.push_back(readAll(*adapter));
        std::printf("refresh %d: %s\n", k + 1, text(history.back()).c_str());
    }
    //! [blend]
    check(history[0] == std::vector<double>{2.0, 4.0} &&
              history[2] == std::vector<double>{2.75, 5.5},
          "each refresh blends once with factor 0.5");

    //! [state]
    const std::vector<IComponentDataItem *> states = adapter->states();
    std::printf("states(): %zu item, the adapter itself: %s\n", states.size(),
                states.at(0) == static_cast<IComponentDataItem *>(adapter.get()) ? "yes" : "no");
    //! [state]
    check(states.size() == 1, "the adapter reports one state item");

    //! [checkpoint]
    auto *checkpointable = dynamic_cast<ICheckpointableAdaptedOutput *>(adapter.get());
    std::string token;
    (void)checkpointable->saveState(token, message);
    std::printf("token: %s\n", token.c_str());
    adapter->refresh(); // moves on to {2.875, 5.75}
    const bool restored = checkpointable->restoreState(token, message);
    std::printf("restored: %s\n", text(readAll(*adapter)).c_str());

    std::string refusal;
    const bool wrong = checkpointable->restoreState("relaxation-v1 3 0 0 0", refusal);
    std::printf("three values for a two-value adaptee: %s\n", refusal.c_str());
    //! [checkpoint]
    check(restored && readAll(*adapter) == std::vector<double>{2.75, 5.5},
          "restoreState() returns the values bit for bit");
    check(!wrong && refusal == "RelaxationAdaptedOutput \"" + adapter->id() +
                                   "\": the token holds 3 values but the adaptee has a "
                                   "different shape",
          "a token of the wrong length is refused");

    //! [derivative]
    // One more refresh to linearize at: y_prev = {2.75, 5.5}, x = {3, 6}.
    adapter->refresh();
    auto *differentiable = dynamic_cast<IDifferentiableAdaptedOutput *>(adapter.get());
    const std::array<int64_t, 1> shape{2}, scalarShape{1};
    std::array<double, 2> seed{1.0, 1.0}, xbar{}, ybarPrev{};
    std::array<double, 1> lambdaBar{};
    auto entry = [](const IComponentDataItem *item, DifferentialRole role, double *data,
                    std::span<const int64_t> extent) {
        DifferentialEntry e;
        e.item = item;
        e.role = role;
        e.value = Helpers::makeContiguous(data, DataKind::Float64, extent);
        return e;
    };
    const IComponentDataItem *self = Differentiation::itemOf<IComponentDataItem>(adapter.get());
    const std::array<DifferentialEntry, 1> seeds{
        entry(self, DifferentialRole::Output, seed.data(), shape)};
    const std::array<DifferentialEntry, 3> results{
        entry(Differentiation::itemOf<IComponentDataItem>(&stage), DifferentialRole::Input,
              xbar.data(), shape),
        entry(self, DifferentialRole::StateBefore, ybarPrev.data(), shape),
        entry(Differentiation::itemOf<IComponentDataItem>(differentiable->differentiableArguments()[0]),
              DifferentialRole::Argument, lambdaBar.data(), scalarShape)};
    const bool vjp = differentiable->vjp(seeds, results, &message);
    std::printf("vjp: x_bar %g %g, y_prev_bar %g %g, lambda_bar %g\n", xbar[0], xbar[1],
                ybarPrev[0], ybarPrev[1], lambdaBar[0]);
    //! [derivative]
    // x_bar = lambda t, y_prev_bar = (1 - lambda) t, lambda_bar = sum t (x - y_prev).
    check(vjp && near(xbar[0], 0.5) && near(ybarPrev[1], 0.5) &&
              near(lambdaBar[0], (3.0 - 2.75) + (6.0 - 5.5)),
          "vjp matches the formulas");

    std::printf("%s\n", failures ? "ch08_relaxation FAILED" : "ch08_relaxation passed");
    return failures ? 1 : 0;
}
