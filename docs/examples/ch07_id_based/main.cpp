// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 7: values keyed by identifier (IdBasedOutput,
 *         IdBasedInput, IdBasedArgument) and a multi-input with two named
 *         provider roles.
 */

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/identity.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/abstractinput.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/data/idbasedargument.h"
#include "hydrocouplesdk/data/idbasedexchangeitems.h"

#include <hydrocouplehelpers.h>

#include <array>
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

std::string joined(const std::vector<std::string> &ids)
{
    std::string text;
    for (const std::string &id : ids)
        text += (text.empty() ? "" : " ") + id;
    return text;
}

//! [multi_input]
//! A multi-input with two roles that sums what its providers hold.
class InflowSum final : public AbstractMultiInput
{
public:
    InflowSum(std::string_view id, Dimension *dimension, ValueDefinition *quantity,
              IModelComponent *component)
        : AbstractMultiInput(id, {dimension}, quantity, component),
          m_upstream("upstream", "Upstream inflow"),
          m_lateral("lateral", "Lateral inflow")
    {}

    std::vector<IIdentity *> providerLabels() const override
    {
        return {const_cast<Identity *>(&m_upstream), const_cast<Identity *>(&m_lateral)};
    }

    bool isRequiredProvider(const IIdentity *label) const override
    {
        return label && label->id() == "upstream"; // lateral may be absent
    }

    bool canConsume(IOutput *provider, std::string &message) const override
    {
        return canConsume(provider, message, nullptr);
    }

    bool canConsume(IOutput *provider, std::string &message,
                    const IIdentity *role) const override
    {
        if (!role)
        {
            message = "name a role: upstream or lateral";
            return false;
        }
        if (!provider || provider->dataKind() != DataKind::Float64 ||
            provider->shape() != std::vector<int64_t>{1})
        {
            message = "expects one Float64 value";
            return false;
        }
        message.clear();
        return true;
    }

    void retrieveValuesFromProvider() override
    {
        m_total = 0.0;
        for (IOutput *provider : providers())
        {
            double value = 0.0;
            const std::array<int64_t, 1> index{0};
            if (Helpers::getValue(*provider, value, index))
                m_total += value;
        }
    }

    void applyData() override {}

    double total() const { return m_total; }

    // The input's own data plane: the sum, read-only.
    std::vector<int64_t> shape() const override { return {1}; }
    DataKind dataKind() const override { return DataKind::Float64; }
    bool getValuesInto(const BufferDescriptor &destination, std::span<const int64_t> start,
                       std::span<const int64_t> count, std::string *message) const override
    {
        const int64_t one = 1;
        return detail::readHyperslab<double>(&m_total, std::span<const int64_t>(&one, 1),
                                             destination, start, count, message);
    }
    bool setValuesFrom(const BufferDescriptor &, std::span<const int64_t>,
                       std::span<const int64_t>, std::string *message) override
    {
        if (message)
            *message = "InflowSum is computed from its providers";
        return false;
    }

private:
    Identity m_upstream;
    Identity m_lateral;
    double m_total = 0.0;
};
//! [multi_input]

} // namespace

int main()
{
    std::unique_ptr<Quantity> flow(Quantity::flowInCMS());
    std::unique_ptr<IUnit> flowUnit(flow->unit());

    //! [id_output]
    // Values attached to junction ids. The item owns its "identifiers"
    // dimension (role Entity).
    IdBasedOutputDouble inflows("node_inflow", {"J1", "J2", "J3"}, flow.get(), nullptr);
    inflows.valueAt("J1") = 0.5;
    inflows.valueAt("J2") = 1.25;
    inflows.valueAt("J3") = 2.0;
    std::printf("output identifiers: %s; shape {%lld}; J2 at position %d\n",
                joined(inflows.identifiers()).c_str(),
                static_cast<long long>(inflows.shape()[0]), inflows.indexOf("J2"));
    //! [id_output]
    check(inflows.identifierDimension()->role() == IDimension::DimensionRole::Entity,
          "identifier dimension has role Entity");

    //! [id_input]
    // Matching is by position: value i lands on identifier i.
    IdBasedInputDouble received("node_inflow", {"J1", "J2", "J3"}, flow.get(), nullptr);
    inflows.addConsumer(&received);
    received.retrieveValuesFromProvider();
    std::printf("input J3 = %g\n", received.valueAt("J3"));

    IdBasedInputDouble tooMany("wide", {"A", "B", "C", "D"}, flow.get(), nullptr);
    std::string why;
    const bool fits = tooMany.canConsume(&inflows, why);
    std::printf("four-id input on a three-value provider: %s\n", why.c_str());
    //! [id_input]
    check(received.valueAt("J3") == 2.0 && received.valueAt("J1") == 0.5,
          "id-based input pulled by position");
    check(!fits && why == "provider must be 1-D with at least 4 values", "short provider refused");

    //! [id_argument]
    Dimension junctions("identifiers", "Junctions");
    IdBasedArgumentDouble invert("invert_elevation", {}, &junctions, flow.get(), nullptr);
    std::string message;
    // Object form: nlohmann::json objects are ordered maps, so ids come back sorted.
    const bool objectForm =
        invert.initialize(R"({"J3": 98.5, "J1": 100.0, "J2": 99.2})", AT::JSON, message);
    std::printf("object form identifiers: %s\n", joined(invert.identifiers()).c_str());
    // Parallel arrays keep the order given.
    const bool arrayForm = invert.initialize(
        R"({"identifiers": ["J3", "J1", "J2"], "values": [98.5, 100.0, 99.2]})", AT::JSON,
        message);
    std::printf("array form identifiers: %s; J1 = %g\n", joined(invert.identifiers()).c_str(),
                invert.valueAt("J1"));
    std::printf("toString: %s\n", invert.toString().c_str());
    std::printf("writeData identifiers: %s\n", invert.writeData()["identifiers"].dump().c_str());
    //! [id_argument]
    check(objectForm && arrayForm &&
              invert.identifiers() == std::vector<std::string>{"J3", "J1", "J2"},
          "array form keeps identifier order");

    //! [roles]
    Dimension single("element", "Element");
    Output1DDouble upstream("flow", &single, 1, flow.get(), nullptr);
    Output1DDouble lateral("runoff", &single, 1, flow.get(), nullptr);
    upstream[0] = 3.0;
    lateral[0] = 0.75;

    InflowSum sum("inflow", &single, flow.get(), nullptr);
    const std::vector<IIdentity *> labels = sum.providerLabels();
    upstream.addConsumer(&sum, labels[0]); // canConsume(.., role), then addProvider
    lateral.addConsumer(&sum, labels[1]);
    sum.retrieveValuesFromProvider();
    std::printf("providers %zu, upstream required %s, lateral required %s, total %g\n",
                sum.providers().size(), sum.isRequiredProvider(labels[0]) ? "yes" : "no",
                sum.isRequiredProvider(labels[1]) ? "yes" : "no", sum.total());
    std::printf("provider() of a multi-input: %s\n", sum.provider() ? "set" : "nullptr");
    //! [roles]
    check(sum.providers().size() == 2 && sum.total() == 3.75, "multi-input sums two roles");
    check(upstream.consumers().size() == 1 && sum.provider() == nullptr,
          "outputs list the multi-input; provider() stays null");

    upstream.removeConsumer(&sum);
    lateral.removeConsumer(&sum);
    inflows.removeConsumer(&received);
    check(sum.providers().empty(), "removeConsumer() removes the providers");

    std::printf("%s\n", failures ? "ch07_id_based FAILED" : "ch07_id_based passed");
    return failures ? 1 : 0;
}
