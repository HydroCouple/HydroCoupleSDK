// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/spatial/raster.h"

#include "gdal_priv.h"

namespace HydroCouple::Spatial {

RasterBand::RasterBand(void *band, int bandIndex) noexcept
    : m_band(band), m_bandIndex(bandIndex) {
}

std::string RasterBand::dataTypeName() const {
    if (!m_band) return "";
    auto band = static_cast<GDALRasterBand *>(m_band);
    return GDALGetDataTypeName(band->GetRasterDataType());
}

int RasterBand::xSize() const {
    if (!m_band) return 0;
    auto band = static_cast<GDALRasterBand *>(m_band);
    return band->GetXSize();
}

int RasterBand::ySize() const {
    if (!m_band) return 0;
    auto band = static_cast<GDALRasterBand *>(m_band);
    return band->GetYSize();
}

bool RasterBand::getNoDataValue(double &nodata) const {
    if (!m_band) return false;
    auto band = static_cast<GDALRasterBand *>(m_band);
    int hasNoData = 0;
    nodata = band->GetNoDataValue(&hasNoData);
    return hasNoData != 0;
}

void RasterBand::getBlockSize(int &xBlk, int &yBlk) const {
    if (!m_band) {
        xBlk = yBlk = 0;
        return;
    }
    auto band = static_cast<GDALRasterBand *>(m_band);
    band->GetBlockSize(&xBlk, &yBlk);
}

double RasterBand::getValue(int x, int y) const {
    if (!m_band || x < 0 || y < 0 || x >= xSize() || y >= ySize()) {
        return 0.0;
    }

    auto band = static_cast<GDALRasterBand *>(m_band);
    int hasNoData = 0;
    double nodata = band->GetNoDataValue(&hasNoData);
    double value = 0.0;
    CPLErr err = band->RasterIO(GF_Read, x, y, 1, 1, &value, 1, 1,
                                GDT_Float64, 0, 0);
    if (err != CE_None) return nodata;
    return value;
}

void RasterBand::setValue(int x, int y, double value) {
    if (!m_band || x < 0 || y < 0 || x >= xSize() || y >= ySize()) {
        return;
    }

    auto band = static_cast<GDALRasterBand *>(m_band);
    band->RasterIO(GF_Write, x, y, 1, 1, &value, 1, 1, GDT_Float64, 0, 0);
}

bool RasterBand::readBlock(int xOff, int yOff, int xSize, int ySize,
                           std::vector<double> &buffer) const {
    if (!m_band) return false;

    auto band = static_cast<GDALRasterBand *>(m_band);
    buffer.resize(xSize * ySize);
    CPLErr err = band->RasterIO(GF_Read, xOff, yOff, xSize, ySize,
                                buffer.data(), xSize, ySize,
                                GDT_Float64, 0, 0);
    return err == CE_None;
}

bool RasterBand::writeBlock(int xOff, int yOff, int xSize, int ySize,
                            const std::vector<double> &buffer) {
    if (!m_band || (int)buffer.size() < xSize * ySize) return false;

    auto band = static_cast<GDALRasterBand *>(m_band);
    CPLErr err = band->RasterIO(GF_Write, xOff, yOff, xSize, ySize,
                                const_cast<double *>(buffer.data()),
                                xSize, ySize, GDT_Float64, 0, 0);
    return err == CE_None;
}

Raster::Raster() : Identity() {
}

Raster::~Raster() {
    close();
}

bool Raster::open(const std::string &filePath, std::string &errorMessage) {
    close();

    auto ds = reinterpret_cast<GDALDataset *>(
        GDALOpenEx(filePath.c_str(), GDAL_OF_READONLY | GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR,
                   nullptr, nullptr, nullptr));

    if (!ds) {
        errorMessage = "Failed to open raster file: " + filePath;
        return false;
    }

    m_dataset = ds;
    return true;
}

void Raster::close() {
    if (m_dataset) {
        GDALClose(m_dataset);
        m_dataset = nullptr;
    }
}

int Raster::xSize() const {
    if (!m_dataset) return 0;
    auto ds = reinterpret_cast<::GDALDataset *>(m_dataset);
    return ds->GetRasterXSize();
}

int Raster::ySize() const {
    if (!m_dataset) return 0;
    auto ds = reinterpret_cast<::GDALDataset *>(m_dataset);
    return ds->GetRasterYSize();
}

int Raster::bandCount() const {
    if (!m_dataset) return 0;
    auto ds = reinterpret_cast<::GDALDataset *>(m_dataset);
    return ds->GetRasterCount();
}

bool Raster::getGeoTransform(std::vector<double> &transform) const {
    if (!m_dataset) return false;
    auto ds = reinterpret_cast<::GDALDataset *>(m_dataset);
    transform.resize(6);
    CPLErr err = ds->GetGeoTransform(transform.data());
    return err == CE_None;
}

bool Raster::setGeoTransform(const std::vector<double> &transform) {
    if (!m_dataset || transform.size() < 6) return false;
    auto ds = reinterpret_cast<::GDALDataset *>(m_dataset);
    CPLErr err = ds->SetGeoTransform(const_cast<double *>(transform.data()));
    return err == CE_None;
}

std::string Raster::projectionRef() const {
    if (!m_dataset) return "";
    auto ds = reinterpret_cast<::GDALDataset *>(m_dataset);
    const char *proj = ds->GetProjectionRef();
    return proj ? std::string(proj) : "";
}

Envelope Raster::envelope() const {
    Envelope env;
    if (!m_dataset) return env;

    auto ds = reinterpret_cast<::GDALDataset *>(m_dataset);
    std::vector<double> geoTrans(6);
    if (ds->GetGeoTransform(geoTrans.data()) == CE_None) {
        int xs = xSize();
        int ys = ySize();

        double minX = geoTrans[0];
        double maxX = geoTrans[0] + xs * geoTrans[1];
        double maxY = geoTrans[3];
        double minY = geoTrans[3] + ys * geoTrans[5];

        if (minX > maxX) std::swap(minX, maxX);
        if (minY > maxY) std::swap(minY, maxY);

        env = Envelope(minX, maxX, minY, maxY);
    }

    return env;
}

RasterBand Raster::band(int bandIndex) {
    if (!m_dataset || bandIndex < 1 || bandIndex > bandCount()) {
        return RasterBand();
    }

    auto ds = reinterpret_cast<::GDALDataset *>(m_dataset);
    auto band = ds->GetRasterBand(bandIndex);
    return RasterBand(band, bandIndex);
}

RasterBand Raster::firstBand() {
    return band(1);
}

std::string Raster::driverName() const {
    if (!m_dataset) return "";
    auto ds = reinterpret_cast<::GDALDataset *>(m_dataset);
    auto driver = ds->GetDriver();
    return driver ? std::string(driver->GetDescription()) : "";
}

} // namespace HydroCouple::Spatial
