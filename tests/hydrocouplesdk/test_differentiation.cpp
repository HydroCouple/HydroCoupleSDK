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
#include "hydrocouplesdk/temporal/temporalinterpolationadaptedoutput.h"

#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
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
    double lambda = 0.35;   //!< the relaxation factor, when the adapter relaxes
    std::vector<double> sA0{6.0, 2.0, 9.0};
    std::vector<double> sB0{1.0, 4.0, 3.0};
};

// ============================================================================
// Adapters the gates need beyond the SDK's own (G2.4)
// ============================================================================

//! A relaxation whose checkpoint tokens carry a serial, so a test sees every
//! token saved and not yet released, every use of a dead one, and how often
//! its vjp() ran.
class CountingRelaxation final : public RelaxationAdaptedOutput
{
public:
    using RelaxationAdaptedOutput::RelaxationAdaptedOutput;

    bool saveState(std::string &token, std::string &message) override
    {
        std::string inner;
        if (!RelaxationAdaptedOutput::saveState(inner, message))
            return false;
        const long serial = m_next++;
        m_live.insert(serial);
        token = std::to_string(serial) + '|' + inner;
        return true;
    }
    bool restoreState(const std::string &token, std::string &message) override
    {
        std::string inner;
        if (!liveSerial(token, inner))
        {
            ++deadTokenUses;
            message = "restore of a released token";
            return false;
        }
        ++restores;
        return RelaxationAdaptedOutput::restoreState(inner, message);
    }
    bool releaseState(const std::string &token, std::string &message) override
    {
        std::string inner;
        if (!liveSerial(token, inner))
        {
            ++deadTokenUses;
            message = "release of a released token";
            return false;
        }
        m_live.erase(std::stol(token));
        return RelaxationAdaptedOutput::releaseState(inner, message);
    }
    bool vjp(DifferentialSet seeds, DifferentialSet results, std::string *message) override
    {
        ++vjpCalls;
        return RelaxationAdaptedOutput::vjp(seeds, results, message);
    }

    [[nodiscard]] int liveCheckpoints() const { return static_cast<int>(m_live.size()); }
    int deadTokenUses = 0;
    int restores = 0;
    int vjpCalls = 0;

private:
    bool liveSerial(const std::string &token, std::string &inner) const
    {
        const size_t bar = token.find('|');
        if (bar == std::string::npos || !m_live.count(std::stol(token.substr(0, bar))))
            return false;
        inner = token.substr(bar + 1);
        return true;
    }
    long m_next = 1;
    std::set<long> m_live;
};

//! The data plane every test adapter shares: rank-1 Float64 values pulled
//! from a rank-1 Float64 adaptee.
class TestAdapter : public AbstractAdaptedOutput
{
public:
    TestAdapter(std::string_view id, IOutput *adaptee)
        : AbstractAdaptedOutput(id, {}, nullptr, adaptee, SdkAdaptedOutputFactory::instance())
    {}
    void initialize() override
    {
        m_values.clear();
        refresh();
    }
    void updateValues() override { refresh(); }
    [[nodiscard]] std::vector<int64_t> shape() const override { return adaptee()->shape(); }
    [[nodiscard]] DataKind dataKind() const override { return DataKind::Float64; }
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        const int64_t extent = static_cast<int64_t>(m_values.size());
        return detail::readHyperslab<double>(m_values.data(),
                                             std::span<const int64_t>(&extent, 1),
                                             destination, start, count, message);
    }
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &, std::span<const int64_t>,
                                     std::span<const int64_t>, std::string *message) override
    {
        if (message)
            *message = "read-only";
        return false;
    }

protected:
    std::vector<double> pull() const
    {
        std::vector<double> x(static_cast<size_t>(adaptee()->shape()[0]));
        const std::vector<int64_t> shape = adaptee()->shape(), start{0};
        BufferDescriptor d = hostBuffer(x, shape);
        EXPECT_TRUE(adaptee()->getValuesInto(d, start, shape));
        return x;
    }
    void publish()
    {
        emitValueChanged({0}, {static_cast<int64_t>(m_values.size())});
        refreshAdaptedOutputs();
    }
    std::vector<double> m_values;
};

//! Copies its adaptee: stateless, and not differentiable.
class PlainCopy final : public TestAdapter
{
public:
    using TestAdapter::TestAdapter;
    void refresh() override
    {
        m_values = pull();
        publish();
    }
};

//! y <- (y + x) / 2: stateful and differentiable, but it cannot be
//! checkpointed, so nothing can return it to an earlier step.
class ForgetfulLag final : public TestAdapter, public virtual IDifferentiableAdaptedOutput
{
public:
    using TestAdapter::TestAdapter;
    void refresh() override
    {
        const std::vector<double> x = pull();
        if (m_values.size() != x.size())
            m_values = x;
        else
            for (size_t i = 0; i < x.size(); ++i)
                m_values[i] = 0.5 * (m_values[i] + x[i]);
        publish();
    }
    [[nodiscard]] std::vector<IComponentDataItem *> states() const override
    {
        return {const_cast<IComponentDataItem *>(itemOf<IComponentDataItem>(this))};
    }
    [[nodiscard]] std::vector<IArgument *> differentiableArguments() const override { return {}; }
    [[nodiscard]] std::vector<IComponentDataItem *> differentiableStates() const override
    {
        return states();
    }
    bool vjp(DifferentialSet seeds, DifferentialSet results, std::string *message) override
    {
        const std::vector<int64_t> shape = this->shape();
        std::vector<double> t(static_cast<size_t>(shape[0]), 0.0), seed;
        for (const auto &e : seeds)
        {
            if (!readDifferential(e, shape, seed, message))
                return false;
            for (size_t i = 0; i < t.size(); ++i)
                t[i] += seed[i];
        }
        for (auto &v : t)
            v *= 0.5;
        for (const auto &e : results)
            if (!writeDifferential(e, shape, t, message))
                return false;
        return true;
    }
    bool jvp(DifferentialSet, DifferentialSet, std::string *message) override
    {
        if (message)
            *message = "not needed";
        return false;
    }
};

enum class AdapterKind
{
    Linear,               //!< LinearTransformAdaptedOutput (m, b): stateless
    Relaxation,           //!< RelaxationAdaptedOutput (λ): stateful, checkpointable
    CountingRelaxation,   //!< the same, counting its tokens and vjp calls
    Forgetful,            //!< stateful, differentiable, not checkpointable
    Plain                 //!< not differentiable
};

//! The composition's shape: which adapter sits between A and B, and whether
//! C reads the adapter too (A -> adapter -> {B, C}) instead of A directly.
struct Config
{
    AdapterKind adapter = AdapterKind::Linear;
    bool adapterFeedsC = false;
};

constexpr int kSteps = 8;

double observed(int step, int cell) { return 1.0 + 0.1 * step + 0.2 * cell; }


struct Rig
{
    explicit Rig(const Params &p, Config config = {}) : config(config)
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

        switch (config.adapter)
        {
        case AdapterKind::Linear:
            adapter = std::make_unique<LinearTransformAdaptedOutput>(
                "unit", A->out, SdkAdaptedOutputFactory::instance());
            break;
        case AdapterKind::Relaxation:
            adapter = std::make_unique<RelaxationAdaptedOutput>(
                "relax", A->out, SdkAdaptedOutputFactory::instance());
            break;
        case AdapterKind::CountingRelaxation:
            adapter = std::make_unique<CountingRelaxation>(
                "relax", A->out, SdkAdaptedOutputFactory::instance());
            break;
        case AdapterKind::Forgetful:
            adapter = std::make_unique<ForgetfulLag>("forgetful", A->out);
            break;
        case AdapterKind::Plain:
            adapter = std::make_unique<PlainCopy>("copy", A->out);
            break;
        }
        A->out->addAdaptedOutput(adapter.get());
        connect(adapter.get(), B->in);
        connect(config.adapterFeedsC ? static_cast<IOutput *>(adapter.get()) : A->out,
                C->in);

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
        setArgument(*adapter, "relaxation_factor", p.lambda);
        // A stateful adapter's state before step 1 is A's prepared outflow.
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

    Config config;
    std::unique_ptr<Reservoir> A, B, C;
    std::unique_ptr<AbstractAdaptedOutput> adapter;
    std::unique_ptr<DifferentiableWorkflow> workflow;
};

//! An output's values, read through the data plane.
std::vector<double> valuesOf(const IOutput *output)
{
    const std::vector<int64_t> shape = output->shape(), start(shape.size(), 0);
    std::vector<double> v(static_cast<size_t>(elementCount(shape)));
    const BufferDescriptor d = hostBuffer(v, shape);
    EXPECT_TRUE(output->getValuesInto(d, start, shape));
    return v;
}

//! The loss by forward runs only.
double lossAt(const Params &p, Config config = {})
{
    Rig rig(p, config);
    return rig.run();
}

template <typename Setter>
double centralDifference(Setter set, Config config = {}, double eps = 1e-6)
{
    Params plus, minus;
    set(plus, +eps);
    set(minus, -eps);
    return (lossAt(plus, config) - lossAt(minus, config)) / (2.0 * eps);
}

struct Differentiated
{
    explicit Differentiated(const Params &p = {}, Config config = {}) : rig(p, config)
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

TEST(DifferentiationTest, EachAdapterSaysWhetherItCarriesState)
{
    // As an engine sees them (G2.4): the relaxation carries its own previous
    // values and offers both contracts; the interpolation adapter carries a
    // record that grows, and offers neither; the linear transform carries
    // nothing.
    Reservoir source("source");
    source.initialize();
    RelaxationAdaptedOutput relax("relax", source.out, SdkAdaptedOutputFactory::instance());
    IAdaptedOutput *adapted = &relax;
    ASSERT_NE(dynamic_cast<IDifferentiableAdaptedOutput *>(adapted), nullptr);
    ASSERT_NE(dynamic_cast<ICheckpointableAdaptedOutput *>(adapted), nullptr);
    const std::vector<IComponentDataItem *> itself{const_cast<IComponentDataItem *>(
        itemOf<IComponentDataItem>(adapted))};
    EXPECT_EQ(adapted->states(), itself);
    EXPECT_EQ(dynamic_cast<IDifferentiableAdaptedOutput *>(adapted)->differentiableStates(),
              itself);

    HydroCouple::SDK::Temporal::TemporalInterpolationAdaptedOutput record(
        "record", source.out, SdkAdaptedOutputFactory::instance());
    adapted = &record;
    EXPECT_EQ(dynamic_cast<IDifferentiableAdaptedOutput *>(adapted), nullptr);
    EXPECT_EQ(dynamic_cast<ICheckpointableAdaptedOutput *>(adapted), nullptr);
    ASSERT_EQ(adapted->states().size(), 1u);
    EXPECT_EQ(adapted->states()[0]->shape(), (std::vector<int64_t>{0, 2}));

    LinearTransformAdaptedOutput unit("unit", source.out, SdkAdaptedOutputFactory::instance());
    EXPECT_TRUE(static_cast<IAdaptedOutput &>(unit).states().empty());
    EXPECT_TRUE(unit.differentiableStates().empty());
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
    // The adapter too: a stateless one shows its adaptee's restored values,
    // not those of the last step the pass replayed.
    EXPECT_EQ(valuesOf(d.rig.adapter.get()), valuesOf(reference.adapter.get()));
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

TEST(DifferentiationTest, ANonDifferentiableAdapterOnTheConnectionIsRefusedByName)
{
    Rig rig(Params{}, {AdapterKind::Plain});
    rig.run();
    Gradients g;
    std::string msg;
    const int callsBefore = rig.B->vjpCalls;
    EXPECT_FALSE(rig.workflow->backward(rig.seeds(), g, msg));
    EXPECT_NE(msg.find("adapted output 'copy' on the connection into 'B.in' is not "
                       "differentiable"),
              std::string::npos) << msg;
    EXPECT_EQ(rig.B->vjpCalls, callsBefore) << "refused up front, before any derivative work";
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
    // Components and adapters read their arguments at every update or
    // refresh, so changing them after a rewind -- the adapter's too, with no
    // initialize() -- runs exactly as a fresh composition would.
    Params changed;
    changed.kB = {0.12, 0.44, 0.27};
    changed.m = 1.1;
    Differentiated fresh(changed);

    Rig rig(Params{});
    rig.run();
    std::string message;
    ASSERT_TRUE(rig.workflow->rewind(message)) << message;
    for (int i = 0; i < kCells; ++i)
        (*rig.B->k)[i] = changed.kB[i];
    setArgument(*rig.adapter, "multiplier", changed.m);
    const double loss = rig.run();
    EXPECT_EQ(loss, fresh.loss);
    Gradients g;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    EXPECT_EQ(flatten(g), flatten(fresh.gradients));
}

// ============================================================================
// G2.4: stateful adapters -- the relaxation's own adjoint and checkpoint, and
// the engine carrying an adapter's state through time and through replays
// ============================================================================

namespace {

DifferentialEntry entry(const IComponentDataItem *item, R role, std::vector<double> &values,
                        const std::vector<int64_t> &shape)
{
    DifferentialEntry e;
    e.item = item;
    e.role = role;
    e.value = hostBuffer(values, shape);
    return e;
}

//! A source reservoir and a relaxation of its outflow, initialized at x0.
struct Relaxed
{
    Relaxed(const std::vector<double> &x0, double lambda)
    {
        source.initialize();   // creates source.out
        source.validate();
        source.prepare();
        adapter = std::make_unique<RelaxationAdaptedOutput>(
            "relax", source.out, SdkAdaptedOutputFactory::instance());
        setArgument(relax(), "relaxation_factor", lambda);
        set(x0);
        relax().initialize();   // y0 = x0: the first refresh passes through
    }
    RelaxationAdaptedOutput &relax() { return *adapter; }
    void set(const std::vector<double> &x)
    {
        for (int i = 0; i < kCells; ++i)
            (*source.out)[i] = x[i];
    }
    void refreshAt(const std::vector<double> &x)
    {
        set(x);
        relax().refresh();
    }
    Reservoir source{"source"};
    std::unique_ptr<RelaxationAdaptedOutput> adapter;
};

//! One relaxation refresh from scratch: y1 = (1 - λ) x0 + λ x1.
std::vector<double> relaxOnce(const std::vector<double> &x0, const std::vector<double> &x1,
                              double lambda)
{
    Relaxed r(x0, lambda);
    r.refreshAt(x1);
    return valuesOf(&r.relax());
}

const std::vector<double> kX0{2.0, -1.0, 0.5}, kX1{0.7, 3.0, -2.0}, kX2{-1.2, 0.4, 1.9};

} // namespace

TEST(DifferentiationTest, TheRelaxationAdjointMatchesFiniteDifferences)
{
    const double lambda = 0.35;
    Relaxed r(kX0, lambda);
    r.refreshAt(kX1);

    const auto *self = itemOf<IComponentDataItem>(&r.relax());
    IArgument *factor = r.relax().differentiableArguments().at(0);
    const std::vector<int64_t> shape{kCells}, scalar{1};
    std::vector<double> outBar{0.3, -1.1, 2.0}, stateBar{-0.4, 0.8, 0.25};
    std::vector<double> xbar(kCells), ybar(kCells), lbar(1);
    const DifferentialEntry seeds[] = {entry(self, R::Output, outBar, shape),
                                       entry(self, R::StateAfter, stateBar, shape)};
    const DifferentialEntry results[] = {entry(itemOf(r.source.out), R::Input, xbar, shape),
                                         entry(self, R::StateBefore, ybar, shape),
                                         entry(itemOf(factor), R::Argument, lbar, scalar)};
    std::string message;
    ASSERT_TRUE(r.relax().vjp(seeds, results, &message)) << message;

    // The output and the state it carries forward are the same values, so
    // the loss sees them through both cotangents.
    auto f = [&](const std::vector<double> &a0, const std::vector<double> &a1, double l) {
        const std::vector<double> y = relaxOnce(a0, a1, l);
        double sum = 0.0;
        for (int i = 0; i < kCells; ++i)
            sum += (outBar[i] + stateBar[i]) * y[i];
        return sum;
    };
    const double eps = 1e-6;
    for (int i = 0; i < kCells; ++i)
    {
        auto p0 = kX0, m0 = kX0, p1 = kX1, m1 = kX1;
        p0[i] += eps;
        m0[i] -= eps;
        p1[i] += eps;
        m1[i] -= eps;
        EXPECT_NEAR(ybar[i], (f(p0, kX1, lambda) - f(m0, kX1, lambda)) / (2 * eps), 1e-8)
            << "state before[" << i << "]";
        EXPECT_NEAR(xbar[i], (f(kX0, p1, lambda) - f(kX0, m1, lambda)) / (2 * eps), 1e-8)
            << "adaptee[" << i << "]";
    }
    EXPECT_NEAR(lbar[0], (f(kX0, kX1, lambda + eps) - f(kX0, kX1, lambda - eps)) / (2 * eps),
                1e-8);

    // Adjointness: <ȳ, J v> = <Jᵀ ȳ, v>; the jvp writes the output and the
    // state after alike.
    std::vector<double> vx{0.2, 0.7, -0.3}, vy{-0.5, 0.1, 0.9}, vl{0.6};
    std::vector<double> outDot(kCells), stateDot(kCells);
    const DifferentialEntry tseeds[] = {entry(itemOf(r.source.out), R::Input, vx, shape),
                                        entry(self, R::StateBefore, vy, shape),
                                        entry(itemOf(factor), R::Argument, vl, scalar)};
    const DifferentialEntry tresults[] = {entry(self, R::Output, outDot, shape),
                                          entry(self, R::StateAfter, stateDot, shape)};
    ASSERT_TRUE(r.relax().jvp(tseeds, tresults, &message)) << message;
    EXPECT_EQ(outDot, stateDot);
    double lhs = 0.0, rhs = lbar[0] * vl[0];
    for (int i = 0; i < kCells; ++i)
    {
        lhs += outBar[i] * outDot[i] + stateBar[i] * stateDot[i];
        rhs += xbar[i] * vx[i] + ybar[i] * vy[i];
    }
    EXPECT_NEAR(lhs, rhs, 1e-12);
}

TEST(DifferentiationTest, ARelaxationsFirstRefreshIsAPassthrough)
{
    // Right after initialize() there is no history: y = x, so the whole
    // cotangent reaches the adaptee and none the state or the factor.
    Relaxed r(kX0, 0.35);
    const auto *self = itemOf<IComponentDataItem>(&r.relax());
    IArgument *factor = r.relax().differentiableArguments().at(0);
    const std::vector<int64_t> shape{kCells}, scalar{1};
    std::vector<double> t{1.5, -2.0, 0.25}, xbar(kCells), ybar(kCells, 9.0), lbar{9.0};
    const DifferentialEntry seeds[] = {entry(self, R::Output, t, shape)};
    const DifferentialEntry results[] = {entry(itemOf(r.source.out), R::Input, xbar, shape),
                                         entry(self, R::StateBefore, ybar, shape),
                                         entry(itemOf(factor), R::Argument, lbar, scalar)};
    std::string message;
    ASSERT_TRUE(r.relax().vjp(seeds, results, &message)) << message;
    EXPECT_EQ(xbar, t);
    EXPECT_EQ(ybar, std::vector<double>(kCells, 0.0));
    EXPECT_EQ(lbar[0], 0.0);
}

TEST(DifferentiationTest, ARelaxationCheckpointRestoresItExactly)
{
    Relaxed r(kX0, 0.35);
    r.refreshAt(kX1);
    const std::vector<double> saved = valuesOf(&r.relax());
    std::string token, message;
    ASSERT_TRUE(r.relax().saveState(token, message)) << message;

    r.refreshAt(kX2);
    const std::vector<double> next = valuesOf(&r.relax());
    ASSERT_NE(next, saved);

    // Restored: the values it held, and no refresh to differentiate until
    // the next one -- which then lands exactly where the first one did.
    ASSERT_TRUE(r.relax().restoreState(token, message)) << message;
    EXPECT_EQ(valuesOf(&r.relax()), saved);
    std::vector<double> t(kCells, 1.0), xbar(kCells);
    const std::vector<int64_t> shape{kCells};
    const DifferentialEntry seeds[] = {
        entry(itemOf<IComponentDataItem>(&r.relax()), R::Output, t, shape)};
    const DifferentialEntry results[] = {entry(itemOf(r.source.out), R::Input, xbar, shape)};
    EXPECT_FALSE(r.relax().vjp(seeds, results, &message));
    r.refreshAt(kX2);
    EXPECT_EQ(valuesOf(&r.relax()), next);

    // The token is the state: another adapter (another process) restores it.
    Relaxed other(kX2, 0.9);
    ASSERT_TRUE(other.relax().restoreState(token, message)) << message;
    EXPECT_EQ(valuesOf(&other.relax()), saved);

    EXPECT_TRUE(r.relax().releaseState(token, message));
}

TEST(DifferentiationTest, ARelaxationTokenIsExactAndRefusesWhatItCannotRestore)
{
    // Bit for bit, whatever the values: a replay restores a diverged state
    // as faithfully as an ordinary one.
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const std::vector<double> odd{std::numeric_limits<double>::infinity(), nan,
                                  std::numeric_limits<double>::denorm_min()};
    Relaxed r(odd, 0.35);   // y0 = the adaptee's values
    std::string token, message;
    ASSERT_TRUE(r.relax().saveState(token, message)) << message;
    Relaxed other(kX0, 0.35);
    ASSERT_TRUE(other.relax().restoreState(token, message)) << message;
    const std::vector<double> back = valuesOf(&other.relax());
    ASSERT_EQ(back.size(), odd.size());
    for (size_t i = 0; i < odd.size(); ++i)
        EXPECT_EQ(std::bit_cast<uint64_t>(back[i]), std::bit_cast<uint64_t>(odd[i])) << i;

    // What it cannot restore it refuses -- without throwing, and without
    // touching its state.
    const std::vector<double> before = valuesOf(&other.relax());
    for (const std::string bad :
         {std::string("something else"), std::string("relaxation-v1"),
          std::string("relaxation-v1 -1"), std::string("relaxation-v1 18446744073709551615"),
          std::string("relaxation-v1 3 3ff0000000000000"),
          std::string("relaxation-v1 2 3ff0000000000000 3ff0000000000000"),
          std::string("relaxation-v1 3 3ff0000000000000 3ff0000000000000 zz"),
          token + " 3ff0000000000000"})
    {
        bool restored = true;
        EXPECT_NO_THROW(restored = other.relax().restoreState(bad, message)) << bad;
        EXPECT_FALSE(restored) << bad;
        EXPECT_FALSE(message.empty()) << bad;
        EXPECT_EQ(std::bit_cast<uint64_t>(valuesOf(&other.relax())[0]),
                  std::bit_cast<uint64_t>(before[0])) << bad;
    }
    // An empty state -- no history yet -- restores anywhere.
    EXPECT_TRUE(other.relax().restoreState("relaxation-v1 0", message)) << message;
}

namespace {

//! The gradients a relaxation composition reports, each checked against
//! finite differences of full forward runs. The adapter's state before
//! step 1 is A's prepared outflow, y0 = kA * sA0 -- a dependence the engine
//! leaves to the caller -- so its gradient is chained into kA and sA0 here,
//! analytically, exactly as a caller would.
void expectRelaxedGradientsMatch(Config config)
{
    const Params p;
    Differentiated d(p, config);
    ASSERT_TRUE(d.ok);
    EXPECT_NEAR(d.loss, lossAt(p, config), 1e-12);

    Rig &rig = d.rig;
    IArgument *factor =
        dynamic_cast<IDifferentiableAdaptedOutput &>(*rig.adapter).differentiableArguments().at(0);
    const auto kA = d.argument(rig.A.get(), rig.A->k);
    const auto kB = d.argument(rig.B.get(), rig.B->k);
    const auto kC = d.argument(rig.C.get(), rig.C->k);
    const auto lambda = d.argument(rig.adapter.get(), factor);
    const StateGradient *sA0 = d.gradients.findState(rig.A.get(), rig.A->storage.get());
    const StateGradient *sB0 = d.gradients.findState(rig.B.get(), rig.B->storage.get());
    const StateGradient *y0 =
        d.gradients.findState(rig.adapter.get(), itemOf<IComponentDataItem>(rig.adapter.get()));
    ASSERT_EQ(kA.size(), size_t(kCells));
    ASSERT_EQ(kB.size(), size_t(kCells));
    ASSERT_EQ(kC.size(), size_t(kCells));
    ASSERT_EQ(lambda.size(), 1u);
    ASSERT_NE(sA0, nullptr);
    ASSERT_NE(sB0, nullptr);
    ASSERT_NE(y0, nullptr) << "the adapter's initial state has a gradient";
    EXPECT_EQ(y0->adapter, rig.adapter.get());
    EXPECT_EQ(y0->component, nullptr);

    const double tol = 1e-5;
    for (int i = 0; i < kCells; ++i)
    {
        EXPECT_NE(y0->values[i], 0.0) << "y0[" << i << "]";
        const double chainedKA = kA[i] + y0->values[i] * p.sA0[i];
        const double chainedSA0 = sA0->values[i] + y0->values[i] * p.kA[i];
        EXPECT_NEAR(chainedKA,
                    centralDifference([i](Params &q, double e) { q.kA[i] += e; }, config),
                    tol * (1 + std::abs(chainedKA))) << "kA[" << i << "]";
        EXPECT_NEAR(chainedSA0,
                    centralDifference([i](Params &q, double e) { q.sA0[i] += e; }, config),
                    tol * (1 + std::abs(chainedSA0))) << "sA0[" << i << "]";
        EXPECT_NEAR(kB[i], centralDifference([i](Params &q, double e) { q.kB[i] += e; }, config),
                    tol * (1 + std::abs(kB[i]))) << "kB[" << i << "]";
        EXPECT_NEAR(kC[i], centralDifference([i](Params &q, double e) { q.kC[i] += e; }, config),
                    tol * (1 + std::abs(kC[i]))) << "kC[" << i << "]";
        EXPECT_NEAR(sB0->values[i],
                    centralDifference([i](Params &q, double e) { q.sB0[i] += e; }, config),
                    tol * (1 + std::abs(sB0->values[i]))) << "sB0[" << i << "]";
    }
    EXPECT_NEAR(lambda[0], centralDifference([](Params &q, double e) { q.lambda += e; }, config),
                tol * (1 + std::abs(lambda[0]))) << "relaxation factor";
}

} // namespace

TEST(DifferentiationTest, ThroughARelaxationEveryGradientMatchesFiniteDifferences)
{
    //   A ──► relax (λ) ──► B ──► loss,   A ──► C ──► loss
    expectRelaxedGradientsMatch({AdapterKind::Relaxation});
}

TEST(DifferentiationTest, ARelaxationFeedingTwoConsumersIsReversedOncePerStep)
{
    //   A ──► relax (λ) ──┬──► B ──► loss
    //                     └──► C ──► loss
    // B and C both add to the adapter's cotangent, and its vjp must run once
    // per step with the sum (and its state) -- not once per consumer.
    expectRelaxedGradientsMatch({AdapterKind::Relaxation, /*adapterFeedsC=*/true});
    Differentiated d(Params{}, {AdapterKind::CountingRelaxation, /*adapterFeedsC=*/true});
    ASSERT_TRUE(d.ok);
    EXPECT_EQ(dynamic_cast<CountingRelaxation &>(*d.rig.adapter).vjpCalls, kSteps);
}

TEST(DifferentiationTest, TheLadderGivesExactlyTheSameGradientsThroughARelaxation)
{
    const Config config{AdapterKind::Relaxation, /*adapterFeedsC=*/true};
    Differentiated reference(Params{}, config);
    Rig rig(Params{}, config);
    rig.workflow->setCheckpointPolicy(CheckpointPolicy::Logarithmic);
    rig.run();
    Gradients g;
    std::string message;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    ASSERT_FALSE(flatten(g).empty());
    EXPECT_EQ(flatten(g), flatten(reference.gradients));
    EXPECT_LT(rig.workflow->storedCheckpoints(), kSteps);
}

TEST(DifferentiationTest, AdapterCheckpointsAreReleasedExactlyOnce)
{
    Rig rig(Params{}, {AdapterKind::CountingRelaxation});
    auto &relax = dynamic_cast<CountingRelaxation &>(*rig.adapter);
    rig.workflow->setCheckpointPolicy(CheckpointPolicy::Logarithmic);
    for (int n = 1; n <= kSteps; ++n)
    {
        rig.step(n);
        EXPECT_EQ(relax.liveCheckpoints(), rig.workflow->storedCheckpoints()) << "after step " << n;
    }
    Gradients g;
    std::string message;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    EXPECT_GT(relax.restores, 0) << "the replays restored the adapter";
    EXPECT_EQ(relax.liveCheckpoints(), rig.workflow->storedCheckpoints())
        << "a temporary or the final state was not released";
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    rig.workflow->finish();
    EXPECT_EQ(relax.liveCheckpoints(), 0);
    EXPECT_EQ(relax.deadTokenUses, 0);
    EXPECT_EQ(rig.workflow->refusedReleases(), 0);
}

TEST(DifferentiationTest, RewindReturnsTheRelaxationToo)
{
    Rig rig(Params{}, {AdapterKind::Relaxation});
    const double first = rig.run();
    std::string message;
    ASSERT_TRUE(rig.workflow->rewind(message)) << message;
    EXPECT_EQ(rig.run(), first);
}

TEST(DifferentiationTest, ARewoundRunWithANewFactorDifferentiatesAsAFreshOne)
{
    // λ is read at every refresh, so a training loop can change it between
    // runs without re-initializing the adapter (which would lose its state).
    Params changed;
    changed.lambda = 0.8;
    Differentiated fresh(changed, {AdapterKind::Relaxation});

    Rig rig(Params{}, {AdapterKind::Relaxation});
    rig.run();
    std::string message;
    ASSERT_TRUE(rig.workflow->rewind(message)) << message;
    setArgument(*rig.adapter, "relaxation_factor", changed.lambda);
    EXPECT_EQ(rig.run(), fresh.loss);
    Gradients g;
    ASSERT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    EXPECT_EQ(flatten(g), flatten(fresh.gradients));
}

TEST(DifferentiationTest, AStatefulAdapterThatCannotBeCheckpointedIsRefusedByName)
{
    Rig rig(Params{}, {AdapterKind::Forgetful});
    rig.step(1);
    Gradients g;
    std::string message;
    // One step needs no replay, exactly as for a component without
    // Capability::Checkpointing ...
    EXPECT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
    EXPECT_NE(g.findState(rig.adapter.get(), itemOf<IComponentDataItem>(rig.adapter.get())),
              nullptr);
    // ... but two do, and nothing can return the adapter to the first.
    rig.step(2);
    const int callsBefore = rig.B->vjpCalls;
    EXPECT_FALSE(rig.workflow->backward(rig.seeds(), g, message));
    EXPECT_NE(message.find("adapted output 'forgetful' carries state but is not an "
                           "ICheckpointableAdaptedOutput"),
              std::string::npos) << message;
    EXPECT_EQ(rig.B->vjpCalls, callsBefore) << "refused up front, before any derivative work";
}

TEST(DifferentiationTest, AStatefulAdapterOffTheDifferentiatedPathStillBlocksReplay)
{
    // No derivative crosses it, but every replay refreshes it: an adapter
    // whose state ran on would serve the replay the wrong record.
    Rig rig(Params{});
    HydroCouple::SDK::Temporal::TemporalInterpolationAdaptedOutput record(
        "record", rig.C->out, SdkAdaptedOutputFactory::instance());
    rig.C->out->addAdaptedOutput(&record);
    record.initialize();
    rig.run();
    Gradients g;
    std::string message;
    EXPECT_FALSE(rig.workflow->backward(rig.seeds(), g, message));
    EXPECT_NE(message.find("adapted output 'record' carries state"), std::string::npos)
        << message;
}

TEST(DifferentiationTest, AnAdapterThatJoinedAfterACheckpointCannotBeReplayedThrough)
{
    Rig rig(Params{});
    rig.run();
    RelaxationAdaptedOutput late("late", rig.C->out, SdkAdaptedOutputFactory::instance());
    rig.C->out->addAdaptedOutput(&late);
    late.initialize();
    Gradients g;
    std::string message;
    EXPECT_FALSE(rig.workflow->backward(rig.seeds(), g, message));
    EXPECT_NE(message.find("'late' carries state but joined the composition after the "
                           "checkpoint"),
              std::string::npos) << message;
    // ... and the composition is still where the forward run left it.
    Rig reference(Params{});
    reference.run();
    for (int i = 0; i < kCells; ++i)
        EXPECT_DOUBLE_EQ((*rig.B->out)[i], (*reference.B->out)[i]);
}

TEST(DifferentiationTest, AfterBackwardEveryAdapterShowsTheStateItWasLeftIn)
{
    // A -> relax -> B: the relaxation is restored from its token, and the
    // stateless adapters' refresh at restore must not move it.
    const Config config{AdapterKind::Relaxation, /*adapterFeedsC=*/true};
    Differentiated d(Params{}, config);
    ASSERT_TRUE(d.ok);
    Rig reference(Params{}, config);
    reference.run();
    EXPECT_EQ(valuesOf(d.rig.adapter.get()), valuesOf(reference.adapter.get()));
    d.rig.step(kSteps + 1);
    reference.step(kSteps + 1);
    EXPECT_EQ(valuesOf(d.rig.B->out), valuesOf(reference.B->out));
}

TEST(DifferentiationTest, AnAdapterOnAConnectionFromOutsideTheWorkflowIsDifferentiated)
{
    // U (not in the workflow) -> unit (m, b) -> B -> loss. U's outflow is a
    // constant, but the adapter's coefficients still reach the loss.
    auto build = [](double m, double b, std::vector<double> kB, Gradients *g) {
        Reservoir U("U"), B("B");
        U.initialize();
        U.validate();
        U.prepare();
        for (int i = 0; i < kCells; ++i)
            (*U.out)[i] = 1.0 + 0.5 * i;
        DifferentiableWorkflow workflow("outside");
        std::string msg;
        EXPECT_TRUE(workflow.addModelComponent(&B, nullptr, &msg)) << msg;
        workflow.initialize();
        LinearTransformAdaptedOutput unit("unit", U.out, SdkAdaptedOutputFactory::instance());
        U.out->addAdaptedOutput(&unit);
        connect(&unit, B.in);
        for (int i = 0; i < kCells; ++i)
            (*B.k)[i] = kB[i];
        EXPECT_TRUE(workflow.validate().empty());
        workflow.prepare();
        setArgument(unit, "multiplier", m);
        setArgument(unit, "offset", b);
        unit.initialize();
        double loss = 0.0;
        for (int n = 1; n <= kSteps; ++n)
        {
            workflow.update();
            for (int i = 0; i < kCells; ++i)
            {
                const double r = (*B.out)[i] - observed(n, i);
                loss += r * r;
            }
        }
        if (g)
        {
            auto seeds = [&](int n, Seeds &s) {
                std::vector<double> gb(kCells);
                for (int i = 0; i < kCells; ++i)
                    gb[i] = 2.0 * ((*B.out)[i] - observed(n, i));
                EXPECT_TRUE(s.add(B.out, gb));
            };
            EXPECT_TRUE(workflow.backward(seeds, *g, msg)) << msg;
            auto args = unit.differentiableArguments();
            const ArgumentGradient *gm = g->find(&unit, args[0]);
            const ArgumentGradient *gbias = g->find(&unit, args[1]);
            EXPECT_NE(gm, nullptr) << "the adapter's multiplier reached nothing";
            EXPECT_NE(gbias, nullptr);
            if (gm && gbias)
                *g = Gradients{{*gm, *gbias}, {}};
        }
        workflow.finish();
        U.out->removeAdaptedOutput(&unit);
        return loss;
    };
    const std::vector<double> kB{0.25, 0.35, 0.15};
    Gradients g;
    build(0.8, 0.3, kB, &g);
    ASSERT_EQ(g.arguments.size(), 2u);
    const double eps = 1e-6;
    const double dm = (build(0.8 + eps, 0.3, kB, nullptr) - build(0.8 - eps, 0.3, kB, nullptr)) / (2 * eps);
    const double db = (build(0.8, 0.3 + eps, kB, nullptr) - build(0.8, 0.3 - eps, kB, nullptr)) / (2 * eps);
    EXPECT_NEAR(g.arguments[0].values[0], dm, 1e-5 * (1 + std::abs(dm)));
    EXPECT_NEAR(g.arguments[1].values[0], db, 1e-5 * (1 + std::abs(db)));
}

TEST(DifferentiationTest, ALaggedSelfCouplingIsRefusedByName)
{
    // B reads its own outflow -- through a relaxation, or directly. The
    // schedule has no self-edges, so it is no feedback loop to it, but the
    // input sees the previous step's value: a lag the sweep does not model.
    for (const bool throughAdapter : {true, false})
    {
        Reservoir b("b");
        DifferentiableWorkflow workflow("self");
        std::string msg;
        ASSERT_TRUE(workflow.addModelComponent(&b, nullptr, &msg)) << msg;
        workflow.initialize();
        RelaxationAdaptedOutput relax("relax", b.out, SdkAdaptedOutputFactory::instance());
        if (throughAdapter)
        {
            b.out->addAdaptedOutput(&relax);
            connect(&relax, b.in);
        }
        else
        {
            connect(b.out, b.in);
        }
        ASSERT_TRUE(workflow.validate().empty());
        workflow.prepare();
        if (throughAdapter)
            relax.initialize();
        workflow.update();
        workflow.update();
        Gradients g;
        auto seeds = [&](int, Seeds &s) {
            EXPECT_TRUE(s.add(b.out, std::vector<double>(kCells, 1.0)));
        };
        EXPECT_FALSE(workflow.backward(seeds, g, msg));
        EXPECT_NE(msg.find("'b.in' reads 'b''s own output 'out': a lagged self-coupling"),
                  std::string::npos) << msg;
        workflow.finish();
        if (throughAdapter)
            b.out->removeAdaptedOutput(&relax);
    }
}

TEST(DifferentiationTest, ASeedOnANonDifferentiableAdapterIsRefusedByName)
{
    // Nothing differentiable reads through the copy, so only a seed can put
    // a cotangent on it -- and a silent zero would read as "no sensitivity".
    Rig rig(Params{});
    PlainCopy copy("copy", rig.C->out);
    rig.C->out->addAdaptedOutput(&copy);
    copy.initialize();
    rig.run();
    const SeedFunction base = rig.seeds();
    Gradients g;
    std::string message;
    EXPECT_FALSE(rig.workflow->backward(
        [&](int n, Seeds &s) {
            base(n, s);
            EXPECT_TRUE(s.add(&copy, std::vector<double>(kCells, 1.0)));
        },
        g, message));
    EXPECT_NE(message.find("a cotangent reached adapted output 'copy'"), std::string::npos)
        << message;
}
