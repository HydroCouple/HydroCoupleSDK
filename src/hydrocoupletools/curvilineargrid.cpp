// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "hydrocouplesdk/tools/curvilineargrid.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace HydroCouple::SDK::Tools {

namespace {

//! Largest coordinate difference at which two boundary curve ends meet.
constexpr double kCornerTolerance = 1e-9;

//! Whether two points agree within kCornerTolerance in x and y.
bool cornersMatch(const Spatial::Point &a, const Spatial::Point &b)
{
    return std::abs(a.x - b.x) <= kCornerTolerance &&
           std::abs(a.y - b.y) <= kCornerTolerance;
}

//! The z component of the 2-D cross product a × b.
double cross(double ax, double ay, double bx, double by)
{
    return ax * by - ay * bx;
}

} // namespace

bool CurvilinearGridGenerator::transfinite(
    std::span<const Spatial::Point> south,
    std::span<const Spatial::Point> north,
    std::span<const Spatial::Point> west,
    std::span<const Spatial::Point> east, StructuredGrid2D &out,
    std::string &message)
{
    const int ni = static_cast<int>(south.size());
    const int nj = static_cast<int>(west.size());
    if (ni < 2 || nj < 2)
    {
        message = "boundary curves need at least 2 points each";
        return false;
    }
    if (north.size() != south.size() || east.size() != west.size())
    {
        message = "opposite boundary curves must have equal point counts";
        return false;
    }
    if (!cornersMatch(south.front(), west.front()) ||
        !cornersMatch(south.back(), east.front()) ||
        !cornersMatch(north.front(), west.back()) ||
        !cornersMatch(north.back(), east.back()))
    {
        message = "boundary curves do not meet at consistent corners";
        return false;
    }

    out.ni = ni;
    out.nj = nj;
    out.x.assign(static_cast<size_t>(ni) * nj, 0.0);
    out.y.assign(static_cast<size_t>(ni) * nj, 0.0);

    const Spatial::Point cornerSW = south.front();
    const Spatial::Point cornerSE = south.back();
    const Spatial::Point cornerNW = north.front();
    const Spatial::Point cornerNE = north.back();

    for (int j = 0; j < nj; ++j)
    {
        const double eta = static_cast<double>(j) / (nj - 1);
        for (int i = 0; i < ni; ++i)
        {
            const double xi = static_cast<double>(i) / (ni - 1);

            const double xCurve =
                (1.0 - eta) * south[static_cast<size_t>(i)].x +
                eta * north[static_cast<size_t>(i)].x +
                (1.0 - xi) * west[static_cast<size_t>(j)].x +
                xi * east[static_cast<size_t>(j)].x;
            const double yCurve =
                (1.0 - eta) * south[static_cast<size_t>(i)].y +
                eta * north[static_cast<size_t>(i)].y +
                (1.0 - xi) * west[static_cast<size_t>(j)].y +
                xi * east[static_cast<size_t>(j)].y;

            const double xCorner =
                (1.0 - xi) * (1.0 - eta) * cornerSW.x +
                xi * (1.0 - eta) * cornerSE.x +
                (1.0 - xi) * eta * cornerNW.x + xi * eta * cornerNE.x;
            const double yCorner =
                (1.0 - xi) * (1.0 - eta) * cornerSW.y +
                xi * (1.0 - eta) * cornerSE.y +
                (1.0 - xi) * eta * cornerNW.y + xi * eta * cornerNE.y;

            out.X(i, j) = xCurve - xCorner;
            out.Y(i, j) = yCurve - yCorner;
        }
    }

    message.clear();
    return true;
}

double CurvilinearGridGenerator::winslowSmooth(StructuredGrid2D &grid,
                                               int iterations)
{
    double lastMaxMove = 0.0;
    for (int sweep = 0; sweep < iterations; ++sweep)
    {
        lastMaxMove = 0.0;
        for (int j = 1; j < grid.nj - 1; ++j)
        {
            for (int i = 1; i < grid.ni - 1; ++i)
            {
                const double xXi  = (grid.X(i + 1, j) - grid.X(i - 1, j)) / 2;
                const double yXi  = (grid.Y(i + 1, j) - grid.Y(i - 1, j)) / 2;
                const double xEta = (grid.X(i, j + 1) - grid.X(i, j - 1)) / 2;
                const double yEta = (grid.Y(i, j + 1) - grid.Y(i, j - 1)) / 2;

                const double alpha = xEta * xEta + yEta * yEta;
                const double beta  = xXi * xEta + yXi * yEta;
                const double gamma = xXi * xXi + yXi * yXi;

                const double denominator = 2.0 * (alpha + gamma);
                if (denominator <= 0.0)
                    continue;

                const double xCrossTerm =
                    grid.X(i + 1, j + 1) - grid.X(i - 1, j + 1) -
                    grid.X(i + 1, j - 1) + grid.X(i - 1, j - 1);
                const double yCrossTerm =
                    grid.Y(i + 1, j + 1) - grid.Y(i - 1, j + 1) -
                    grid.Y(i + 1, j - 1) + grid.Y(i - 1, j - 1);

                const double xNew =
                    (alpha * (grid.X(i + 1, j) + grid.X(i - 1, j)) +
                     gamma * (grid.X(i, j + 1) + grid.X(i, j - 1)) -
                     0.5 * beta * xCrossTerm) /
                    denominator;
                const double yNew =
                    (alpha * (grid.Y(i + 1, j) + grid.Y(i - 1, j)) +
                     gamma * (grid.Y(i, j + 1) + grid.Y(i, j - 1)) -
                     0.5 * beta * yCrossTerm) /
                    denominator;

                lastMaxMove = std::max(
                    lastMaxMove, std::max(std::abs(xNew - grid.X(i, j)),
                                          std::abs(yNew - grid.Y(i, j))));
                grid.X(i, j) = xNew;
                grid.Y(i, j) = yNew;
            }
        }
    }
    return lastMaxMove;
}

double CurvilinearGridGenerator::minCornerJacobian(
    const StructuredGrid2D &grid)
{
    double minimum = std::numeric_limits<double>::max();
    for (int j = 0; j + 1 < grid.nj; ++j)
    {
        for (int i = 0; i + 1 < grid.ni; ++i)
        {
            // Cell corners counter-clockwise.
            const double px[4] = {grid.X(i, j), grid.X(i + 1, j),
                                  grid.X(i + 1, j + 1), grid.X(i, j + 1)};
            const double py[4] = {grid.Y(i, j), grid.Y(i + 1, j),
                                  grid.Y(i + 1, j + 1), grid.Y(i, j + 1)};
            for (int c = 0; c < 4; ++c)
            {
                const int prev = (c + 3) % 4;
                const int next = (c + 1) % 4;
                minimum = std::min(
                    minimum, cross(px[next] - px[c], py[next] - py[c],
                                   px[prev] - px[c], py[prev] - py[c]));
            }
        }
    }
    return minimum;
}

double CurvilinearGridGenerator::maxOrthogonalityDeviation(
    const StructuredGrid2D &grid)
{
    double maxDeviation = 0.0;
    for (int j = 1; j < grid.nj - 1; ++j)
    {
        for (int i = 1; i < grid.ni - 1; ++i)
        {
            const double xXi  = grid.X(i + 1, j) - grid.X(i - 1, j);
            const double yXi  = grid.Y(i + 1, j) - grid.Y(i - 1, j);
            const double xEta = grid.X(i, j + 1) - grid.X(i, j - 1);
            const double yEta = grid.Y(i, j + 1) - grid.Y(i, j - 1);
            const double magnitudes =
                std::hypot(xXi, yXi) * std::hypot(xEta, yEta);
            if (magnitudes <= 0.0)
                continue;
            const double cosAngle =
                std::clamp((xXi * xEta + yXi * yEta) / magnitudes, -1.0, 1.0);
            const double angleDeg =
                std::acos(cosAngle) * 180.0 / 3.14159265358979323846;
            maxDeviation = std::max(maxDeviation, std::abs(angleDeg - 90.0));
        }
    }
    return maxDeviation;
}

IO::MeshDefinition CurvilinearGridGenerator::toMeshDefinition(
    const StructuredGrid2D &grid, const std::string &meshName)
{
    IO::MeshDefinition mesh;
    mesh.meshName = meshName;
    mesh.nodeX    = grid.x;
    mesh.nodeY    = grid.y;

    mesh.faceNodeOffsets.push_back(0);
    for (int j = 0; j + 1 < grid.nj; ++j)
    {
        for (int i = 0; i + 1 < grid.ni; ++i)
        {
            const int64_t n00 = static_cast<int64_t>(j) * grid.ni + i;
            const int64_t n10 = n00 + 1;
            const int64_t n01 = n00 + grid.ni;
            const int64_t n11 = n01 + 1;
            mesh.faceNodes.insert(mesh.faceNodes.end(),
                                  {n00, n10, n11, n01});
            mesh.faceNodeOffsets.push_back(
                static_cast<int64_t>(mesh.faceNodes.size()));
        }
    }
    return mesh;
}

bool CurvilinearGridGenerator::extrude(const StructuredGrid2D &grid,
                                       std::span<const double> zLevels,
                                       StructuredGrid3D &out,
                                       std::string &message)
{
    if (grid.ni < 2 || grid.nj < 2)
    {
        message = "2-D grid is empty";
        return false;
    }
    if (zLevels.size() < 2)
    {
        message = "extrusion needs at least 2 z-levels";
        return false;
    }
    for (size_t k = 1; k < zLevels.size(); ++k)
    {
        if (zLevels[k] == zLevels[k - 1] ||
            (zLevels[k] > zLevels[k - 1]) != (zLevels[1] > zLevels[0]))
        {
            message = "z-levels must be strictly monotone";
            return false;
        }
    }

    out.ni = grid.ni;
    out.nj = grid.nj;
    out.nk = static_cast<int>(zLevels.size());
    const size_t layerNodes = static_cast<size_t>(grid.ni) * grid.nj;
    out.x.resize(layerNodes * zLevels.size());
    out.y.resize(out.x.size());
    out.z.resize(out.x.size());

    for (size_t k = 0; k < zLevels.size(); ++k)
    {
        std::copy(grid.x.begin(), grid.x.end(),
                  out.x.begin() + static_cast<int64_t>(k * layerNodes));
        std::copy(grid.y.begin(), grid.y.end(),
                  out.y.begin() + static_cast<int64_t>(k * layerNodes));
        std::fill_n(out.z.begin() + static_cast<int64_t>(k * layerNodes),
                    layerNodes, zLevels[k]);
    }

    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Tools
