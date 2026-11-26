// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 17: reverse-mode gradients of a two-reservoir composition
 *         through DifferentiableWorkflow, checked against finite differences
 *         of forward runs, plus the JVP/VJP dot-product test of one step.
 */

#include "reservoir.h"

#include "hydrocouplesdk/differentiation/differentiableworkflow.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK::Differentiation;

namespace {

constexpr int kSteps = 12;

//! The observations the loss compares B's outflow with (m³/h).
double observed(int step, int cell) { return 0.4 + 0.05 * step + 0.1 * cell; }

//! The parameters a gradient is taken with respect to.
struct Params
{
    double kA[kCells]{0.30, 0.20}; // 1/h
    double kB[kCells]{0.25, 0.35}; // 1/h
    double sA0[kCells]{4.0, 2.0};  // m³
    double sB0[kCells]{1.0, 3.0};  // m³
};

//! [rig]
/*!
 * A -> B: the forced reservoir A feeds reservoir B; the loss is
 * L = sum over steps and cells of (Q_B - observed)^2 / 2.
 */
struct Rig
{
    explicit Rig(const Params &p, CheckpointPolicy policy = CheckpointPolicy::EveryStep)
    {
        std::string message;
        workflow.setCheckpointPolicy(policy);
        ok = workflow.addModelComponent(&a, nullptr, &message) &&
             workflow.addModelComponent(&b, nullptr, &message);
        workflow.initialize(); // initializes the components: their items exist now
        b.inflow->setProvider(a.outflow.get());
        a.outflow->addConsumer(b.inflow.get());
        for (int i = 0; i < kCells; ++i)
        {
            (*a.recession)[i] = p.kA[i];
            (*b.recession)[i] = p.kB[i];
            (*a.storage)[i]   = p.sA0[i];
            (*b.storage)[i]   = p.sB0[i];
        }
        ok = ok && workflow.validate().empty();
        workflow.prepare();
    }

    //! Runs the steps forward and returns the loss.
    double run()
    {
        double loss = 0.0;
        for (int n = 1; n <= kSteps; ++n)
        {
            workflow.update();
            for (int i = 0; i < kCells; ++i)
            {
                const double r = (*b.outflow)[i] - observed(n, i);
                loss += 0.5 * r * r;
            }
        }
        return loss;
    }

    //! dL/dQ_B at step n. The composition holds step n's values when it runs.
    SeedFunction seeds()
    {
        return [this](int n, Seeds &s) {
            std::vector<double> cotangent(kCells);
            for (int i = 0; i < kCells; ++i)
                cotangent[i] = (*b.outflow)[i] - observed(n, i);
            (void)s.add(b.outflow.get(), cotangent);
        };
    }

    // Components before the workflow: its destructor uses them, so they must
    // outlive it (members are destroyed in reverse order).
    Reservoir a{"A", /*forced=*/true};
    Reservoir b{"B", /*forced=*/false};
    DifferentiableWorkflow workflow{"two-reservoirs"};
    bool ok = false;
};
//! [rig]

//! The loss by forward runs only: the finite-difference oracle never calls vjp().
double lossAt(const Params &p)
{
    Rig rig(p);
    return rig.run();
}

double dot(const std::vector<double> &x, const std::vector<double> &y)
{
    double sum = 0.0;
    for (size_t i = 0; i < x.size(); ++i)
        sum += x[i] * y[i];
    return sum;
}

int fail(const std::string &what)
{
    std::fprintf(stderr, "FAILED: %s\n", what.c_str());
    return 1;
}

} // namespace

int main()
{
    const Params p;

    //! [backward]
    Rig rig(p);
    if (!rig.ok || !rig.workflow.differentiabilityProblems().empty())
        return fail("the composition should be differentiable");
    const double loss = rig.run();

    Gradients gradients;
    std::string message;
    if (!rig.workflow.backward(rig.seeds(), gradients, message))
        return fail(message);
    std::printf("loss %.10f over %d steps; %d checkpoints, %d replays, peak %d\n", loss,
                rig.workflow.recordedSteps(), rig.workflow.storedCheckpoints(),
                rig.workflow.replays(), rig.workflow.peakCheckpoints());

    const ArgumentGradient *dkA = gradients.find(&rig.a, rig.a.recession.get());
    const ArgumentGradient *dkB = gradients.find(&rig.b, rig.b.recession.get());
    const StateGradient *dsA0 = gradients.findState(&rig.a, rig.a.storage.get());
    const StateGradient *dsB0 = gradients.findState(&rig.b, rig.b.storage.get());
    if (!dkA || !dkB || !dsA0 || !dsB0)
        return fail("a gradient is missing");
    //! [backward]

    //! [check]
    // Central differences of forward runs, one parameter at a time.
    struct Probe
    {
        const char *name;
        double *(*slot)(Params &);
        double gradient;
    };
    const Probe probes[] = {
        {"kA[0]", [](Params &q) { return &q.kA[0]; }, dkA->values[0]},
        {"kA[1]", [](Params &q) { return &q.kA[1]; }, dkA->values[1]},
        {"kB[0]", [](Params &q) { return &q.kB[0]; }, dkB->values[0]},
        {"kB[1]", [](Params &q) { return &q.kB[1]; }, dkB->values[1]},
        {"SA0[0]", [](Params &q) { return &q.sA0[0]; }, dsA0->values[0]},
        {"SB0[1]", [](Params &q) { return &q.sB0[1]; }, dsB0->values[1]},
    };
    const double eps = 1e-6;
    std::printf("%-7s %16s %16s %10s\n", "param", "backward()", "central diff", "rel err");
    for (const Probe &probe : probes)
    {
        Params plus = p, minus = p;
        *probe.slot(plus) += eps;
        *probe.slot(minus) -= eps;
        const double fd = (lossAt(plus) - lossAt(minus)) / (2.0 * eps);
        const double error = std::abs(fd - probe.gradient) / std::max(1e-12, std::abs(fd));
        std::printf("%-7s %16.10f %16.10f %10.2e\n", probe.name, probe.gradient, fd, error);
        if (error > 1e-6)
            return fail(std::string("gradient of ") + probe.name);
    }
    //! [check]

    //! [ladder]
    // The logarithmic ladder keeps fewer checkpoints and gives the same bits.
    Rig ladder(p, CheckpointPolicy::Logarithmic);
    ladder.run();
    Gradients again;
    if (!ladder.workflow.backward(ladder.seeds(), again, message))
        return fail(message);
    std::printf("Logarithmic: %d checkpoints, %d replays, peak %d\n",
                ladder.workflow.storedCheckpoints(), ladder.workflow.replays(),
                ladder.workflow.peakCheckpoints());
    for (size_t g = 0; g < gradients.arguments.size(); ++g)
        if (again.arguments[g].values != gradients.arguments[g].values)
            return fail("the ladder changed a gradient");
    //! [ladder]

    //! [dottest]
    // The dot-product test of B's most recent step: for any tangent t of
    // (I, k, S) and any cotangent w of (Q, S'), <w, J t> must equal <J^T w, t>.
    // After backward() the composition is restored, not linearized, so step
    // it once more to give B a step to differentiate.
    rig.workflow.update();
    const std::vector<int64_t> shape{kCells};
    std::vector<double> tI{0.3, -0.7}, tK{0.11, 0.05}, tS{-0.2, 0.9}; // tangents
    std::vector<double> wQ{1.3, -0.4}, wS{0.6, 0.25};                // cotangents
    std::vector<double> jQ(kCells), jS(kCells);                      // J t
    std::vector<double> rI(kCells), rK(kCells), rS(kCells);          // J^T w

    auto entry = [&](const IComponentDataItem *item, DifferentialRole role,
                     std::vector<double> &values) {
        return DifferentialEntry{item, role, hostBuffer(values, shape)};
    };
    const DifferentialEntry tangents[] = {
        entry(itemOf(rig.b.inflow.get()), DifferentialRole::Input, tI),
        entry(itemOf(rig.b.recession.get()), DifferentialRole::Argument, tK),
        entry(itemOf(rig.b.storage.get()), DifferentialRole::StateBefore, tS)};
    const DifferentialEntry products[] = {
        entry(itemOf(rig.b.outflow.get()), DifferentialRole::Output, jQ),
        entry(itemOf(rig.b.storage.get()), DifferentialRole::StateAfter, jS)};
    const DifferentialEntry cotangents[] = {
        entry(itemOf(rig.b.outflow.get()), DifferentialRole::Output, wQ),
        entry(itemOf(rig.b.storage.get()), DifferentialRole::StateAfter, wS)};
    const DifferentialEntry pullbacks[] = {
        entry(itemOf(rig.b.inflow.get()), DifferentialRole::Input, rI),
        entry(itemOf(rig.b.recession.get()), DifferentialRole::Argument, rK),
        entry(itemOf(rig.b.storage.get()), DifferentialRole::StateBefore, rS)};

    if (!rig.b.jvp(tangents, products, &message) || !rig.b.vjp(cotangents, pullbacks, &message))
        return fail(message);
    const double forward = dot(wQ, jQ) + dot(wS, jS);
    const double reverse = dot(rI, tI) + dot(rK, tK) + dot(rS, tS);
    std::printf("<w, J t> = %.15f\n<J^T w, t> = %.15f\n", forward, reverse);
    if (std::abs(forward - reverse) > 1e-13 * std::abs(forward))
        return fail("the JVP and the VJP are not transposes");
    //! [dottest]

    //! [rewind]
    // A training loop repeats the run with new arguments without preparing
    // again: rewind() returns every component to before step 1.
    if (!ladder.workflow.rewind(message))
        return fail(message);
    for (int i = 0; i < kCells; ++i)
        (*ladder.b.recession)[i] = p.kB[i] - 0.01 * dkB->values[i]; // one descent step
    const double improved = ladder.run();
    std::printf("after one gradient step on kB: loss %.10f (was %.10f)\n", improved, loss);
    if (!(improved < loss))
        return fail("a gradient step should lower the loss");
    //! [rewind]

    rig.workflow.finish();
    ladder.workflow.finish();
    std::printf("released %d checkpoint tokens, %d refused\n",
                rig.workflow.releasedTokens() + ladder.workflow.releasedTokens(),
                rig.workflow.refusedReleases() + ladder.workflow.refusedReleases());
    return 0;
}
