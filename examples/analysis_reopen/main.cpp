// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \author Caleb Buahin
 * \version 2.1.0
 * \brief  Analysis example — reopen a completed run and summarize it
 *         without any model binary.
 *
 * This is the analysis and visualization case the run manifest exists
 * for. The program links only HydroCoupleSDK: it has no knowledge of the
 * catchment or channel components that produced the results, no headers
 * for them, and no way to instantiate them. Everything it reports comes
 * from the manifest's results catalog and the artifacts it points at.
 *
 * A real UI would do exactly this, then plot instead of print:
 *
 * 1. read the run manifest;
 * 2. build a composition of \c execution.mode "open" blocks from it
 *    (compositionForRun) and hand that to ModelInitializer — which
 *    substitutes a ResultsModelComponent per recorded component;
 * 3. browse results() for each component and read hyperslabs through the
 *    ordinary v2 data plane.
 *
 * Run it on the serial example's output:
 * \code
 *   ./serial_coupling composition.yaml out/
 *   ./analysis_reopen out/run.json
 * \endcode
 */

#include "hydrocouplesdk/io/modelinitializer.h"
#include "hydrocouplesdk/io/runmanifest.h"
#include "hydrocouplesdk/component/resultsmodelcomponent.h"
#include "hydrocouplesdk/data/timesliceoutput.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;

namespace {

/*!
 * \brief Reads a whole recorded variable into a flat host vector.
 */
bool readAll(IComponentDataItem &item, std::vector<double> &values,
             std::string &message)
{
    const std::vector<int64_t> shape = item.shape();
    int64_t elements = 1;
    for (int64_t extent : shape)
        elements *= extent;

    values.assign(static_cast<size_t>(elements), 0.0);
    const std::vector<int64_t> start(shape.size(), 0);
    const int64_t              flat = elements;

    BufferDescriptor destination;
    destination.data  = values.data();
    destination.kind  = DataKind::Float64;
    destination.rank  = 1;
    destination.shape = &flat;
    destination.space = MemorySpace::Host;

    return item.getValuesInto(destination, start, shape, &message);
}

} // namespace

int main(int argc, char **argv)
{
    const std::filesystem::path manifestPath =
        argc > 1 ? argv[1] : "output/run.json";

    // ── 1. The manifest is the only input ────────────────────────────────
    IO::RunManifest manifest;
    std::string     message;
    if (!IO::RunManifest::read(manifestPath, manifest, message))
    {
        std::cerr << message << '\n';
        return 1;
    }

    std::cout << "run        : " << manifest.id << '\n'
              << "caption    : " << manifest.caption << '\n'
              << "status     : " << IO::runStatusName(manifest.status) << '\n'
              << "sdk        : " << manifest.sdkVersion << '\n'
              << "finished   : " << manifest.finished << '\n'
              << "components : " << manifest.components.size()
              << ", recorded items: " << manifest.results.size() << "\n\n";

    // ── 2. Reopen every recorded component ───────────────────────────────
    // No resolver: this program has no model components to offer, which is
    // exactly the point.
    IO::ModelInitializer initializer(
        [](const std::string &) -> IModelComponent * { return nullptr; });

    const IO::CompositionSpec analysis =
        IO::compositionForRun(manifest, manifestPath.string());
    if (!initializer.initialize(analysis.toJson(), message))
    {
        std::cerr << "cannot reopen run: " << message << '\n';
        return 1;
    }

    // ── 3. Browse and summarize ──────────────────────────────────────────
    std::cout << std::fixed << std::setprecision(4);
    for (const IO::ComponentSpec &block : analysis.components)
    {
        IModelComponent *component = initializer.component(block.id);
        if (!component)
            continue;

        std::cout << "component \"" << component->id() << "\" ("
                  << (component->status() ==
                              IModelComponent::ComponentStatus::Finished
                          ? "finished"
                          : "unexpected state")
                  << ")\n";

        // Ask the component which catalog entry it is actually serving:
        // an item recorded by several writers is exposed once, from the
        // most capable format this build can read.
        auto *reopened = dynamic_cast<ResultsModelComponent *>(component);

        for (IComponentDataItem *item : component->results())
        {
            const IO::ResultEntry *entry =
                reopened ? reopened->entryFor(item->id())
                         : manifest.entry(block.id, item->id());

            std::vector<double> values;
            if (!readAll(*item, values, message))
            {
                std::cerr << "  " << item->id() << ": " << message << '\n';
                continue;
            }

            const auto [minimum, maximum] =
                std::minmax_element(values.begin(), values.end());
            double sum = 0.0;
            for (double value : values)
                sum += value;

            std::cout << "  " << item->id();
            if (entry && !entry->units.empty())
                std::cout << " [" << entry->units << "]";
            std::cout << "  shape {";
            for (size_t d = 0; d < item->shape().size(); ++d)
                std::cout << (d ? ", " : "") << item->shape()[d];
            std::cout << "}"
                      << "  min " << *minimum << "  max " << *maximum
                      << "  mean " << (sum / values.size());
            if (entry && entry->format.size())
                std::cout << "  (from " << entry->artifact << ")";
            std::cout << '\n';

            // The last recorded time level, the way a UI's "current state"
            // panel would show it.
            if (item->shape().size() >= 2)
            {
                TimeSliceOutput slice(item->id() + "_last", item);
                slice.setTimeIndex(slice.timeCount() - 1);

                std::vector<double> level;
                if (readAll(slice, level, message))
                {
                    std::cout << "    last level:";
                    for (double value : level)
                        std::cout << ' ' << value;
                    std::cout << '\n';
                }
            }
        }
        std::cout << '\n';
    }

    std::cout << "summarized " << manifest.results.size()
              << " recorded item(s) with no model component linked.\n";
    return 0;
}
