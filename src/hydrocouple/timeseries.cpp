// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/timeseries.h"

#include <fstream>
#include <sstream>
#include <cmath>

namespace HydroCouple::Temporal {

TimeSeries::TimeSeries(std::string_view id, InterpolationMethod method, int expectedSize)
    : Identity(id), m_method(method) {
    m_times.reserve(expectedSize);
    m_values.reserve(expectedSize);
}

TimeSeries::TimeSeries(std::string_view id,
                       const std::vector<double> &times,
                       const std::vector<double> &values,
                       InterpolationMethod method)
    : Identity(id), m_times(times), m_values(values), m_method(method) {}

void TimeSeries::addValue(double time, double value) {
    if (!m_times.empty() && time <= m_times.back()) {
        throw std::invalid_argument("TimeSeries: times must be strictly increasing");
    }
    m_times.push_back(time);
    m_values.push_back(value);
}

int TimeSeries::findLowerBound(double time) const {
    if (m_times.empty() || time < m_times.front()) return -1;
    if (time > m_times.back()) return static_cast<int>(m_times.size()) - 1;

    // Binary search for lower bound
    auto it = std::lower_bound(m_times.begin(), m_times.end(), time);
    if (it == m_times.begin()) return 0;
    return static_cast<int>(std::distance(m_times.begin(), it)) - 1;
}

double TimeSeries::interpolateLinear(int i, double time) const {
    if (i < 0 || i >= static_cast<int>(m_times.size()) - 1) return std::nan("");

    double t0 = m_times[i], t1 = m_times[i + 1];
    double v0 = m_values[i], v1 = m_values[i + 1];
    double alpha = (time - t0) / (t1 - t0);

    return v0 + alpha * (v1 - v0);
}

double TimeSeries::interpolateNearest(int i, double time) const {
    if (i < 0 || i >= static_cast<int>(m_times.size())) return std::nan("");

    int j = i;
    if (i < static_cast<int>(m_times.size()) - 1) {
        double d0 = std::abs(time - m_times[i]);
        double d1 = std::abs(time - m_times[i + 1]);
        if (d1 < d0) j = i + 1;
    }
    return m_values[j];
}

double TimeSeries::interpolateConstant(int i) const {
    if (i < 0 || i >= static_cast<int>(m_times.size())) return std::nan("");
    return m_values[i];
}

double TimeSeries::getValue(double time) const {
    int i = findLowerBound(time);

    switch (m_method) {
    case InterpolationMethod::Linear:
        return interpolateLinear(i, time);
    case InterpolationMethod::Nearest:
        return interpolateNearest(i, time);
    case InterpolationMethod::Constant:
        return interpolateConstant(i);
    }
    return std::nan("");
}

std::vector<double> TimeSeries::getValues(const std::vector<double> &times) const {
    std::vector<double> result;
    result.reserve(times.size());
    for (double t : times) {
        result.push_back(getValue(t));
    }
    return result;
}

bool TimeSeries::loadFromCSV(const std::string &filename, bool skipHeader) {
    std::ifstream ifs(filename);
    if (!ifs) return false;

    m_times.clear();
    m_values.clear();

    std::string line;
    if (skipHeader && std::getline(ifs, line)) {
        // Skip header line
    }

    while (std::getline(ifs, line)) {
        std::stringstream ss(line);
        double time, value;
        if (ss >> time >> value) {
            try {
                addValue(time, value);
            } catch (...) {
                return false;
            }
        }
    }

    return true;
}

bool TimeSeries::saveToCSV(const std::string &filename) const {
    std::ofstream ofs(filename);
    if (!ofs) return false;

    ofs << "Time,Value\n";
    for (int i = 0; i < static_cast<int>(m_times.size()); ++i) {
        ofs << m_times[i] << "," << m_values[i] << "\n";
    }

    return true;
}

} // namespace HydroCouple::Temporal
