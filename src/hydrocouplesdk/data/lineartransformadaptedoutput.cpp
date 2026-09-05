// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/lineartransformadaptedoutput.h"
#include "hydrocouplesdk/data/bufferio.h"

namespace HydroCouple::SDK {

LinearTransformAdaptedOutput::LinearTransformAdaptedOutput(
    std::string_view id, HydroCouple::IOutput *adaptee,
    HydroCouple::IAdaptedOutputFactory *factory)
    : AbstractAdaptedOutput(id, {}, nullptr, adaptee, factory),
      m_multiplier("multiplier", &m_argDimension, 1,
                   m_multiplierQuantity.get(), nullptr),
      m_offset("offset", &m_argDimension, 1, m_offsetQuantity.get(), nullptr)
{
    m_multiplier[0] = 1.0;
    m_offset[0]     = 0.0;

    addArgument(&m_multiplier);
    addArgument(&m_offset);
}

void LinearTransformAdaptedOutput::initialize()
{
    m_m = m_multiplier[0];
    m_b = m_offset[0];
    refresh();
}

void LinearTransformAdaptedOutput::refresh()
{
    std::vector<double> source;
    if (!pullAdaptee(source))
        return;

    m_values.resize(source.size());
    for (size_t i = 0; i < source.size(); ++i)
        m_values[i] = m_m * source[i] + m_b;

    emitValueChanged({0}, {static_cast<int64_t>(m_values.size())});
    refreshAdaptedOutputs();
}

void LinearTransformAdaptedOutput::updateValues()
{
    refresh();
}

std::vector<int64_t> LinearTransformAdaptedOutput::shape() const
{
    HydroCouple::IOutput *provider = adaptee();
    return provider ? provider->shape() : std::vector<int64_t>{};
}

DataKind LinearTransformAdaptedOutput::dataKind() const
{
    return DataKind::Float64;
}

bool LinearTransformAdaptedOutput::getValuesInto(
    const BufferDescriptor &destination, std::span<const int64_t> start,
    std::span<const int64_t> count, std::string *message) const
{
    if (m_values.empty())
    {
        if (message)
            *message = "LinearTransformAdaptedOutput \"" + id() +
                       "\" has no values; initialize() has not run";
        return false;
    }

    const int64_t extent = static_cast<int64_t>(m_values.size());
    return detail::readHyperslab<double>(
        m_values.data(), std::span<const int64_t>(&extent, 1), destination,
        start, count, message);
}

bool LinearTransformAdaptedOutput::setValuesFrom(const BufferDescriptor &,
                                                 std::span<const int64_t>,
                                                 std::span<const int64_t>,
                                                 std::string *message)
{
    if (message)
        *message = "LinearTransformAdaptedOutput is read-only";
    return false;
}

bool LinearTransformAdaptedOutput::pullAdaptee(std::vector<double> &into) const
{
    HydroCouple::IOutput *provider = adaptee();
    if (!provider)
        return false;

    const std::vector<int64_t> providerShape = provider->shape();
    if (providerShape.size() != 1 || providerShape[0] < 0)
        return false;

    into.resize(static_cast<size_t>(providerShape[0]));

    HydroCouple::BufferDescriptor destination;
    destination.data  = into.data();
    destination.kind  = DataKind::Float64;
    destination.rank  = 1;
    destination.shape = providerShape.data();

    const int64_t start = 0;
    return provider->getValuesInto(destination,
                                   std::span<const int64_t>(&start, 1),
                                   std::span<const int64_t>(&providerShape[0], 1),
                                   nullptr);
}

} // namespace HydroCouple::SDK
