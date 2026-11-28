// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 7: typed arguments read from strings, JSON, a file and
 *         another data item; serialization; the input types an argument
 *         advertises; and the messages a bad value produces.
 */

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/data/argument2d.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
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

std::string readFile(const std::filesystem::path &path)
{
    std::ifstream file(path);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

} // namespace

int main()
{
    Dimension reaches("reaches", "Reaches");
    std::unique_ptr<Quantity> roughnessQuantity(Quantity::unitLess("Manning n"));

    //! [declare]
    // What a component does in createArguments(): length 0, so the argument
    // takes the size of whatever the host gives it.
    Argument1DDouble roughness("roughness", &reaches, 0,
                               roughnessQuantity.get(), nullptr);
    roughness.setCaption("Manning roughness per reach");
    roughness.setRole(IArgument::ArgumentRole::Parameter);
    roughness.setIsOptional(false);
    roughness.addFileFilter("JSON files (*.json)");
    //! [declare]

    //! [initialize]
    std::string message;
    const bool fromString = roughness.initialize("[0.025, 0.030]", AT::String, message);
    std::printf("String: length %d, values %s\n", roughness.length(),
                roughness.toString().c_str());

    const bool fromJson =
        roughness.initialize(R"({"values": [0.02, 0.03, 0.04, 0.05]})", AT::JSON, message);
    std::printf("JSON object: length %d, values %s\n", roughness.length(),
                roughness.toString().c_str());

    // File: copy the input next to the program first, because saveData()
    // writes back to the file the values came from.
    const std::filesystem::path source =
        std::filesystem::path(DOCS_EXAMPLE_SOURCE_DIR) / "roughness.json";
    const std::filesystem::path local = "roughness.json";
    std::filesystem::copy_file(source, local,
                               std::filesystem::copy_options::overwrite_existing);
    const bool fromFile = roughness.initialize(local.string(), AT::File, message);
    std::printf("File: length %d, values %s, input type File %s\n", roughness.length(),
                roughness.toString().c_str(),
                roughness.currentArgumentInputType() == AT::File ? "yes" : "no");
    //! [initialize]
    check(fromString && fromJson && fromFile && roughness.length() == 3 &&
              roughness[2] == 0.040,
          "String, JSON and File initialization");

    //! [save]
    roughness[0] = 0.033; // direct storage write
    roughness.saveData(); // rewrites roughness.json as a bare array
    const std::string saved = readFile(local);
    std::printf("saveData wrote: %s\n", saved.c_str());
    //! [save]
    check(saved == "[0.033,0.035,0.04]", "saveData writes the values back");

    //! [serialize]
    std::string asJson, asYaml;
    const bool json = roughness.serialize(AT::JSON, asJson, message);
    const bool yaml = roughness.serialize(AT::YAML, asYaml, message);
    std::printf("serialize(JSON):\n%s\n", asJson.c_str());
    std::printf("serialize(YAML): %s (%s)\n", yaml ? "true" : "false", message.c_str());
    //! [serialize]
    check(json && !yaml && nlohmann::json::parse(asJson)["values"].size() == 3,
          "JSON serialization holds the values");

    //! [valid_types]
    const AT types[] = {AT::String, AT::File, AT::JSON, AT::YAML,
                        AT::XML,    AT::URL,  AT::MEMORY_OBJECT};
    const char *names[] = {"String", "File", "JSON", "YAML", "XML", "URL", "MEMORY_OBJECT"};
    std::printf("isValidArgType:");
    for (size_t i = 0; i < 7; ++i)
        std::printf(" %s=%d", names[i], roughness.isValidArgType(types[i]) ? 1 : 0);
    std::printf("\n");
    //! [valid_types]
    check(roughness.isValidArgType(AT::File) && !roughness.isValidArgType(AT::YAML) &&
              !roughness.isValidArgType(AT::MEMORY_OBJECT),
          "advertised input types follow the filters and registered types");

    //! [errors]
    std::string notArray, malformed, wrongType, missing;
    const bool e1 = roughness.initialize("0.03", AT::JSON, notArray);
    const bool e2 = roughness.initialize("[0.03,", AT::JSON, malformed);
    const bool e3 = roughness.initialize(R"(["rough"])", AT::JSON, wrongType);
    const bool e4 = roughness.initialize("no_such_file.json", AT::File, missing);
    std::printf("scalar: %s\n", notArray.c_str());
    std::printf("malformed: %s\n", malformed.c_str());
    std::printf("string element: %s\n", wrongType.c_str());
    std::printf("missing file: %s\n", missing.c_str());
    //! [errors]
    check(!e1 && notArray == "Expected JSON array of values", "a scalar is refused");
    check(!e2 && !malformed.empty(), "malformed JSON is refused");
    check(!e3 && !wrongType.empty(), "an element of the wrong type is refused");
    check(!e4 && missing == "Cannot open file: no_such_file.json", "a missing file is refused");

    //! [two_d]
    Dimension layers("layers", "Layers"), cells("cells", "Cells");
    std::unique_ptr<Quantity> conductivity(Quantity::unitLess("Conductivity"));
    Argument2DInt zones("zones", &layers, &cells, 0, 0, conductivity.get(), nullptr);
    const bool table = zones.initialize("[[1, 1, 2], [3, 3, 3]]", AT::JSON, message);
    std::printf("Argument2D shape {%lld, %lld}, row 1 column 2 = %d\n",
                static_cast<long long>(zones.shape()[0]),
                static_cast<long long>(zones.shape()[1]), zones(1, 2));
    std::string ragged;
    const bool raggedOk = zones.initialize("[[1, 2], [3]]", AT::JSON, ragged);
    std::printf("ragged rows: %s\n", ragged.c_str());
    //! [two_d]
    check(table && zones.shape() == std::vector<int64_t>{2, 3} && zones(1, 2) == 3,
          "Argument2D reads rows");
    check(!raggedOk && ragged == "Rows must be arrays of equal length", "ragged rows refused");

    //! [from_item]
    // Another component's output can initialize an argument.
    std::unique_ptr<Quantity> flow(Quantity::flowInCMS());
    std::unique_ptr<IUnit> flowUnit(flow->unit());
    Output1DDouble measured("measured", &reaches, 2, flow.get(), nullptr);
    measured[0] = 1.5;
    measured[1] = 2.5;
    Argument1DDouble initialFlow("initial_flow", &reaches, 0, flow.get(), nullptr);
    const bool copied = initialFlow.initialize(measured, message);
    std::printf("from an output: %s\n", initialFlow.toString().c_str());

    Output1DInt labels("labels", &reaches, 2, flow.get(), nullptr);
    std::string kind;
    const bool kindOk = initialFlow.initialize(labels, kind);
    std::printf("from an Int32 output: %s\n", kind.c_str());
    //! [from_item]
    check(copied && initialFlow.toString() == "[1.5,2.5]", "copied from a data item");
    check(!kindOk && kind == "Source data kind mismatch", "kind mismatch refused");

    std::printf("%s\n", failures ? "ch07_arguments FAILED" : "ch07_arguments passed");
    return failures ? 1 : 0;
}
