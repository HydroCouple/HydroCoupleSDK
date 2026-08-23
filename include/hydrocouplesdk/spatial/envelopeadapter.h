// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   envelopeadapter.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  EnvelopeAdapter — exposes an SDK Envelope value through the
 *         standard's IEnvelope interface.
 *
 * The SDK's spatial primitives (Point, Envelope) are lightweight value
 * types; the standard's accessors return interface pointers. This adapter
 * bridges the two for data items that must serve
 * HydroCouple::Spatial::IEnvelope.
 *
 * \threadsafety Reads are safe after construction; mutations are not.
 */

#ifndef HYDROCOUPLESDK_SPATIAL_ENVELOPEADAPTER_H
#define HYDROCOUPLESDK_SPATIAL_ENVELOPEADAPTER_H

#include "hydrocouplesdk/spatial/envelope.h"
#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

/*!
 * \brief IEnvelope over an owned SDK Envelope value.
 */
class EnvelopeAdapter final : public HydroCouple::Spatial::IEnvelope
{
public:
    EnvelopeAdapter() = default;

    /*!
     * \brief Construct over a copy of \p envelope.
     */
    explicit EnvelopeAdapter(const Envelope &envelope) : m_envelope(envelope)
    {
    }

    ~EnvelopeAdapter() override = default;

    /*!
     * \brief Replaces the wrapped envelope value.
     */
    void setEnvelope(const Envelope &envelope) { m_envelope = envelope; }

    /*!
     * \brief The wrapped envelope value.
     */
    [[nodiscard]] const Envelope &envelope() const noexcept
    {
        return m_envelope;
    }

    // ── IEnvelope ─────────────────────────────────────────────────────────

    [[nodiscard]] double minX() const override { return m_envelope.minX; }
    [[nodiscard]] double maxX() const override { return m_envelope.maxX; }
    [[nodiscard]] double minY() const override { return m_envelope.minY; }
    [[nodiscard]] double maxY() const override { return m_envelope.maxY; }
    [[nodiscard]] double minZ() const override { return m_envelope.minZ; }
    [[nodiscard]] double maxZ() const override { return m_envelope.maxZ; }

private:
    Envelope m_envelope;
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_SPATIAL_ENVELOPEADAPTER_H
