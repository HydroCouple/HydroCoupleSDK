// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/relaxationadaptedoutput.h"
#include "hydrocouplesdk/data/bufferio.h"

namespace HydroCouple::SDK {

RelaxationAdaptedOutput::RelaxationAdaptedOutput(
    std::string_view id, HydroCouple::IOutput *adaptee,
    HydroCouple::IAdaptedOutputFactory *factory)
    : AbstractAdaptedOutput(id, {}, nullptr, adaptee, factory),
      m_relaxationFactor("relaxation_factor", &m_argDimension, 1,
                         m_factorQuantity.get(), nullptr)
{
    m_relaxationFactor[0] = 1.0;
    addArgument(&m_relaxationFactor);
}

void RelaxationAdaptedOutput::initialize()
{
    m_lambda = m_relaxationFactor[0];
    m_values.clear();
    refresh();
}

void RelaxationAdaptedOutput::refresh()
{
    std::vector<double> source;
    if (!pullAdaptee(source))
        return;

    if (m_values.size() != source.size())
    {
        // No history (first refresh, or the adaptee changed shape): pass
        // the adaptee through and start blending from here.
        m_values = source;
    }
    else
    {
        for (size_t i = 0; i < source.size(); ++i)
            m_values[i] = (1.0 - m_lambda) * m_values[i] + m_lambda * source[i];
    }

    emitValueChanged({0}, {static_cast<int64_t>(m_values.size())});
    refreshAdaptedOutputs();
}

void RelaxationAdaptedOutput::updateValues()
{
    refresh();
}

std::vector<int64_t> RelaxationAdaptedOutput::shape() const
{
    HydroCouple::IOutput *provider = adaptee();
    return provider ? provider->shape() : std::vector<int64_t>{};
}

DataKind RelaxationAdaptedOutput::dataKind() const
{
    return DataKind::Float64;
}

bool RelaxationAdaptedOutput::getValuesInto(const BufferDescriptor &destination,
                                            std::span<const int64_t> start,
                                            std::span<const int64_t> count,
                                            std::string *message) const
{
    if (m_values.empty())
    {
        if (message)
            *message = "RelaxationAdaptedOutput \"" + id() +
                       "\" has no values; initialize() has not run";
        return false;
    }

    const int64_t extent = static_cast<int64_t>(m_values.size());
    return detail::readHyperslab<double>(
        m_values.data(), std::span<const int64_t>(&extent, 1), destination,
        start, count, message);
}

bool RelaxationAdaptedOutput::setValuesFrom(const BufferDescriptor &,
                                            std::span<const int64_t>,
                                            std::span<const int64_t>,
                                            std::string *message)
{
    if (message)
        *message = "RelaxationAdaptedOutput is read-only";
    return false;
}

bool RelaxationAdaptedOutput::pullAdaptee(std::vector<double> &into) const
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
