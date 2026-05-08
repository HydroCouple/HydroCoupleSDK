// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   timeseriesargument.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Time-series argument for model configuration.
 */

#ifndef HYDROCOUPLESDK_TIMESERIESARGUMENT_H
#define HYDROCOUPLESDK_TIMESERIESARGUMENT_H

#include "abstractargument.h"
#include "timeseries.h"

#include <memory>
#include <string>

namespace HydroCouple::Temporal {

/*!
 * \brief Time-series argument holding scalar TimeSeries.
 *
 * Loads time-series data from CSV files and provides interpolated
 * values at query times. Used for temporal model configuration.
 *
 * \threadsafety Not thread-safe on its own.
 */
class HYDROCOUPLESDK_EXPORT TimeSeriesArgumentDouble
    : public AbstractArgument
{
public:
    /*!
     * \brief Construct a time-series argument.
     * \param id               Identifier
     * \param caption          Human-readable caption
     * \param modelComponent   Owning component
     * \param timeDim          Dimension for time axis
     */
    TimeSeriesArgumentDouble(std::string_view id,
                             std::string_view caption,
                             HydroCouple::IModelComponent *modelComponent,
                             HydroCouple::Dimension *timeDim);

    ~TimeSeriesArgumentDouble() override = default;

    // ── File I/O ────────────────────────────────────────────────────────────

    /*!
     * \brief Load time-series from CSV file (time, value columns).
     */
    bool loadFromCSV(const std::string &filePath);

    /*!
     * \brief Get interpolated value at query time.
     */
    double getInterpolatedValue(double julianDay) const;

    /*!
     * \brief Access the underlying TimeSeries.
     */
    std::shared_ptr<TimeSeries> timeSeries() const noexcept { return m_timeSeries; }

protected:
    std::shared_ptr<TimeSeries> m_timeSeries;
    HydroCouple::Dimension *m_timeDim;

    // IComponentDataItem (read-only)
    void getValue(HydroCouple::hydrocouple_variant &data,
                  const std::initializer_list<int> &dimensionIndexes) const override;

    void getValues(HydroCouple::hydrocouple_variant *data,
                   const std::initializer_list<int> &dimensionIndexes,
                   const std::initializer_list<int> &dimensionLengths) const override;

    void setValue(const HydroCouple::hydrocouple_variant &data,
                  const std::initializer_list<int> &dimensionIndexes) override {}

    void setValues(const HydroCouple::hydrocouple_variant *data,
                   const std::initializer_list<int> &dimensionIndexes,
                   const std::initializer_list<int> &dimensionLengths) override {}

    int dimensionLength(const std::initializer_list<int> &dims) const override;
};

} // namespace HydroCouple::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESARGUMENT_H
