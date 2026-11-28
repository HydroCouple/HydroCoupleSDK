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
 *
 * Holds a copy of the envelope, so the adapter does not depend on the
 * lifetime of the value it was built from. GeometryAdapter and
 * TimeGeometryComponentDataItem embed one and return its address from their
 * envelope() accessors; that pointer is valid for the life of its owner, and
 * setEnvelope() changes what it reports in place. The class is header-only
 * and not exported.
 *
 * \threadsafety Concurrent reads are safe; setEnvelope() must not overlap
 * them.
 */
class EnvelopeAdapter final : public HydroCouple::Spatial::IEnvelope
{
public:
    //! Constructs an adapter over an empty Envelope (isValid() false).
    EnvelopeAdapter() = default;

    /*!
     * \brief Construct over a copy of \p envelope.
     * \param[in] envelope The bounds to report; copied.
     */
    explicit EnvelopeAdapter(const Envelope &envelope) : m_envelope(envelope)
    {
    }

    //! Destroys the adapter.
    ~EnvelopeAdapter() override = default;

    /*!
     * \brief Replaces the wrapped envelope value.
     * \param[in] envelope The new bounds; copied.
     */
    void setEnvelope(const Envelope &envelope) { m_envelope = envelope; }

    /*!
     * \brief The wrapped envelope value.
     * \returns The stored Envelope; valid for the life of the adapter.
     */
    [[nodiscard]] const Envelope &envelope() const noexcept
    {
        return m_envelope;
    }

    // ── IEnvelope ─────────────────────────────────────────────────────────

    /*!
     * \brief The smallest x (IEnvelope::minX()).
     * \returns Envelope::minX of the stored value.
     */
    [[nodiscard]] double minX() const override { return m_envelope.minX; }
    /*!
     * \brief The largest x (IEnvelope::maxX()).
     * \returns Envelope::maxX of the stored value.
     */
    [[nodiscard]] double maxX() const override { return m_envelope.maxX; }
    /*!
     * \brief The smallest y (IEnvelope::minY()).
     * \returns Envelope::minY of the stored value.
     */
    [[nodiscard]] double minY() const override { return m_envelope.minY; }
    /*!
     * \brief The largest y (IEnvelope::maxY()).
     * \returns Envelope::maxY of the stored value.
     */
    [[nodiscard]] double maxY() const override { return m_envelope.maxY; }
    /*!
     * \brief The smallest z (IEnvelope::minZ()).
     * \returns Envelope::minZ of the stored value; 0 for a two-dimensional
     *          envelope.
     */
    [[nodiscard]] double minZ() const override { return m_envelope.minZ; }
    /*!
     * \brief The largest z (IEnvelope::maxZ()).
     * \returns Envelope::maxZ of the stored value; 0 for a two-dimensional
     *          envelope.
     */
    [[nodiscard]] double maxZ() const override { return m_envelope.maxZ; }

private:
    Envelope m_envelope; //!< The bounds reported, owned.
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_SPATIAL_ENVELOPEADAPTER_H
