// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   timeseriescomponentdataitem.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Time-series data storage mixin for temporal component data items.
 */

#ifndef HYDROCOUPLESDK_TIMESERIESCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_TIMESERIESCOMPONENTDATAITEM_H

#include "hydrocouple.h"
#include "timedata.h"

#include <vector>
#include <memory>
#include <algorithm>

namespace HydroCouple::Temporal {

/*!
 * \brief Time-series data storage mixin providing temporal indexing.
 *
 * This is a pure storage/accessor mixin (like ComponentDataItem1D) that provides:
 * - Time management (add time steps, query current time)
 * - Flat value storage indexed by (timeIndex, spatialIndex)
 * - Implementation of ITimeSeriesComponentDataItem methods
 *
 * Concrete classes multiply-inherit from an Input/Output base AND this mixin.
 *
 * Data layout (for spatial dimension of size N):
 * - m_times[0..T-1]: shared_ptr<IDateTime> for each time step
 * - m_values[t * N + i]: value at time t, spatial index i
 *
 * \tparam T Scalar value type (int, double, std::string, etc.)
 * \threadsafety Not thread-safe on its own; caller synchronizes concurrent access.
 */
template<typename T>
class TimeSeriesComponentDataItem
{
public:
    /*!
     * \brief Construct a time-series storage mixin.
     * \param expectedTimeSteps Hint for vector pre-allocation
     * \param spatialSize Number of spatial elements at each time step (default 1 for scalar)
     */
    explicit TimeSeriesComponentDataItem(int expectedTimeSteps = 100, int spatialSize = 1)
        : m_spatialSize(spatialSize), m_timeSpan(std::make_shared<TimeSpan>("TimeSpan", 0.0, 0.0))
    {
        m_times.reserve(expectedTimeSteps);
        m_values.reserve(expectedTimeSteps * spatialSize);
    }

    virtual ~TimeSeriesComponentDataItem() = default;

    // ── Time management ──────────────────────────────────────────────────
    /*!
     * \brief Add a time step with initial spatial values.
     * \param time     DateTime for this time step (should be strictly increasing)
     * \param values   Vector of T values for this time step (must match spatialSize)
     */
    void addTimeStep(std::shared_ptr<IDateTime> time, const std::vector<T> &values) {
        if (values.size() != (size_t)m_spatialSize) {
            throw std::invalid_argument("TimeSeriesComponentDataItem: values size must match spatialSize");
        }
        m_times.push_back(time);
        m_values.insert(m_values.end(), values.begin(), values.end());

        // Update timeSpan to cover [min, max] time range
        if (m_times.size() == 1) {
            m_timeSpan = std::make_shared<TimeSpan>("TimeSpan", time->julianDay(), 0.0);
        } else {
            double start = m_times.front()->julianDay();
            double end = time->julianDay();
            m_timeSpan = std::make_shared<TimeSpan>("TimeSpan", start, end - start);
        }
    }

    /*!
     * \brief ITimeSeriesComponentDataItem::time implementation.
     */
    const IDateTime *time(int timeIndex) const {
        if (timeIndex < 0 || timeIndex >= (int)m_times.size()) return nullptr;
        return m_times[timeIndex].get();
    }

    /*!
     * \brief ITimeSeriesComponentDataItem::timeCount implementation.
     */
    int timeCount() const noexcept { return (int)m_times.size(); }

    /*!
     * \brief ITimeSeriesComponentDataItem::timeSpan implementation.
     */
    ITimeSpan *timeSpan() const noexcept { return m_timeSpan.get(); }

    /*!
     * \brief ITimeSeriesComponentDataItem::timeDimension implementation.
     * Subclass must provide this via mixin or override.
     */
    virtual IDimension *timeDimension() const = 0;

    // ── ITimeSeriesComponentDataItem::getValue/getValues/setValue/setValues ──

    /*!
     * \brief Get value at (timeIndex, spatialIndex) as variant.
     */
    void getValue(HydroCouple::hydrocouple_variant &data,
                  int timeDimensionIndex,
                  const std::initializer_list<int> &dimensionIndexes = {}) const
    {
        if (timeDimensionIndex < 0 || timeDimensionIndex >= (int)m_times.size())
            return;
        if (dimensionIndexes.size() == 0) return;

        int spatialIdx = *dimensionIndexes.begin();
        if (spatialIdx >= 0 && spatialIdx < m_spatialSize) {
            int idx = timeDimensionIndex * m_spatialSize + spatialIdx;
            if (idx >= 0 && idx < (int)m_values.size()) {
                data = m_values[idx];
            }
        }
    }

    /*!
     * \brief Get multiple values at (timeIndex, spatialIndexes...) as variant array.
     */
    void getValues(HydroCouple::hydrocouple_variant *data,
                   int timeDimensionIndex,
                   const std::initializer_list<int> &dimensionIndexes = {},
                   int timeDimensionIndexLength = 1,
                   const std::initializer_list<int> &dimensionLengths = {}) const
    {
        if (timeDimensionIndex < 0 || timeDimensionIndex + timeDimensionIndexLength > (int)m_times.size())
            return;
        if (!data) return;

        if (dimensionIndexes.size() == 0) {
            // Get a time slice (all spatial indices across time range)
            for (int t = 0; t < timeDimensionIndexLength; ++t) {
                for (int s = 0; s < m_spatialSize; ++s) {
                    int idx = (timeDimensionIndex + t) * m_spatialSize + s;
                    if (idx >= 0 && idx < (int)m_values.size()) {
                        data[t * m_spatialSize + s] = m_values[idx];
                    }
                }
            }
        } else {
            // Get subset of spatial indices across time
            auto spatialStart = *dimensionIndexes.begin();
            int spatialLen = dimensionLengths.size() > 0 ? *dimensionLengths.begin() : 1;

            for (int t = 0; t < timeDimensionIndexLength; ++t) {
                for (int s = 0; s < spatialLen; ++s) {
                    int idx = (timeDimensionIndex + t) * m_spatialSize + (spatialStart + s);
                    if (idx >= 0 && idx < (int)m_values.size()) {
                        data[t * spatialLen + s] = m_values[idx];
                    }
                }
            }
        }
    }

    /*!
     * \brief Set value at (timeIndex, spatialIndex) from variant.
     */
    void setValue(const HydroCouple::hydrocouple_variant &data,
                  int timeDimensionIndex,
                  const std::initializer_list<int> &dimensionIndexes = {})
    {
        if (timeDimensionIndex < 0 || timeDimensionIndex >= (int)m_times.size())
            return;
        if (dimensionIndexes.size() == 0) return;

        int spatialIdx = *dimensionIndexes.begin();
        if (spatialIdx >= 0 && spatialIdx < m_spatialSize) {
            int idx = timeDimensionIndex * m_spatialSize + spatialIdx;
            if (idx >= 0 && idx < (int)m_values.size()) {
                m_values[idx] = std::get<T>(data);
            }
        }
    }

    /*!
     * \brief Set multiple values at (timeIndex, spatialIndexes...) from variant array.
     */
    void setValues(const HydroCouple::hydrocouple_variant *data,
                   int timeDimensionIndex,
                   const std::initializer_list<int> &dimensionIndexes = {},
                   int timeDimensionIndexLength = 1,
                   const std::initializer_list<int> &dimensionLengths = {})
    {
        if (timeDimensionIndex < 0 || timeDimensionIndex + timeDimensionIndexLength > (int)m_times.size())
            return;
        if (!data) return;

        if (dimensionIndexes.size() == 0) {
            // Set a time slice (all spatial indices across time range)
            for (int t = 0; t < timeDimensionIndexLength; ++t) {
                for (int s = 0; s < m_spatialSize; ++s) {
                    int idx = (timeDimensionIndex + t) * m_spatialSize + s;
                    if (idx >= 0 && idx < (int)m_values.size()) {
                        m_values[idx] = std::get<T>(data[t * m_spatialSize + s]);
                    }
                }
            }
        } else {
            // Set subset of spatial indices across time
            auto spatialStart = *dimensionIndexes.begin();
            int spatialLen = dimensionLengths.size() > 0 ? *dimensionLengths.begin() : 1;

            for (int t = 0; t < timeDimensionIndexLength; ++t) {
                for (int s = 0; s < spatialLen; ++s) {
                    int idx = (timeDimensionIndex + t) * m_spatialSize + (spatialStart + s);
                    if (idx >= 0 && idx < (int)m_values.size()) {
                        m_values[idx] = std::get<T>(data[t * spatialLen + s]);
                    }
                }
            }
        }
    }

    // ── Direct access ──────────────────────────────────────────────────────
    /*!
     * \brief Get value at specific time and spatial index.
     */
    T getValueAt(int timeIndex, int spatialIndex) const {
        if (timeIndex < 0 || timeIndex >= (int)m_times.size()) return T{};
        int idx = timeIndex * m_spatialSize + spatialIndex;
        if (idx < 0 || idx >= (int)m_values.size()) return T{};
        return m_values[idx];
    }

    /*!
     * \brief Set value at specific time and spatial index.
     */
    void setValueAt(int timeIndex, int spatialIndex, const T &value) {
        if (timeIndex < 0 || timeIndex >= (int)m_times.size()) return;
        int idx = timeIndex * m_spatialSize + spatialIndex;
        if (idx >= 0 && idx < (int)m_values.size())
            m_values[idx] = value;
    }

    int spatialSize() const noexcept { return m_spatialSize; }

protected:
    std::vector<std::shared_ptr<IDateTime>> m_times;
    std::vector<T> m_values;  // flat array: [time * spatialSize + spatialIndex]
    int m_spatialSize;
    std::shared_ptr<TimeSpan> m_timeSpan;
};

} // namespace HydroCouple::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESCOMPONENTDATAITEM_H
