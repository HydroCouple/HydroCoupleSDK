// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 5: one component through every lifecycle path the
 *         AbstractModelComponent base class supports, with each status
 *         event printed.
 *
 * A detention pond stores water, S [m3], and releases Q = k S [m3/s]. With
 * no inflow, one explicit step of length dt gives S(n+1) = S(n) (1 - k dt),
 * which the program checks after each update. It then shows, in order:
 * plugin publication through the ABI entry points, the status events of a
 * full run, validate() warnings on the error queue, restarting through
 * finish() -> Created, an initialization failure and its cleanup, and how
 * an illegal transition is reported.
 */

#include "hydrocouplesdk/componentabi.h"
#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/component/abstractmodelcomponentinfo.h"
#include "hydrocouplesdk/core/diagnosticcodes.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/signal.h"
#include "hydrocouplesdk/core/unit.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using Status = IModelComponent::ComponentStatus;

namespace {

int g_failures = 0;

//! Records a failed expectation; main() returns nonzero if any failed.
void check(bool ok, const std::string &what)
{
    if (!ok)
    {
        std::printf("CHECK FAILED: %s\n", what.c_str());
        ++g_failures;
    }
}

constexpr double kTimeStep = 3600.0; // s
constexpr int    kSteps    = 3;

//! [component]
/*!
 * A detention pond. It owns every object it registers: the units,
 * quantities and dimension are members, and the arguments and exchange
 * items are held in unique_ptr members, because the base class's
 * registries do not own what they hold.
 */
class DetentionPond : public AbstractModelComponent
{
public:
    DetentionPond(std::string_view id, AbstractModelComponentInfo *info)
        : AbstractModelComponent(id, "Detention pond", info)
    {
        // Units and quantities exist before createArguments() may run
        // (arguments() is legal while Created). Quantity borrows its unit.
        m_perSecond->dimensionsInternal()->setPower(
            IUnitDimensions::FundamentalUnitDimension::Time, -1.0);
        m_rate.setValueKind(ValueKind::Intensive);
        m_volume.setValueKind(ValueKind::Extensive);
        m_flow.setValueKind(ValueKind::Intensive);
    }

    std::vector<std::string> validate() override
    {
        setStatus(ComponentStatus::Validating);
        std::vector<std::string> messages;
        if (!m_inflow->provider())
            messages.push_back("inflow is not connected; the pond drains with zero inflow");
        // The interface asks for every returned message on the error queue
        // too: Warning while Valid, Error when Invalid.
        for (const std::string &text : messages)
            reportError(ErrorEntry::Severity::Warning, 0, text);
        setStatus(ComponentStatus::Valid);
        return messages;
    }

    void prepare() override
    {
        setStatus(ComponentStatus::Preparing);
        m_step    = 0;
        m_storage = (*m_initialStorage)[0];
        (*m_outflow)[0] = m_k * m_storage; // the initial output is published here
        setStatus(ComponentStatus::Updated, "ready to step");
    }

    void update(const std::vector<IOutput *> & = {}) override
    {
        setStatus(ComponentStatus::Updating);
        applyInputValues(); // pull the provider's value into the inflow input
        m_storage += kTimeStep * ((*m_inflow)[0] - m_k * m_storage);
        (*m_outflow)[0] = m_k * m_storage;
        ++m_step;
        setStatus(m_step >= kSteps ? ComponentStatus::Done : ComponentStatus::Updated,
                  "step " + std::to_string(m_step),
                  100.0f * static_cast<float>(m_step) / kSteps);
    }

    void finish() override
    {
        setStatus(ComponentStatus::Finishing);
        // Created, not Finished: this pond can be initialized again.
        setStatus(ComponentStatus::Created, "run state released");
    }

    [[nodiscard]] double storage() const { return m_storage; }
    [[nodiscard]] double outflow() const { return (*m_outflow)[0]; }
    [[nodiscard]] int cleanups() const { return m_cleanups; }
    [[nodiscard]] const Input1DDouble *inflowItem() const { return m_inflow.get(); }

protected:
    void createArguments() override
    {
        // Runs once per instance. Defaults go in here; a host may replace
        // them before initialize().
        m_recession = std::make_unique<Argument1DDouble>(
            "recession_constant", &m_scalar, 1, &m_rate, this);
        m_initialStorage = std::make_unique<Argument1DDouble>(
            "initial_storage", &m_scalar, 1, &m_volume, this);
        (*m_recession)[0]      = 1.0e-4; // 1/s
        (*m_initialStorage)[0] = 1000.0; // m3
        addArgument(m_recession.get());
        addArgument(m_initialStorage.get());
    }

    bool initializeArguments(std::string &message) override
    {
        const double k = (*m_recession)[0];
        if (!(k > 0.0) || k * kTimeStep > 1.0)
        {
            message = "recession_constant must be in (0, " +
                      std::to_string(1.0 / kTimeStep) + "] 1/s, got " + std::to_string(k);
            return false;
        }
        if (!((*m_initialStorage)[0] >= 0.0))
        {
            message = "initial_storage must not be negative";
            return false;
        }
        m_k = k;
        message.clear();
        return true;
    }

    void createInputs() override
    {
        // Called again on re-initialization: replacing the unique_ptr
        // deletes the previous input, which nothing may still reference.
        m_inflow = std::make_unique<Input1DDouble>("inflow", &m_scalar, 1, &m_flow, this);
        addInput(m_inflow.get());
    }

    void createOutputs() override
    {
        m_outflow = std::make_unique<Output1DDouble>("outflow", &m_scalar, 1, &m_flow, this);
        addOutput(m_outflow.get());
    }

    void initializeFailureCleanUp() override
    {
        // Release what initializeArguments() acquired. This pond acquires
        // nothing; it counts the call so the example can check it.
        ++m_cleanups;
    }

private:
    std::unique_ptr<Unit> m_perSecond{new Unit("per second", 1.0, 0.0)};
    std::unique_ptr<Unit> m_cubicMeters{Unit::volumeInCubicMeters()};
    std::unique_ptr<Unit> m_cms{Unit::flowInCMS()};
    Quantity  m_rate{"Recession constant", typeid(double), m_perSecond.get()};
    Quantity  m_volume{"Storage", typeid(double), m_cubicMeters.get()};
    Quantity  m_flow{"Flow", typeid(double), m_cms.get()};
    Dimension m_scalar{"scalar", "Single value"};

    std::unique_ptr<Argument1DDouble> m_recession;
    std::unique_ptr<Argument1DDouble> m_initialStorage;
    std::unique_ptr<Input1DDouble>    m_inflow;
    std::unique_ptr<Output1DDouble>   m_outflow;

    double m_k        = 0.0;
    double m_storage  = 0.0;
    int    m_step     = 0;
    int    m_cleanups = 0;
};
//! [component]

//! [info]
/*!
 * The component type a host lists before it creates anything. Each
 * instance gets its own id; the info passes itself so componentInfo()
 * answers.
 */
class DetentionPondInfo : public AbstractModelComponentInfo
{
public:
    DetentionPondInfo()
        : AbstractModelComponentInfo("org.hydrocouple.examples.detention-pond")
    {
        setCaption("Detention pond");
        setDescription("Single storage whose outflow is proportional to storage.");
        setDeveloper("HydroCouple SDK examples");
        setVersion("1.0.0");
        setLicenseText("MIT");
        addTag("Hydrology");
        addTag("Storage");
    }

    std::unique_ptr<IModelComponent> createComponentInstance() override
    {
        return std::make_unique<DetentionPond>("pond-" + std::to_string(++m_created), this);
    }

private:
    int m_created = 0;
};
//! [info]

/*!
 * The mistake the transition check exists for: prepare() jumps from Valid
 * straight to Updated without passing through Preparing.
 */
class ShortcutPond : public DetentionPond
{
public:
    using DetentionPond::DetentionPond;

    void prepare() override
    {
        setStatus(ComponentStatus::Updated, "prepared without Preparing");
    }
};

//! Prints every status event of a component on one line.
std::shared_ptr<FunctionSlot<const std::shared_ptr<IComponentStatusChangeEventArgs> &>>
printStatuses()
{
    using Event = const std::shared_ptr<IComponentStatusChangeEventArgs> &;
    return std::make_shared<FunctionSlot<Event>>(
        [](const ISignal<Event> &, Event event) {
            std::printf("  [%s] %s -> %s", event->component()->id().c_str(),
                        AbstractModelComponent::statusToString(event->previousStatus()).c_str(),
                        AbstractModelComponent::statusToString(event->status()).c_str());
            if (!event->message().empty())
                std::printf(": %s", event->message().c_str());
            if (event->hasProgressMonitor())
                std::printf(" (%.0f%%)", event->percentProgress());
            std::printf("\n");
        });
}

const char *severityName(ErrorEntry::Severity severity)
{
    switch (severity)
    {
        case ErrorEntry::Severity::Information: return "Information";
        case ErrorEntry::Severity::Warning:     return "Warning";
        case ErrorEntry::Severity::Error:       return "Error";
        case ErrorEntry::Severity::Fatal:       return "Fatal";
    }
    return "?";
}

void printErrors(const std::vector<ErrorEntry> &entries)
{
    for (const ErrorEntry &entry : entries)
        std::printf("  error queue: %s %d from %s: %s\n", severityName(entry.severity),
                    entry.code, entry.source.c_str(), entry.message.c_str());
}

IArgument *argument(IModelComponent &component, const std::string &id)
{
    for (IArgument *candidate : component.arguments())
        if (candidate->id() == id)
            return candidate;
    return nullptr;
}

} // namespace

//! [declare]
// Publishes hydrocouple_component_abi_v1() and hydrocouple_component_info_v1().
// In a component library this line is the library's whole public surface.
HYDROCOUPLE_DECLARE_COMPONENT(DetentionPondInfo)
//! [declare]

int main()
{
    //! [host]
    // What a host does after dlopen()/LoadLibrary(): call the stamp first,
    // compare it with its own, and only then call into C++.
    const char *stamp = hydrocouple_component_abi_v1();
    std::printf("library stamp: %s\n", stamp);
    if (std::strcmp(stamp, HYDROCOUPLE_COMPONENT_ABI_STAMP) != 0)
    {
        std::printf("refused: built with another toolchain or interface ABI\n");
        return 1;
    }
    auto *info = dynamic_cast<IModelComponentInfo *>(hydrocouple_component_info_v1());
    check(info != nullptr, "the library publishes a model component");
    if (!info)
        return 1;
    std::printf("component type %s, version %s, license %s\n", info->id().c_str(),
                info->version().c_str(), info->license().c_str());
    //! [host]

    // ── A full run ───────────────────────────────────────────────────────
    std::printf("\nfull run\n");
    //! [run]
    std::unique_ptr<IModelComponent> pond = info->createComponentInstance();
    auto slot = printStatuses(); // keep it alive: the signal holds it weakly
    pond->connect(slot);
    check(pond->componentInfo() == info, "componentInfo() is the creating info");

    // Arguments exist while Created; the host sets them before initialize().
    std::string message;
    check(argument(*pond, "recession_constant")
              ->initialize("[2.0e-4]", IArgument::ArgumentInputType::JSON, message),
          "set recession_constant: " + message);

    pond->initialize();
    check(pond->status() == Status::Initialized, "initialized");
    const std::vector<std::string> warnings = pond->validate();
    check(pond->status() == Status::Valid && warnings.size() == 1, "valid with one warning");
    printErrors(pond->errors(true));

    pond->prepare();
    auto *typed = dynamic_cast<DetentionPond *>(pond.get());
    double expected = 1000.0;
    while (pond->status() == Status::Updated)
    {
        pond->update();
        expected *= 1.0 - 2.0e-4 * kTimeStep;
        std::printf("  storage %.1f m3, outflow %.4f m3/s\n", typed->storage(),
                    typed->outflow());
        check(std::abs(typed->storage() - expected) < 1e-9, "storage matches S0 (1 - k dt)^n");
    }
    check(pond->status() == Status::Done, "Done after the last step");
    pond->finish();
    check(pond->status() == Status::Created, "finish() returned to Created");
    //! [run]

    // ── Initialize the same instance again ───────────────────────────────
    std::printf("\nre-initialization\n");
    //! [reinitialize]
    const Input1DDouble *firstInflow = typed->inflowItem();
    pond->initialize();
    check(pond->status() == Status::Initialized, "initialized again from Created");
    check(argument(*pond, "recession_constant")->toString() == "[0.0002]",
          "argument values survive re-initialization");
    std::printf("  recession_constant is still %s; inflow item replaced: %s\n",
                argument(*pond, "recession_constant")->toString().c_str(),
                typed->inflowItem() != firstInflow ? "yes" : "no");
    pond->finish();
    //! [reinitialize]

    // ── A failed initialization ──────────────────────────────────────────
    std::printf("\nfailed initialization\n");
    //! [failure]
    std::unique_ptr<IModelComponent> unstable = info->createComponentInstance();
    unstable->connect(slot);
    check(argument(*unstable, "recession_constant")
              ->initialize("[0.5]", IArgument::ArgumentInputType::JSON, message),
          "0.5 is valid JSON");
    unstable->initialize();
    check(unstable->status() == Status::Failed, "initialize() failed");
    check(unstable->inputs().empty() && unstable->outputs().empty(),
          "no ports after a failed initialize()");
    check(dynamic_cast<DetentionPond *>(unstable.get())->cleanups() == 1,
          "initializeFailureCleanUp() ran once");
    const std::vector<ErrorEntry> fatal = unstable->errors();
    printErrors(fatal);
    check(fatal.size() == 1 && fatal[0].severity == ErrorEntry::Severity::Fatal,
          "one Fatal entry");
    unstable->finish(); // legal from Failed
    //! [failure]

    // ── An illegal transition ────────────────────────────────────────────
    std::printf("\nillegal transition\n");
    //! [illegal]
    ShortcutPond shortcut("shortcut", nullptr);
    shortcut.connect(slot);
    shortcut.initialize();
    (void)shortcut.validate();
    (void)shortcut.errors(true); // drop the unconnected-inflow warning
    shortcut.prepare();
    check(shortcut.status() == Status::Updated, "the transition is performed, not refused");
    const std::vector<ErrorEntry> illegal = shortcut.errors();
    printErrors(illegal);
    check(illegal.size() == 1 && illegal[0].code == Diagnostic::IllegalStatusTransition,
          "reported once as IllegalStatusTransition");
    //! [illegal]

    std::printf("\n%s\n", g_failures == 0 ? "all checks passed" : "CHECKS FAILED");
    return g_failures == 0 ? 0 : 1;
}
