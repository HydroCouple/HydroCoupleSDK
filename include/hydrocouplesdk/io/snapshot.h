// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   snapshot.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Snapshot — owned value copies of data-item fields at one time
 *         level, the unit of work handed to the IOThread.
 *
 * A Snapshot deep-copies field values through the v2 typed hyperslab data
 * plane (IComponentDataItem::getValuesInto over a contiguous host
 * BufferDescriptor), so the simulation may mutate its stores immediately
 * after capture. Snapshots are move-only; the deep copy at capture is the
 * only copy on the write path.
 *
 * Typical use, once per recorded time level on the simulation thread:
 * construct a Snapshot with the time, capture() each recorded item, and
 * hand it to IOThread::enqueue() by move.
 */

#ifndef HYDROCOUPLESDK_IO_SNAPSHOT_H
#define HYDROCOUPLESDK_IO_SNAPSHOT_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"

#include <cstddef>
#include <string>
#include <vector>

namespace HydroCouple::SDK::IO {

/*!
 * \brief One captured field: identity, type, shape, and an owned
 *        contiguous row-major copy of the values.
 *
 * \threadsafety Immutable after capture; safe to read from the writer
 * thread while the simulation thread proceeds.
 */
struct HYDROCOUPLESDK_EXPORT FieldSlice
{
    std::string           itemId;  //!< IIdentity::id() of the source item; writers match fields by it.
    DataKind              kind = DataKind::Unknown; //!< Element type (a fixed-width kind).
    std::vector<int64_t>  shape;   //!< Extent per dimension (row-major), without a time dimension.
    std::vector<std::byte> data;   //!< Owned contiguous value bytes: elementCount() times the kind's size.

    /*!
     * \brief Number of elements described by \c shape.
     *
     * \return The product of the extents; 1 for an empty shape (a scalar).
     */
    [[nodiscard]] int64_t elementCount() const noexcept
    {
        int64_t n = 1;
        for (int64_t extent : shape)
            n *= extent;
        return n;
    }

    /*!
     * \brief Typed read-only view of the owned bytes.
     *
     * No check is made that \p T matches \c kind; a mismatch reads the
     * bytes as the wrong type.
     *
     * \tparam T Element type; must match \c kind.
     * \return Pointer to elementCount() values, valid while \c data is
     *         unchanged.
     */
    template <typename T>
    [[nodiscard]] const T *values() const noexcept
    {
        return reinterpret_cast<const T *>(data.data());
    }
};

/*!
 * \brief Move-only bundle of field slices captured at one time level.
 *
 * Reuse pattern: writers receive const references; the IOThread owns the
 * instance until delivery completes.
 *
 * \threadsafety Immutable after capture (see FieldSlice).
 */
class HYDROCOUPLESDK_EXPORT Snapshot
{
public:
    /*!
     * \brief Construct an empty snapshot stamped with a time value.
     * \param[in] time Time level (Julian day by SDK convention; writers
     *                 treat it as an opaque monotone coordinate). The CSV,
     *                 GeoPackage, HDF5 and NetCDF writers store it as given;
     *                 the HDF5 and NetCDF files label it "days since
     *                 -4713-01-01" with the julian calendar.
     */
    explicit Snapshot(double time = 0.0);

    //! Moves the time and the field slices; the source is left empty.
    Snapshot(Snapshot &&) noexcept            = default;
    //! Move assignment. \return This snapshot.
    Snapshot &operator=(Snapshot &&) noexcept = default;
    //! Deleted: a snapshot owns potentially large buffers and is moved, never copied.
    Snapshot(const Snapshot &)                = delete;
    //! Deleted: see the deleted copy constructor.
    Snapshot &operator=(const Snapshot &)     = delete;
    //! Releases the owned field buffers.
    ~Snapshot()                               = default;

    /*!
     * \brief The snapshot's time coordinate.
     *
     * \return The value given to the constructor.
     */
    [[nodiscard]] double time() const noexcept { return m_time; }

    /*!
     * \brief The captured field slices, in capture order.
     *
     * \return The slices added by capture(), captureGeometry() and
     *         addField().
     */
    [[nodiscard]] const std::vector<FieldSlice> &fields() const noexcept
    {
        return m_fields;
    }

    /*!
     * \brief Deep-copies a data item's full value block into this snapshot.
     *
     * Reads the item through getValuesInto() over a contiguous host
     * BufferDescriptor covering the item's entire shape. Only fixed-width
     * numeric/boolean DataKinds are supported (String and Opaque items
     * cannot be captured as raw bytes).
     *
     * \param[in]  item    Source data item (host-resident values). Its
     *                     id() becomes the slice's itemId; capturing two
     *                     items with the same id in one snapshot appends
     *                     both; the UGRID writers then use the first, the
     *                     CSV and GeoPackage writers write both.
     * \param[out] message Failure description when returning false
     *                     ("Snapshot::capture: item '\<id\>' has a
     *                     non-fixed-width DataKind ..." or the item's own
     *                     read error); cleared on success.
     * \return true when the field was captured and appended.
     */
    bool capture(const HydroCouple::IComponentDataItem &item,
                 std::string &message);

    /*!
     * \brief Appends an already-materialized field slice (move).
     *
     * No validation: the caller guarantees that \c data holds
     * elementCount() values of \c kind.
     *
     * \param[in] field The slice to append.
     */
    void addField(FieldSlice field);
    /*!
     * \brief Captures live layer interfaces once per shared vertical
     *        coordinate.
     *
     * Hosts recording layered items call this at the same instant as
     * capture(). For an item that implements Spatial::ILayering, appends a
     * Float64 slice of shape { columns, layers + 1 } holding the vertical
     * coordinate's interface elevations, under the reserved id
     * verticalFieldId(). Items sharing one vertical coordinate share one
     * slice: a second call finds it and appends nothing.
     * RunRecorder::finishToFile() attaches that slice to each layered
     * item's catalog entry and removes it from the result list.
     *
     * \param[in]  item    The item being recorded.
     * \param[out] message Failure description when returning false
     *                     ("layered item has no prepared vertical
     *                     coordinate", "shared recorded vertical coordinate
     *                     changed shape", "vertical interface shape
     *                     mismatch", "nonfinite recorded interface
     *                     elevation"); cleared on success.
     * \return True when the item is not layered or its interfaces were
     *         captured (or already were).
     */
    bool captureGeometry(const HydroCouple::IComponentDataItem &item, std::string &message);

private:
    double                  m_time;   //!< Time coordinate given at construction.
    std::vector<FieldSlice> m_fields; //!< Owned slices in capture order.
};

/*!
 * \brief Stable reserved field identity shared by the recorder and
 *        capturing hosts.
 *
 * The id is "__hc_interfaces_" followed by the lower-case hexadecimal
 * bytes of "\<component id\>/\<output id\>", where the output is the first
 * output of the item's component that shares the item's vertical
 * coordinate (the item itself when it has no component).
 *
 * \param[in] item A recorded data item.
 * \return The reserved id, or an empty string when \p item is not
 *         layered or has no vertical coordinate.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string verticalFieldId(const HydroCouple::IComponentDataItem &item);
} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_SNAPSHOT_H
