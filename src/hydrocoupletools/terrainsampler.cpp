// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "hydrocouplesdk/tools/terrainsampler.h"

#include <cmath>

namespace HydroCouple::SDK::Tools {

bool TerrainSampler::idw(std::span<const Spatial::Point> samples,
                         double power, std::span<const double> queryX,
                         std::span<const double> queryY,
                         std::span<double> elevationOut, std::string &message,
                         double searchRadius)
{
    if (samples.empty())
    {
        message = "no samples";
        return false;
    }
    if (power <= 0.0)
    {
        message = "IDW power must be positive";
        return false;
    }
    if (queryX.size() != queryY.size() ||
        queryX.size() != elevationOut.size())
    {
        message = "query and output extents differ";
        return false;
    }

    const double radiusSquared =
        searchRadius > 0.0 ? searchRadius * searchRadius : 0.0;

    for (size_t q = 0; q < queryX.size(); ++q)
    {
        double weightSum   = 0.0;
        double weightedSum = 0.0;
        bool   exact       = false;

        for (const Spatial::Point &sample : samples)
        {
            const double dx = queryX[q] - sample.x;
            const double dy = queryY[q] - sample.y;
            const double distanceSquared = dx * dx + dy * dy;
            if (radiusSquared > 0.0 && distanceSquared > radiusSquared)
                continue;
            if (distanceSquared == 0.0)
            {
                elevationOut[q] = sample.z;
                exact           = true;
                break;
            }
            const double weight =
                1.0 / std::pow(std::sqrt(distanceSquared), power);
            weightSum += weight;
            weightedSum += weight * sample.z;
        }

        if (exact)
            continue;
        if (weightSum <= 0.0)
        {
            message = "query " + std::to_string(q) +
                      " has no sample within the search radius";
            return false;
        }
        elevationOut[q] = weightedSum / weightSum;
    }

    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Tools
