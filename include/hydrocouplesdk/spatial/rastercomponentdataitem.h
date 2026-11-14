// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   rastercomponentdataitem.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Component data item for GDAL raster data.
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
     */
    RasterComponentDataItem(std::string_view id,
                           const std::vector<HydroCouple::SDK::Dimension *> &dimensions,
                           HydroCouple::SDK::ValueDefinition *valueDefinition,
                           HydroCouple::IModelComponent *modelComponent,
                           Raster *raster,
                           int bandIndex);

    ~RasterComponentDataItem() override = default;

    // ── v2 typed hyperslab data plane ─────────────────────────────────────

    /*!
     * \brief The band's shape: { ySize, xSize } (row-major, as GDAL uses).
     */
    [[nodiscard]] std::vector<int64_t> shape() const override;

    /*!
     * \brief Always DataKind::Float64 — band values are read as double.
     */
    [[nodiscard]] DataKind dataKind() const override;

    /*!
     * \brief Reads the selected block from the band into \p destination.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override;

    /*!
     * \brief Writes \p source into the selected block of the band.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── Raster-specific interface ──────────────────────────────────────────

    /*!
     * \brief Get the raster.
     */
    Raster *raster() const { return m_raster; }

    /*!
     * \brief Get band index (1-based).
     */
    int bandIndex() const { return m_bandIndex; }

    /*!
     * \brief Get band dimensions (xSize, ySize).
     */
    void getBandDimensions(int &xSize, int &ySize) const;

    /*!
     * \brief Read a rectangular region from the band.
     * \param xOff Column offset
     * \param yOff Row offset
     * \param xSize Width
     * \param ySize Height
     * \param buffer Output buffer (must be large enough for xSize*ySize values)
     */
    bool readBlock(int xOff, int yOff, int xSize, int ySize,
                   std::vector<double> &buffer) const;

    /*!
     * \brief Write a rectangular region to the band.
     */
    bool writeBlock(int xOff, int yOff, int xSize, int ySize,
                    const std::vector<double> &buffer);

private:
    Raster *m_raster;
    int m_bandIndex;
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_RASTERCOMPONENTDATAITEM_H
