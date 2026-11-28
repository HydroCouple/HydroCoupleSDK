// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   raster.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  GDAL-backed raster data wrapper.
 *
 * Raster and RasterBand give a component read access to a raster file
 * through GDAL. They are compiled into the library only when it is built with
 * the CMake option USE_GDAL (raster.cpp is not built otherwise, so a program
 * that calls them without it fails to link). Neither class implements the
 * standard's HydroCouple::Spatial::IRaster or IRasterBand, and the SDK has no
 * adapter to them; RasterComponentDataItem exposes one band's values on the
 * data plane instead.
 */

#ifndef HYDROCOUPLESDK_RASTER_H
#define HYDROCOUPLESDK_RASTER_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/core/identity.h"
#include "envelope.h"

#include <vector>
#include <memory>
#include <string>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

class RasterBand;

/*!
 * \brief A single band of raster data.
 *
 * Wraps a GDAL GDALRasterBand and provides simplified access
 * to band metadata and data values.
 *
 * A RasterBand is a lightweight handle obtained from Raster::band(): it holds
 * a pointer to the GDAL band, which belongs to the Raster's dataset, and is
 * valid only while that Raster stays open. A default-constructed band (or one
 * Raster::band() returns for a bad index) holds no band; every accessor then
 * returns 0, an empty string or false. Values are transferred as 64-bit
 * floating point whatever the band's storage type; GDAL converts. Pixel
 * coordinates are zero-based: x is the column (pixel) counted from the left
 * edge, y the row (line) counted from the top.
 *
 * \threadsafety Not thread-safe, like the GDAL dataset it reads.
 */
class HYDROCOUPLESDK_EXPORT RasterBand {
    friend class Raster;

public:
    //! Constructs a handle that holds no band.
    RasterBand() = default;

    /*!
     * \brief Constructs a handle over a GDAL band.
     * \param[in] band      The band, a GDALRasterBand pointer passed as
     *                      void * so this header does not include GDAL; not
     *                      owned. May be nullptr.
     * \param[in] bandIndex The band's 1-based index in its dataset.
     */
    explicit RasterBand(void *band, int bandIndex) noexcept;

    /*!
     * \brief Get band index (1-based).
     * \returns The index given at construction; -1 for a handle that holds no
     *          band.
     */
    int index() const { return m_bandIndex; }

    /*!
     * \brief Get data type name (e.g., "Byte", "Float32", "Float64").
     * \returns GDAL's name for the band's storage type; empty when the handle
     *          holds no band.
     */
    std::string dataTypeName() const;

    /*!
     * \brief Get number of columns in this band.
     * \returns The band width in pixels; 0 when the handle holds no band.
     */
    int xSize() const;

    /*!
     * \brief Get number of rows in this band.
     * \returns The band height in lines; 0 when the handle holds no band.
     */
    int ySize() const;

    /*!
     * \brief Get nodata value (returns true if nodata is set).
     * \param[out] nodata Receives the band's nodata value when the handle
     *                    holds a band (GDAL's default value when the band
     *                    declares none); unchanged otherwise.
     * \returns True when the band declares a nodata value; false when it
     *          does not or the handle holds no band.
     */
    bool getNoDataValue(double &nodata) const;

    /*!
     * \brief Get block size (typical I/O chunk).
     * \param[out] xBlk Receives the natural block width in pixels; 0 when the
     *                  handle holds no band.
     * \param[out] yBlk Receives the natural block height in lines; 0 when the
     *                  handle holds no band.
     */
    void getBlockSize(int &xBlk, int &yBlk) const;

    /*!
     * \brief Read a single value at (x, y).
     * \param[in] x Zero-based column.
     * \param[in] y Zero-based row.
     * \returns The pixel value as a double; 0 when the handle holds no band
     *          or (x, y) is outside the band; the band's nodata value (as
     *          getNoDataValue() reports it) when GDAL fails to read.
     */
    double getValue(int x, int y) const;

    /*!
     * \brief Write a single value at (x, y).
     *
     * Does nothing when the handle holds no band or (x, y) is outside the
     * band, and ignores GDAL's result. Raster::open() opens datasets read
     * only, so a write to a band obtained from it fails without notice.
     *
     * \param[in] x     Zero-based column.
     * \param[in] y     Zero-based row.
     * \param[in] value The value, converted by GDAL to the band's type.
     */
    void setValue(int x, int y, double value);

    /*!
     * \brief Read a block of data into buffer.
     * Buffer must be large enough for xSize*ySize elements.
     *
     * The buffer is resized to xSize·ySize before the read (so any size on
     * entry is accepted) and receives the window row by row, row 0 first.
     *
     * \param[in]  xOff   Zero-based column of the window's left edge.
     * \param[in]  yOff   Zero-based row of the window's top edge.
     * \param[in]  xSize  Window width in pixels.
     * \param[in]  ySize  Window height in lines.
     * \param[out] buffer Receives xSize·ySize values.
     * \returns True on success; false when the handle holds no band or GDAL
     *          refuses the read (a window outside the band, for example).
     */
    bool readBlock(int xOff, int yOff, int xSize, int ySize, std::vector<double> &buffer) const;

    /*!
     * \brief Write a block of data from buffer.
     * \param[in] xOff   Zero-based column of the window's left edge.
     * \param[in] yOff   Zero-based row of the window's top edge.
     * \param[in] xSize  Window width in pixels.
     * \param[in] ySize  Window height in lines.
     * \param[in] buffer The values, row by row; at least xSize·ySize of them.
     * \returns True on success; false when the handle holds no band, the
     *          buffer is too small, or GDAL refuses the write (always the
     *          case for a dataset Raster::open() opened, which is read only).
     */
    bool writeBlock(int xOff, int yOff, int xSize, int ySize, const std::vector<double> &buffer);

private:
    void *m_band = nullptr;  //!< The GDALRasterBand; owned by its dataset.
    int m_bandIndex = -1;    //!< 1-based band index; -1 when unset.
};

/*!
 * \brief GDAL-backed raster data.
 *
 * A Raster wraps a GDAL dataset and provides access to raster bands,
 * geotransform, and coordinate/pixel conversion utilities.
 *
 * The raster is opened in read-only mode by default.
 *
 * In fact open() always opens read only; the class has no way to open a
 * dataset for update or to create one. It does not register GDAL's drivers:
 * the program calls GDALAllRegister() (from gdal.h) once before the first
 * open(). The Raster owns its dataset and closes it in close() or on
 * destruction; it cannot be copied or moved (its Identity base is
 * non-copyable), so hold it by pointer when it must be shared. Coordinate
 * and pixel conversion is left to the caller, from getGeoTransform().
 *
 * \note envelope() passes its bounds to the Envelope constructor in the
 * wrong order; see envelope().
 *
 * \threadsafety Not thread-safe (GDAL is not thread-safe for concurrent access).
 */
class HYDROCOUPLESDK_EXPORT Raster : public Identity {
public:
    /*!
     * \brief Construct empty raster.
     *
     * No dataset is open; the id is a generated UUID.
     */
    Raster();

    /*!
     * \brief Destruct and close dataset.
     */
    ~Raster() override;

    // ── File I/O ───────────────────────────────────────────────────────────

    /*!
     * \brief Open a raster file via GDAL.
     *
     * Closes any dataset already open, then opens \p filePath read only as a
     * raster with GDAL's verbose errors. Every RasterBand obtained before the
     * call becomes invalid.
     *
     * \param[in]  filePath     Path or GDAL connection string of the raster.
     * \param[out] errorMessage Receives "Failed to open raster file: " and the
     *                          path on failure; unchanged on success.
     * \return true on success; fills errorMessage on failure.
     */
    bool open(const std::string &filePath, std::string &errorMessage);

    /*!
     * \brief Close the dataset if open.
     *
     * Invalidates every RasterBand obtained from this raster. Safe to call
     * when nothing is open.
     */
    void close();

    /*!
     * \brief Check if dataset is open.
     * \returns True between a successful open() and the next close().
     */
    bool isOpen() const noexcept { return m_dataset != nullptr; }

    // ── Raster geometry ───────────────────────────────────────────────────

    /*!
     * \brief Get number of columns (pixels).
     * \returns The raster width in pixels; 0 when nothing is open.
     */
    int xSize() const;

    /*!
     * \brief Get number of rows (pixels).
     * \returns The raster height in lines; 0 when nothing is open.
     */
    int ySize() const;

    /*!
     * \brief Get number of bands.
     * \returns The band count; 0 when nothing is open.
     */
    int bandCount() const;

    /*!
     * \brief Get geotransform: [minX, pixelWidth, rotX, maxY, rotY, -pixelHeight].
     * Pixel (i,j) maps to world coordinate:
     *   x = minX + i * pixelWidth + j * rotX
     *   y = maxY + i * rotY + j * (-pixelHeight)
     *
     * Here i is the column and j the row of the pixel corner, so the first
     * and fourth coefficients are the upper-left corner of the upper-left
     * pixel; this is GDAL's affine geotransform.
     *
     * \param[out] transform Resized to six elements and filled with GDAL's
     *                       coefficients (GDAL's identity transform when the
     *                       dataset has none).
     * \returns True when the dataset has a geotransform; false when it does
     *          not or nothing is open (then \p transform is left unchanged).
     */
    bool getGeoTransform(std::vector<double> &transform) const;

    /*!
     * \brief Set geotransform (6 coefficients).
     * \param[in] transform The six coefficients in getGeoTransform() order;
     *                      extra elements are ignored.
     * \returns True when GDAL accepts the transform; false when nothing is
     *          open, \p transform has fewer than six elements, or the driver
     *          refuses (a dataset opened read only may).
     */
    bool setGeoTransform(const std::vector<double> &transform);

    /*!
     * \brief Get spatial reference system WKT (if available).
     * \returns The dataset's coordinate reference system as WKT; empty when
     *          it declares none or nothing is open.
     */
    std::string projectionRef() const;

    /*!
     * \brief Get raster envelope (bounding box in world coordinates).
     *
     * Computes the extent from the geotransform's origin and pixel sizes
     * (rotation terms are ignored) and orders each pair, but then passes the
     * values to the two-dimensional Envelope constructor as
     * (minX, maxX, minY, maxY) where it expects (minX, minY, maxX, maxY): the
     * returned envelope's minY holds the maximum x and its maxX the minimum
     * y. Swap those two fields, or compute the extent from
     * getGeoTransform(), until this is fixed.
     *
     * \returns The envelope as described, z bounds 0; an empty Envelope when
     *          nothing is open or the dataset has no geotransform.
     */
    Envelope envelope() const;

    // ── Band access ────────────────────────────────────────────────────────

    /*!
     * \brief Get band by 1-based index.
     * \param[in] bandIndex Band number, from 1 to bandCount().
     * \returns A handle to the band, valid until close(), open() or
     *          destruction; a handle that holds no band when nothing is open
     *          or \p bandIndex is out of range.
     */
    RasterBand band(int bandIndex);

    /*!
     * \brief Get first band (convenience for single-band rasters).
     * \returns band(1).
     */
    RasterBand firstBand();

    // ── Statistics ─────────────────────────────────────────────────────────

    /*!
     * \brief Check if raster is empty (not open or no bands).
     * \returns True when nothing is open or the dataset has no bands.
     */
    bool isEmpty() const noexcept { return !isOpen() || bandCount() == 0; }

    /*!
     * \brief Get driver name (e.g., "GTiff", "HDF5", "NetCDF").
     * \returns The GDAL driver's short name; empty when nothing is open.
     */
    std::string driverName() const;

private:
    void *m_dataset = nullptr; //!< The GDALDataset, owned; nullptr when closed.
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_RASTER_H
