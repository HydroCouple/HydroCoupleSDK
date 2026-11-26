// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   rastercomponentdataitem.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Component data item for GDAL raster data.
 *
 * Built only when the library is built with USE_GDAL (see raster.h).
 */

#ifndef HYDROCOUPLESDK_RASTERCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_RASTERCOMPONENTDATAITEM_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "raster.h"

#include <vector>
#include <memory>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

/*!
 * \brief Data item for GDAL raster band access.
 *
 * Provides component data item interface to GDAL raster band data.
 * Supports reading/writing entire bands or rectangular regions.
 * Lazy loading: data is read from disk on demand via getValue/getValues.
 *
 * Dimensions: [ySize, xSize] (row-major layout as GDAL uses)
 *
 * The item is a plain AbstractComponentDataItem over one band of a borrowed
 * Raster: it does not implement HydroCouple::Spatial::IRasterComponentDataItem
 * (which expects all bands, band first), so a consumer sees a rank-2 Float64
 * item and no raster geometry. Every getValuesInto() reads the requested
 * window from the file through GDAL; nothing is cached. Writes go back to the
 * band and therefore fail for a raster Raster::open() opened, which is read
 * only. Typical use is an argument or internal item that lets a component
 * read a gridded input (a digital elevation model, say) window by window.
 *
 * \threadsafety Not thread-safe, like the GDAL dataset it reads.
 */
class HYDROCOUPLESDK_EXPORT RasterComponentDataItem : public HydroCouple::SDK::AbstractComponentDataItem {
public:
    /*!
     * \brief Construct component data item for a raster band.
     * \param id              Unique identifier
     * \param dimensions      Dimension list (typically 2D for raster)
     * \param valueDefinition Value definition (borrowed)
     * \param modelComponent  Owning model component (may be nullptr)
     * \param raster          The Raster object (not owned)
     * \param bandIndex       1-based band index
     *
     * \p dimensions are borrowed and reported by dimensions(); pass two, rows
     * then columns (ids "rows" and "columns" get the Row and Column roles),
     * to match shape(). Nothing is read and \p bandIndex is not checked at
     * construction: a raster that is null, closed, or lacks the band makes
     * shape() report { 0, 0 }. \p raster must outlive the item.
     */
    RasterComponentDataItem(std::string_view id,
                           const std::vector<HydroCouple::SDK::Dimension *> &dimensions,
                           HydroCouple::SDK::ValueDefinition *valueDefinition,
                           HydroCouple::IModelComponent *modelComponent,
                           Raster *raster,
                           int bandIndex);

    //! Destroys the item; the raster and the dimensions are not owned.
    ~RasterComponentDataItem() override = default;

    // ── v2 typed hyperslab data plane ─────────────────────────────────────

    /*!
     * \brief The band's shape: { ySize, xSize } (row-major, as GDAL uses).
     * \returns { rows, columns } read from the band on every call; { 0, 0 }
     *          when no raster is attached, it is closed, or it lacks the band.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override;

    /*!
     * \brief Always DataKind::Float64 — band values are read as double.
     * \returns DataKind::Float64, whatever the band's storage type.
     */
    [[nodiscard]] DataKind dataKind() const override;

    /*!
     * \brief Reads the selected block from the band into \p destination.
     *
     * Validates the selection against shape(), reads the window through
     * RasterBand::readBlock() into a dense staging buffer, then places it
     * into the destination, which may be strided.
     *
     * \param[in]  destination Host buffer of kind Float64 shaped like
     *                         \p count (or any shape with as many elements).
     * \param[in]  start       { first row, first column }, zero-based.
     * \param[in]  count       { rows, columns } to read.
     * \param[out] message     On failure, receives "No raster attached",
     *                         "Raster band read failed", or the selection
     *                         error; may be nullptr.
     * \returns True when the window was read.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override;

    /*!
     * \brief Writes \p source into the selected block of the band.
     *
     * Validates the selection, gathers the source into a staging buffer and
     * writes it with RasterBand::writeBlock(); on success emits the
     * value-changed signal for the window.
     *
     * \param[in]  source  Host buffer of kind Float64.
     * \param[in]  start   { first row, first column }, zero-based.
     * \param[in]  count   { rows, columns } to write.
     * \param[out] message On failure, receives "No raster attached",
     *                     "Raster band write failed" (always, for a raster
     *                     opened read only by Raster::open()), or the
     *                     selection error; may be nullptr.
     * \returns True when the window was written.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── Raster-specific interface ──────────────────────────────────────────

    /*!
     * \brief Get the raster.
     * \returns The borrowed raster; may be nullptr.
     */
    Raster *raster() const { return m_raster; }

    /*!
     * \brief Get band index (1-based).
     * \returns The band index given at construction.
     */
    int bandIndex() const { return m_bandIndex; }

    /*!
     * \brief Get band dimensions (xSize, ySize).
     * \param[out] xSize Receives the band width in pixels; 0 without a band.
     * \param[out] ySize Receives the band height in lines; 0 without a band.
     */
    void getBandDimensions(int &xSize, int &ySize) const;

    /*!
     * \brief Read a rectangular region from the band.
     * \param xOff Column offset
     * \param yOff Row offset
     * \param xSize Width
     * \param ySize Height
     * \param buffer Output buffer (must be large enough for xSize*ySize values)
     * \returns RasterBand::readBlock()'s result (which resizes \p buffer to
     *          xSize·ySize); false when no raster is attached.
     */
    bool readBlock(int xOff, int yOff, int xSize, int ySize,
                   std::vector<double> &buffer) const;

    /*!
     * \brief Write a rectangular region to the band.
     *
     * Unlike setValuesFrom(), this does not emit the value-changed signal.
     *
     * \param[in] xOff   Zero-based column of the window's left edge.
     * \param[in] yOff   Zero-based row of the window's top edge.
     * \param[in] xSize  Window width in pixels.
     * \param[in] ySize  Window height in lines.
     * \param[in] buffer The values, row by row; at least xSize·ySize.
     * \returns RasterBand::writeBlock()'s result; false when no raster is
     *          attached.
     */
    bool writeBlock(int xOff, int yOff, int xSize, int ySize,
                    const std::vector<double> &buffer);

private:
    Raster *m_raster; //!< The borrowed raster; may be nullptr.
    int m_bandIndex;  //!< 1-based index of the band this item reads.
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_RASTERCOMPONENTDATAITEM_H
