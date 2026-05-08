// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   raster.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  GDAL-backed raster data wrapper.
 */

#ifndef HYDROCOUPLESDK_RASTER_H
#define HYDROCOUPLESDK_RASTER_H

#include "hydrocouplesdk.h"
#include "identity.h"
#include "envelope.h"

#include <vector>
#include <memory>
#include <string>

namespace HydroCouple::Spatial {

class RasterBand;

/*!
 * \brief A single band of raster data.
 *
 * Wraps a GDAL GDALRasterBand and provides simplified access
 * to band metadata and data values.
 */
class HYDROCOUPLESDK_EXPORT RasterBand {
    friend class Raster;

public:
    RasterBand() = default;
    explicit RasterBand(void *band, int bandIndex) noexcept;

    /*!
     * \brief Get band index (1-based).
     */
    int index() const { return m_bandIndex; }

    /*!
     * \brief Get data type name (e.g., "Byte", "Float32", "Float64").
     */
    std::string dataTypeName() const;

    /*!
     * \brief Get number of columns in this band.
     */
    int xSize() const;

    /*!
     * \brief Get number of rows in this band.
     */
    int ySize() const;

    /*!
     * \brief Get nodata value (returns true if nodata is set).
     */
    bool getNoDataValue(double &nodata) const;

    /*!
     * \brief Get block size (typical I/O chunk).
     */
    void getBlockSize(int &xBlk, int &yBlk) const;

    /*!
     * \brief Read a single value at (x, y).
     */
    double getValue(int x, int y) const;

    /*!
     * \brief Write a single value at (x, y).
     */
    void setValue(int x, int y, double value);

    /*!
     * \brief Read a block of data into buffer.
     * Buffer must be large enough for xSize*ySize elements.
     */
    bool readBlock(int xOff, int yOff, int xSize, int ySize, std::vector<double> &buffer) const;

    /*!
     * \brief Write a block of data from buffer.
     */
    bool writeBlock(int xOff, int yOff, int xSize, int ySize, const std::vector<double> &buffer);

private:
    void *m_band = nullptr;
    int m_bandIndex = -1;
};

/*!
 * \brief GDAL-backed raster data.
 *
 * A Raster wraps a GDAL dataset and provides access to raster bands,
 * geotransform, and coordinate/pixel conversion utilities.
 *
 * The raster is opened in read-only mode by default.
 *
 * \threadsafety Not thread-safe (GDAL is not thread-safe for concurrent access).
 */
class HYDROCOUPLESDK_EXPORT Raster : public Identity {
public:
    /*!
     * \brief Construct empty raster.
     */
    Raster();

    /*!
     * \brief Destruct and close dataset.
     */
    ~Raster() override;

    // ── File I/O ───────────────────────────────────────────────────────────

    /*!
     * \brief Open a raster file via GDAL.
     * \return true on success; fills errorMessage on failure.
     */
    bool open(const std::string &filePath, std::string &errorMessage);

    /*!
     * \brief Close the dataset if open.
     */
    void close();

    /*!
     * \brief Check if dataset is open.
     */
    bool isOpen() const noexcept { return m_dataset != nullptr; }

    // ── Raster geometry ───────────────────────────────────────────────────

    /*!
     * \brief Get number of columns (pixels).
     */
    int xSize() const;

    /*!
     * \brief Get number of rows (pixels).
     */
    int ySize() const;

    /*!
     * \brief Get number of bands.
     */
    int bandCount() const;

    /*!
     * \brief Get geotransform: [minX, pixelWidth, rotX, maxY, rotY, -pixelHeight].
     * Pixel (i,j) maps to world coordinate:
     *   x = minX + i * pixelWidth + j * rotX
     *   y = maxY + i * rotY + j * (-pixelHeight)
     */
    bool getGeoTransform(std::vector<double> &transform) const;

    /*!
     * \brief Set geotransform (6 coefficients).
     */
    bool setGeoTransform(const std::vector<double> &transform);

    /*!
     * \brief Get spatial reference system WKT (if available).
     */
    std::string projectionRef() const;

    /*!
     * \brief Get raster envelope (bounding box in world coordinates).
     */
    Envelope envelope() const;

    // ── Band access ────────────────────────────────────────────────────────

    /*!
     * \brief Get band by 1-based index.
     */
    RasterBand band(int bandIndex);

    /*!
     * \brief Get first band (convenience for single-band rasters).
     */
    RasterBand firstBand();

    // ── Statistics ─────────────────────────────────────────────────────────

    /*!
     * \brief Check if raster is empty (not open or no bands).
     */
    bool isEmpty() const noexcept { return !isOpen() || bandCount() == 0; }

    /*!
     * \brief Get driver name (e.g., "GTiff", "HDF5", "NetCDF").
     */
    std::string driverName() const;

private:
    void *m_dataset = nullptr;
};

} // namespace HydroCouple::Spatial

#endif // HYDROCOUPLESDK_RASTER_H
