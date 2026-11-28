// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/io/meshdefinition.h"

namespace HydroCouple::SDK::IO {

bool MeshDefinition::validate(std::string &message) const
{
    if (nodeX.empty() || nodeX.size() != nodeY.size())
    {
        message = "MeshDefinition: nodeX/nodeY must be non-empty and equal "
                  "length";
        return false;
    }
    if (!nodeZ.empty() && nodeZ.size() != nodeX.size())
    {
        message = "MeshDefinition: nodeZ must be empty or match nodeX length";
        return false;
    }
    if (!faceNodeOffsets.empty())
    {
        if (faceNodeOffsets.front() != 0 ||
            faceNodeOffsets.back() != static_cast<int64_t>(faceNodes.size()))
        {
            message = "MeshDefinition: faceNodeOffsets must start at 0 and "
                      "end at faceNodes.size()";
            return false;
        }
        for (size_t f = 0; f + 1 < faceNodeOffsets.size(); ++f)
            if (faceNodeOffsets[f + 1] < faceNodeOffsets[f])
            {
                message = "MeshDefinition: faceNodeOffsets must be "
                          "non-decreasing";
                return false;
            }
        for (int64_t node : faceNodes)
            if (node < 0 || node >= nodeCount())
            {
                message = "MeshDefinition: faceNodes index out of range";
                return false;
            }
    }
    for (const auto &edge : edgeNodes)
        for (int64_t node : edge)
            if (node < 0 || node >= nodeCount())
            {
                message = "MeshDefinition: edgeNodes index out of range";
                return false;
            }
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::IO
