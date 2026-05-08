// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/timeseriesexchangeitems.h"

namespace HydroCouple::Temporal {

// ═══════════════════════════════════════════════════════════════════════════
// TIME-SERIES INPUT IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════

TimeSeriesInputInt::TimeSeriesInputInt(
    std::string_view id,
    const std::vector<HydroCouple::Dimension *> &dimensions,
    HydroCouple::ValueDefinition *valueDef,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractInput(id, dimensions, valueDef, modelComponent),
      TimeSeriesComponentDataItem<int>(100, 1),
      m_timeDim(dimensions.size() > 0 ? dimensions[0] : nullptr)
{
}

HydroCouple::IDimension *TimeSeriesInputInt::timeDimension() const {
    return reinterpret_cast<HydroCouple::IDimension *>(m_timeDim);
}

TimeSeriesInputDouble::TimeSeriesInputDouble(
    std::string_view id,
    const std::vector<HydroCouple::Dimension *> &dimensions,
    HydroCouple::ValueDefinition *valueDef,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractInput(id, dimensions, valueDef, modelComponent),
      TimeSeriesComponentDataItem<double>(100, 1),
      m_timeDim(dimensions.size() > 0 ? dimensions[0] : nullptr)
{
}

HydroCouple::IDimension *TimeSeriesInputDouble::timeDimension() const {
    return reinterpret_cast<HydroCouple::IDimension *>(m_timeDim);
}

TimeSeriesInputString::TimeSeriesInputString(
    std::string_view id,
    const std::vector<HydroCouple::Dimension *> &dimensions,
    HydroCouple::ValueDefinition *valueDef,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractInput(id, dimensions, valueDef, modelComponent),
      TimeSeriesComponentDataItem<std::string>(100, 1),
      m_timeDim(dimensions.size() > 0 ? dimensions[0] : nullptr)
{
}

HydroCouple::IDimension *TimeSeriesInputString::timeDimension() const {
    return reinterpret_cast<HydroCouple::IDimension *>(m_timeDim);
}

// ═══════════════════════════════════════════════════════════════════════════
// TIME-SERIES OUTPUT IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════════

TimeSeriesOutputInt::TimeSeriesOutputInt(
    std::string_view id,
    const std::vector<HydroCouple::Dimension *> &dimensions,
    HydroCouple::ValueDefinition *valueDef,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractOutput(id, dimensions, valueDef, modelComponent),
      TimeSeriesComponentDataItem<int>(100, 1),
      m_timeDim(dimensions.size() > 0 ? dimensions[0] : nullptr)
{
}

HydroCouple::IDimension *TimeSeriesOutputInt::timeDimension() const {
    return reinterpret_cast<HydroCouple::IDimension *>(m_timeDim);
}

TimeSeriesOutputDouble::TimeSeriesOutputDouble(
    std::string_view id,
    const std::vector<HydroCouple::Dimension *> &dimensions,
    HydroCouple::ValueDefinition *valueDef,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractOutput(id, dimensions, valueDef, modelComponent),
      TimeSeriesComponentDataItem<double>(100, 1),
      m_timeDim(dimensions.size() > 0 ? dimensions[0] : nullptr)
{
}

HydroCouple::IDimension *TimeSeriesOutputDouble::timeDimension() const {
    return reinterpret_cast<HydroCouple::IDimension *>(m_timeDim);
}

TimeSeriesOutputString::TimeSeriesOutputString(
    std::string_view id,
    const std::vector<HydroCouple::Dimension *> &dimensions,
    HydroCouple::ValueDefinition *valueDef,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractOutput(id, dimensions, valueDef, modelComponent),
      TimeSeriesComponentDataItem<std::string>(100, 1),
      m_timeDim(dimensions.size() > 0 ? dimensions[0] : nullptr)
{
}

HydroCouple::IDimension *TimeSeriesOutputString::timeDimension() const {
    return reinterpret_cast<HydroCouple::IDimension *>(m_timeDim);
}

} // namespace HydroCouple::Temporal
