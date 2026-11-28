// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 3: drive the linear reservoir the way a host does, through
 *         the HydroCouple interfaces only, and check it against the exact
 *         solution S(t) = I/k + (S0 - I/k) exp(-k t).
 */

#include "linearreservoir.h"

#include "hydrocouplesdk/componentabi.h"

#include <hydrocouplehelpers.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

// The entry points HYDROCOUPLE_DECLARE_COMPONENT defines in linearreservoir.cpp.
// A host that loads a library gets the same two functions from
// dlsym/GetProcAddress with HYDROCOUPLE_COMPONENT_ABI_SYMBOL and
// HYDROCOUPLE_COMPONENT_INFO_SYMBOL.
extern "C" const char *hydrocouple_component_abi_v1(void);
extern "C" HydroCouple::IComponentInfo *hydrocouple_component_info_v1(void);

using namespace HydroCouple;

namespace {

template <typename Item>
Item *find(const std::vector<Item *> &items, const std::string &id)
{
    for (Item *item : items)
        if (item->id() == id)
            return item;
    return nullptr;
}

int fail(const std::string &what, IModelComponent *component = nullptr)
{
    std::fprintf(stderr, "FAILED: %s\n", what.c_str());
    if (component)
        for (const ErrorEntry &entry : component->errors())
            std::fprintf(stderr, "  component error: %s\n", entry.message.c_str());
    return 1;
}

} // namespace

int main()
{
    //! [load]
    // A host that loaded a library would look these two symbols up by name;
    // here they are linked in. The stamp is checked before anything else.
    HydroCoupleComponentAbiFn abi = &hydrocouple_component_abi_v1;
    HydroCoupleComponentInfoFn infoEntry = &hydrocouple_component_info_v1;
    if (std::strcmp(abi(), HYDROCOUPLE_COMPONENT_ABI_STAMP) != 0)
        return fail("component built with a different toolchain or interface ABI");
    auto *info = dynamic_cast<IModelComponentInfo *>(infoEntry());
    if (!info)
        return fail("the library does not publish a model component");
    std::unique_ptr<IModelComponent> component = info->createComponentInstance();
    std::printf("created '%s' from component type '%s'\n",
                component->id().c_str(), info->id().c_str());
    //! [load]

    //! [arguments]
    // Arguments exist from construction, so a host can set them while the
    // component is still Created. JSON is one of the accepted input types.
    const std::array<std::pair<const char *, const char *>, 4> settings{{
        {"recession_constant", "[0.001]"}, // 1/s
        {"initial_storage", "[1000]"},     // m³
        {"time_step", "[600]"},            // s
        {"duration", "[7200]"},            // s: 12 steps
    }};
    for (const auto &[id, json] : settings)
    {
        IArgument *argument = find(component->arguments(), id);
        std::string message;
        if (!argument ||
            !argument->initialize(json, IArgument::ArgumentInputType::JSON, message))
            return fail(std::string("argument ") + id + ": " + message, component.get());
    }
    //! [arguments]

    //! [initialize]
    component->initialize();
    if (component->status() != IModelComponent::ComponentStatus::Initialized)
        return fail("initialize()", component.get());
    std::printf("initialized: %zu arguments, %zu input, %zu outputs\n",
                component->arguments().size(), component->inputs().size(),
                component->outputs().size());
    //! [initialize]

    //! [inflow]
    // No provider is connected, so the host writes the inflow itself.
    IInput *inflow = find(component->inputs(), "inflow");
    const double inflowRate = 0.5; // m³/s
    const std::array<int64_t, 1> first{0};
    if (!inflow || !Helpers::setValue(*inflow, inflowRate, std::span<const int64_t>(first)))
        return fail("set inflow", component.get());
    //! [inflow]

    //! [run]
    if (!component->validate().empty() ||
        component->status() != IModelComponent::ComponentStatus::Valid)
        return fail("validate()", component.get());
    component->prepare();

    IOutput *storage = find(component->outputs(), "storage");
    IOutput *outflow = find(component->outputs(), "outflow");
    const double k = 1.0e-3, s0 = 1000.0, dt = 600.0;
    const double equilibrium = inflowRate / k;

    std::printf("%8s %12s %12s %12s\n", "t (s)", "S (m3)", "Q (m3/s)", "exact S");
    int steps = 0;
    while (component->status() == IModelComponent::ComponentStatus::Updated)
    {
        component->update();
        ++steps;
        double s = 0.0, q = 0.0;
        if (!Helpers::getValue(*storage, s, std::span<const int64_t>(first)) ||
            !Helpers::getValue(*outflow, q, std::span<const int64_t>(first)))
            return fail("read outputs", component.get());
        const double t = steps * dt;
        const double exact = equilibrium + (s0 - equilibrium) * std::exp(-k * t);
        std::printf("%8.0f %12.4f %12.6f %12.4f\n", t, s, q, exact);
        if (std::abs(s - exact) > 1e-9 * exact || std::abs(q - k * s) > 1e-12)
            return fail("storage differs from the exact solution");
    }
    if (component->status() != IModelComponent::ComponentStatus::Done || steps != 12)
        return fail("expected Done after 12 steps", component.get());
    //! [run]

    //! [finish]
    component->finish();
    if (component->status() != IModelComponent::ComponentStatus::Finished)
        return fail("finish()", component.get());
    std::printf("finished after %d steps\n", steps);
    //! [finish]

    //! [failure]
    // A value initializeArguments() rejects: initialize() ends in Failed and
    // the component's message is on its error queue.
    std::unique_ptr<IModelComponent> rejected = info->createComponentInstance();
    std::string message;
    if (!find(rejected->arguments(), std::string("recession_constant"))
             ->initialize("[-1]", IArgument::ArgumentInputType::JSON, message))
        return fail("a negative value is valid JSON and should be accepted here");
    rejected->initialize();
    const std::vector<ErrorEntry> errors = rejected->errors();
    if (rejected->status() != IModelComponent::ComponentStatus::Failed || errors.empty())
        return fail("expected initialize() to fail for a negative recession constant");
    std::printf("rejected instance: status %s\n",
                SDK::AbstractModelComponent::statusToString(rejected->status()).c_str());
    for (const ErrorEntry &entry : errors)
        std::printf("  error: %s\n", entry.message.c_str());
    //! [failure]
    return 0;
}
