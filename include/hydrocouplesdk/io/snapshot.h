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
    std::string           itemId;  //!< IIdentity::id() of the source item.
    DataKind              kind = DataKind::Unknown; //!< Element type.
    std::vector<int64_t>  shape;   //!< Extent per dimension (row-major).
    std::vector<std::byte> data;   //!< Owned contiguous value bytes.

    /*!
     * \brief Number of elements described by \c shape.
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
     * \tparam T Element type; must match \c kind.
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
     *                 treat it as an opaque monotone coordinate).
     */
    explicit Snapshot(double time = 0.0);

    Snapshot(Snapshot &&) noexcept            = default;
    Snapshot &operator=(Snapshot &&) noexcept = default;
    Snapshot(const Snapshot &)                = delete;
    Snapshot &operator=(const Snapshot &)     = delete;
    ~Snapshot()                               = default;

    /*!
     * \brief The snapshot's time coordinate.
     */
    [[nodiscard]] double time() const noexcept { return m_time; }

    /*!
     * \brief The captured field slices, in capture order.
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
     * \param[in]  item    Source data item (host-resident values).
     * \param[out] message Failure description when returning false.
     * \return true when the field was captured and appended.
     */
    bool capture(const HydroCouple::IComponentDataItem &item,
                 std::string &message);

    /*!
     * \brief Appends an already-materialized field slice (move).
     */
    void addField(FieldSlice field);
    //! Captures live layer interfaces once per shared vertical coordinate.
    //! Hosts recording layered items call this at the same instant as capture().
    bool captureGeometry(const HydroCouple::IComponentDataItem &item, std::string &message);

private:
    double                  m_time;
    std::vector<FieldSlice> m_fields;
};

//! Stable reserved field identity shared by the recorder and capturing hosts.
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string verticalFieldId(const HydroCouple::IComponentDataItem &item);
} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_SNAPSHOT_H
