// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   polyhedralsurface.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  PolyhedralSurface, a surface made of polygon patches.
 *
 * PolyhedralSurface is the SDK's mesh-as-geometry value type and the base of
 * TIN. It does not implement HydroCouple::Spatial::IPolyhedralSurface;
 * PolyhedralSurfaceAdapter (meshadapters.h) serves one through that interface,
 * and polyhedralSurfaceFromMesh() builds one from a MeshDefinition.
 */

#ifndef HYDROCOUPLESDK_POLYHEDRALSURFACE_H
#define HYDROCOUPLESDK_POLYHEDRALSURFACE_H

#include "hydrocouplesdk/export.h"
#include "polygon.h"
#include "envelope.h"

#include <vector>
#include <memory>
#include <unordered_map>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

/*!
 * \brief Surface composed of polygons (patches/faces).
 *
 * A collection of Polygons sharing edges. Base class for TIN (triangulated
 * irregular network). Polygons are stored as shared pointers for flexible
 * memory management.
 *
 * The surface holds two independent lists: the patches, each a Polygon with
 * its own copy of its corner coordinates, and a vertex list. Adding a patch
 * does not add vertices and adding a vertex does not create a patch; nothing
 * records which vertices a patch uses. That is why
 * PolyhedralSurfaceAdapter takes the MeshDefinition a surface was built from
 * when the caller has it: the connectivity cannot be recovered from the
 * surface alone. The surface keeps an envelope of its patches, updated as
 * patches are added; there is no spatial index, and findPatch() is a linear
 * search.
 *
 * Patches are shared with the caller: addPatch() stores the pointer, not a
 * copy, so a patch changed through another reference changes here too (and
 * the stored envelope does not follow it).
 *
 * \threadsafety Not thread-safe: concurrent reads are safe, but adding
 * patches or vertices must not overlap other access to the same surface.
 */
class HYDROCOUPLESDK_EXPORT PolyhedralSurface
{
public:
    /*!
     * \brief Construct empty surface.
     */
    PolyhedralSurface() = default;

    //! Destroys the surface and releases its references to the patches.
    virtual ~PolyhedralSurface() = default;

    // ── Polygon management ─────────────────────────────────────────────────

    /*!
     * \brief Get number of polygons (patches/faces).
     * \returns The number of patches added, null ones included.
     */
    int numPatches() const noexcept { return (int)m_patches.size(); }

    /*!
     * \brief Add a polygon patch.
     *
     * Appends the patch and grows the envelope by its envelope. A null
     * pointer is stored as a patch but leaves the envelope unchanged.
     *
     * \param[in] patch The patch; shared with the caller, not copied.
     */
    void addPatch(std::shared_ptr<Polygon> patch) {
        m_patches.push_back(patch);
        if (patch) m_envelope.expandToInclude(patch->envelope());
    }

    /*!
     * \brief Get patch by index.
     * \param[in] i Zero-based patch index, from 0 to numPatches() - 1.
     * \returns The patch (shared); nullptr when \p i is out of range or the
     *          patch stored there is null.
     */
    std::shared_ptr<Polygon> patch(int i) const {
        if (i < 0 || i >= (int)m_patches.size()) return nullptr;
        return m_patches[i];
    }

    /*!
     * \brief Get all patches.
     * \returns The patches in the order they were added; valid until the
     *          surface is modified or destroyed.
     */
    const std::vector<std::shared_ptr<Polygon>> &patches() const noexcept {
        return m_patches;
    }

    // ── Vertex management ──────────────────────────────────────────────────

    /*!
     * \brief Returns the number of vertices in the vertex list.
     *
     * The count is of addVertex() calls; vertices are not deduplicated, so
     * "unique" holds only if the caller adds each position once.
     *
     * \returns The length of the vertex list.
     */
    int numVertices() const noexcept { return (int)m_vertices.size(); }

    /*!
     * \brief Add a vertex.
     * \param[in] p The vertex position; copied. No duplicate check is made.
     * \returns The new vertex's zero-based index, which is the vertex count
     *          before the call.
     */
    int addVertex(const Point &p) {
        int id = numVertices();
        m_vertices.push_back(p);
        return id;
    }

    /*!
     * \brief Get vertex by index.
     * \param[in] i Zero-based vertex index, from 0 to numVertices() - 1.
     * \returns A copy of the vertex; the origin Point() when \p i is out of
     *          range.
     */
    Point vertex(int i) const {
        if (i < 0 || i >= (int)m_vertices.size()) return Point();
        return m_vertices[i];
    }

    /*!
     * \brief Get all vertices.
     * \returns The vertex list in index order; valid until the surface is
     *          modified or destroyed.
     */
    const std::vector<Point> &vertices() const noexcept { return m_vertices; }

    // ── Geometry ───────────────────────────────────────────────────────────

    /*!
     * \brief Check if surface is empty.
     * \returns True when the surface has no patches (vertices are not
     *          counted).
     */
    bool isEmpty() const noexcept { return m_patches.empty(); }

    /*!
     * \brief Get bounding envelope.
     * \returns The envelope of the patches' exterior rings, z included; an
     *          empty Envelope when no non-null patch has been added. The
     *          vertex list does not contribute.
     */
    Envelope envelope() const noexcept { return m_envelope; }

    /*!
     * \brief Calculate total surface area.
     * \returns The sum of Polygon::area() over the non-null patches: a plan
     *          area in squared coordinate units, not the area of tilted
     *          patches in three dimensions.
     */
    double area() const;

    /*!
     * \brief Find patch containing a point (brute force).
     * \param[in] p The point; tested in plan with Polygon::containsPoint().
     * \returns The first patch, in insertion order, that contains \p p;
     *          nullptr when none does.
     */
    std::shared_ptr<Polygon> findPatch(const Point &p) const;

    /*!
     * \brief Compute centroid weighted by patch areas.
     *
     * \f$ c = \sum_k A_k\,c_k \,/\, \sum_k A_k \f$ over the non-null patches
     * of nonzero area, with \f$c_k\f$ from Polygon::centroid(). It inherits
     * that function's error: the result is half the true centroid (see
     * Polygon::centroid()).
     *
     * \returns The weighted centroid in plan; the origin for an empty
     *          surface or one whose patches all have zero area.
     */
    Point centroid() const;

protected:
    //! The patches, in insertion order; may contain null pointers.
    std::vector<std::shared_ptr<Polygon>> m_patches;
    //! The vertex list, in index order; independent of the patches.
    std::vector<Point> m_vertices;
    //! Envelope of the non-null patches, grown by addPatch().
    Envelope m_envelope;

    /*!
     * \brief Recomputes m_envelope from the current patches.
     *
     * For a subclass that replaces or edits patches after adding them, which
     * addPatch() alone would leave reflected in a stale envelope.
     */
    void updateEnvelope() {
        m_envelope = Envelope();
        for (const auto &p : m_patches) {
            if (p) m_envelope.expandToInclude(p->envelope());
        }
    }
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_POLYHEDRALSURFACE_H
