// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Serial coupling example — YAML/JSON-initialized two-component
 *         composition, orchestrated by a workflow, persisted through the
 *         IO thread to CSV, GeoPackage, and HDF5/UGRID.
 *
 * Demonstrates the whole SDK spine end to end:
 *
 * 1. Two model components built on AbstractModelComponent, exchanging
 *    values through the v2 typed hyperslab data plane (no variants);
 * 2. ModelInitializer applying a composition document — arguments,
 *    component initialization, and the provider wiring — from YAML when
 *    yaml-cpp is present, otherwise the identical JSON;
 * 3. TimeSteppedWorkflow deriving its execution schedule from the
 *    connection graph at prepare() and advancing the composition;
 * 4. IOThread carrying owned Snapshots to the format writers off the
 *    simulation thread (CSV always; GeoPackage and HDF5/UGRID when the
 *    SDK was built with those features);
 * 5. RunRecorder writing a run manifest — the catalog that lets analysis
 *    and visualization tools reopen this run later without any model
 *    binary (see the analysis_reopen example).
 *
 * Build (from an installed SDK):
 * \code
 *   cmake -S examples/serial_coupling -B build \
 *         -DCMAKE_PREFIX_PATH=<sdk-install-prefix>
 *   cmake --build build && ./build/serial_coupling composition.yaml out/
 * \endcode
 */

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/component/timesteppedworkflow.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/io/csvwriter.h"
#include "hydrocouplesdk/io/iothread.h"
#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouplesdk/io/modelinitializer.h"
#include "hydrocouplesdk/io/runmanifest.h"
#include "hydrocouplesdk/io/snapshot.h"

#ifdef HYDROCOUPLESDK_HAVE_GEOPACKAGE
#include "hydrocouplesdk/io/geopackagewriter.h"
#endif
#ifdef HYDROCOUPLESDK_HAVE_HDF5
#include "hydrocouplesdk/io/hdf5ugridwriter.h"
#endif

#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;

namespace {

constexpr int kReachCount = 3;
constexpr int kTimeSteps  = 24;

//! [catchment]
/*!
 * \brief Rainfall-runoff component: one argument (per-reach runoff
 *        coefficient), one output (runoff per reach).
 */
class CatchmentComponent : public AbstractModelComponent
{
public:
    explicit CatchmentComponent(std::string_view id)
        : AbstractModelComponent(id, "Catchment")
    {
    }

    std::vector<std::string> validate() override
    {
        setStatus(ComponentStatus::Validating);
        setStatus(ComponentStatus::Valid);
        return {};
    }

    void prepare() override
    {
        setStatus(ComponentStatus::Preparing);
        m_step = 0;
        setStatus(ComponentStatus::Updated, "prepared");
    }

    void update(const std::vector<IOutput *> & = {}) override
    {
        setStatus(ComponentStatus::Updating);

        // A synthetic storm: rainfall peaks mid-run.
        const double hours    = static_cast<double>(m_step);
        const double rainfall = 12.0 * std::exp(-0.02 * (hours - 8.0) *
                                                (hours - 8.0));
        for (int reach = 0; reach < kReachCount; ++reach)
            (*runoff)[reach] = rainfall * (*coefficients)[reach];

        ++m_step;
        setStatus(m_step >= kTimeSteps ? ComponentStatus::Done
                                       : ComponentStatus::Updated,
                  "stepped");
    }

    void finish() override
    {
        setStatus(ComponentStatus::Finishing);
        setStatus(ComponentStatus::Finished);
    }

    [[nodiscard]] int step() const noexcept { return m_step; }

    Argument1DDouble *coefficients = nullptr;
    Output1DDouble   *runoff       = nullptr;

protected:
    void createArguments() override
    {
        m_quantity.reset(Quantity::unitLess("RunoffCoefficient"));
        coefficients = new Argument1DDouble("coefficients", &m_dimension,
                                            kReachCount, m_quantity.get(),
                                            this);
        addArgument(coefficients);
    }

    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }

    void createInputs() override {}

    void createOutputs() override
    {
        runoff = new Output1DDouble("runoff", &m_dimension, kReachCount,
                                    m_quantity.get(), this);
        addOutput(runoff);
    }

    void initializeFailureCleanUp() override {}

private:
    Dimension                 m_dimension{"reach", "Reach index"};
    std::unique_ptr<Quantity> m_quantity;
    int                       m_step = 0;
};

//! [catchment]
//! [channel]
/*!
 * \brief Channel routing component: consumes lateral inflow, produces
 *        routed flow with a simple Muskingum-style lag.
 */
class ChannelComponent : public AbstractModelComponent
{
public:
    explicit ChannelComponent(std::string_view id)
        : AbstractModelComponent(id, "Channel")
    {
    }

    std::vector<std::string> validate() override
    {
        setStatus(ComponentStatus::Validating);
        setStatus(ComponentStatus::Valid);
        return {};
    }

    void prepare() override
    {
        setStatus(ComponentStatus::Preparing);
        m_step = 0;
        for (int reach = 0; reach < kReachCount; ++reach)
            (*flow)[reach] = 0.0;
        setStatus(ComponentStatus::Updated, "prepared");
    }

    void update(const std::vector<IOutput *> & = {}) override
    {
        setStatus(ComponentStatus::Updating);

        // Pull the provider's values into lateralInflow, then route.
        applyInputValues();
        for (int reach = 0; reach < kReachCount; ++reach)
        {
            const double weight  = (*routingWeight)[reach];
            const double inflow  = (*lateralInflow)[reach];
            const double routed  = (*flow)[reach];
            (*flow)[reach] = weight * routed + (1.0 - weight) * inflow;
        }

        ++m_step;
        setStatus(m_step >= kTimeSteps ? ComponentStatus::Done
                                       : ComponentStatus::Updated,
                  "stepped");
    }

    void finish() override
    {
        setStatus(ComponentStatus::Finishing);
        setStatus(ComponentStatus::Finished);
    }

    Argument1DDouble *routingWeight = nullptr;
    Input1DDouble    *lateralInflow = nullptr;
    Output1DDouble   *flow          = nullptr;

protected:
    void createArguments() override
    {
        m_quantity.reset(Quantity::unitLess("Flow"));
        routingWeight = new Argument1DDouble("routing_weight", &m_dimension,
                                             kReachCount, m_quantity.get(),
                                             this);
        addArgument(routingWeight);
    }

    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }

    void createInputs() override
    {
        lateralInflow = new Input1DDouble("lateral_inflow", &m_dimension,
                                          kReachCount, m_quantity.get(),
                                          this);
        addInput(lateralInflow);
    }

    void createOutputs() override
    {
        flow = new Output1DDouble("flow", &m_dimension, kReachCount,
                                  m_quantity.get(), this);
        addOutput(flow);
    }

    void initializeFailureCleanUp() override {}

private:
    Dimension                 m_dimension{"reach", "Reach index"};
    std::unique_ptr<Quantity> m_quantity;
    int                       m_step = 0;
};

//! [channel]
/*!
 * \brief Three reach centroids used for the GeoPackage feature layer and
 *        the UGRID mesh (a 1-D chain of nodes).
 */
IO::MeshDefinition reachMesh()
{
    IO::MeshDefinition mesh;
    mesh.meshName = "reaches";
    mesh.nodeX    = {0.0, 500.0, 1000.0};
    mesh.nodeY    = {0.0, 250.0, 400.0};
    mesh.edgeNodes.push_back({0, 1});
    mesh.edgeNodes.push_back({1, 2});
    return mesh;
}

} // namespace

int main(int argc, char **argv)
{
    const std::filesystem::path documentPath =
        argc > 1 ? argv[1] : "composition.yaml";
    const std::filesystem::path outputDirectory =
        argc > 2 ? argv[2] : "output";
    std::filesystem::create_directories(outputDirectory);

    //! [initializer]
    // ── 1. Components and composition document ───────────────────────────
    CatchmentComponent catchment("catchment");
    ChannelComponent   channel("channel");

    IO::ModelInitializer initializer(
        [&](const std::string &id) -> IModelComponent * {
            if (id == "catchment")
                return &catchment;
            if (id == "channel")
                return &channel;
            return nullptr;
        });

    std::string message;
    if (documentPath.extension() == ".yaml" &&
        !IO::ModelInitializer::yamlSupported())
    {
        std::cerr << "This SDK build has no YAML support; pass the .json "
                     "composition instead.\n";
        return 2;
    }
    if (!initializer.initializeFromFile(documentPath, message))
    {
        std::cerr << "composition failed: " << message << '\n';
        return 1;
    }
    std::cout << "initialized composition from " << documentPath << '\n';

    //! [initializer]
    //! [writers]
    // ── 2. Output writers behind the IO thread ───────────────────────────
    IO::IOThread ioThread(/*capacity=*/32);
    IO::RunRecorder recorder;
    recorder.setCaption("serial coupling example");
    recorder.setComposition(documentPath);

    auto csv = std::make_shared<IO::CSVWriter>(outputDirectory /
                                               "results.csv");
    ioThread.addWriter(csv);
    recorder.addWriter(csv);

#ifdef HYDROCOUPLESDK_HAVE_GEOPACKAGE
    auto geopackage = std::make_shared<IO::GeoPackageWriter>(
        outputDirectory / "results.gpkg", "serial_coupling_example");
    geopackage->setMesh(reachMesh());
    geopackage->addField("flow", "m3/s", "Routed channel flow");
    geopackage->addField("runoff", "m3/s", "Catchment runoff");
    ioThread.addWriter(geopackage);
    recorder.addWriter(geopackage);
#endif
#ifdef HYDROCOUPLESDK_HAVE_HDF5
    auto hdf5 = std::make_shared<IO::HDF5UGRIDWriter>(
        outputDirectory / "results.h5", reachMesh());
    // UGRID writers need their fields declared before initialize(): the
    // datasets are created up front and appended to each step.
    hdf5->addField("runoff", IO::MeshLocation::Node, "m3/s",
                   "catchment_runoff");
    hdf5->addField("flow", IO::MeshLocation::Node, "m3/s",
                   "routed_channel_flow");
    ioThread.addWriter(hdf5);
    recorder.addWriter(hdf5);
#endif

    if (!ioThread.start())
    {
        std::cerr << "IO thread failed to start (code "
                  << ioThread.errorCode() << ")\n";
        return 1;
    }

    //! [writers]
    //! [workflow]
    // ── 3. Workflow orchestration ────────────────────────────────────────
    TimeSteppedWorkflow workflow("serial-coupling");
    recorder.addComponent(&catchment);
    recorder.addComponent(&channel);
    if (!workflow.addModelComponent(&catchment, nullptr, &message) ||
        !workflow.addModelComponent(&channel, nullptr, &message))
    {
        std::cerr << "workflow wiring failed: " << message << '\n';
        return 1;
    }

    workflow.initialize();
    const std::vector<std::string> problems = workflow.validate();
    if (!problems.empty())
    {
        for (const std::string &problem : problems)
            std::cerr << "invalid composition: " << problem << '\n';
        return 1;
    }
    workflow.prepare();

    // The schedule is derived from the connection graph: catchment first.
    std::cout << "execution schedule:";
    for (const auto &group : workflow.schedule())
    {
        std::cout << " [";
        for (const IModelComponent *component : group)
            std::cout << ' ' << component->id();
        std::cout << " ]";
    }
    std::cout << '\n';

    //! [workflow]
    //! [run]
    // ── 4. Run, snapshotting each step to the writers ────────────────────
    int guard = 0;
    while (workflow.status() !=
               IWorkflowComponent::WorkflowStatus::Done &&
           ++guard <= kTimeSteps + 1)
    {
        workflow.update();
        if (workflow.status() == IWorkflowComponent::WorkflowStatus::Failed)
        {
            for (const ErrorEntry &entry : workflow.errors())
                std::cerr << "workflow error: " << entry.message << '\n';
            return 1;
        }

        IO::Snapshot snapshot(static_cast<double>(catchment.step()));
        if (!snapshot.capture(*catchment.runoff, message) ||
            !snapshot.capture(*channel.flow, message))
        {
            std::cerr << "snapshot failed: " << message << '\n';
            return 1;
        }
        if (!ioThread.enqueue(std::move(snapshot)))
        {
            std::cerr << "writer error (code " << ioThread.errorCode()
                      << ")\n";
            return 1;
        }
    }

    workflow.finish();
    ioThread.finish();

    if (ioThread.errorCode() != 0)
    {
        std::cerr << "IO finished with error code " << ioThread.errorCode()
                  << '\n';
        return 1;
    }

    //! [run]
    //! [manifest]
    // ── 5. Record the run so it can be reopened for analysis ─────────────
    // Writer catalogs are only valid after the IO thread has finalized.
    if (!recorder.finishToFile(outputDirectory / "run.json", message,
                               IO::RunStatus::Completed))
    {
        std::cerr << "run manifest failed: " << message << '\n';
        return 1;
    }

    std::cout << "completed " << catchment.step() << " steps; outputs in "
              << std::filesystem::absolute(outputDirectory) << '\n';
    std::cout << "run manifest written to run.json\n";

    //! [manifest]
    //! [roundtrip]
    // ── 5. Round-trip the composition back out ───────────────────────────
    if (!initializer.serializeToFile({"catchment", "channel"},
                                     outputDirectory / "composition.out.json",
                                     message))
    {
        std::cerr << "serialization failed: " << message << '\n';
        return 1;
    }
    std::cout << "composition round-tripped to composition.out.json\n";
    //! [roundtrip]
    return 0;
}
