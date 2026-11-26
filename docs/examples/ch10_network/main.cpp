// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 10: a river network as an SDK Network, served through
 *         INetwork by NetworkAdapter, with edge flows in a
 *         TimeNetworkComponentDataItem.
 *
 * A main stem and one tributary join at a confluence: four vertices, three
 * edges, all pointing downstream. Exits nonzero when any value is not the
 * one the guide states.
 */

#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/spatial/meshadapters.h"
#include "hydrocouplesdk/spatiotemporal/timenetworkcomponentdataitem.h"
#include "hydrocouplesdk/temporal/timedata.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple::SDK::Spatial;
namespace HCS = HydroCouple::Spatial;
using HydroCouple::SDK::SpatioTemporal::TimeNetworkComponentDataItemDouble;
using HydroCouple::SDK::Temporal::TimeData;

namespace {

int failures = 0;

void check(bool ok, const char *what)
{
    if (!ok)
    {
        std::printf("FAILED: %s\n", what);
        ++failures;
    }
}

bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

} // namespace

int main()
{
    //! [network]
    Network river;
    const int headwater  = river.addVertex(0.0, 2000.0, 120.0);
    const int tributary  = river.addVertex(1500.0, 1000.0, 118.0);
    const int confluence = river.addVertex(0.0, 1000.0, 110.0);
    const int outlet     = river.addVertex(0.0, 0.0, 100.0);

    const int upperMain = river.addEdge(headwater, confluence);
    const int branch    = river.addEdge(tributary, confluence);
    const int lowerMain = river.addEdge(confluence, outlet);
    const int bad       = river.addEdge(outlet, 7); // no vertex 7: refused
    //! [network]

    std::printf("network: %d vertices, %d edges, bad edge index %d, total length %.3f m\n",
                river.vertexCount(), river.edgeCount(), bad, river.totalLength());
    check(bad == -1 && river.edgeCount() == 3, "an edge to a missing vertex is refused");
    check(near(river.edge(upperMain).length(&river), std::sqrt(1000.0 * 1000.0 + 10.0 * 10.0)),
          "NetworkEdge::length includes z");

    //! [adapter]
    NetworkAdapter adapter(river);
    const HCS::INetwork &graph = adapter;
    const HCS::IMeshView *view = graph.meshView();
    //! [adapter]

    std::printf("adapter: id is the network's (%zu characters), %lld vertices, %lld edges; edge %d runs %s -> %s\n",
                adapter.id().size(), static_cast<long long>(graph.vertexCount()),
                static_cast<long long>(graph.edgeCount()), branch,
                graph.edge(branch)->orig()->id().c_str(),
                graph.edge(branch)->dest()->id().c_str());
    std::printf("vertex edges: headwater -> edge %lld, confluence -> edge %lld, outlet -> %s\n",
                static_cast<long long>(graph.vertex(headwater)->edge()->index()),
                static_cast<long long>(graph.vertex(confluence)->edge()->index()),
                graph.vertex(outlet)->edge() ? "an edge" : "nullptr");
    std::printf("mesh view: %lld nodes, %lld edges, %lld faces, nodeZ %zu values, "
                "edge %d plan length %.1f m, normal (%.1f, %.1f), sym %s\n",
                static_cast<long long>(view->nodeCount()),
                static_cast<long long>(view->edgeCount()),
                static_cast<long long>(view->faceCount()), view->nodeZ().size(), branch,
                view->edgeLengths()[branch], view->edgeNormalX()[branch],
                view->edgeNormalY()[branch], graph.edge(branch)->sym() ? "edge" : "nullptr");

    check(adapter.id() == river.id(), "the adapter takes the network's id");
    check(graph.edge(branch)->orig() == graph.vertex(tributary), "edges point at the adapter's vertices");
    check(graph.vertex(outlet)->edge() == nullptr, "no edge leaves the outlet");
    check(view->faceCount() == 0 && view->nodeZ().empty(), "a network view has no faces and no z");
    check(near(view->edgeLengths()[branch], 1500.0), "mesh-view lengths are plan lengths");
    check(near(view->edgeNormalX()[branch], 0.0) && near(view->edgeNormalY()[branch], 1.0),
          "normal is (dy, -dx) / L");
    check(view->edgeNodes()[2 * lowerMain] == confluence, "edge pairs keep the network's order");

    //! [network_item]
    // Flow on each edge (m3/s), hourly; the vector basis of a scalar is irrelevant.
    TimeNetworkComponentDataItemDouble flow(
        "flow", &adapter, HCS::MeshLocation::Edge,
        HCS::SpatialDataType::Scalar, static_cast<int>(graph.edgeCount()));
    flow.setTimeKind(HydroCouple::Temporal::TimeKind::Instantaneous);
    flow.setTimeInterpolation(HydroCouple::Temporal::TimeInterpolation::Linear);
    flow.addTimeStep(std::make_shared<TimeData>("t0", 2026, 1, 1, 0), {4.0, 1.5, 5.5});
    flow.addTimeStep(std::make_shared<TimeData>("t1", 2026, 1, 1, 1), {4.4, 1.9, 6.3});

    // One edge through every time step: a strided read of column lowerMain.
    std::array<double, 2> outletFlow{};
    const std::array<int64_t, 2> start{0, lowerMain}, count{2, 1};
    const std::array<int64_t, 1> bufferShape{2};
    std::string message;
    const bool read = flow.getValuesInto(
        HydroCouple::SDK::detail::hostDescriptor(outletFlow.data(),
                                                 HydroCouple::DataKind::Float64,
                                                 bufferShape),
        start, count, &message);
    //! [network_item]

    std::printf("flow item: dimensions \"%s\" x \"%s\", shape {%lld, %lld}, "
                "outlet reach %.1f then %.1f m3/s\n",
                flow.timeDimension()->id().c_str(), flow.entityDimension()->id().c_str(),
                static_cast<long long>(flow.shape()[0]), static_cast<long long>(flow.shape()[1]),
                outletFlow[0], outletFlow[1]);
    check(read, message.c_str());
    check(flow.entityDimension()->id() == "edges", "edge values get an edges dimension");
    check(near(outletFlow[0], 5.5) && near(outletFlow[1], 6.3), "column read");
    check(flow.network() == &adapter, "the item borrows the adapter");

    std::printf("%s\n", failures == 0 ? "all checks passed" : "checks FAILED");
    return failures == 0 ? 0 : 1;
}
