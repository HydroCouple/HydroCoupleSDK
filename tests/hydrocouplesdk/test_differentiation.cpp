// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Phase G2.1: reverse-mode differentiation of a time-stepped composition,
// in the SDK, with no LibTorch.
//
// The oracle for every gradient gate is central finite differences taken
// by re-running the composition FORWARD from scratch -- a path that never
// calls vjp() -- so an adjoint, a routing rule or a replay that is wrong
// cannot agree with it by construction.
//
// The composition under test (the fixture every gate below shares):
//
//          ┌──── LinearTransform (m, b) ───► B ──► loss
//     A ───┤
//          └───────────────────────────────► C ──► loss
//
// A, B, C are nonlinear reservoirs with a hand-written adjoint and a state.
// A's outflow reaches the loss twice: through a unit-conversion adapter
// into B, and directly into C, so its cotangent must be the SUM of what
// flows back along both connections. The loss is a misfit summed over
// eight steps, so every gradient but the adapter's also flows through
// time.
//
// Falsifiers: verification/g2/falsify.sh.

#include <gtest/gtest.h>

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/data/lineartransformadaptedoutput.h"
#include "hydrocouplesdk/data/relaxationadaptedoutput.h"
#include "hydrocouplesdk/data/sdkadaptedoutputfactory.h"
#include "hydrocouplesdk/differentiation/differentiableworkflow.h"
#include "hydrocouplesdk/differentiation/differentials.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Differentiation;

#include "reservoir_fixture.h"

using namespace hcsdk_test;

namespace {

// ============================================================================
// The composition, parameterized so finite differences can rebuild it
// ============================================================================

struct Params
{
    std::vector<double> kA{0.30, 0.20, 0.45};
    std::vector<double> kB{0.25, 0.35, 0.15};
    std::vector<double> kC{0.40, 0.10, 0.30};
    double m = 0.8;
    double b = 0.3;
    std::vector<double> sA0{6.0, 2.0, 9.0};
    std::vector<double> sB0{1.0, 4.0, 3.0};
};

constexpr int kSteps = 8;

double observed(int step, int cell) { return 1.0 + 0.1 * step + 0.2 * cell; }


struct Rig
{
    explicit Rig(const Params &p, bool adapterIsRelaxation = false)
    {
        A = std::make_unique<Reservoir>("A", true, true, /*forced=*/true);
        B = std::make_unique<Reservoir>("B");
        C = std::make_unique<Reservoir>("C");
        workflow = std::make_unique<DifferentiableWorkflow>("diff");
        std::string msg;
        EXPECT_TRUE(workflow->addModelComponent(A.get(), nullptr, &msg)) << msg;
        EXPECT_TRUE(workflow->addModelComponent(B.get(), nullptr, &msg)) << msg;
        EXPECT_TRUE(workflow->addModelComponent(C.get(), nullptr, &msg)) << msg;
        workflow->initialize();

        if (adapterIsRelaxation)
            adapter = std::make_unique<RelaxationAdaptedOutput>(
                "relax", A->out, SdkAdaptedOutputFactory::instance());
        else
            adapter = std::make_unique<LinearTransformAdaptedOutput>(
                "unit", A->out, SdkAdaptedOutputFactory::instance());
        A->out->addAdaptedOutput(adapter.get());
        connect(adapter.get(), B->in);
        connect(A->out, C->in);

        for (int i = 0; i < kCells; ++i)
        {
            (*A->k)[i] = p.kA[i];
            (*B->k)[i] = p.kB[i];
            (*C->k)[i] = p.kC[i];
            (*A->storage)[i] = p.sA0[i];
            (*B->storage)[i] = p.sB0[i];
        }
        EXPECT_TRUE(workflow->validate().empty());
        workflow->prepare();
        setArgument(*adapter, "multiplier", p.m);
        setArgument(*adapter, "offset", p.b);
        adapter->initialize();
    }

    ~Rig()
    {
        if (adapter)
            A->out->removeAdaptedOutput(adapter.get());
    }

    void step(int) { workflow->update(); }

    //! The loss's contribution at the current state (step n's outputs).
    double stepLoss(int n) const
    {
        double loss = 0.0;
        for (int i = 0; i < kCells; ++i)
        {
            const double r = (*B->out)[i] - observed(n, i);
            loss += r * r + 0.5 * (*C->out)[i] * (*C->out)[i];
        }
        return loss;
    }

    double run()
    {
        double loss = 0.0;
        for (int n = 1; n <= kSteps; ++n)
        {
            step(n);
            loss += stepLoss(n);
        }
        return loss;
    }

    //! d(loss)/d(outputs) at step n; outputs hold step n's values when the
    //! seed function runs.
    SeedFunction seeds() const
    {
        return [this](int n, Seeds &s) {
            std::vector<double> gb(kCells), gc(kCells);
            for (int i = 0; i < kCells; ++i)
            {
                gb[i] = 2.0 * ((*B->out)[i] - observed(n, i));
                gc[i] = (*C->out)[i];
            }
            EXPECT_TRUE(s.add(B->out, gb));
            EXPECT_TRUE(s.add(C->out, gc));
        };
    }

    std::unique_ptr<Reservoir> A, B, C;
    std::unique_ptr<AbstractAdaptedOutput> adapter;
    std::unique_ptr<DifferentiableWorkflow> workflow;
};

//! The loss by forward runs only.
double lossAt(const Params &p)
{
    Rig rig(p);
    return rig.run();
}

template <typename Setter>
double centralDifference(Setter set, double eps = 1e-6)
{
    Params plus, minus;
    set(plus, +eps);
    set(minus, -eps);
    return (lossAt(plus) - lossAt(minus)) / (2.0 * eps);
}

struct Differentiated
{
    explicit Differentiated(const Params &p = {}) : rig(p)
    {
        loss = rig.run();
        std::string message;
        ok = rig.workflow->backward(rig.seeds(), gradients, message);
        EXPECT_TRUE(ok) << message;
    }

    std::vector<double> argument(const IIdentity *owner, IArgument *argument) const
    {
        const ArgumentGradient *g = gradients.find(owner, argument);
        return g ? g->values : std::vector<double>();
    }

    Rig rig;
    double loss = 0.0;
    bool ok = false;
    Gradients gradients;
};

} // namespace

// ============================================================================
// The adapter's own adjoint
// ============================================================================

TEST(DifferentiationTest, TheLinearTransformAdjointMatchesFiniteDifferences)
{
    Reservoir source("source");
    source.initialize();
    source.validate();
    source.prepare();
    LinearTransformAdaptedOutput adapted("unit", source.out,
                                         SdkAdaptedOutputFactory::instance());
    setArgument(adapted, "multiplier", 1.7);
    setArgument(adapted, "offset", -0.4);
    const std::vector<double> x0{2.0, -1.0, 0.5};
    for (int i = 0; i < kCells; ++i)
        (*source.out)[i] = x0[i];
    adapted.initialize();

    const std::vector<double> ybar{0.3, -1.1, 2.0};
    std::vector<double> xbar(kCells), mbar(1), bbar(1);
    const std::vector<int64_t> shape{kCells}, scalar{1};
    auto seedEntry = [&](const IComponentDataItem *item, R role, std::vector<double> &v,
                         const std::vector<int64_t> &s) {
        DifferentialEntry e;
        e.item = item;
        e.role = role;
        e.value = hostBuffer(v, s);
        return e;
    };
    std::vector<double> ybarCopy = ybar;
    std::vector<IArgument *> args = adapted.differentiableArguments();
    ASSERT_EQ(args.size(), 2u);
    DifferentialEntry seeds[] = {seedEntry(&adapted, R::Output, ybarCopy, shape)};
    DifferentialEntry results[] = {seedEntry(itemOf(source.out), R::Input, xbar, shape),
                                   seedEntry(itemOf(args[0]), R::Argument, mbar, scalar),
                                   seedEntry(itemOf(args[1]), R::Argument, bbar, scalar)};
    std::string message;
    ASSERT_TRUE(adapted.vjp(seeds, results, &message)) << message;

    auto value = [&](const std::vector<double> &x, double m, double b) {
        double y = 0.0;
        for (int i = 0; i < kCells; ++i)
            y += ybar[i] * (m * x[i] + b);
        return y;
    };
    const double eps = 1e-6;
    for (int i = 0; i < kCells; ++i)
    {
        std::vector<double> xp = x0, xm = x0;
        xp[i] += eps;
        xm[i] -= eps;
        EXPECT_NEAR(xbar[i], (value(xp, 1.7, -0.4) - value(xm, 1.7, -0.4)) / (2 * eps), 1e-8);
    }
    EXPECT_NEAR(mbar[0], (value(x0, 1.7 + eps, -0.4) - value(x0, 1.7 - eps, -0.4)) / (2 * eps), 1e-8);
    EXPECT_NEAR(bbar[0], (value(x0, 1.7, -0.4 + eps) - value(x0, 1.7, -0.4 - eps)) / (2 * eps), 1e-8);

    // Adjointness: <ybar, J v> == <J^T ybar, v>.
    std::vector<double> vx{0.2, 0.7, -0.3}, vm{0.9}, vb{-0.6}, ydot(kCells);
    DifferentialEntry tseeds[] = {seedEntry(itemOf(source.out), R::Input, vx, shape),
                                  seedEntry(itemOf(args[0]), R::Argument, vm, scalar),
                                  seedEntry(itemOf(args[1]), R::Argument, vb, scalar)};
    DifferentialEntry tresults[] = {seedEntry(&adapted, R::Output, ydot, shape)};
    ASSERT_TRUE(adapted.jvp(tseeds, tresults, &message)) << message;
    double lhs = 0.0, rhs = mbar[0] * vm[0] + bbar[0] * vb[0];
    for (int i = 0; i < kCells; ++i)
    {
        lhs += ybar[i] * ydot[i];
        rhs += xbar[i] * vx[i];
    }
    EXPECT_NEAR(lhs, rhs, 1e-12);

    source.out->removeAdaptedOutput(&adapted);
}

TEST(DifferentiationTest, TheStatefulAdaptersDoNotClaimTheContract)
{
    Reservoir source("source");
    source.initialize();
    RelaxationAdaptedOutput relax("relax", source.out,
                                  SdkAdaptedOutputFactory::instance());
    IAdaptedOutput *adapted = &relax;   // as an engine sees it
    EXPECT_EQ(dynamic_cast<IDifferentiableAdaptedOutput *>(adapted), nullptr);
}

// ============================================================================
// The composition: every gradient against finite differences
// ============================================================================

TEST(DifferentiationTest, EveryGradientMatchesFiniteDifferences)
{
    Differentiated d;
    ASSERT_TRUE(d.ok);
    EXPECT_NEAR(d.loss, lossAt(Params{}), 1e-12);

    const auto kA = d.argument(d.rig.A.get(), d.rig.A->k);
    const auto kB = d.argument(d.rig.B.get(), d.rig.B->k);
    const auto kC = d.argument(d.rig.C.get(), d.rig.C->k);
    auto adapterArgs =
        dynamic_cast<IDifferentiableAdaptedOutput &>(*d.rig.adapter).differentiableArguments();
    const auto m = d.argument(d.rig.adapter.get(), adapterArgs[0]);
    const auto b = d.argument(d.rig.adapter.get(), adapterArgs[1]);
    const StateGradient *sA0 = d.gradients.findState(d.rig.A.get(), d.rig.A->storage.get());
    const StateGradient *sB0 = d.gradients.findState(d.rig.B.get(), d.rig.B->storage.get());
    ASSERT_EQ(kA.size(), size_t(kCells));
    ASSERT_EQ(kB.size(), size_t(kCells));
    ASSERT_EQ(kC.size(), size_t(kCells));
    ASSERT_EQ(m.size(), 1u);
    ASSERT_EQ(b.size(), 1u);
    ASSERT_NE(sA0, nullptr);
    ASSERT_NE(sB0, nullptr);

    const double tol = 1e-5;
    for (int i = 0; i < kCells; ++i)
    {
        EXPECT_NEAR(kA[i], centralDifference([i](Params &p, double e) { p.kA[i] += e; }),
                    tol * (1 + std::abs(kA[i]))) << "kA[" << i << "]";
        EXPECT_NEAR(kB[i], centralDifference([i](Params &p, double e) { p.kB[i] += e; }),
                    tol * (1 + std::abs(kB[i]))) << "kB[" << i << "]";
        EXPECT_NEAR(kC[i], centralDifference([i](Params &p, double e) { p.kC[i] += e; }),
                    tol * (1 + std::abs(kC[i]))) << "kC[" << i << "]";
        EXPECT_NEAR(sA0->values[i],
                    centralDifference([i](Params &p, double e) { p.sA0[i] += e; }),
                    tol * (1 + std::abs(sA0->values[i]))) << "sA0[" << i << "]";
        EXPECT_NEAR(sB0->values[i],
                    centralDifference([i](Params &p, double e) { p.sB0[i] += e; }),
                    tol * (1 + std::abs(sB0->values[i]))) << "sB0[" << i << "]";
    }
    EXPECT_NEAR(m[0], centralDifference([](Params &p, double e) { p.m += e; }),
                tol * (1 + std::abs(m[0]))) << "multiplier";
    EXPECT_NEAR(b[0], centralDifference([](Params &p, double e) { p.b += e; }),
                tol * (1 + std::abs(b[0]))) << "offset";
}

TEST(DifferentiationTest, ZeroIsNotAGradient)
{
    // A dropped connection reads as "no sensitivity". Every parameter here
    // reaches the loss, so every entry must be non-zero.
    Differentiated d;
    ASSERT_TRUE(d.ok);
    auto nonZero = [](const std::vector<double> &v) {
        if (v.empty())
            return false;
        for (double x : v)
            if (std::abs(x) < 1e-9)
                return false;
        return true;
    };
    auto adapterArgs =
        dynamic_cast<IDifferentiableAdaptedOutput &>(*d.rig.adapter).differentiableArguments();
    EXPECT_TRUE(nonZero(d.argument(d.rig.A.get(), d.rig.A->k))) << "kA: upstream of both paths";
    EXPECT_TRUE(nonZero(d.argument(d.rig.B.get(), d.rig.B->k)));
    EXPECT_TRUE(nonZero(d.argument(d.rig.C.get(), d.rig.C->k)));
    EXPECT_TRUE(nonZero(d.argument(d.rig.adapter.get(), adapterArgs[0])));
    EXPECT_TRUE(nonZero(d.argument(d.rig.adapter.get(), adapterArgs[1])));
    const StateGradient *sA0 = d.gradients.findState(d.rig.A.get(), d.rig.A->storage.get());
    ASSERT_NE(sA0, nullptr);
    EXPECT_TRUE(nonZero(sA0->values)) << "initial storage: reachable only through time";
}

// ============================================================================
// Replay and restoration
// ============================================================================

TEST(DifferentiationTest, EveryStepButTheLastIsReplayedOnce)
{
    Differentiated d;
    EXPECT_EQ(d.rig.workflow->recordedSteps(), kSteps);
    EXPECT_EQ(d.rig.workflow->replays(), kSteps - 1);
}

TEST(DifferentiationTest, BackwardLeavesTheCompositionWhereItFoundIt)
{
    Params p;
    Differentiated d(p);
    Rig reference(p);
    reference.run();
    for (int i = 0; i < kCells; ++i)
    {
        EXPECT_DOUBLE_EQ((*d.rig.B->out)[i], (*reference.B->out)[i]);
        EXPECT_DOUBLE_EQ((*d.rig.A->storage)[i], (*reference.A->storage)[i]);
    }
    // ...and the run continues as if backward() had never happened.
    d.rig.step(kSteps + 1);
    reference.step(kSteps + 1);
    for (int i = 0; i < kCells; ++i)
        EXPECT_DOUBLE_EQ((*d.rig.C->out)[i], (*reference.C->out)[i]);
}

TEST(DifferentiationTest, ASecondBackwardAgreesWithTheFirst)
{
    Differentiated d;
    Gradients again;
    std::string message;
    ASSERT_TRUE(d.rig.workflow->backward(d.rig.seeds(), again, message)) << message;
    EXPECT_EQ(again.find(d.rig.A.get(), d.rig.A->k)->values,
              d.gradients.find(d.rig.A.get(), d.rig.A->k)->values);
    // The last step was restored, not left linearized: it is replayed too.
    EXPECT_EQ(d.rig.workflow->replays(), kSteps);
}

// ============================================================================
// What is refused, and that the refusal names the offender
// ============================================================================

TEST(DifferentiationTest, AFeedbackLoopIsRefused)
{
    Reservoir a("a"), b("b");
    DifferentiableWorkflow workflow("loop");
    std::string msg;
    ASSERT_TRUE(workflow.addModelComponent(&a, nullptr, &msg));
    ASSERT_TRUE(workflow.addModelComponent(&b, nullptr, &msg));
    workflow.initialize();
    connect(a.out, b.in);
    connect(b.out, a.in);
    ASSERT_TRUE(workflow.validate().empty());
    workflow.prepare();
    workflow.update();
    Gradients g;
    EXPECT_FALSE(workflow.backward({}, g, msg));
    EXPECT_NE(msg.find("feedback loop"), std::string::npos) << msg;
}

TEST(DifferentiationTest, AStatefulAdapterOnTheConnectionIsRefusedByName)
{
    Rig rig(Params{}, /*adapterIsRelaxation=*/true);
    rig.run();
    Gradients g;
    std::string msg;
    EXPECT_FALSE(rig.workflow->backward(rig.seeds(), g, msg));
    EXPECT_NE(msg.find("'relax'"), std::string::npos) << msg;
    EXPECT_NE(msg.find("not differentiable"), std::string::npos) << msg;
}

TEST(DifferentiationTest, ANonDifferentiableComponentIsRefused)
{
    Reservoir a("a"), plain("plain", true, /*differentiable=*/false);
    DifferentiableWorkflow workflow("wf");
    std::string msg;
    ASSERT_TRUE(workflow.addModelComponent(&a, nullptr, &msg));
    ASSERT_TRUE(workflow.addModelComponent(&plain, nullptr, &msg));
    workflow.initialize();
    connect(a.out, plain.in);
    ASSERT_TRUE(workflow.validate().empty());
    workflow.prepare();
    workflow.update();
    Gradients g;
    EXPECT_FALSE(workflow.backward({}, g, msg));
    EXPECT_NE(msg.find("'plain' is not differentiable"), std::string::npos) << msg;
}

TEST(DifferentiationTest, AnOutputItsProviderDoesNotDifferentiateIsRefused)
{
    Reservoir a("a"), b("b");
    DifferentiableWorkflow workflow("wf");
    std::string msg;
    ASSERT_TRUE(workflow.addModelComponent(&a, nullptr, &msg));
    ASSERT_TRUE(workflow.addModelComponent(&b, nullptr, &msg));
    workflow.initialize();
    connect(a.spill, b.in);
    ASSERT_TRUE(workflow.validate().empty());
    workflow.prepare();
    workflow.update();
    Gradients g;
    EXPECT_FALSE(workflow.backward({}, g, msg));
    EXPECT_NE(msg.find("'a' does not differentiate"), std::string::npos) << msg;
}

TEST(DifferentiationTest, WithoutCheckpointingOnlyOneStepDifferentiates)
{
    Reservoir a("a", /*checkpoints=*/false), b("b", /*checkpoints=*/false);
    DifferentiableWorkflow workflow("wf");
    std::string msg;
    ASSERT_TRUE(workflow.addModelComponent(&a, nullptr, &msg));
    ASSERT_TRUE(workflow.addModelComponent(&b, nullptr, &msg));
    workflow.initialize();
    connect(a.out, b.in);
    ASSERT_TRUE(workflow.validate().empty());
    workflow.prepare();

    auto seeds = [&](int, Seeds &s) {
        EXPECT_TRUE(s.add(b.out, std::vector<double>(kCells, 1.0)));
    };
    workflow.update();
    Gradients g;
    EXPECT_TRUE(workflow.backward(seeds, g, msg)) << msg;
    EXPECT_EQ(workflow.replays(), 0);
    ASSERT_NE(g.find(&a, a.k), nullptr);

    workflow.update();
    const int callsBefore = b.vjpCalls;
    EXPECT_FALSE(workflow.backward(seeds, g, msg));
    EXPECT_NE(msg.find("lacks Capability::Checkpointing"), std::string::npos) << msg;
    // Refused up front, before any derivative work was done.
    EXPECT_EQ(b.vjpCalls, callsBefore);
}

TEST(DifferentiationTest, AComponentThatFinishedEarlyIsRefused)
{
    // b stops after two steps; the third step is then a different function
    // (b is skipped), which a replay could not reproduce honestly.
    Reservoir a("a"), b("b");
    b.doneAfter = 2;
    DifferentiableWorkflow workflow("wf");
    std::string msg;
    ASSERT_TRUE(workflow.addModelComponent(&a, nullptr, &msg));
    ASSERT_TRUE(workflow.addModelComponent(&b, nullptr, &msg));
    workflow.initialize();
    connect(a.out, b.in);
    ASSERT_TRUE(workflow.validate().empty());
    workflow.prepare();
    for (int n = 0; n < 3; ++n)
        workflow.update();
    ASSERT_EQ(b.steps, 2);
    ASSERT_EQ(workflow.recordedSteps(), 3);
    Gradients g;
    EXPECT_FALSE(workflow.backward({}, g, msg));
    EXPECT_NE(msg.find("'b' had finished by step 3"), std::string::npos) << msg;
}

TEST(DifferentiationTest, ARestoreThatLeavesAComponentFinishedIsRefused)
{
    // b finishes at the last step, legitimately; but its restoreState does
    // not resume it, so replaying an earlier step would silently skip it.
    Reservoir a("a"), b("b");
    b.doneAfter = 3;
    b.restoreResetsStatus = false;
    DifferentiableWorkflow workflow("wf");
    std::string msg;
    ASSERT_TRUE(workflow.addModelComponent(&a, nullptr, &msg));
    ASSERT_TRUE(workflow.addModelComponent(&b, nullptr, &msg));
    workflow.initialize();
    connect(a.out, b.in);
    ASSERT_TRUE(workflow.validate().empty());
    workflow.prepare();
    for (int n = 0; n < 3; ++n)
        workflow.update();
    Gradients g;
    auto seeds = [&](int, Seeds &s) {
        EXPECT_TRUE(s.add(b.out, std::vector<double>(kCells, 1.0)));
    };
    EXPECT_FALSE(workflow.backward(seeds, g, msg));
    EXPECT_NE(msg.find("restoreState left 'b' finished"), std::string::npos) << msg;
}

TEST(DifferentiationTest, NothingRecordedNothingToDifferentiate)
{
    Rig rig(Params{});
    Gradients g;
    std::string msg;
    EXPECT_FALSE(rig.workflow->backward(rig.seeds(), g, msg));
    EXPECT_NE(msg.find("no step has been recorded"), std::string::npos) << msg;
}

TEST(DifferentiationTest, ASeedOfTheWrongLengthIsRefused)
{
    Rig rig(Params{});
    Seeds s;
    EXPECT_FALSE(s.add(rig.B->out, std::vector<double>(kCells + 1, 1.0)));
    EXPECT_TRUE(s.add(rig.B->out, std::vector<double>(kCells, 1.0)));
}

// ============================================================================
// G2.2: logarithmic checkpointing
// ============================================================================

namespace {

//! Every gradient of a Gradients, flattened in a fixed order.
std::vector<double> flatten(const Gradients &g)
{
    std::vector<double> all;
    for (const ArgumentGradient &a : g.arguments)
        all.insert(all.end(), a.values.begin(), a.values.end());
    for (const StateGradient &s : g.initialStates)
        all.insert(all.end(), s.values.begin(), s.values.end());
    return all;
}

struct ChainRun
{
    int stored = 0, peak = 0, replays = 0;
    std::vector<int> rungs;
    std::vector<double> gradients;
};

//! A -> B, both stateful, N steps, loss = sum of B's outflow over all steps.
ChainRun runChain(int steps, CheckpointPolicy policy)
{
    Reservoir a("a", true, true, /*forced=*/true), b("b");
    DifferentiableWorkflow workflow("chain");
    workflow.setCheckpointPolicy(policy);
    std::string msg;
    EXPECT_TRUE(workflow.addModelComponent(&a, nullptr, &msg));
    EXPECT_TRUE(workflow.addModelComponent(&b, nullptr, &msg));
    workflow.initialize();
    connect(a.out, b.in);
    EXPECT_TRUE(workflow.validate().empty());
    workflow.prepare();
    for (int n = 0; n < steps; ++n)
        workflow.update();

    ChainRun run;
    run.stored = workflow.storedCheckpoints();
    run.rungs = workflow.checkpointedSteps();
    Gradients g;
    auto seeds = [&](int, Seeds &s) {
        EXPECT_TRUE(s.add(b.out, std::vector<double>(kCells, 1.0)));
    };
    EXPECT_TRUE(workflow.backward(seeds, g, msg)) << msg;
    run.peak = workflow.peakCheckpoints();
    run.replays = workflow.replays();
    run.gradients = flatten(g);
    // backward() must not leave its temporaries behind.
    EXPECT_EQ(workflow.checkpointedSteps(), run.rungs);
    return run;
}

int ceilLog2(int n)
{
    int k = 0;
    while ((1 << k) < n)
        ++k;
    return k;
}

} // namespace

TEST(DifferentiationTest, TheLadderGivesExactlyTheSameGradients)
{
    // Every step is still differentiated at its own linearization point, so
    // the order of recomputation cannot change a single bit.
    Params p;
    Differentiated reference(p);
    Rig rig(p);
    rig.workflow->setCheckpointPolicy(CheckpointPolicy::Logarithmic);
    rig.run();
    Gradients g;
    std::string message;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    ASSERT_EQ(g.arguments.size(), reference.gradients.arguments.size());
    for (size_t i = 0; i < g.arguments.size(); ++i)
        EXPECT_EQ(g.arguments[i].values, reference.gradients.arguments[i].values) << i;
    ASSERT_EQ(g.initialStates.size(), reference.gradients.initialStates.size());
    for (size_t i = 0; i < g.initialStates.size(); ++i)
        EXPECT_EQ(g.initialStates[i].values, reference.gradients.initialStates[i].values);
    EXPECT_LT(rig.workflow->storedCheckpoints(), kSteps);
}

TEST(DifferentiationTest, CheckpointsGrowAsTheLogarithmOfTheSteps)
{
    std::ostringstream table;
    table << "N stored peak replays (EveryStep: stored=N, replays=N-1)\n";
    for (int steps : {10, 100, 1000})
    {
        const ChainRun ladder = runChain(steps, CheckpointPolicy::Logarithmic);
        const int lg = ceilLog2(steps);
        table << steps << ' ' << ladder.stored << ' ' << ladder.peak << ' '
              << ladder.replays << '\n';
        // At most two rungs per power of two, plus the first and the newest.
        EXPECT_LE(ladder.stored, 2 * lg + 2) << "N=" << steps;
        // + one temporary per bisection level + the final state -- and the
        // metric must not under-report: the final state is always held, and
        // a long run's bisection always holds temporaries.
        EXPECT_LE(ladder.peak, ladder.stored + lg + 1) << "N=" << steps;
        EXPECT_GE(ladder.peak, ladder.stored + 1) << "N=" << steps;
        if (steps >= 100)
            EXPECT_GT(ladder.peak, ladder.stored + 1) << "N=" << steps;
        EXPECT_LE(ladder.replays, steps * lg) << "N=" << steps;
        // The first and newest rungs are always there.
        ASSERT_FALSE(ladder.rungs.empty());
        EXPECT_EQ(ladder.rungs.front(), 0);
        EXPECT_EQ(ladder.rungs.back(), steps - 1);
    }
    std::cout << table.str();

    // And every step keeps one checkpoint under EveryStep, as in G2.1.
    const ChainRun every = runChain(100, CheckpointPolicy::EveryStep);
    EXPECT_EQ(every.stored, 100);
    EXPECT_EQ(every.peak, 101);   // every step + the final state; no temporaries
    EXPECT_EQ(every.replays, 99);
}

TEST(DifferentiationTest, TheLadderAndEveryStepAgreeOverALongRun)
{
    const ChainRun ladder = runChain(300, CheckpointPolicy::Logarithmic);
    const ChainRun every = runChain(300, CheckpointPolicy::EveryStep);
    ASSERT_FALSE(every.gradients.empty());
    EXPECT_EQ(ladder.gradients, every.gradients);
    EXPECT_LT(ladder.stored * 10, every.stored);
}

TEST(DifferentiationTest, SwitchingToTheLadderMidRunThinsAtOnce)
{
    Params p;
    Differentiated reference(p);
    Rig rig(p);
    rig.run();
    ASSERT_EQ(rig.workflow->storedCheckpoints(), kSteps);
    rig.workflow->setCheckpointPolicy(CheckpointPolicy::Logarithmic);
    EXPECT_LT(rig.workflow->storedCheckpoints(), kSteps);
    Gradients g;
    std::string message;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    EXPECT_EQ(flatten(g), flatten(reference.gradients));
}

TEST(DifferentiationTest, ASecondBackwardUnderTheLadderAgreesAndRestores)
{
    Rig rig(Params{});
    rig.workflow->setCheckpointPolicy(CheckpointPolicy::Logarithmic);
    rig.run();
    const std::vector<int> rungs = rig.workflow->checkpointedSteps();
    Gradients first, second;
    std::string message;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), first, message)) << message;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), second, message)) << message;
    EXPECT_EQ(flatten(first), flatten(second));
    EXPECT_EQ(rig.workflow->checkpointedSteps(), rungs);

    Rig reference(Params{});
    reference.run();
    for (int i = 0; i < kCells; ++i)
        EXPECT_DOUBLE_EQ((*rig.B->out)[i], (*reference.B->out)[i]);
}

// ============================================================================
// Releasing checkpoints (ICheckpointableModelComponent::releaseState)
// ============================================================================

namespace {

int liveIn(const Rig &rig)
{
    return rig.A->liveCheckpoints() + rig.B->liveCheckpoints() +
           rig.C->liveCheckpoints();
}

int deadUsesIn(const Rig &rig)
{
    return rig.A->deadTokenUses + rig.B->deadTokenUses + rig.C->deadTokenUses;
}

} // namespace

TEST(DifferentiationTest, EveryCheckpointTheLadderDropsIsReleased)
{
    // Each composition checkpoint is one token per component, so the tokens
    // a component holds must always equal the rungs the workflow holds.
    Rig rig(Params{});
    rig.workflow->setCheckpointPolicy(CheckpointPolicy::Logarithmic);
    for (int n = 1; n <= kSteps; ++n)
    {
        rig.step(n);
        EXPECT_EQ(rig.A->liveCheckpoints(), rig.workflow->storedCheckpoints())
            << "after step " << n;
    }
    EXPECT_LT(rig.A->liveCheckpoints(), kSteps);
    EXPECT_EQ(deadUsesIn(rig), 0);
}

TEST(DifferentiationTest, BackwardReleasesItsTemporariesAndTheFinalState)
{
    Rig rig(Params{});
    rig.workflow->setCheckpointPolicy(CheckpointPolicy::Logarithmic);
    rig.run();
    const int before = liveIn(rig);
    Gradients g;
    std::string message;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    // Temporaries and the final state were saved during the pass, and all
    // of them -- and only them -- were released by its end.
    EXPECT_GT(rig.workflow->peakCheckpoints(), rig.workflow->storedCheckpoints());
    EXPECT_EQ(liveIn(rig), before);
    EXPECT_EQ(deadUsesIn(rig), 0);
}

TEST(DifferentiationTest, ClearingOrFinishingReleasesTheRecord)
{
    {
        Rig rig(Params{});
        rig.run();
        ASSERT_EQ(liveIn(rig), 3 * kSteps);   // EveryStep: one per step each
        rig.workflow->clearRecording();
        EXPECT_EQ(liveIn(rig), 0);
    }
    {
        Rig rig(Params{});
        rig.run();
        rig.workflow->finish();
        EXPECT_EQ(liveIn(rig), 0);
        EXPECT_EQ(rig.workflow->refusedReleases(), 0);
        EXPECT_EQ(rig.workflow->releasedTokens(), 3 * kSteps);
    }
    // (prepare() again is not a way out: the workflow lifecycle refuses to
    // prepare once it has stepped, so there is no record for it to drop.)
}

TEST(DifferentiationTest, NoTokenIsEverReleasedTwice)
{
    // Across forward, two backward passes and a finish, every token is
    // released exactly once: the fixture counts any use of a dead one.
    Rig rig(Params{});
    rig.workflow->setCheckpointPolicy(CheckpointPolicy::Logarithmic);
    rig.run();
    Gradients g;
    std::string message;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    rig.workflow->finish();
    EXPECT_EQ(liveIn(rig), 0);
    EXPECT_EQ(deadUsesIn(rig), 0);
    EXPECT_EQ(rig.workflow->refusedReleases(), 0);
}

TEST(DifferentiationTest, AFailedStepReleasesTheCheckpointItTook)
{
    // Every component is checkpointed before a step; when a component then
    // fails, that step never happened and nothing can restore to before it.
    Rig rig(Params{});
    rig.B->failAt = 3;
    rig.step(1);
    rig.step(2);
    ASSERT_EQ(liveIn(rig), 3 * 2);
    rig.step(3);
    ASSERT_EQ(rig.B->status(), CStatus::Failed);
    EXPECT_EQ(rig.workflow->recordedSteps(), 2);
    EXPECT_EQ(liveIn(rig), 3 * 2) << "the failed step's checkpoint was not released";
    EXPECT_EQ(deadUsesIn(rig), 0);
}

// ============================================================================
// rewind(): repeat a run without preparing again (a training loop's need)
// ============================================================================

TEST(DifferentiationTest, RewindRepeatsTheRunExactly)
{
    Rig rig(Params{});
    const double first = rig.run();
    std::vector<double> outFirst(kCells);
    for (int i = 0; i < kCells; ++i)
        outFirst[i] = (*rig.B->out)[i];

    std::string message;
    ASSERT_TRUE(rig.workflow->rewind(message)) << message;
    EXPECT_EQ(rig.workflow->recordedSteps(), 0);
    EXPECT_EQ(liveIn(rig), 0) << "the record's checkpoints were released";

    const double second = rig.run();
    EXPECT_EQ(second, first);
    for (int i = 0; i < kCells; ++i)
        EXPECT_EQ((*rig.B->out)[i], outFirst[i]);

    ASSERT_TRUE(rig.workflow->rewind(message)) << message;
    EXPECT_FALSE(rig.workflow->rewind(message));
    EXPECT_NE(message.find("nothing to rewind"), std::string::npos) << message;
}

TEST(DifferentiationTest, RewindWorksUnderTheLadder)
{
    // The ladder always keeps the state before step 1 (k = 0 is a multiple
    // of every power of two), so a thinned record can still be rewound.
    Rig rig(Params{});
    rig.workflow->setCheckpointPolicy(CheckpointPolicy::Logarithmic);
    const double first = rig.run();
    std::string message;
    ASSERT_TRUE(rig.workflow->rewind(message)) << message;
    EXPECT_EQ(liveIn(rig), 0);
    EXPECT_EQ(rig.run(), first);
}

TEST(DifferentiationTest, ARewoundRunWithNewArgumentsDifferentiatesAsAFreshOne)
{
    Params changed;
    changed.kB = {0.12, 0.44, 0.27};
    Differentiated fresh(changed);

    Rig rig(Params{});
    rig.run();
    std::string message;
    ASSERT_TRUE(rig.workflow->rewind(message)) << message;
    for (int i = 0; i < kCells; ++i)
        (*rig.B->k)[i] = changed.kB[i];
    const double loss = rig.run();
    EXPECT_EQ(loss, fresh.loss);
    Gradients g;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    EXPECT_EQ(flatten(g), flatten(fresh.gradients));
}
