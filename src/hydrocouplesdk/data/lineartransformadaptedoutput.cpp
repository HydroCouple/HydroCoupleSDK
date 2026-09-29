// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/lineartransformadaptedoutput.h"
#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/differentiation/differentials.h"

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
    m_source = std::move(source);

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

namespace HydroCouple::SDK {

namespace {
using HydroCouple::DifferentialRole;
using Differentiation::itemOf;
using Differentiation::readDifferential;
using Differentiation::writeDifferential;

bool refuse(std::string *message, const std::string &text)
{
    if (message)
        *message = text;
    return false;
}
} // namespace

std::vector<HydroCouple::IArgument *>
LinearTransformAdaptedOutput::differentiableArguments() const
{
    return {const_cast<Argument1DDouble *>(&m_multiplier),
            const_cast<Argument1DDouble *>(&m_offset)};
}

bool LinearTransformAdaptedOutput::vjp(HydroCouple::DifferentialSet seeds,
                                       HydroCouple::DifferentialSet results,
                                       std::string *message)
{
    if (m_source.empty())
        return refuse(message, "LinearTransformAdaptedOutput \"" + id() +
                                   "\" has no refresh to differentiate");
    const std::vector<int64_t> shape{static_cast<int64_t>(m_source.size())};
    const std::vector<int64_t> scalar{1};

    std::vector<double> ybar(m_source.size(), 0.0);
    for (const HydroCouple::DifferentialEntry &e : seeds)
    {
        if (e.item != itemOf<HydroCouple::IComponentDataItem>(this) ||
            e.role != DifferentialRole::Output)
            return refuse(message, "seed names an item or role this adapter "
                                   "does not differentiate");
        if (!readDifferential(e, shape, ybar, message))
            return false;
    }

    for (const HydroCouple::DifferentialEntry &e : results)
    {
        if (e.item == itemOf(adaptee()) && e.role == DifferentialRole::Input)
        {
            std::vector<double> xbar(ybar.size());
            for (size_t i = 0; i < ybar.size(); ++i)
                xbar[i] = m_m * ybar[i];
            if (!writeDifferential(e, shape, xbar, message))
                return false;
        }
        else if (e.item == itemOf<HydroCouple::IComponentDataItem>(&m_multiplier) &&
                 e.role == DifferentialRole::Argument)
        {
            double mbar = 0.0;
            for (size_t i = 0; i < ybar.size(); ++i)
                mbar += ybar[i] * m_source[i];
            if (!writeDifferential(e, scalar, {mbar}, message))
                return false;
        }
        else if (e.item == itemOf<HydroCouple::IComponentDataItem>(&m_offset) &&
                 e.role == DifferentialRole::Argument)
        {
            double bbar = 0.0;
            for (double v : ybar)
                bbar += v;
            if (!writeDifferential(e, scalar, {bbar}, message))
                return false;
        }
        else
        {
            return refuse(message, "result names an item or role this adapter "
                                   "does not differentiate");
        }
    }
    return true;
}

bool LinearTransformAdaptedOutput::jvp(HydroCouple::DifferentialSet seeds,
                                       HydroCouple::DifferentialSet results,
                                       std::string *message)
{
    if (m_source.empty())
        return refuse(message, "LinearTransformAdaptedOutput \"" + id() +
                                   "\" has no refresh to differentiate");
    const std::vector<int64_t> shape{static_cast<int64_t>(m_source.size())};
    const std::vector<int64_t> scalar{1};

    std::vector<double> xdot(m_source.size(), 0.0), mdot{0.0}, bdot{0.0};
    for (const HydroCouple::DifferentialEntry &e : seeds)
    {
        bool ok = false;
        if (e.item == itemOf(adaptee()) && e.role == DifferentialRole::Input)
            ok = readDifferential(e, shape, xdot, message);
        else if (e.item == itemOf<HydroCouple::IComponentDataItem>(&m_multiplier) &&
                 e.role == DifferentialRole::Argument)
            ok = readDifferential(e, scalar, mdot, message);
        else if (e.item == itemOf<HydroCouple::IComponentDataItem>(&m_offset) &&
                 e.role == DifferentialRole::Argument)
            ok = readDifferential(e, scalar, bdot, message);
        else
            return refuse(message, "seed names an item or role this adapter "
                                   "does not differentiate");
        if (!ok)
            return false;
    }

    std::vector<double> ydot(m_source.size());
    for (size_t i = 0; i < ydot.size(); ++i)
        ydot[i] = m_m * xdot[i] + mdot[0] * m_source[i] + bdot[0];

    for (const HydroCouple::DifferentialEntry &e : results)
    {
        if (e.item != itemOf<HydroCouple::IComponentDataItem>(this) ||
            e.role != DifferentialRole::Output)
            return refuse(message, "result names an item or role this adapter "
                                   "does not differentiate");
        if (!writeDifferential(e, shape, ydot, message))
            return false;
    }
    return true;
}

} // namespace HydroCouple::SDK
