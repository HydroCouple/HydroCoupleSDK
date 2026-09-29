// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// reservoir_fixture.h -- the stateful, differentiable, checkpointable SDK
// component the differentiation suites share (G2.1 onwards): the
// Torch-free suite (test_differentiation.cpp) and the LibTorch suite
// (test_torchdifferentiation.cpp), which checks an ATen-native component
// against this hand-written adjoint.
#pragma once

#include <gtest/gtest.h>

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/differentiation/differentials.h"

#include <cmath>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace hcsdk_test {

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Differentiation;


using CStatus = IModelComponent::ComponentStatus;
using R = DifferentialRole;

constexpr int kCells = 3;
constexpr double kDt = 1.0;
constexpr double kC = 0.02;

//! Boundary inflow of a forced reservoir at a (1-based) step.
inline double forcing(int step, int cell) { return 1.5 + std::sin(0.7 * step + cell); }

// ============================================================================
// A differentiable, checkpointable SDK component
// ============================================================================

/*!
 * S' = S + dt (I - k S - c S^2),  Q = k S'.
 * The S^2 term makes the derivative depend on the primal state, so a vjp
 * taken at the wrong step cannot pass.
 */
class Reservoir : public AbstractModelComponent,
                  public virtual IDifferentiableModelComponent,
                  public virtual ICheckpointableModelComponent
{
public:
    //! \p forced: add forcing(step) to the inflow -- a boundary condition
    //! the component owns (and its checkpoint restores), as a real model's
    //! time series does. Inputs poked from outside between steps are NOT
    //! recorded by the workflow, so a test must not drive A that way.
    explicit Reservoir(std::string_view id, bool checkpoints = true,
                       bool differentiable = true, bool forced = false)
        : AbstractModelComponent(id), m_checkpoints(checkpoints),
          m_differentiable(differentiable), m_forced(forced)
    {}

    std::vector<std::string> validate() override
    {
        setStatus(CStatus::Validating);
        setStatus(CStatus::Valid);
        return {};
    }

    void prepare() override
    {
        setStatus(CStatus::Preparing);
        for (int i = 0; i < kCells; ++i)
            (*out)[i] = (*k)[i] * (*storage)[i];
        setStatus(CStatus::Updated);
    }

    void update(const std::vector<IOutput *> & = {}) override
    {
        setStatus(CStatus::Updating);
        if (failAt > 0 && steps + 1 == failAt)
        {
            setStatus(CStatus::Failed, "planted failure");
            return;
        }
        applyInputValues();
        m_sBefore.assign(kCells, 0.0);
        m_inflow.assign(kCells, 0.0);
        m_k.assign(kCells, 0.0);
        for (int i = 0; i < kCells; ++i)
        {
            const double s = (*storage)[i];
            m_sBefore[i] = s;
            m_inflow[i] = (*in)[i] + (m_forced ? forcing(steps + 1, i) : 0.0);
            m_k[i] = (*k)[i];
            (*storage)[i] = s + kDt * (m_inflow[i] - m_k[i] * s - kC * s * s);
            (*out)[i] = m_k[i] * (*storage)[i];
            (*spill)[i] = (*storage)[i];
        }
        m_haveStep = true;
        ++steps;
        setStatus(doneAfter > 0 && steps >= doneAfter ? CStatus::Done
                                                      : CStatus::Updated);
    }

    void finish() override
    {
        setStatus(CStatus::Finishing);
        setStatus(CStatus::Finished);
    }

    [[nodiscard]] std::set<Capability> capabilities() const override
    {
        std::set<Capability> caps = AbstractModelComponent::capabilities();
        if (m_checkpoints)
            caps.insert(Capability::Checkpointing);
        if (m_differentiable)
            caps.insert(Capability::Differentiable);
        return caps;
    }

    // -- ICheckpointableModelComponent: the token is the state itself ------
    // Each token carries a serial, so a test can see every checkpoint that
    // was saved and not yet released (liveCheckpoints) and every restore or
    // release of a dead token (deadTokenUses).
    bool saveState(std::string &token, std::string &) override
    {
        const long serial = m_nextSerial++;
        m_live.insert(serial);
        std::ostringstream o;
        o.precision(17);
        o << serial << ' ' << steps;
        for (int i = 0; i < kCells; ++i)
            o << ' ' << (*storage)[i] << ' ' << (*out)[i] << ' ' << (*spill)[i];
        token = o.str();
        return true;
    }

    bool restoreState(const std::string &token, std::string &message) override
    {
        std::istringstream in(token);
        long serial = 0;
        in >> serial;
        if (!m_live.count(serial))
        {
            ++deadTokenUses;
            message = "restore of a released token";
            return false;
        }
        in >> steps;
        for (int i = 0; i < kCells; ++i)
            in >> (*storage)[i] >> (*out)[i] >> (*spill)[i];
        if (in.fail())
        {
            message = "bad token";
            return false;
        }
        m_haveStep = false;
        ++restores;
        if (restoreResetsStatus)
            setStatus(CStatus::Updated);
        return true;
    }

    bool releaseState(const std::string &token, std::string &message) override
    {
        std::istringstream in(token);
        long serial = 0;
        in >> serial;
        if (m_live.erase(serial) != 1)
        {
            ++deadTokenUses;
            message = "release of a released token";
            return false;
        }
        return true;
    }

    [[nodiscard]] int liveCheckpoints() const { return static_cast<int>(m_live.size()); }

    // -- IDifferentiableModelComponent -------------------------------------
    std::vector<IInput *> differentiableInputs() const override { return {in}; }
    std::vector<IArgument *> differentiableArguments() const override { return {k}; }
    std::vector<IOutput *> differentiableOutputs() const override { return {out}; }
    std::vector<IComponentDataItem *> differentiableStates() const override
    {
        return {storage.get()};
    }

    bool vjp(DifferentialSet seeds, DifferentialSet results, std::string *message) override
    {
        ++vjpCalls;
        if (!m_haveStep)
            return fail(message, "no step to differentiate");
        const std::vector<int64_t> shape{kCells};
        std::vector<double> qb(kCells, 0.0), sab(kCells, 0.0);
        for (const auto &e : seeds)
        {
            std::vector<double> *into = nullptr;
            if (e.item == itemOf(out) && e.role == R::Output)
                into = &qb;
            else if (e.item == itemOf(storage.get()) && e.role == R::StateAfter)
                into = &sab;
            else
                return fail(message, "unknown seed");
            if (!readDifferential(e, shape, *into, message))
                return false;
        }
        std::vector<double> ib(kCells), kb(kCells), sb(kCells);
        for (int i = 0; i < kCells; ++i)
        {
            const double s = m_sBefore[i], kk = m_k[i];
            const double sa = s + kDt * (m_inflow[i] - kk * s - kC * s * s);
            const double total = sab[i] + kk * qb[i];
            ib[i] = kDt * total;
            kb[i] = qb[i] * sa - kDt * s * total;
            sb[i] = (1.0 - kDt * kk - 2.0 * kDt * kC * s) * total;
        }
        for (const auto &e : results)
        {
            const std::vector<double> *from = nullptr;
            if (e.item == itemOf(in) && e.role == R::Input)
                from = &ib;
            else if (e.item == itemOf(k) && e.role == R::Argument)
                from = &kb;
            else if (e.item == itemOf(storage.get()) && e.role == R::StateBefore)
                from = &sb;
            else
                return fail(message, "unknown result");
            if (!writeDifferential(e, shape, *from, message))
                return false;
        }
        return true;
    }

    bool jvp(DifferentialSet, DifferentialSet, std::string *message) override
    {
        return fail(message, "not needed by G2.1");
    }

    Input1DDouble *in = nullptr;
    Output1DDouble *out = nullptr;
    Output1DDouble *spill = nullptr;   //!< an output this component does NOT differentiate
    Argument1DDouble *k = nullptr;
    std::unique_ptr<Output1DDouble> storage;
    int steps = 0;
    int restores = 0;
    int vjpCalls = 0;
    int deadTokenUses = 0;             //!< restores/releases of a released token
    int doneAfter = 0;                 //!< report Done after this many steps (0: never)
    int failAt = 0;                    //!< fail the update() that would take this step (0: never)
    bool restoreResetsStatus = true;   //!< a well-behaved restore resumes as Updated

protected:
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
    static bool fail(std::string *message, const char *text)
    {
        if (message)
            *message = text;
        return false;
    }

    Dimension m_dim{"cell", "Cell"};
    std::unique_ptr<Quantity> m_quantity;
    bool m_checkpoints;
    bool m_differentiable;
    bool m_forced;
    bool m_haveStep = false;
    std::vector<double> m_sBefore, m_inflow, m_k;
    long m_nextSerial = 1;
    std::set<long> m_live;
};

inline void connect(IOutput *provider, Input1DDouble *consumer)
{
    ASSERT_TRUE(consumer->setProvider(provider));
    provider->addConsumer(consumer);
}

inline void setArgument(IAdaptedOutput &adapted, const std::string &id, double value)
{
    for (IArgument *argument : adapted.arguments())
        if (argument->id() == id)
            (*dynamic_cast<Argument1DDouble *>(argument))[0] = value;
}

} // namespace hcsdk_test
