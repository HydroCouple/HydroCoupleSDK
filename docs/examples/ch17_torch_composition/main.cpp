// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 17: the optional LibTorch layer. A reservoir written once
 *         in ATen ops (AtenModelComponent), two of them run as one
 *         differentiable torch function (TorchComposition), the gradient
 *         checked against finite differences, then a few optimizer steps.
 *         Built only with HYDROCOUPLESDK_WITH_TORCH=ON.
 */

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/differentiation/differentiableworkflow.h"
#include "hydrocouplesdk/differentiation/torch/atenmodelcomponent.h"
#include "hydrocouplesdk/differentiation/torch/torchcomposition.h"

#include <torch/torch.h>

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Differentiation;
using namespace HydroCouple::SDK::Differentiation::Torch;

namespace {

constexpr int    kCells = 2;
constexpr int    kSteps = 12;
constexpr double kDt    = 1.0;  // h
constexpr double kDrag  = 0.02; // 1/(m³ h)

//! [aten]
/*!
 * The reservoir S' = S + dt (I - k S - c S²), Q = k S', written once as a
 * function of tensors. No adjoint: vjp() and jvp() come from autograd.
 */
class AtenReservoir : public AtenModelComponent
{
    // Metadata first: the items below point to it, and members are destroyed
    // in reverse order of declaration.
    Dimension m_cell{"cell", "Reservoir cell"};
    std::unique_ptr<Quantity> m_quantity;

public:
    AtenReservoir(std::string_view id, bool forced)
        : AtenModelComponent(id), m_quantity(Quantity::unitLess("reservoir")), m_forced(forced)
    {
    }

    void prepare() override
    {
        AtenModelComponent::prepare();
        for (int i = 0; i < kCells; ++i)
            (*outflow)[i] = (*recession)[i] * (*storage)[i];
    }

    std::vector<IInput *> differentiableInputs() const override { return {inflow.get()}; }
    std::vector<IArgument *> differentiableArguments() const override
    {
        return {recession.get()};
    }
    std::vector<IOutput *> differentiableOutputs() const override { return {outflow.get()}; }
    std::vector<IComponentDataItem *> differentiableStates() const override
    {
        return {storage.get()};
    }

    // The component owns its items; the add calls only register them.
    std::unique_ptr<Input1DDouble> inflow;
    std::unique_ptr<Output1DDouble> outflow;
    std::unique_ptr<Argument1DDouble> recession;
    std::unique_ptr<Output1DDouble> storage;

protected:
    StepResult step(const std::vector<torch::Tensor> &states,
                    const std::vector<torch::Tensor> &inputs,
                    const std::vector<torch::Tensor> &arguments) override
    {
        const torch::Tensor &s = states[0];
        const torch::Tensor &k = arguments[0];
        torch::Tensor i = inputs[0];
        if (m_forced) // a boundary inflow the component owns, indexed by step
        {
            std::vector<double> f(kCells);
            for (int c = 0; c < kCells; ++c)
                f[c] = 1.0 + 0.5 * std::sin(0.6 * stepIndex() + c);
            i = i + torch::tensor(f, torch::kFloat64);
        }
        const torch::Tensor after = s + kDt * (i - k * s - kDrag * s * s);
        return {{after}, {k * after}};
    }

    void createArguments() override
    {
        recession = std::make_unique<Argument1DDouble>("recession", &m_cell, kCells,
                                                       m_quantity.get(), this);
        addArgument(recession.get());
    }
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        inflow = std::make_unique<Input1DDouble>("inflow", &m_cell, kCells, m_quantity.get(),
                                                 this);
        addInput(inflow.get());
    }
    void createOutputs() override
    {
        outflow = std::make_unique<Output1DDouble>("outflow", &m_cell, kCells,
                                                   m_quantity.get(), this);
        addOutput(outflow.get());
        storage = std::make_unique<Output1DDouble>("storage", &m_cell, kCells,
                                                   m_quantity.get(), this);
    }
    void initializeFailureCleanUp() override {}

private:
    bool m_forced;
};
//! [aten]

int fail(const std::string &what)
{
    std::fprintf(stderr, "FAILED: %s\n", what.c_str());
    return 1;
}

} // namespace

int main()
{
    //! [compose]
    // Components first: the workflow's destructor uses them, so they must
    // outlive it (locals are destroyed in reverse order).
    AtenReservoir a("A", /*forced=*/true), b("B", /*forced=*/false);
    DifferentiableWorkflow workflow("aten-pair");
    std::string message;
    if (!workflow.addModelComponent(&a, nullptr, &message) ||
        !workflow.addModelComponent(&b, nullptr, &message))
        return fail(message);
    workflow.initialize();
    b.inflow->setProvider(a.outflow.get());
    a.outflow->addConsumer(b.inflow.get());
    const double sB0[kCells] = {1.0, 3.0}; // B's initial storage, m³ (not a parameter)
    for (int c = 0; c < kCells; ++c)
        (*b.storage)[c] = sB0[c];
    if (!workflow.validate().empty())
        return fail("validate");
    workflow.prepare(); // the first call needs a freshly prepared workflow

    TorchComposition composition(workflow, kSteps);
    composition.bindArgument(&a, a.recession.get());          // parameter 0
    composition.bindArgument(&b, b.recession.get());          // parameter 1
    composition.bindInitialState(&a, a.storage.get());  // parameter 2
    composition.observe(b.outflow.get());                     // trajectory 0
    //! [compose]

    //! [loss]
    std::vector<double> obs;
    for (int n = 1; n <= kSteps; ++n)
        for (int c = 0; c < kCells; ++c)
            obs.push_back(0.4 + 0.05 * n + 0.1 * c);
    const torch::Tensor observed =
        torch::tensor(obs, torch::kFloat64).reshape({kSteps, kCells});
    auto loss = [&](const std::vector<torch::Tensor> &parameters) {
        const std::vector<torch::Tensor> trajectories = composition(parameters);
        return 0.5 * (trajectories[0] - observed).pow(2).sum();
    };

    torch::Tensor kA  = torch::tensor({0.30, 0.20}, torch::kFloat64).requires_grad_(true);
    torch::Tensor kB  = torch::tensor({0.25, 0.35}, torch::kFloat64).requires_grad_(true);
    torch::Tensor sA0 = torch::tensor({4.0, 2.0}, torch::kFloat64).requires_grad_(true);

    torch::Tensor l = loss({kA, kB, sA0});
    l.backward(); // runs DifferentiableWorkflow::backward() with per-step seeds
    std::printf("loss %.10f; dL/dkB = [%.10f, %.10f]\n", l.item<double>(),
                kB.grad()[0].item<double>(), kB.grad()[1].item<double>());
    //! [loss]

    //! [fd]
    // Finite differences through the same function: every call rewinds and
    // reruns the composition.
    torch::NoGradGuard noGrad;
    const double eps = 1e-6;
    for (int c = 0; c < kCells; ++c)
    {
        torch::Tensor plus = kB.detach().clone(), minus = kB.detach().clone();
        plus[c] += eps;
        minus[c] -= eps;
        const double fd = (loss({kA.detach(), plus, sA0.detach()}).item<double>() -
                           loss({kA.detach(), minus, sA0.detach()}).item<double>()) /
                          (2.0 * eps);
        const double g = kB.grad()[c].item<double>();
        std::printf("kB[%d]: autograd node %.10f, central difference %.10f\n", c, g, fd);
        if (std::abs(g - fd) > 1e-6 * std::abs(fd))
            return fail("TorchComposition gradient");
    }
    //! [fd]

    //! [train]
    // A torch optimizer drives the SDK reverse engine: fit kB to the data.
    torch::Tensor fitted = kB.detach().clone().requires_grad_(true);
    torch::optim::SGD optimizer({fitted}, torch::optim::SGDOptions(0.05));
    double first = 0.0, last = 0.0;
    {
        torch::AutoGradMode enable(true);
        for (int iteration = 0; iteration < 20; ++iteration)
        {
            optimizer.zero_grad();
            torch::Tensor value = loss({kA.detach(), fitted, sA0.detach()});
            value.backward();
            optimizer.step();
            (iteration == 0 ? first : last) = value.item<double>();
        }
    }
    std::printf("SGD: loss %.10f -> %.10f in 20 steps (%d runs), kB = [%.6f, %.6f]\n",
                first, last, composition.runs(), fitted[0].item<double>(),
                fitted[1].item<double>());
    if (!(last < first))
        return fail("the optimizer should lower the loss");
    //! [train]

    workflow.finish();
    return 0;
}
