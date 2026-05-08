// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   polyhedralsurface.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Polyhedral surface (collection of polygons).
 */

#ifndef HYDROCOUPLESDK_POLYHEDRALSURFACE_H
#define HYDROCOUPLESDK_POLYHEDRALSURFACE_H

#include "hydrocouplesdk.h"
#include "polygon.h"
#include "envelope.h"

#include <vector>
#include <memory>
#include <unordered_map>

namespace HydroCouple::Spatial {

/*!
 * \brief Surface composed of polygons (patches/faces).
 *
 * A collection of Polygons sharing edges. Base class for TIN (triangulated
 * irregular network). Supports spatial indexing via bounding envelope.
 * Polygons are stored as shared pointers for flexible memory management.
 *
 * \threadsafety Not thread-safe.
 */
class HYDROCOUPLESDK_EXPORT PolyhedralSurface
{
public:
    /*!
     * \brief Construct empty surface.
     */
    PolyhedralSurface() = default;

    virtual ~PolyhedralSurface() = default;

    // ── Polygon management ─────────────────────────────────────────────────

    /*!
     * \brief Get number of polygons (patches/faces).
     */
    int numPatches() const noexcept { return (int)m_patches.size(); }

    /*!
     * \brief Add a polygon patch.
     */
    void addPatch(std::shared_ptr<Polygon> patch) {
        m_patches.push_back(patch);
        if (patch) m_envelope.expandToInclude(patch->envelope());
    }

    /*!
     * \brief Get patch by index.
     */
    std::shared_ptr<Polygon> patch(int i) const {
        if (i < 0 || i >= (int)m_patches.size()) return nullptr;
        return m_patches[i];
    }

    /*!
     * \brief Get all patches.
     */
    const std::vector<std::shared_ptr<Polygon>> &patches() const noexcept {
        return m_patches;
    }

    // ── Vertex management ──────────────────────────────────────────────────

    /*!
     * \brief Get number of unique vertices.
     */
    int numVertices() const noexcept { return (int)m_vertices.size(); }

    /*!
     * \brief Add a vertex.
     */
    int addVertex(const Point &p) {
        int id = numVertices();
        m_vertices.push_back(p);
        return id;
    }

    /*!
     * \brief Get vertex by index.
     */
    Point vertex(int i) const {
        if (i < 0 || i >= (int)m_vertices.size()) return Point();
        return m_vertices[i];
    }

    /*!
     * \brief Get all vertices.
     */
    const std::vector<Point> &vertices() const noexcept { return m_vertices; }

    // ── Geometry ───────────────────────────────────────────────────────────

    /*!
     * \brief Check if surface is empty.
     */
    bool isEmpty() const noexcept { return m_patches.empty(); }

    /*!
     * \brief Get bounding envelope.
     */
    Envelope envelope() const noexcept { return m_envelope; }

    /*!
     * \brief Calculate total surface area.
     */
    double area() const;

    /*!
     * \brief Find patch containing a point (brute force).
     */
    std::shared_ptr<Polygon> findPatch(const Point &p) const;

    /*!
     * \brief Compute centroid weighted by patch areas.
     */
    Point centroid() const;

protected:
    std::vector<std::shared_ptr<Polygon>> m_patches;
    std::vector<Point> m_vertices;
    Envelope m_envelope;

    void updateEnvelope() {
        m_envelope = Envelope();
        for (const auto &p : m_patches) {
            if (p) m_envelope.expandToInclude(p->envelope());
        }
    }
};

} // namespace HydroCouple::Spatial

#endif // HYDROCOUPLESDK_POLYHEDRALSURFACE_H
