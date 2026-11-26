// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   reservoir.h
 * \brief  Chapter 17: a differentiable, checkpointable reservoir component
 *         with a hand-written VJP and JVP.
 */

#ifndef DOCS_EXAMPLE_CH17_RESERVOIR_H
#define DOCS_EXAMPLE_CH17_RESERVOIR_H

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

constexpr int    kCells = 2;    //!< Cells per reservoir.
constexpr double kDt    = 1.0;  //!< Step length (h).
constexpr double kDrag  = 0.02; //!< Quadratic loss coefficient c (1/(m³ h)).

//! [class]
/*!
 * \brief Two reservoir cells: S' = S + dt (I - k S - c S²), Q = k S'.
 *
 * Storage S (m³) is the state, the recession constants k (1/h) are a
 * differentiable argument, the inflow I (m³/h) is a differentiable input and
 * the outflow Q (m³/h) a differentiable output. A forced reservoir adds its
 * own boundary inflow f(n) at step n; the component owns that forcing, so a
 * restored checkpoint replays it exactly.
 */
class Reservoir : public HydroCouple::SDK::AbstractModelComponent,
                  public virtual HydroCouple::IDifferentiableModelComponent,
                  public virtual HydroCouple::ICheckpointableModelComponent
{
    // Metadata first: the items below point to it, and members are destroyed
    // in reverse order of declaration.
    HydroCouple::SDK::Dimension m_cell{"cell", "Reservoir cell"};
    std::unique_ptr<HydroCouple::SDK::Quantity> m_quantity;

public:
    using Status = HydroCouple::IModelComponent::ComponentStatus;
    using Role   = HydroCouple::DifferentialRole;

    Reservoir(std::string_view id, bool forced)
        : AbstractModelComponent(id),
          m_quantity(HydroCouple::SDK::Quantity::unitLess("reservoir")), m_forced(forced)
    {
    }

    // -- lifecycle -----------------------------------------------------------
    std::vector<std::string> validate() override
    {
        setStatus(Status::Validating);
        setStatus(Status::Valid);
        return {};
    }
    void prepare() override
    {
        setStatus(Status::Preparing);
        for (int i = 0; i < kCells; ++i)
            (*outflow)[i] = (*recession)[i] * (*storage)[i];
        setStatus(Status::Updated);
    }
    void update(const std::vector<HydroCouple::IOutput *> & = {}) override
    {
        setStatus(Status::Updating);
        applyInputValues();
        ++m_steps;
        for (int i = 0; i < kCells; ++i)
        {
            // Keep the linearization point: the values this step started from.
            m_before[i] = (*storage)[i];
            m_inflow[i] = (*inflow)[i] + (m_forced ? forcing(m_steps, i) : 0.0);
            m_k[i]      = (*recession)[i];
            const double s = m_before[i];
            (*storage)[i] = s + kDt * (m_inflow[i] - m_k[i] * s - kDrag * s * s);
            (*outflow)[i] = m_k[i] * (*storage)[i];
        }
        m_haveStep = true;
        setStatus(Status::Updated);
    }
    void finish() override
    {
        setStatus(Status::Finishing);
        setStatus(Status::Finished);
    }
    std::set<HydroCouple::Capability> capabilities() const override
    {
        std::set<HydroCouple::Capability> caps = AbstractModelComponent::capabilities();
        caps.insert(HydroCouple::Capability::Checkpointing);
        caps.insert(HydroCouple::Capability::Differentiable);
        return caps;
    }
    std::vector<HydroCouple::IComponentDataItem *> states() const override
    {
        return {storage.get()};
    }

    //! The forced reservoir's boundary inflow at step n (m³/h).
    static double forcing(int n, int cell) { return 1.0 + 0.5 * std::sin(0.6 * n + cell); }

    // -- ICheckpointableModelComponent: the token is the state ----------------
    bool saveState(std::string &token, std::string &) override
    {
        std::ostringstream out;
        out.precision(17);
        out << m_steps;
        for (int i = 0; i < kCells; ++i)
            out << ' ' << (*storage)[i] << ' ' << (*outflow)[i];
        token = out.str();
        return true;
    }
    bool restoreState(const std::string &token, std::string &message) override
    {
        std::istringstream in(token);
        in >> m_steps;
        for (int i = 0; i < kCells; ++i)
            in >> (*storage)[i] >> (*outflow)[i];
        if (in.fail())
        {
            message = "bad token";
            return false;
        }
        m_haveStep = false; // a restore is not a step: nothing to differentiate
        setStatus(Status::Updated);
        return true;
    }
    bool releaseState(const std::string &, std::string &message) override
    {
        message.clear();
        return true; // nothing to free
    }
    //! [class]

    //! [lists]
    std::vector<HydroCouple::IInput *> differentiableInputs() const override
    {
        return {inflow.get()};
    }
    std::vector<HydroCouple::IArgument *> differentiableArguments() const override
    {
        return {recession.get()};
    }
    std::vector<HydroCouple::IOutput *> differentiableOutputs() const override
    {
        return {outflow.get()};
    }
    std::vector<HydroCouple::IComponentDataItem *> differentiableStates() const override
    {
        return {storage.get()};
    }
    //! [lists]

    //! [vjp]
    bool vjp(HydroCouple::DifferentialSet seeds, HydroCouple::DifferentialSet results,
             std::string *message) override
    {
        using namespace HydroCouple::SDK::Differentiation;
        if (!m_haveStep)
            return fail(message, "no step to differentiate");
        const std::vector<int64_t> shape{kCells};
        std::vector<double> qBar(kCells, 0.0), sAfterBar(kCells, 0.0);
        for (const HydroCouple::DifferentialEntry &e : seeds)
        {
            std::vector<double> *into = nullptr;
            if (e.item == itemOf(outflow.get()) && e.role == Role::Output)
                into = &qBar;
            else if (e.item == itemOf(storage.get()) && e.role == Role::StateAfter)
                into = &sAfterBar;
            else
                return fail(message, "vjp: unknown seed");
            if (!readDifferential(e, shape, *into, message))
                return false;
        }
        std::vector<double> iBar(kCells), kBar(kCells), sBar(kCells);
        for (int i = 0; i < kCells; ++i)
        {
            const double s = m_before[i], k = m_k[i];
            const double after = s + kDt * (m_inflow[i] - k * s - kDrag * s * s);
            const double total = sAfterBar[i] + k * qBar[i]; // d/dS' of the seeds
            iBar[i] = kDt * total;
            kBar[i] = qBar[i] * after - kDt * s * total;
            sBar[i] = (1.0 - kDt * k - 2.0 * kDt * kDrag * s) * total;
        }
        return writeResults(results, iBar, kBar, sBar, message);
    }
    //! [vjp]

    //! [jvp]
    bool jvp(HydroCouple::DifferentialSet seeds, HydroCouple::DifferentialSet results,
             std::string *message) override
    {
        using namespace HydroCouple::SDK::Differentiation;
        if (!m_haveStep)
            return fail(message, "no step to differentiate");
        const std::vector<int64_t> shape{kCells};
        std::vector<double> iDot(kCells, 0.0), kDot(kCells, 0.0), sDot(kCells, 0.0);
        for (const HydroCouple::DifferentialEntry &e : seeds)
        {
            std::vector<double> *into = nullptr;
            if (e.item == itemOf(inflow.get()) && e.role == Role::Input)
                into = &iDot;
            else if (e.item == itemOf(recession.get()) && e.role == Role::Argument)
                into = &kDot;
            else if (e.item == itemOf(storage.get()) && e.role == Role::StateBefore)
                into = &sDot;
            else
                return fail(message, "jvp: unknown seed");
            if (!readDifferential(e, shape, *into, message))
                return false;
        }
        std::vector<double> qDot(kCells), sAfterDot(kCells);
        for (int i = 0; i < kCells; ++i)
        {
            const double s = m_before[i], k = m_k[i];
            const double after = s + kDt * (m_inflow[i] - k * s - kDrag * s * s);
            sAfterDot[i] = (1.0 - kDt * k - 2.0 * kDt * kDrag * s) * sDot[i] +
                           kDt * (iDot[i] - kDot[i] * s);
            qDot[i] = kDot[i] * after + k * sAfterDot[i];
        }
        for (const HydroCouple::DifferentialEntry &e : results)
        {
            const std::vector<double> *from = nullptr;
            if (e.item == itemOf(outflow.get()) && e.role == Role::Output)
                from = &qDot;
            else if (e.item == itemOf(storage.get()) && e.role == Role::StateAfter)
                from = &sAfterDot;
            else
                return fail(message, "jvp: unknown result");
            if (!writeDifferential(e, shape, *from, message))
                return false;
        }
        return true;
    }
    //! [jvp]

    // The component owns its items: addArgument(), addInput() and addOutput()
    // only register them with the base class, which never deletes them.
    std::unique_ptr<HydroCouple::SDK::Input1DDouble> inflow;       //!< I (m³/h)
    std::unique_ptr<HydroCouple::SDK::Output1DDouble> outflow;     //!< Q (m³/h)
    std::unique_ptr<HydroCouple::SDK::Argument1DDouble> recession; //!< k (1/h)
    std::unique_ptr<HydroCouple::SDK::Output1DDouble> storage;     //!< S (m³), the state

protected:
    void createArguments() override
    {
        recession = std::make_unique<HydroCouple::SDK::Argument1DDouble>(
            "recession", &m_cell, kCells, m_quantity.get(), this);
        addArgument(recession.get());
    }
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        inflow = std::make_unique<HydroCouple::SDK::Input1DDouble>(
            "inflow", &m_cell, kCells, m_quantity.get(), this);
        addInput(inflow.get());
    }
    void createOutputs() override
    {
        outflow = std::make_unique<HydroCouple::SDK::Output1DDouble>(
            "outflow", &m_cell, kCells, m_quantity.get(), this);
        addOutput(outflow.get());
        storage = std::make_unique<HydroCouple::SDK::Output1DDouble>(
            "storage", &m_cell, kCells, m_quantity.get(), this);
    }
    void initializeFailureCleanUp() override {}

private:
    static bool fail(std::string *message, const char *text)
    {
        if (message)
            *message = text;
        return false;
    }
    bool writeResults(HydroCouple::DifferentialSet results, const std::vector<double> &iBar,
                      const std::vector<double> &kBar, const std::vector<double> &sBar,
                      std::string *message) const
    {
        using namespace HydroCouple::SDK::Differentiation;
        const std::vector<int64_t> shape{kCells};
        for (const HydroCouple::DifferentialEntry &e : results)
        {
            const std::vector<double> *from = nullptr;
            if (e.item == itemOf(inflow.get()) && e.role == Role::Input)
                from = &iBar;
            else if (e.item == itemOf(recession.get()) && e.role == Role::Argument)
                from = &kBar;
            else if (e.item == itemOf(storage.get()) && e.role == Role::StateBefore)
                from = &sBar;
            else
                return fail(message, "vjp: unknown result");
            if (!writeDifferential(e, shape, *from, message))
                return false;
        }
        return true;
    }

    bool m_forced;
    int m_steps = 0;
    bool m_haveStep = false;
    double m_before[kCells]{}, m_inflow[kCells]{}, m_k[kCells]{};
};

#endif // DOCS_EXAMPLE_CH17_RESERVOIR_H
