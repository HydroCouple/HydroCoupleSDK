// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/distributed/fluxpayload.h"

#include <algorithm>
#include <stdexcept>

namespace HydroCouple::SDK::Distributed {

FluxDerivativePayload::FluxDerivativePayload(int64_t count)
    : m_count(std::max<int64_t>(0, count)),
      m_values(static_cast<size_t>(2 * m_count), 0.0),
      m_shape{2, m_count}
{
}

int64_t FluxDerivativePayload::count() const noexcept
{
    return m_count;
}

std::span<double> FluxDerivativePayload::flux() noexcept
{
    return {m_values.data(), static_cast<size_t>(m_count)};
}

std::span<const double> FluxDerivativePayload::flux() const noexcept
{
    return {m_values.data(), static_cast<size_t>(m_count)};
}

std::span<double> FluxDerivativePayload::derivative() noexcept
{
    return {m_values.data() + m_count, static_cast<size_t>(m_count)};
}

std::span<const double> FluxDerivativePayload::derivative() const noexcept
{
    return {m_values.data() + m_count, static_cast<size_t>(m_count)};
}

BufferDescriptor FluxDerivativePayload::descriptor() const noexcept
{
    BufferDescriptor descriptor;
    descriptor.data  = const_cast<double *>(m_values.data());
    descriptor.kind  = DataKind::Float64;
    descriptor.rank  = 2;
    descriptor.shape = m_shape.data();
    descriptor.space = MemorySpace::Host;
    return descriptor;
}

double FluxDerivativePayload::linearizedFlux(int64_t index, double state,
                                             double referenceState) const
{
    if (index < 0 || index >= m_count)
        throw std::out_of_range(
            "FluxDerivativePayload::linearizedFlux: index " +
            std::to_string(index) + " outside [0, " + std::to_string(m_count) +
            ")");
    return m_values[static_cast<size_t>(index)] +
           m_values[static_cast<size_t>(m_count + index)] *
               (state - referenceState);
}

bool FluxDerivativePayload::conformsToConvention(
    const BufferDescriptor &descriptor, int64_t count, std::string &message)
{
    if (descriptor.kind != DataKind::Float64)
    {
        message = "flux+derivative payload must be Float64";
        return false;
    }
    if (descriptor.rank != 2 || descriptor.shape == nullptr)
    {
        message = "flux+derivative payload must have rank 2 (shape {2, N})";
        return false;
    }
    if (descriptor.shape[0] != 2)
    {
        message = "flux+derivative payload row dimension must be 2 "
                  "(flux row then derivative row), got " +
                  std::to_string(descriptor.shape[0]);
        return false;
    }
    if (count >= 0 && descriptor.shape[1] != count)
    {
        message = "flux+derivative payload carries " +
                  std::to_string(descriptor.shape[1]) +
                  " entities; expected " + std::to_string(count);
        return false;
    }
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Distributed
