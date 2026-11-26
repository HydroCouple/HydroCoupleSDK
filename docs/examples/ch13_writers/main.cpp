// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Chapter 13 example: one run recorded by all four writers. Two mesh fields
// (a depth per face and a head per node) are written for four time levels to
// CSV, HDF5 UGRID, NetCDF UGRID and GeoPackage through one IOThread. The
// program prints each writer's catalog, reads every artifact back through
// createResultReader(), compares the values with what was written, reads the
// mesh back where the format stores one, reopens the run with a
// ResultsModelComponent, and shows how a writer failure is latched.
// Needs an SDK built with USE_HDF5, USE_NETCDF and USE_GEOPACKAGE.

#include "hydrocouplesdk/component/resultsmodelcomponent.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/io/csvwriter.h"
#include "hydrocouplesdk/io/geopackagewriter.h"
#include "hydrocouplesdk/io/hdf5ugridwriter.h"
#include "hydrocouplesdk/io/iothread.h"
#include "hydrocouplesdk/io/netcdfugridwriter.h"
#include "hydrocouplesdk/io/resultreader.h"
#include "hydrocouplesdk/io/runmanifest.h"
#include "hydrocouplesdk/io/snapshot.h"
#include "hydrocouplesdk/io/ugridreader.h"

#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::IO;

namespace {

int failures = 0;

void check(bool condition, const std::string &what)
{
    if (!condition)
    {
        std::cout << "CHECK FAILED: " << what << '\n';
        ++failures;
    }
}

constexpr int    kLevels = 4;
constexpr double kStart  = 2461370.5;

//! [mesh]
// A square of four nodes split into two triangles, with its five edges.
MeshDefinition squareMesh()
{
    MeshDefinition mesh;
    mesh.meshName        = "square";
    mesh.nodeX           = {0.0, 10.0, 10.0, 0.0};
    mesh.nodeY           = {0.0, 0.0, 10.0, 10.0};
    mesh.nodeZ           = {1.0, 1.5, 2.0, 1.2};
    mesh.faceNodeOffsets = {0, 3, 6};
    mesh.faceNodes       = {0, 1, 2, 0, 2, 3};
    mesh.edgeNodes       = {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 0}};
    return mesh;
}
//! [mesh]

std::string shapeText(const std::vector<int64_t> &shape)
{
    std::string text = "{";
    for (size_t d = 0; d < shape.size(); ++d)
        text += (d ? ", " : "") + std::to_string(shape[d]);
    return text + "}";
}

} // namespace

int main()
{
    std::filesystem::remove_all("run");
    const MeshDefinition mesh = squareMesh();

    Dimension faces{"face", "Face"};
    Dimension nodes{"node", "Node"};
    std::unique_ptr<Quantity> meters{Quantity::unitLess("Length")};
    Output1DDouble depth("depth", &faces, 2, meters.get(), nullptr);
    Output1DDouble head("head", &nodes, 4, meters.get(), nullptr);

    //! [writers]
    auto csv = std::make_shared<CSVWriter>("run/results.csv");

    auto hdf5 = std::make_shared<HDF5UGRIDWriter>("run/results.h5", mesh);
    hdf5->addField("depth", MeshLocation::Face, "m", "water_depth");
    hdf5->addField("head", MeshLocation::Node, "m");

    auto netcdf = std::make_shared<NetCDFUGRIDWriter>("run/results.nc", mesh);
    netcdf->addField("depth", MeshLocation::Face, "m", "water_depth");
    netcdf->addField("head", MeshLocation::Node, "m");

    auto geopackage = std::make_shared<GeoPackageWriter>("run/results.gpkg",
                                                         "chapter-13");
    geopackage->setMesh(mesh);
    geopackage->addField("depth", "m", "Water depth per face");
    geopackage->addField("head", "m", "Hydraulic head per node");

    IOThread    io(16);
    RunRecorder recorder("chapter-13-writers");
    for (const std::shared_ptr<IOutputWriter> &writer :
         std::vector<std::shared_ptr<IOutputWriter>>{csv, hdf5, netcdf,
                                                     geopackage})
    {
        io.addWriter(writer);
        recorder.addWriter(writer);
    }
    //! [writers]

    //! [run]
    if (!io.start())
    {
        std::cout << "a writer failed to start: " << io.errorMessage() << '\n';
        return 1;
    }
    std::map<std::string, std::vector<double>> written;
    std::string message;
    for (int level = 0; level < kLevels; ++level)
    {
        for (int f = 0; f < 2; ++f)
            depth[f] = 0.25 * (level + 1) + 0.1 * f;
        for (int n = 0; n < 4; ++n)
            head[n] = mesh.nodeZ[static_cast<size_t>(n)] + 0.25 * (level + 1);
        Snapshot snapshot(kStart + level / 24.0);
        if (!snapshot.capture(depth, message) || !snapshot.capture(head, message))
        {
            std::cout << message << '\n';
            return 1;
        }
        for (int f = 0; f < 2; ++f)
            written["depth"].push_back(depth[f]);
        for (int n = 0; n < 4; ++n)
            written["head"].push_back(head[n]);
        (void)io.enqueue(std::move(snapshot));
    }
    io.finish();
    if (io.errorCode() != 0)
    {
        std::cout << "writer error: " << io.errorMessage() << '\n';
        return 1;
    }
    // No component was registered, so the entries stay unattributed.
    if (!recorder.finishToFile("run/run.json", message))
    {
        std::cout << message << '\n';
        return 1;
    }
    //! [run]

    RunManifest manifest;
    if (!RunManifest::read("run/run.json", manifest, message))
    {
        std::cout << message << '\n';
        return 1;
    }

    //! [catalog]
    for (const ResultEntry &entry : manifest.results)
    {
        std::cout << entry.format << " " << entry.itemId << ": "
                  << entry.artifact << ", " << dataKindName(entry.kind)
                  << " " << shapeText(entry.shape) << " (" << entry.dimensions[0]
                  << ", " << entry.dimensions[1] << ")";
        if (!entry.mesh.empty())
            std::cout << ", mesh " << entry.mesh;
        if (entry.location)
            std::cout << " at " << meshLocationName(*entry.location);
        if (!entry.units.empty())
            std::cout << ", units " << entry.units;
        std::cout << '\n';
    }
    //! [catalog]
    check(manifest.results.size() == 8, "two items from four writers");

    //! [readers]
    for (const ResultEntry &entry : manifest.results)
    {
        std::unique_ptr<IResultReader> reader =
            createResultReader(entry.format, message);
        if (!reader || !reader->open(manifest.artifactPath(entry), message))
        {
            std::cout << entry.format << ": " << message << '\n';
            ++failures;
            continue;
        }
        std::vector<double> values(static_cast<size_t>(entry.elementCount()));
        const int64_t       count = entry.elementCount();
        BufferDescriptor    into;
        into.data  = values.data();
        into.kind  = DataKind::Float64;
        into.rank  = 1;
        into.shape = &count;
        into.space = MemorySpace::Host;
        const std::vector<int64_t> start(entry.shape.size(), 0);
        if (!reader->read(entry, start, entry.shape, into, message))
        {
            std::cout << entry.format << ": " << message << '\n';
            ++failures;
            continue;
        }
        std::vector<double> times;
        (void)reader->readTimes(entry, times, message);

        MeshDefinition readBack;
        const bool hasMesh = reader->readMesh(entry, readBack, message);

        std::cout << entry.format << " " << entry.itemId << " reads back "
                  << (values == written[entry.itemId] ? "identical" : "DIFFERENT")
                  << " values, " << times.size() << " times, ";
        if (hasMesh)
            std::cout << "mesh " << readBack.meshName << " with "
                      << readBack.nodeCount() << " nodes, "
                      << readBack.faceCount() << " faces, "
                      << readBack.edgeCount() << " edges\n";
        else
            std::cout << "no mesh: " << message << '\n';
        check(values == written[entry.itemId], entry.format + " values");
        check(times.size() == static_cast<size_t>(kLevels),
              entry.format + " times");
    }
    //! [readers]

    //! [ugrid]
    for (const char *file : {"run/results.nc", "run/results.h5"})
    {
        const std::vector<std::string> names = ugridMeshNames(file, message);
        MeshDefinition read;
        const bool ok = readUGRIDMesh(file, "", read, message);
        std::cout << file << ": mesh " << (names.empty() ? "?" : names[0])
                  << ", " << read.nodeCount() << " nodes, " << read.faceCount()
                  << " faces, " << read.nodeZ.size() << " elevations\n";
        check(ok && read.faceNodes == mesh.faceNodes, std::string(file) + " mesh");
    }
    //! [ugrid]

    //! [reopen]
    // Every entry, from every writer: one item per id, served by the most
    // capable readable format (HDF5 first, then NetCDF, GeoPackage, CSV).
    ResultsModelComponent reopened(manifest, "", "reopened");
    reopened.initialize();
    for (IComponentDataItem *item : reopened.results())
        std::cout << "reopened " << item->id() << " is served from "
                  << reopened.entryFor(item->id())->format << '\n';
    //! [reopen]
    check(reopened.results().size() == 2 &&
              reopened.entryFor("depth")->format == "hdf5_ugrid",
          "the HDF5 copy is preferred");

    //! [failure]
    // A UGRID writer needs every registered field in every snapshot.
    auto strict = std::make_shared<HDF5UGRIDWriter>("run/strict.h5", mesh);
    strict->addField("depth", MeshLocation::Face);
    strict->addField("head", MeshLocation::Node);
    IOThread failing(4);
    failing.addWriter(strict);
    (void)failing.start();
    Snapshot partial(kStart);
    (void)partial.capture(depth, message); // no "head"
    const bool queued = failing.enqueue(std::move(partial));
    failing.finish();
    std::cout << "enqueue returned " << (queued ? "true" : "false")
              << "; latched error " << failing.errorCode() << ": "
              << failing.errorMessage() << '\n';
    //! [failure]
    check(queued && failing.errorCode() == 21, "missing field is error 21");

    std::cout << (failures == 0 ? "all checks passed" : "checks failed")
              << '\n';
    return failures == 0 ? 0 : 1;
}
