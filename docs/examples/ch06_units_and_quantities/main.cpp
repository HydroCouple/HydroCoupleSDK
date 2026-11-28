// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 6: units, quantities, qualities and dimensions, and what
 *         the connection validator concludes when two ends declare them.
 *
 * The SDK never converts values. A Unit states how to reach SI,
 * x_SI = A x + B, so a host or an adapter can convert; the program does the
 * arithmetic itself and checks it. Then it wires pairs of 1-D items with
 * different declarations and prints the findings validateConnection()
 * returns for each pair, checking their codes and severities.
 */

#include "hydrocouplesdk/component/connectionvalidator.h"
#include "hydrocouplesdk/core/diagnosticcodes.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/identity.h"
#include "hydrocouplesdk/core/signal.h"
#include "hydrocouplesdk/core/unit.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using Dim = IUnitDimensions::FundamentalUnitDimension;

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok)
    {
        std::printf("CHECK FAILED: %s\n", what.c_str());
        ++g_failures;
    }
}

//! [convert]
//! Converts \p x from one unit to another through SI. The units must share
//! their dimensions; the SDK checks that when two items connect, not here.
double convert(double x, const IUnit &from, const IUnit &to)
{
    const double si = from.conversionFactorToSI() * x + from.offsetToSI();
    return (si - to.offsetToSI()) / to.conversionFactorToSI();
}
//! [convert]

//! Wires an output to an input over the given value definitions and axes,
//! and returns what validateConnection() finds. Both items are unconnected
//! stand-ins; only their declarations matter.
template <typename Out, typename In>
std::vector<ConnectionFinding> compare(ValueDefinition *produced, ValueDefinition *expected,
                                       const char *outAxis = "i", const char *inAxis = "i")
{
    Dimension outDimension(outAxis), inDimension(inAxis);
    Out out("out", &outDimension, 2, produced, nullptr);
    In  in("in", &inDimension, 2, expected, nullptr);
    return validateConnection(out, in);
}

const char *severityName(ErrorEntry::Severity severity)
{
    return severity == ErrorEntry::Severity::Error     ? "Error"
           : severity == ErrorEntry::Severity::Warning ? "Warning"
                                                       : "Other";
}

//! Prints the findings of one case and checks that exactly the expected
//! code appears, with the expected severity, and that hasConnectionError()
//! agrees.
void report(const char *title, const std::vector<ConnectionFinding> &findings,
            int32_t expectedCode, ErrorEntry::Severity expectedSeverity)
{
    std::printf("%s\n", title);
    if (findings.empty())
        std::printf("  no findings\n");
    bool found = false;
    for (const ConnectionFinding &finding : findings)
    {
        std::printf("  %s %d: %s\n", severityName(finding.severity), finding.code,
                    finding.message.c_str());
        found = found || (finding.code == expectedCode && finding.severity == expectedSeverity);
    }
    if (expectedCode == 0)
        check(findings.empty(), std::string(title) + ": no findings");
    else
        check(found, std::string(title) + ": expected code " + std::to_string(expectedCode));
    check(hasConnectionError(findings) == (expectedSeverity == ErrorEntry::Severity::Error &&
                                           expectedCode != 0),
          std::string(title) + ": hasConnectionError()");
}

} // namespace

int main()
{
    // ── Units and conversion ─────────────────────────────────────────────
    //! [units]
    // Factory units are owned by the caller. Quantity only borrows its unit,
    // so the units are kept in unique_ptr for the whole program.
    std::unique_ptr<Unit> meters(Unit::lengthInMeters());
    std::unique_ptr<Unit> feet(Unit::lengthInFeet());
    std::unique_ptr<Unit> cms(Unit::flowInCMS());
    std::unique_ptr<Unit> cfs(Unit::flowInCFS());

    // A unit the factories do not offer: temperature with an offset.
    auto celsius = std::make_unique<Unit>("degrees Celsius", 1.0, 273.15);
    celsius->dimensionsInternal()->setPower(Dim::Temperature, 1.0);
    celsius->setSymbol("degC");
    celsius->setUcumCode("Cel");
    auto fahrenheit = std::make_unique<Unit>("degrees Fahrenheit", 5.0 / 9.0,
                                             273.15 - 32.0 * 5.0 / 9.0);
    fahrenheit->dimensionsInternal()->setPower(Dim::Temperature, 1.0);
    fahrenheit->setSymbol("degF");
    fahrenheit->setUcumCode("[degF]");
    //! [units]

    std::printf("conversions through SI\n");
    const double tenFeet  = convert(10.0, *feet, *meters);
    const double room     = convert(20.0, *celsius, *fahrenheit);
    const double tenCfs   = convert(10.0, *cfs, *cms);
    std::printf("  10 ft      = %.4f m\n", tenFeet);
    std::printf("  20 degC    = %.4f degF\n", room);
    std::printf("  10 ft3/s   = %.6f m3/s (factor %.7f)\n", tenCfs, cfs->conversionFactorToSI());
    std::printf("  flow powers: L^%g T^%g, symbol \"%s\", UCUM \"%s\"\n",
                cms->dimensions()->power(Dim::Length), cms->dimensions()->power(Dim::Time),
                cms->symbol().c_str(), cms->ucumCode().c_str());
    check(std::abs(tenFeet - 3.048) < 1e-12, "10 ft is 3.048 m");
    check(std::abs(room - 68.0) < 1e-9, "20 degC is 68 degF");
    check(std::abs(tenCfs - 0.283168) < 1e-12, "10 ft3/s is 0.283168 m3/s");

    // ── Quantities and what a connection compares ────────────────────────
    //! [quantities]
    Quantity flowSI("Discharge", typeid(double), cms.get());
    Quantity flowUS("Discharge", typeid(double), cfs.get());
    Quantity depth("Depth", typeid(double), meters.get());
    for (Quantity *quantity : {&flowSI, &flowUS, &depth})
        quantity->setValueKind(ValueKind::Intensive);
    flowSI.setConceptURI("http://qudt.org/vocab/quantitykind/VolumeFlowRate");
    flowSI.setMinValue(0.0);

    std::unique_ptr<Unit> one(Unit::unitlessCoefficient());
    Quantity placeholder("Placeholder", typeid(double), one.get());
    placeholder.setValueKind(ValueKind::Intensive);

    Quantity storage("Storage", typeid(double), cms.get()); // same unit, other kind
    storage.setValueKind(ValueKind::Extensive);
    Quantity undeclared("Discharge", typeid(double), cms.get()); // ValueKind::Unknown

    Quantity precipitation("Precipitation", typeid(double), cms.get());
    precipitation.setValueKind(ValueKind::Intensive);
    precipitation.setConceptURI("http://qudt.org/vocab/quantitykind/Rainfall");

    Quality landUse("Land use", {"forest", "urban", "water"});
    Quality cover("Land use", {"urban", "forest", "water"}); // same labels, other order
    landUse.setValueKind(ValueKind::Intensive);
    cover.setValueKind(ValueKind::Intensive);
    //! [quantities]

    std::printf("\nconnection findings\n");
    //! [compare]
    report("m3/s -> m3/s", compare<Output1DDouble, Input1DDouble>(&flowSI, &flowSI), 0,
           ErrorEntry::Severity::Error);
    report("ft3/s -> m3/s", compare<Output1DDouble, Input1DDouble>(&flowUS, &flowSI),
           Diagnostic::UnitMismatch, ErrorEntry::Severity::Error);
    report("m3/s -> m", compare<Output1DDouble, Input1DDouble>(&flowSI, &depth),
           Diagnostic::UnitMismatch, ErrorEntry::Severity::Error);
    report("unitless -> m3/s", compare<Output1DDouble, Input1DDouble>(&placeholder, &flowSI),
           Diagnostic::UnitMismatch, ErrorEntry::Severity::Warning);
    report("Intensive -> Extensive", compare<Output1DDouble, Input1DDouble>(&flowSI, &storage),
           Diagnostic::ValueKindMismatch, ErrorEntry::Severity::Error);
    report("Unknown kind -> Intensive",
           compare<Output1DDouble, Input1DDouble>(&undeclared, &flowSI),
           Diagnostic::ValueKindMismatch, ErrorEntry::Severity::Warning);
    report("other concept URI",
           compare<Output1DDouble, Input1DDouble>(&precipitation, &flowSI),
           Diagnostic::QuantityNameMismatch, ErrorEntry::Severity::Warning);
    report("no value definition", compare<Output1DDouble, Input1DDouble>(nullptr, &flowSI),
           Diagnostic::ValueDefinitionMismatch, ErrorEntry::Severity::Warning);
    report("quality -> quantity", compare<Output1DInt, Input1DInt>(&landUse, &flowSI),
           Diagnostic::ValueDefinitionMismatch, ErrorEntry::Severity::Error);
    report("categories reordered", compare<Output1DInt, Input1DInt>(&landUse, &cover),
           Diagnostic::ValueDefinitionMismatch, ErrorEntry::Severity::Error);
    report("Int32 -> Float64", compare<Output1DInt, Input1DDouble>(&flowSI, &flowSI),
           Diagnostic::DataKindMismatch, ErrorEntry::Severity::Error);
    report("axis time -> nodes",
           compare<Output1DDouble, Input1DDouble>(&flowSI, &flowSI, "time", "nodes"),
           Diagnostic::DimensionRoleMismatch, ErrorEntry::Severity::Error);
    //! [compare]

    // ── Dimensions and identities ────────────────────────────────────────
    std::printf("\ndimensions and identities\n");
    //! [dimensions]
    Dimension faces("faces"), bins("depth_bins");
    std::printf("  role of \"faces\": %s; role of \"depth_bins\": %s\n",
                faces.role() == IDimension::DimensionRole::Entity ? "Entity" : "other",
                bins.role() == IDimension::DimensionRole::Unknown ? "Unknown" : "other");
    bins.setRole(IDimension::DimensionRole::Other);
    Identity generated("");
    std::printf("  an empty id became \"%s\" (%zu characters)\n", generated.id().c_str(),
                generated.id().size());
    //! [dimensions]
    check(faces.role() == IDimension::DimensionRole::Entity, "faces is an Entity axis");
    check(generated.id().size() == 36 && generated.id()[14] == '4', "UUID v4");

    // ── Persistence ──────────────────────────────────────────────────────
    std::printf("\nJSON\n");
    //! [json]
    const nlohmann::json written = flowSI.toJson();
    std::printf("  %s\n", written.dump().c_str());
    Quantity restored("", typeid(double), meters.get());
    restored.fromJson(written);
    std::printf("  restored: caption \"%s\", min %g, unit still \"%s\"\n",
                restored.caption().c_str(), restored.minValue(),
                restored.unit()->caption().c_str());
    //! [json]
    check(restored.minValue() == 0.0 && restored.conceptURI() == flowSI.conceptURI(),
          "fromJson() restores bounds and names");
    check(restored.unit() == meters.get(), "fromJson() does not replace the unit");

    // ── Property-change signal ───────────────────────────────────────────
    std::printf("\nproperty changes\n");
    //! [signal]
    std::vector<std::string> changed;
    auto slot = std::make_shared<FunctionSlot<std::string>>(
        [&changed](const ISignal<std::string> &, std::string name) {
            changed.push_back(name);
        });
    depth.connect(slot);
    depth.setStandardName("sea_floor_depth_below_sea_surface");
    depth.setMaxValue(11000.0);
    depth.blockSignals(true);
    depth.setCaption("Water depth"); // applied, not announced
    depth.blockSignals(false);
    for (const std::string &name : changed)
        std::printf("  changed: %s\n", name.c_str());
    //! [signal]
    check(changed == std::vector<std::string>{"StandardName", "MaxValue"},
          "two notifications, none while blocked");
    check(depth.caption() == "Water depth", "the blocked change was applied");

    std::printf("\n%s\n", g_failures == 0 ? "all checks passed" : "CHECKS FAILED");
    return g_failures == 0 ? 0 : 1;
}
