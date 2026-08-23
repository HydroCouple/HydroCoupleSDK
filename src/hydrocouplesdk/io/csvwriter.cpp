// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/io/csvwriter.h"

#include <iomanip>

namespace HydroCouple::SDK::IO {

CSVWriter::CSVWriter(std::filesystem::path path)
    : m_path(std::move(path))
{
}

std::string CSVWriter::name() const
{
    return "csv";
}

int CSVWriter::initialize()
{
    std::error_code ec;
    if (m_path.has_parent_path())
        std::filesystem::create_directories(m_path.parent_path(), ec);

    m_stream.open(m_path, std::ios::out | std::ios::trunc);
    if (!m_stream)
    {
        m_error = "cannot open '" + m_path.string() + "' for writing";
        return 1;
    }
    m_stream << "time,item,index,value\n";
    return 0;
}

int CSVWriter::write(const Snapshot &snapshot)
{
    m_stream << std::setprecision(17);

    // Track the time axis and per-item extents so catalog() can describe a
    // long-format file that states neither.
    if (!m_haveTimes)
    {
        m_firstTime = snapshot.time();
        m_haveTimes = true;
    }
    m_lastTime = snapshot.time();

    for (const FieldSlice &field : snapshot.fields())
    {
        ItemExtent &extent = m_extents[field.itemId];
        if (extent.timeLevels == 0)
        {
            m_itemOrder.push_back(field.itemId);
            extent.elementShape = field.shape;
            extent.kind         = field.kind;
        }
        ++extent.timeLevels;

        const int64_t count = field.elementCount();
        for (int64_t i = 0; i < count; ++i)
        {
            double value = 0.0;
            if (!elementAsDouble(field, i, value))
            {
                m_error = "unsupported DataKind for field '" + field.itemId + "'";
                return 3;
            }
            m_stream << snapshot.time() << ',' << field.itemId << ',' << i
                     << ',' << value << '\n';
        }
    }
    if (!m_stream)
    {
        m_error = "stream failure writing '" + m_path.string() + "'";
        return 2;
    }
    return 0;
}

int CSVWriter::finalize()
{
    if (m_stream.is_open())
    {
        m_stream.flush();
        const bool ok = static_cast<bool>(m_stream);
        m_stream.close();
        if (!ok)
        {
            m_error = "stream failure finalizing '" + m_path.string() + "'";
            return 2;
        }
    }
    return 0;
}

std::string CSVWriter::errorMessage() const
{
    return m_error;
}

bool CSVWriter::elementAsDouble(const FieldSlice &field, int64_t index,
                                double &value)
{
    switch (field.kind)
    {
    case DataKind::Float64: value = field.values<double>()[index];    return true;
    case DataKind::Float32: value = field.values<float>()[index];     return true;
    case DataKind::Int64:   value = static_cast<double>(field.values<int64_t>()[index]);  return true;
    case DataKind::UInt64:  value = static_cast<double>(field.values<uint64_t>()[index]); return true;
    case DataKind::Int32:   value = field.values<int32_t>()[index];   return true;
    case DataKind::UInt32:  value = field.values<uint32_t>()[index];  return true;
    case DataKind::Int16:   value = field.values<int16_t>()[index];   return true;
    case DataKind::UInt16:  value = field.values<uint16_t>()[index];  return true;
    case DataKind::Int8:    value = field.values<int8_t>()[index];    return true;
    case DataKind::UInt8:   value = field.values<uint8_t>()[index];   return true;
    case DataKind::Boolean: value = field.values<bool>()[index] ? 1.0 : 0.0; return true;
    default: return false;
    }
}

std::vector<ResultEntry> CSVWriter::catalog() const
{
    std::vector<ResultEntry> entries;
    entries.reserve(m_itemOrder.size());

    for (const std::string &itemId : m_itemOrder)
    {
        const ItemExtent &extent = m_extents.at(itemId);

        ResultEntry entry;
        entry.itemId   = itemId;
        entry.artifact = m_path.filename().string();
        entry.format   = "csv";
        entry.variable = itemId;
        entry.kind     = extent.kind;

        // Long format stores one row per (time, element): the logical
        // variable is { time, elements... }.
        entry.shape.push_back(extent.timeLevels);
        entry.dimensions.emplace_back("time");
        for (size_t d = 0; d < extent.elementShape.size(); ++d)
        {
            entry.shape.push_back(extent.elementShape[d]);
            entry.dimensions.push_back(extent.elementShape.size() == 1
                                           ? "element"
                                           : "dim" + std::to_string(d));
        }

        ResultTimeAxis axis;
        axis.count = extent.timeLevels;
        axis.start = m_firstTime;
        axis.end   = m_lastTime;
        entry.time = axis;

        entries.push_back(std::move(entry));
    }
    return entries;
}

} // namespace HydroCouple::SDK::IO
