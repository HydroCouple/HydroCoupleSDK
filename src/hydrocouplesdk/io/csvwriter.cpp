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
    for (const FieldSlice &field : snapshot.fields())
    {
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

} // namespace HydroCouple::SDK::IO
