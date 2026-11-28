// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/relaxationadaptedoutput.h"
#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/differentiation/differentials.h"

#include <bit>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace HydroCouple::SDK {

namespace {
using HydroCouple::DifferentialRole;
using Differentiation::itemOf;
using Differentiation::readDifferential;
using Differentiation::writeDifferential;

//! Stores \p text in \p message (when given) and returns false, for the
//! refusals of vjp() and jvp().
//! \param[out] message Optional failure description.
//! \param[in]  text    The reason.
//! \return false.
bool refuse(std::string *message, const std::string &text)
{
    if (message)
        *message = text;
    return false;
}

//! Version tag of the checkpoint token's layout: the tag, the value count,
//! then each value's IEEE-754 bit pattern as 16 hex digits -- exact for
//! every double (subnormals, infinities and NaNs included) and independent
//! of locale, so another process restores it bit for bit.
constexpr std::string_view kTokenTag = "relaxation-v1";

//! The next space-separated field of \p rest, or an empty view at the end.
//! \param[in,out] rest Remaining token text; advanced past the field.
//! \return The field (a view into the token).
std::string_view nextField(std::string_view &rest)
{
    const size_t start = rest.find_first_not_of(' ');
    if (start == std::string_view::npos)
    {
        rest = {};
        return {};
    }
    rest.remove_prefix(start);
    const size_t end = rest.find(' ');
    const std::string_view field = rest.substr(0, end);
    rest.remove_prefix(end == std::string_view::npos ? rest.size() : end);
    return field;
}

//! Parses a whole field as an integer with std::from_chars.
//! \tparam T Integer type.
//! \param[in]  field Text to parse.
//! \param[out] value Parsed value.
//! \param[in]  base  Number base (16 for the value bit patterns).
//! \return true when the field is non-empty and parsed completely.
template <typename T>
bool parseField(std::string_view field, T &value, int base = 10)
{
    const char *first = field.data();
    const char *last = field.data() + field.size();
    const auto [end, error] = std::from_chars(first, last, value, base);
    return !field.empty() && error == std::errc() && end == last;
}
} // namespace

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
    m_values.clear();
    m_haveRefresh = false;
    refresh();
}

void RelaxationAdaptedOutput::refresh()
{
    // Whatever happens below, the previous refresh is no longer the latest.
    m_haveRefresh = false;
    std::vector<double> source;
    if (!pullAdaptee(source))
        return;

    m_lambda = m_relaxationFactor[0];
    m_source = source;
    if (m_values.size() != source.size())
    {
        // No history (first refresh, or the adaptee changed shape): pass
        // the adaptee through and start blending from here.
        m_blended = false;
        m_previous.clear();
        m_values = std::move(source);
    }
    else
    {
        m_blended = true;
        m_previous = m_values;
        for (size_t i = 0; i < source.size(); ++i)
            m_values[i] = (1.0 - m_lambda) * m_values[i] + m_lambda * source[i];
    }
    m_haveRefresh = true;

    emitValueChanged({0}, {static_cast<int64_t>(m_values.size())});
    refreshAdaptedOutputs();
}

void RelaxationAdaptedOutput::updateValues()
{
    refresh();
}

std::vector<HydroCouple::IComponentDataItem *> RelaxationAdaptedOutput::states() const
{
    return {const_cast<HydroCouple::IComponentDataItem *>(
        itemOf<HydroCouple::IComponentDataItem>(this))};
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

// ── IDifferentiableAdaptedOutput ────────────────────────────────────────────

std::vector<HydroCouple::IArgument *>
RelaxationAdaptedOutput::differentiableArguments() const
{
    return {const_cast<Argument1DDouble *>(&m_relaxationFactor)};
}

std::vector<HydroCouple::IComponentDataItem *>
RelaxationAdaptedOutput::differentiableStates() const
{
    return states();
}

bool RelaxationAdaptedOutput::vjp(HydroCouple::DifferentialSet seeds,
                                  HydroCouple::DifferentialSet results,
                                  std::string *message)
{
    if (!m_haveRefresh)
        return refuse(message, "RelaxationAdaptedOutput \"" + id() +
                                   "\" has no refresh to differentiate");
    const std::vector<int64_t> shape{static_cast<int64_t>(m_source.size())};
    const std::vector<int64_t> scalar{1};
    const auto *self = itemOf<HydroCouple::IComponentDataItem>(this);

    // t = ȳ_out + ȳ_state: the output and the state it carries forward are
    // the same values, so their cotangents add.
    std::vector<double> t(m_source.size(), 0.0), seed;
    for (const HydroCouple::DifferentialEntry &e : seeds)
    {
        if (e.item != self || (e.role != DifferentialRole::Output &&
                               e.role != DifferentialRole::StateAfter))
            return refuse(message, "seed names an item or role this adapter "
                                   "does not differentiate");
        if (!readDifferential(e, shape, seed, message))
            return false;
        for (size_t i = 0; i < t.size(); ++i)
            t[i] += seed[i];
    }

    for (const HydroCouple::DifferentialEntry &e : results)
    {
        if (e.item == itemOf(adaptee()) && e.role == DifferentialRole::Input)
        {
            std::vector<double> xbar(t.size());
            for (size_t i = 0; i < t.size(); ++i)
                xbar[i] = (m_blended ? m_lambda : 1.0) * t[i];
            if (!writeDifferential(e, shape, xbar, message))
                return false;
        }
        else if (e.item == self && e.role == DifferentialRole::StateBefore)
        {
            std::vector<double> ybar(t.size(), 0.0);
            if (m_blended)
                for (size_t i = 0; i < t.size(); ++i)
                    ybar[i] = (1.0 - m_lambda) * t[i];
            if (!writeDifferential(e, shape, ybar, message))
                return false;
        }
        else if (e.item == itemOf<HydroCouple::IComponentDataItem>(&m_relaxationFactor) &&
                 e.role == DifferentialRole::Argument)
        {
            double lbar = 0.0;
            if (m_blended)
                for (size_t i = 0; i < t.size(); ++i)
                    lbar += t[i] * (m_source[i] - m_previous[i]);
            if (!writeDifferential(e, scalar, {lbar}, message))
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

bool RelaxationAdaptedOutput::jvp(HydroCouple::DifferentialSet seeds,
                                  HydroCouple::DifferentialSet results,
                                  std::string *message)
{
    if (!m_haveRefresh)
        return refuse(message, "RelaxationAdaptedOutput \"" + id() +
                                   "\" has no refresh to differentiate");
    const std::vector<int64_t> shape{static_cast<int64_t>(m_source.size())};
    const std::vector<int64_t> scalar{1};
    const auto *self = itemOf<HydroCouple::IComponentDataItem>(this);

    std::vector<double> xdot(m_source.size(), 0.0), ydot(m_source.size(), 0.0),
        ldot{0.0};
    for (const HydroCouple::DifferentialEntry &e : seeds)
    {
        bool ok = false;
        if (e.item == itemOf(adaptee()) && e.role == DifferentialRole::Input)
            ok = readDifferential(e, shape, xdot, message);
        else if (e.item == self && e.role == DifferentialRole::StateBefore)
            ok = readDifferential(e, shape, ydot, message);
        else if (e.item == itemOf<HydroCouple::IComponentDataItem>(&m_relaxationFactor) &&
                 e.role == DifferentialRole::Argument)
            ok = readDifferential(e, scalar, ldot, message);
        else
            return refuse(message, "seed names an item or role this adapter "
                                   "does not differentiate");
        if (!ok)
            return false;
    }

    std::vector<double> out(m_source.size());
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = m_blended ? m_lambda * xdot[i] + (1.0 - m_lambda) * ydot[i] +
                                 ldot[0] * (m_source[i] - m_previous[i])
                           : xdot[i];

    for (const HydroCouple::DifferentialEntry &e : results)
    {
        if (e.item != self || (e.role != DifferentialRole::Output &&
                               e.role != DifferentialRole::StateAfter))
            return refuse(message, "result names an item or role this adapter "
                                   "does not differentiate");
        if (!writeDifferential(e, shape, out, message))
            return false;
    }
    return true;
}

// ── ICheckpointableAdaptedOutput ────────────────────────────────────────────

bool RelaxationAdaptedOutput::saveState(std::string &token, std::string &)
{
    token.assign(kTokenTag);
    token += ' ' + std::to_string(m_values.size());
    char hex[17];
    for (double v : m_values)
    {
        std::snprintf(hex, sizeof hex, "%016llx",
                      static_cast<unsigned long long>(std::bit_cast<std::uint64_t>(v)));
        token += ' ';
        token += hex;
    }
    return true;
}

bool RelaxationAdaptedOutput::restoreState(const std::string &token,
                                           std::string &message)
{
    auto refuse = [&](const std::string &why) {
        message = "RelaxationAdaptedOutput \"" + id() + "\": " + why;
        return false;
    };
    std::string_view rest(token);
    std::uint64_t count = 0;
    if (nextField(rest) != kTokenTag || !parseField(nextField(rest), count))
        return refuse("not a relaxation token");
    // An empty state (no history) restores anywhere; values must match the
    // adaptee they will be blended with.
    const std::vector<int64_t> extent = shape();
    if (count != 0 && (extent.size() != 1 || count != static_cast<std::uint64_t>(extent[0])))
        return refuse("the token holds " + std::to_string(count) +
                      " values but the adaptee has a different shape");
    std::vector<double> values;
    values.reserve(static_cast<size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i)
    {
        std::uint64_t bits = 0;
        if (!parseField(nextField(rest), bits, 16))
            return refuse("truncated or malformed token");
        values.push_back(std::bit_cast<double>(bits));
    }
    if (!nextField(rest).empty())
        return refuse("trailing data in token");
    m_values = std::move(values);
    // The restored state is a starting point, not a refresh: nothing to
    // differentiate until the next one.
    m_haveRefresh = false;
    m_source.clear();
    m_previous.clear();
    message.clear();
    return true;
}

bool RelaxationAdaptedOutput::releaseState(const std::string &, std::string &message)
{
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK
