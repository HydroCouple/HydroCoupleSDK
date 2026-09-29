// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Phase G2.3: the optional LibTorch layer (HYDROCOUPLESDK_WITH_TORCH).
//
//  1. AtenModelComponent -- a component written once in ATen ops, whose
//     vjp and jvp come from torch::autograd, never from a hand-written
//     adjoint. The oracle is the hand-written Reservoir of the Torch-free
//     suite (reservoir_fixture.h), which is the same physics, and central
//     finite differences over forward runs.
//  2. TorchComposition -- a recorded composition run as one autograd node,
//     so C++ LibTorch code (a loss, torch::optim) drives the SDK's reverse
//     engine. The oracle is DifferentiableWorkflow::backward() called
//     directly, and a training loop that must recover a parameter it did
//     not start from.
//
// The composition is the Torch-free suite's, with any of its reservoirs
// swapped for the ATen-native one:
//
//          ┌──── LinearTransform (m, b) ───► B ──► loss
//     A ───┤
//          └───────────────────────────────► C ──► loss
//
// Falsifiers: verification/g2/falsify.sh (rows T*).

#include "reservoir_fixture.h"

#include "hydrocouplesdk/data/lineartransformadaptedoutput.h"
#include "hydrocouplesdk/data/sdkadaptedoutputfactory.h"
#include "hydrocouplesdk/differentiation/differentiableworkflow.h"
#include "hydrocouplesdk/differentiation/torch/atenmodelcomponent.h"
#include "hydrocouplesdk/differentiation/torch/torchcomposition.h"

#include <torch/torch.h>

#include <cmath>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace hcsdk_test;
using namespace HydroCouple::SDK::Differentiation::Torch;

namespace {

// ============================================================================
// The ATen-native reservoir: the same physics as the hand-written one,
// with no adjoint in sight
// ============================================================================

class AtenReservoir : public AtenModelComponent
{
public:
    explicit AtenReservoir(std::string_view id, bool forced = false)
        : AtenModelComponent(id), m_forced(forced)
    {}

    void prepare() override
    {
        AtenModelComponent::prepare();
        for (int i = 0; i < kCells; ++i)
            (*out)[i] = (*k)[i] * (*storage)[i];
    }

    std::vector<IInput *> differentiableInputs() const override { return {in}; }
    std::vector<IArgument *> differentiableArguments() const override { return {k}; }
    std::vector<IOutput *> differentiableOutputs() const override { return {out}; }
    std::vector<IComponentDataItem *> differentiableStates() const override
    {
        return {storage.get()};
    }

    Input1DDouble *in = nullptr;
    Output1DDouble *out = nullptr;
    Output1DDouble *spill = nullptr;
    Argument1DDouble *k = nullptr;
    std::unique_ptr<Output1DDouble> storage;
    int stepCalls = 0;
    int returnStates = 1;   //!< how many states step() returns (a planted fault when != 1)

protected:
    StepResult step(const std::vector<torch::Tensor> &states,
                    const std::vector<torch::Tensor> &inputs,
                    const std::vector<torch::Tensor> &arguments) override
    {
        ++stepCalls;
        const torch::Tensor &s = states[0];
        const torch::Tensor &kk = arguments[0];
        torch::Tensor inflow = inputs[0];
        if (m_forced)
        {
            std::vector<double> f(kCells);
            for (int i = 0; i < kCells; ++i)
                f[i] = forcing(stepIndex(), i);
            inflow = inflow + torch::tensor(f, torch::kFloat64);
        }
        torch::Tensor after = s + kDt * (inflow - kk * s - kC * s * s);
        StepResult r;
        for (int i = 0; i < returnStates; ++i)
            r.states.push_back(after);
        r.outputs.push_back(kk * after);
        return r;
    }

    void afterStep(const StepResult &result) override
    {
        std::string message;
        EXPECT_TRUE(writeItem(spill, result.states[0], &message)) << message;
    }

    void createArguments() override
    {
        m_quantity.reset(Quantity::unitLess("R"));
        k = new Argument1DDouble("k", &m_dim, kCells, m_quantity.get(), this);
        for (int i = 0; i < kCells; ++i)
            (*k)[i] = 0.3;
        addArgument(k);
    }
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        in = new Input1DDouble("in", &m_dim, kCells, m_quantity.get(), this);
        addInput(in);
    }
    void createOutputs() override
    {
        out = new Output1DDouble("out", &m_dim, kCells, m_quantity.get(), this);
        spill = new Output1DDouble("spill", &m_dim, kCells, m_quantity.get(), this);
        addOutput(out);
        addOutput(spill);
        storage = std::make_unique<Output1DDouble>("storage", &m_dim, kCells,
                                                   m_quantity.get(), this);
        for (int i = 0; i < kCells; ++i)
            (*storage)[i] = 5.0;
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension m_dim{"cell", "Cell"};
    std::unique_ptr<Quantity> m_quantity;
    bool m_forced;
};

// ============================================================================
// Either reservoir, behind one view
// ============================================================================

enum class Kind { HandWritten, Aten };

struct Res
{
    Res(Kind kind, std::string_view id, bool forced = false)
    {
        if (kind == Kind::Aten)
        {
            auto r = std::make_unique<AtenReservoir>(id, forced);
            aten = r.get();
            owner = std::move(r);
        }
        else
        {
            auto r = std::make_unique<Reservoir>(id, true, true, forced);
            hand = r.get();
            owner = std::move(r);
        }
    }
    // Valid after initialize().
    Input1DDouble *in() const { return aten ? aten->in : hand->in; }
    Output1DDouble *out() const { return aten ? aten->out : hand->out; }
    Argument1DDouble *k() const { return aten ? aten->k : hand->k; }
    Output1DDouble *storage() const { return aten ? aten->storage.get() : hand->storage.get(); }
    IModelComponent *component() const { return owner.get(); }
    IDifferentiableModelComponent *diff() const
    {
        return dynamic_cast<IDifferentiableModelComponent *>(owner.get());
    }

    std::unique_ptr<AbstractModelComponent> owner;
    AtenReservoir *aten = nullptr;
    Reservoir *hand = nullptr;
};

// ============================================================================
// One step of one reservoir, standalone
// ============================================================================

struct Point
{
    std::vector<double> s{6.0, 2.0, 9.0};
    std::vector<double> inflow{0.7, 1.3, 0.2};
    std::vector<double> k{0.30, 0.20, 0.45};
};

//! A standalone reservoir, prepared, stepped once from \p p.
std::unique_ptr<Res> steppedOnce(Kind kind, const Point &p)
{
    auto r = std::make_unique<Res>(kind, "r");
    r->component()->initialize();
    for (int i = 0; i < kCells; ++i)
    {
        (*r->storage())[i] = p.s[i];
        (*r->k())[i] = p.k[i];
        (*r->in())[i] = p.inflow[i];
    }
    EXPECT_TRUE(r->component()->validate().empty());
    r->component()->prepare();
    r->component()->update();
    EXPECT_EQ(r->component()->status(), CStatus::Updated);
    return r;
}

struct Vjp
{
    std::vector<double> in, k, s;
};

Vjp vjpOf(Res &r, const std::vector<double> &qbar, const std::vector<double> &sbar)
{
    const std::vector<int64_t> shape{kCells};
    std::vector<double> qb = qbar, sb = sbar;
    Vjp v;
    v.in.assign(kCells, -99.0);
    v.k.assign(kCells, -99.0);
    v.s.assign(kCells, -99.0);
    const DifferentialEntry seeds[] = {
        {itemOf(r.out()), R::Output, hostBuffer(qb, shape)},
        {itemOf(r.storage()), R::StateAfter, hostBuffer(sb, shape)}};
    const DifferentialEntry results[] = {
        {itemOf(r.in()), R::Input, hostBuffer(v.in, shape)},
        {itemOf(r.k()), R::Argument, hostBuffer(v.k, shape)},
        {itemOf(r.storage()), R::StateBefore, hostBuffer(v.s, shape)}};
    std::string message;
    EXPECT_TRUE(r.diff()->vjp(seeds, results, &message)) << message;
    return v;
}

//! <qbar, Q'> + <sbar, S'> after one step from \p p (forward only).
double functional(Kind kind, const Point &p, const std::vector<double> &qbar,
                  const std::vector<double> &sbar)
{
    auto r = steppedOnce(kind, p);
    double f = 0.0;
    for (int i = 0; i < kCells; ++i)
        f += qbar[i] * (*r->out())[i] + sbar[i] * (*r->storage())[i];
    return f;
}

const std::vector<double> kQbar{0.9, -1.4, 0.35};
const std::vector<double> kSbar{-0.2, 0.6, 1.1};

double dot(const std::vector<double> &a, const std::vector<double> &b)
{
    double d = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
        d += a[i] * b[i];
    return d;
}

} // namespace

// ============================================================================
// 1. AtenModelComponent
// ============================================================================

TEST(TorchDifferentiationTest, AnAtenStepIsTheHandWrittenStep)
{
    auto hand = steppedOnce(Kind::HandWritten, Point{});
    auto aten = steppedOnce(Kind::Aten, Point{});
    for (int i = 0; i < kCells; ++i)
    {
        EXPECT_DOUBLE_EQ((*aten->storage())[i], (*hand->storage())[i]);
        EXPECT_DOUBLE_EQ((*aten->out())[i], (*hand->out())[i]);
        EXPECT_DOUBLE_EQ((*aten->aten->spill)[i], (*hand->hand->spill)[i]);
    }
}

TEST(TorchDifferentiationTest, AnAtenVjpIsTheHandWrittenAdjoint)
{
    // No adjoint was written for AtenReservoir: this is torch::autograd.
    auto hand = steppedOnce(Kind::HandWritten, Point{});
    auto aten = steppedOnce(Kind::Aten, Point{});
    const Vjp h = vjpOf(*hand, kQbar, kSbar);
    const Vjp a = vjpOf(*aten, kQbar, kSbar);
    for (int i = 0; i < kCells; ++i)
    {
        EXPECT_NEAR(a.in[i], h.in[i], 1e-14);
        EXPECT_NEAR(a.k[i], h.k[i], 1e-13);
        EXPECT_NEAR(a.s[i], h.s[i], 1e-14);
    }
}

TEST(TorchDifferentiationTest, AnAtenVjpMatchesFiniteDifferences)
{
    auto aten = steppedOnce(Kind::Aten, Point{});
    const Vjp a = vjpOf(*aten, kQbar, kSbar);
    const double eps = 1e-6;
    for (int i = 0; i < kCells; ++i)
    {
        auto fd = [&](std::vector<double> Point::*field) {
            Point plus, minus;
            (plus.*field)[i] += eps;
            (minus.*field)[i] -= eps;
            return (functional(Kind::Aten, plus, kQbar, kSbar) -
                    functional(Kind::Aten, minus, kQbar, kSbar)) / (2 * eps);
        };
        EXPECT_NEAR(a.in[i], fd(&Point::inflow), 1e-8) << "cell " << i;
        EXPECT_NEAR(a.k[i], fd(&Point::k), 1e-8) << "cell " << i;
        EXPECT_NEAR(a.s[i], fd(&Point::s), 1e-8) << "cell " << i;
    }
}

TEST(TorchDifferentiationTest, AnAtenJvpIsTheTransposeOfItsVjp)
{
    // <J t, s> == <t, J^T s> for random t and s; and J t is the directional
    // derivative finite differences see.
    auto aten = steppedOnce(Kind::Aten, Point{});
    std::mt19937 rng(7);
    std::uniform_real_distribution<double> u(-1.0, 1.0);
    auto random = [&] {
        std::vector<double> v(kCells);
        for (double &x : v)
            x = u(rng);
        return v;
    };
    const std::vector<int64_t> shape{kCells};
    for (int trial = 0; trial < 3; ++trial)
    {
        std::vector<double> tIn = random(), tK = random(), tS = random();
        std::vector<double> qbar = random(), sbar = random();
        std::vector<double> jQ(kCells, -99.0), jS(kCells, -99.0);
        const DifferentialEntry tangents[] = {
            {itemOf(aten->in()), R::Input, hostBuffer(tIn, shape)},
            {itemOf(aten->k()), R::Argument, hostBuffer(tK, shape)},
            {itemOf(aten->storage()), R::StateBefore, hostBuffer(tS, shape)}};
        const DifferentialEntry results[] = {
            {itemOf(aten->out()), R::Output, hostBuffer(jQ, shape)},
            {itemOf(aten->storage()), R::StateAfter, hostBuffer(jS, shape)}};
        std::string message;
        ASSERT_TRUE(aten->diff()->jvp(tangents, results, &message)) << message;

        const Vjp v = vjpOf(*aten, qbar, sbar);
        const double lhs = dot(jQ, qbar) + dot(jS, sbar);
        const double rhs = dot(tIn, v.in) + dot(tK, v.k) + dot(tS, v.s);
        EXPECT_NEAR(lhs, rhs, 1e-12) << "trial " << trial;

        const double eps = 1e-6;
        Point plus, minus;
        for (int i = 0; i < kCells; ++i)
        {
            plus.inflow[i] += eps * tIn[i];
            plus.k[i] += eps * tK[i];
            plus.s[i] += eps * tS[i];
            minus.inflow[i] -= eps * tIn[i];
            minus.k[i] -= eps * tK[i];
            minus.s[i] -= eps * tS[i];
        }
        const double fd = (functional(Kind::Aten, plus, qbar, sbar) -
                           functional(Kind::Aten, minus, qbar, sbar)) / (2 * eps);
        EXPECT_NEAR(lhs, fd, 1e-8) << "trial " << trial;
    }
}

TEST(TorchDifferentiationTest, AnUnseededOutputContributesNothing)
{
    // Seeding only the state: Q's cotangent is zero, not stale.
    auto aten = steppedOnce(Kind::Aten, Point{});
    auto hand = steppedOnce(Kind::HandWritten, Point{});
    const std::vector<int64_t> shape{kCells};
    std::vector<double> sb = kSbar, a(kCells, -99.0);
    const DifferentialEntry seeds[] = {{itemOf(aten->storage()), R::StateAfter, hostBuffer(sb, shape)}};
    const DifferentialEntry results[] = {{itemOf(aten->k()), R::Argument, hostBuffer(a, shape)}};
    std::string message;
    ASSERT_TRUE(aten->diff()->vjp(seeds, results, &message)) << message;
    const Vjp h = vjpOf(*hand, std::vector<double>(kCells, 0.0), kSbar);
    for (int i = 0; i < kCells; ++i)
        EXPECT_NEAR(a[i], h.k[i], 1e-13);
}

TEST(TorchDifferentiationTest, WithNoSeedsEveryResultIsWrittenAsZero)
{
    // Results are written, not accumulated and not left alone: a result
    // buffer holding garbage comes back zero when nothing is seeded.
    auto aten = steppedOnce(Kind::Aten, Point{});
    const std::vector<int64_t> shape{kCells};
    std::vector<double> in(kCells, -99.0), k(kCells, -99.0), s(kCells, -99.0);
    const DifferentialEntry results[] = {
        {itemOf(aten->in()), R::Input, hostBuffer(in, shape)},
        {itemOf(aten->k()), R::Argument, hostBuffer(k, shape)},
        {itemOf(aten->storage()), R::StateBefore, hostBuffer(s, shape)}};
    std::string message;
    ASSERT_TRUE(aten->diff()->vjp({}, results, &message)) << message;
    for (int i = 0; i < kCells; ++i)
    {
        EXPECT_EQ(in[i], 0.0);
        EXPECT_EQ(k[i], 0.0);
        EXPECT_EQ(s[i], 0.0);
    }
}

TEST(TorchDifferentiationTest, AnAtenComponentWithNoStepRefusesToDifferentiate)
{
    Res r(Kind::Aten, "r");
    r.component()->initialize();
    ASSERT_TRUE(r.component()->validate().empty());
    r.component()->prepare();
    const std::vector<int64_t> shape{kCells};
    std::vector<double> qb = kQbar, out(kCells);
    const DifferentialEntry seeds[] = {{itemOf(r.out()), R::Output, hostBuffer(qb, shape)}};
    const DifferentialEntry results[] = {{itemOf(r.k()), R::Argument, hostBuffer(out, shape)}};
    std::string message;
    EXPECT_FALSE(r.diff()->vjp(seeds, results, &message));
    EXPECT_NE(message.find("no step to differentiate"), std::string::npos) << message;
}

TEST(TorchDifferentiationTest, AnEntryThatIsNotTheComponentsIsRefused)
{
    auto aten = steppedOnce(Kind::Aten, Point{});
    const std::vector<int64_t> shape{kCells};
    std::vector<double> qb = kQbar, out(kCells);
    // The spill output is not differentiated; asking for it is an error,
    // not a silent zero.
    const DifferentialEntry seeds[] = {{itemOf(aten->aten->spill), R::Output, hostBuffer(qb, shape)}};
    const DifferentialEntry results[] = {{itemOf(aten->k()), R::Argument, hostBuffer(out, shape)}};
    std::string message;
    EXPECT_FALSE(aten->diff()->vjp(seeds, results, &message));
    EXPECT_NE(message.find("spill"), std::string::npos) << message;
}

TEST(TorchDifferentiationTest, AStepThatReturnsTheWrongStatesFailsTheComponent)
{
    Res r(Kind::Aten, "r");
    r.component()->initialize();
    r.aten->returnStates = 2;
    ASSERT_TRUE(r.component()->validate().empty());
    r.component()->prepare();
    r.component()->update();
    EXPECT_EQ(r.component()->status(), CStatus::Failed);
}

TEST(TorchDifferentiationTest, AnAtenCheckpointRestoresExactly)
{
    auto r = steppedOnce(Kind::Aten, Point{});
    auto *cp = dynamic_cast<ICheckpointableModelComponent *>(r->component());
    ASSERT_NE(cp, nullptr);
    std::string token, message;
    ASSERT_TRUE(cp->saveState(token, message)) << message;
    std::vector<double> s1(kCells), q1(kCells), sp1(kCells);
    for (int i = 0; i < kCells; ++i)
        s1[i] = (*r->storage())[i];

    r->component()->update();
    r->component()->update();
    for (int i = 0; i < kCells; ++i)
    {
        q1[i] = (*r->out())[i];
        sp1[i] = (*r->aten->spill)[i];
    }
    const int stepsAt3 = r->aten->completedSteps();

    ASSERT_TRUE(cp->restoreState(token, message)) << message;
    EXPECT_EQ(r->aten->completedSteps(), 1);
    for (int i = 0; i < kCells; ++i)
        EXPECT_EQ((*r->storage())[i], s1[i]);
    // Restored, it cannot differentiate a step it has not taken since.
    std::vector<double> qb = kQbar, o(kCells);
    const std::vector<int64_t> shape{kCells};
    const DifferentialEntry seeds[] = {{itemOf(r->out()), R::Output, hostBuffer(qb, shape)}};
    const DifferentialEntry results[] = {{itemOf(r->k()), R::Argument, hostBuffer(o, shape)}};
    EXPECT_FALSE(r->diff()->vjp(seeds, results, &message));

    r->component()->update();
    r->component()->update();
    EXPECT_EQ(r->aten->completedSteps(), stepsAt3);
    for (int i = 0; i < kCells; ++i)
    {
        EXPECT_EQ((*r->out())[i], q1[i]);
        EXPECT_EQ((*r->aten->spill)[i], sp1[i]);
    }
    EXPECT_TRUE(cp->releaseState(token, message)) << message;
}

TEST(TorchDifferentiationTest, ABadTokenChangesNothing)
{
    auto r = steppedOnce(Kind::Aten, Point{});
    auto *cp = dynamic_cast<ICheckpointableModelComponent *>(r->component());
    std::string token, message;
    ASSERT_TRUE(cp->saveState(token, message));
    std::vector<double> before(kCells);
    for (int i = 0; i < kCells; ++i)
        before[i] = (*r->storage())[i];
    const std::string truncated = token.substr(0, token.size() / 2);
    r->component()->update();
    std::vector<double> after(kCells);
    for (int i = 0; i < kCells; ++i)
        after[i] = (*r->storage())[i];
    EXPECT_FALSE(cp->restoreState(truncated, message));
    EXPECT_FALSE(cp->restoreState("not a token", message));
    for (int i = 0; i < kCells; ++i)
        EXPECT_EQ((*r->storage())[i], after[i]);
}

// ============================================================================
// The composition, with any reservoir hand-written or ATen-native
// ============================================================================

namespace {

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

struct Kinds
{
    Kind a, b, c;
};

constexpr Kinds kAllHand{Kind::HandWritten, Kind::HandWritten, Kind::HandWritten};
constexpr Kinds kAllAten{Kind::Aten, Kind::Aten, Kind::Aten};
constexpr Kinds kMixed{Kind::Aten, Kind::HandWritten, Kind::Aten};

constexpr int kSteps = 8;

double observed(int step, int cell) { return 1.0 + 0.1 * step + 0.2 * cell; }

struct Rig
{
    Rig(const Kinds &kinds, const Params &p)
        : A(kinds.a, "A", /*forced=*/true), B(kinds.b, "B"), C(kinds.c, "C")
    {
        workflow = std::make_unique<DifferentiableWorkflow>("diff");
        std::string msg;
        EXPECT_TRUE(workflow->addModelComponent(A.component(), nullptr, &msg)) << msg;
        EXPECT_TRUE(workflow->addModelComponent(B.component(), nullptr, &msg)) << msg;
        EXPECT_TRUE(workflow->addModelComponent(C.component(), nullptr, &msg)) << msg;
        workflow->initialize();

        adapter = std::make_unique<LinearTransformAdaptedOutput>(
            "unit", A.out(), SdkAdaptedOutputFactory::instance());
        A.out()->addAdaptedOutput(adapter.get());
        connect(adapter.get(), B.in());
        connect(A.out(), C.in());

        for (int i = 0; i < kCells; ++i)
        {
            (*A.k())[i] = p.kA[i];
            (*B.k())[i] = p.kB[i];
            (*C.k())[i] = p.kC[i];
            (*A.storage())[i] = p.sA0[i];
            (*B.storage())[i] = p.sB0[i];
        }
        EXPECT_TRUE(workflow->validate().empty());
        workflow->prepare();
        setArgument(*adapter, "multiplier", p.m);
        setArgument(*adapter, "offset", p.b);
        adapter->initialize();
    }

    ~Rig()
    {
        workflow->finish();
        A.out()->removeAdaptedOutput(adapter.get());
    }

    IArgument *adapterArgument(const std::string &id) const
    {
        for (IArgument *argument : adapter->arguments())
            if (argument->id() == id)
                return argument;
        return nullptr;
    }

    double run()
    {
        double loss = 0.0;
        for (int n = 1; n <= kSteps; ++n)
        {
            workflow->update();
            for (int i = 0; i < kCells; ++i)
            {
                const double r = (*B.out())[i] - observed(n, i);
                loss += r * r + 0.5 * (*C.out())[i] * (*C.out())[i];
            }
        }
        return loss;
    }

    SeedFunction seeds() const
    {
        return [this](int n, Seeds &s) {
            std::vector<double> gb(kCells), gc(kCells);
            for (int i = 0; i < kCells; ++i)
            {
                gb[i] = 2.0 * ((*B.out())[i] - observed(n, i));
                gc[i] = (*C.out())[i];
            }
            EXPECT_TRUE(s.add(B.out(), gb));
            EXPECT_TRUE(s.add(C.out(), gc));
        };
    }

    //! The gradients in a fixed order: kA, kB, kC, m, b, sA0, sB0.
    std::vector<double> flatten(const Gradients &g) const
    {
        std::vector<double> all;
        auto add = [&all](const std::vector<double> *v, size_t n) {
            if (v)
                all.insert(all.end(), v->begin(), v->end());
            else
                all.insert(all.end(), n, 0.0);
        };
        auto arg = [&g](const IIdentity *owner, const IArgument *a) {
            const ArgumentGradient *x = g.find(owner, a);
            return x ? &x->values : nullptr;
        };
        auto state = [&g](const IModelComponent *c, const IComponentDataItem *s) {
            const StateGradient *x = g.findState(c, s);
            return x ? &x->values : nullptr;
        };
        add(arg(A.component(), A.k()), kCells);
        add(arg(B.component(), B.k()), kCells);
        add(arg(C.component(), C.k()), kCells);
        add(arg(adapter.get(), adapterArgument("multiplier")), 1);
        add(arg(adapter.get(), adapterArgument("offset")), 1);
        add(state(A.component(), A.storage()), kCells);
        add(state(B.component(), B.storage()), kCells);
        return all;
    }

    Res A, B, C;
    std::unique_ptr<LinearTransformAdaptedOutput> adapter;
    std::unique_ptr<DifferentiableWorkflow> workflow;
};

struct Differentiated
{
    Differentiated(const Kinds &kinds, const Params &p = {},
                   CheckpointPolicy policy = CheckpointPolicy::EveryStep)
        : rig(kinds, p)
    {
        rig.workflow->setCheckpointPolicy(policy);
        loss = rig.run();
        std::string message;
        Gradients g;
        EXPECT_TRUE(rig.workflow->backward(rig.seeds(), g, message)) << message;
        gradients = rig.flatten(g);
    }
    Rig rig;
    double loss = 0.0;
    std::vector<double> gradients;
};

double lossAt(const Kinds &kinds, const Params &p)
{
    Rig rig(kinds, p);
    return rig.run();
}

//! Every parameter the gradients are taken with respect to, in flatten()
//! order, as a setter FD can perturb.
std::vector<std::function<void(Params &, double)>> perturbations()
{
    std::vector<std::function<void(Params &, double)>> all;
    for (int i = 0; i < kCells; ++i)
        all.push_back([i](Params &p, double e) { p.kA[i] += e; });
    for (int i = 0; i < kCells; ++i)
        all.push_back([i](Params &p, double e) { p.kB[i] += e; });
    for (int i = 0; i < kCells; ++i)
        all.push_back([i](Params &p, double e) { p.kC[i] += e; });
    all.push_back([](Params &p, double e) { p.m += e; });
    all.push_back([](Params &p, double e) { p.b += e; });
    for (int i = 0; i < kCells; ++i)
        all.push_back([i](Params &p, double e) { p.sA0[i] += e; });
    for (int i = 0; i < kCells; ++i)
        all.push_back([i](Params &p, double e) { p.sB0[i] += e; });
    return all;
}

} // namespace

TEST(TorchDifferentiationTest, AnAtenCompositionDifferentiatesAsTheHandWrittenOne)
{
    Differentiated hand(kAllHand), aten(kAllAten);
    EXPECT_NEAR(aten.loss, hand.loss, 1e-12 * std::abs(hand.loss));
    ASSERT_EQ(aten.gradients.size(), hand.gradients.size());
    ASSERT_EQ(hand.gradients.size(), 17u);
    for (size_t i = 0; i < hand.gradients.size(); ++i)
        EXPECT_NEAR(aten.gradients[i], hand.gradients[i],
                    1e-11 * (1.0 + std::abs(hand.gradients[i])))
            << "gradient " << i;
}

TEST(TorchDifferentiationTest, AMixedCompositionMatchesFiniteDifferences)
{
    // A and C are ATen-native (no hand-written adjoint), B is hand-written,
    // and the adapter between A and B carries its own adjoint: every kind of
    // derivative in one reverse pass.
    Differentiated d(kMixed);
    const auto perturb = perturbations();
    ASSERT_EQ(d.gradients.size(), perturb.size());
    const double eps = 1e-6;
    for (size_t j = 0; j < perturb.size(); ++j)
    {
        Params plus, minus;
        perturb[j](plus, +eps);
        perturb[j](minus, -eps);
        const double fd = (lossAt(kMixed, plus) - lossAt(kMixed, minus)) / (2 * eps);
        EXPECT_NEAR(d.gradients[j], fd, 1e-5 * (1.0 + std::abs(fd))) << "gradient " << j;
    }
}

TEST(TorchDifferentiationTest, TheLadderGivesTheSameGradientsWithAtenComponents)
{
    Differentiated every(kMixed), ladder(kMixed, Params{}, CheckpointPolicy::Logarithmic);
    EXPECT_EQ(ladder.gradients, every.gradients);
}

// ============================================================================
// 2. TorchComposition: the composition as one autograd node
// ============================================================================

namespace {

torch::Tensor tensorOf(const std::vector<double> &v)
{
    return torch::tensor(v, torch::kFloat64);
}

std::vector<double> valuesOf(const torch::Tensor &t)
{
    torch::Tensor c = t.detach().contiguous();
    return {c.data_ptr<double>(), c.data_ptr<double>() + c.numel()};
}

torch::Tensor observations()
{
    std::vector<double> v;
    for (int n = 1; n <= kSteps; ++n)
        for (int i = 0; i < kCells; ++i)
            v.push_back(observed(n, i));
    return torch::tensor(v, torch::kFloat64).reshape({kSteps, kCells});
}

//! Binds all seven parameters of the Torch-free gradient set, in flatten() order.
struct Bound
{
    explicit Bound(Rig &rig) : composition(*rig.workflow, kSteps)
    {
        composition.bindArgument(rig.A.component(), rig.A.k());
        composition.bindArgument(rig.B.component(), rig.B.k());
        composition.bindArgument(rig.C.component(), rig.C.k());
        composition.bindArgument(rig.adapter.get(), rig.adapterArgument("multiplier"));
        composition.bindArgument(rig.adapter.get(), rig.adapterArgument("offset"));
        composition.bindInitialState(rig.A.component(), rig.A.storage());
        composition.bindInitialState(rig.B.component(), rig.B.storage());
        composition.observe(rig.B.out());
        composition.observe(rig.C.out());
    }

    static std::vector<torch::Tensor> parameters(const Params &p)
    {
        std::vector<torch::Tensor> ps{tensorOf(p.kA), tensorOf(p.kB), tensorOf(p.kC),
                                      tensorOf({p.m}), tensorOf({p.b}),
                                      tensorOf(p.sA0), tensorOf(p.sB0)};
        for (auto &t : ps)
            t.requires_grad_(true);
        return ps;
    }

    static torch::Tensor loss(const std::vector<torch::Tensor> &trajectories)
    {
        const torch::Tensor r = trajectories[0] - observations();
        return (r * r).sum() + 0.5 * (trajectories[1] * trajectories[1]).sum();
    }

    TorchComposition composition;
};

} // namespace

TEST(TorchDifferentiationTest, TorchCompositionGradientsAreTheEngines)
{
    Differentiated direct(kMixed);

    Rig rig(kMixed, Params{});
    Bound bound(rig);
    auto params = Bound::parameters(Params{});
    torch::Tensor loss = Bound::loss(bound.composition(params));
    EXPECT_NEAR(loss.item<double>(), direct.loss, 1e-12 * direct.loss);
    loss.backward();

    std::vector<double> viaTorch;
    for (const auto &p : params)
    {
        ASSERT_TRUE(p.grad().defined());
        const auto v = valuesOf(p.grad());
        viaTorch.insert(viaTorch.end(), v.begin(), v.end());
    }
    ASSERT_EQ(viaTorch.size(), direct.gradients.size());
    for (size_t i = 0; i < viaTorch.size(); ++i)
        EXPECT_NEAR(viaTorch[i], direct.gradients[i],
                    1e-12 * (1.0 + std::abs(direct.gradients[i])))
            << "gradient " << i;
}

TEST(TorchDifferentiationTest, TorchCompositionRunsAreRepeatable)
{
    // Each call rewinds, rewrites the parameters and runs again: the second
    // run at new parameters is what a fresh composition at those parameters
    // gives, and a third at the first parameters is the first again.
    Params other;
    other.kB = {0.12, 0.44, 0.27};
    other.m = 1.1;
    other.sA0 = {3.0, 7.0, 1.0};
    Differentiated fresh(kMixed, other);

    Rig rig(kMixed, Params{});
    Bound bound(rig);
    const double first = Bound::loss(bound.composition(Bound::parameters(Params{}))).item<double>();

    auto params = Bound::parameters(other);
    torch::Tensor second = Bound::loss(bound.composition(params));
    EXPECT_NEAR(second.item<double>(), fresh.loss, 1e-12 * fresh.loss);
    second.backward();
    std::vector<double> g;
    for (const auto &p : params)
    {
        const auto v = valuesOf(p.grad());
        g.insert(g.end(), v.begin(), v.end());
    }
    for (size_t i = 0; i < g.size(); ++i)
        EXPECT_NEAR(g[i], fresh.gradients[i], 1e-12 * (1.0 + std::abs(fresh.gradients[i])))
            << "gradient " << i;

    const double third = Bound::loss(bound.composition(Bound::parameters(Params{}))).item<double>();
    EXPECT_EQ(third, first);
    EXPECT_EQ(bound.composition.runs(), 3);
}

TEST(TorchDifferentiationTest, OnlyTheMostRecentRunCanBeDifferentiated)
{
    Rig rig(kMixed, Params{});
    Bound bound(rig);
    torch::Tensor stale = Bound::loss(bound.composition(Bound::parameters(Params{})));
    torch::Tensor latest = Bound::loss(bound.composition(Bound::parameters(Params{})));
    try
    {
        stale.backward();
        FAIL() << "backward through a rewound run was not refused";
    }
    catch (const c10::Error &e)
    {
        EXPECT_NE(std::string(e.what()).find("run again"), std::string::npos) << e.what();
    }
    EXPECT_NO_THROW(latest.backward());
}

TEST(TorchDifferentiationTest, RewindingForTheNextRunReleasesTheLastRunsCheckpoints)
{
    Rig rig(kMixed, Params{});
    Bound bound(rig);
    for (int run = 0; run < 4; ++run)
    {
        torch::Tensor loss = Bound::loss(bound.composition(Bound::parameters(Params{})));
        loss.backward();
        // The hand-written B counts its live tokens: only this run's.
        EXPECT_EQ(rig.B.hand->liveCheckpoints(), kSteps) << "run " << run;
    }
    EXPECT_EQ(rig.B.hand->deadTokenUses, 0);
}

TEST(TorchDifferentiationTest, AParameterOfTheWrongShapeIsRefusedByName)
{
    Rig rig(kMixed, Params{});
    Bound bound(rig);
    auto params = Bound::parameters(Params{});
    params[1] = torch::zeros({kCells + 1}, torch::kFloat64);
    try
    {
        bound.composition(params);
        FAIL() << "a wrong-shaped parameter was accepted";
    }
    catch (const c10::Error &e)
    {
        EXPECT_NE(std::string(e.what()).find("\"k\""), std::string::npos) << e.what();
    }
}

TEST(TorchDifferentiationTest, ATrainingLoopRecoversAKnownParameter)
{
    // Synthetic truth: B's outflow at the true kB. Start elsewhere; Adam over
    // the SDK composition must find kB again.
    const std::vector<double> truth{0.25, 0.35, 0.15};
    torch::Tensor target;
    {
        Rig rig(kMixed, Params{});
        TorchComposition c(*rig.workflow, kSteps);
        c.bindArgument(rig.B.component(), rig.B.k());
        c.observe(rig.B.out());
        torch::NoGradGuard noGrad;
        target = c({tensorOf(truth)})[0].clone();
    }

    Rig rig(kMixed, Params{});
    TorchComposition c(*rig.workflow, kSteps);
    c.bindArgument(rig.B.component(), rig.B.k());
    c.observe(rig.B.out());
    torch::Tensor kB = tensorOf({0.6, 0.1, 0.5}).requires_grad_(true);
    torch::optim::Adam optimizer({kB}, torch::optim::AdamOptions(0.02));

    double first = 0.0, last = 0.0;
    for (int iteration = 0; iteration < 400; ++iteration)
    {
        optimizer.zero_grad();
        torch::Tensor loss = (c({kB})[0] - target).pow(2).mean();
        loss.backward();
        optimizer.step();
        if (iteration == 0)
            first = loss.item<double>();
        last = loss.item<double>();
    }
    EXPECT_LT(last, 1e-6 * first);
    for (int i = 0; i < kCells; ++i)
        EXPECT_NEAR(kB[i].item<double>(), truth[i], 1e-3) << "cell " << i;
}
