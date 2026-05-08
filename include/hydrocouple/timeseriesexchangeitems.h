// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   timeseriesexchangeitems.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Time-series exchange items (Input/Output) for temporal data flow.
 */

#ifndef HYDROCOUPLESDK_TIMESERIESEXCHANGEITEMS_H
#define HYDROCOUPLESDK_TIMESERIESEXCHANGEITEMS_H

#include "abstractinput.h"
#include "abstractoutput.h"
#include "timeseriescomponentdataitem.h"

namespace HydroCouple::Temporal {

// ═══════════════════════════════════════════════════════════════════════════
// TIME-SERIES INPUT — receives time-varying scalar data
// ═══════════════════════════════════════════════════════════════════════════

/*!
 * \brief Time-series input for integer values.
 *
 * Combines AbstractInput with TimeSeriesComponentDataItem<int> to accept
 * time-varying integer data from provider components.
 * Dimensions are expected to include the time axis as the first dimension.
 */
class HYDROCOUPLESDK_EXPORT TimeSeriesInputInt
    : public AbstractInput,
      public TimeSeriesComponentDataItem<int>
{
public:
    TimeSeriesInputInt(std::string_view id,
                       const std::vector<HydroCouple::Dimension *> &dimensions,
                       HydroCouple::ValueDefinition *valueDef,
                       HydroCouple::IModelComponent *modelComponent);

    ~TimeSeriesInputInt() override = default;

    HydroCouple::IDimension *timeDimension() const override;

    using TimeSeriesComponentDataItem<int>::getValue;
    using TimeSeriesComponentDataItem<int>::getValues;
    using TimeSeriesComponentDataItem<int>::setValue;
    using TimeSeriesComponentDataItem<int>::setValues;

private:
    HydroCouple::Dimension *m_timeDim;
};

/*!
 * \brief Time-series input for double values.
 */
class HYDROCOUPLESDK_EXPORT TimeSeriesInputDouble
    : public AbstractInput,
      public TimeSeriesComponentDataItem<double>
{
public:
    TimeSeriesInputDouble(std::string_view id,
                          const std::vector<HydroCouple::Dimension *> &dimensions,
                          HydroCouple::ValueDefinition *valueDef,
                          HydroCouple::IModelComponent *modelComponent);

    ~TimeSeriesInputDouble() override = default;

    HydroCouple::IDimension *timeDimension() const override;

    using TimeSeriesComponentDataItem<double>::getValue;
    using TimeSeriesComponentDataItem<double>::getValues;
    using TimeSeriesComponentDataItem<double>::setValue;
    using TimeSeriesComponentDataItem<double>::setValues;

private:
    HydroCouple::Dimension *m_timeDim;
};

/*!
 * \brief Time-series input for string values.
 */
class HYDROCOUPLESDK_EXPORT TimeSeriesInputString
    : public AbstractInput,
      public TimeSeriesComponentDataItem<std::string>
{
public:
    TimeSeriesInputString(std::string_view id,
                          const std::vector<HydroCouple::Dimension *> &dimensions,
                          HydroCouple::ValueDefinition *valueDef,
                          HydroCouple::IModelComponent *modelComponent);

    ~TimeSeriesInputString() override = default;

    HydroCouple::IDimension *timeDimension() const override;

    using TimeSeriesComponentDataItem<std::string>::getValue;
    using TimeSeriesComponentDataItem<std::string>::getValues;
    using TimeSeriesComponentDataItem<std::string>::setValue;
    using TimeSeriesComponentDataItem<std::string>::setValues;

private:
    HydroCouple::Dimension *m_timeDim;
};

// ═══════════════════════════════════════════════════════════════════════════
// TIME-SERIES OUTPUT — provides time-varying scalar data
// ═══════════════════════════════════════════════════════════════════════════

/*!
 * \brief Time-series output for integer values.
 *
 * Combines AbstractOutput with TimeSeriesComponentDataItem<int> to provide
 * time-varying integer data to consumer components.
 * Dimensions are expected to include the time axis as the first dimension.
 */
class HYDROCOUPLESDK_EXPORT TimeSeriesOutputInt
    : public AbstractOutput,
      public TimeSeriesComponentDataItem<int>
{
public:
    TimeSeriesOutputInt(std::string_view id,
                        const std::vector<HydroCouple::Dimension *> &dimensions,
                        HydroCouple::ValueDefinition *valueDef,
                        HydroCouple::IModelComponent *modelComponent);

    ~TimeSeriesOutputInt() override = default;

    HydroCouple::IDimension *timeDimension() const override;

    using TimeSeriesComponentDataItem<int>::getValue;
    using TimeSeriesComponentDataItem<int>::getValues;
    using TimeSeriesComponentDataItem<int>::setValue;
    using TimeSeriesComponentDataItem<int>::setValues;

private:
    HydroCouple::Dimension *m_timeDim;
};

/*!
 * \brief Time-series output for double values.
 */
class HYDROCOUPLESDK_EXPORT TimeSeriesOutputDouble
    : public AbstractOutput,
      public TimeSeriesComponentDataItem<double>
{
public:
    TimeSeriesOutputDouble(std::string_view id,
                           const std::vector<HydroCouple::Dimension *> &dimensions,
                           HydroCouple::ValueDefinition *valueDef,
                           HydroCouple::IModelComponent *modelComponent);

    ~TimeSeriesOutputDouble() override = default;

    HydroCouple::IDimension *timeDimension() const override;

    using TimeSeriesComponentDataItem<double>::getValue;
    using TimeSeriesComponentDataItem<double>::getValues;
    using TimeSeriesComponentDataItem<double>::setValue;
    using TimeSeriesComponentDataItem<double>::setValues;

private:
    HydroCouple::Dimension *m_timeDim;
};

/*!
 * \brief Time-series output for string values.
 */
class HYDROCOUPLESDK_EXPORT TimeSeriesOutputString
    : public AbstractOutput,
      public TimeSeriesComponentDataItem<std::string>
{
public:
    TimeSeriesOutputString(std::string_view id,
                           const std::vector<HydroCouple::Dimension *> &dimensions,
                           HydroCouple::ValueDefinition *valueDef,
                           HydroCouple::IModelComponent *modelComponent);

    ~TimeSeriesOutputString() override = default;

    HydroCouple::IDimension *timeDimension() const override;

    using TimeSeriesComponentDataItem<std::string>::getValue;
    using TimeSeriesComponentDataItem<std::string>::getValues;
    using TimeSeriesComponentDataItem<std::string>::setValue;
    using TimeSeriesComponentDataItem<std::string>::setValues;

private:
    HydroCouple::Dimension *m_timeDim;
};

} // namespace HydroCouple::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESEXCHANGEITEMS_H
