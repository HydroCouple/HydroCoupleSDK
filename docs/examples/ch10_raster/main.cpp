// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 10: reading a raster through GDAL with Raster, RasterBand
 *         and RasterComponentDataItem.
 *
 * Writes elevation.tif (4 x 3 pixels, 10 m x 5 m) in the working directory
 * with GDAL, then reads it back through the SDK. Needs an SDK built with
 * USE_GDAL (see REQUIRES). Exits nonzero when any value is not the one the
 * guide states.
 */

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/spatial/raster.h"
#include "hydrocouplesdk/spatial/rastercomponentdataitem.h"

#include "gdal_priv.h"

#include <array>
#include <cstdio>
#include <string>
#include <vector>

using namespace HydroCouple::SDK::Spatial;

namespace {

int failures = 0;

void check(bool ok, const char *what)
{
    if (!ok)
    {
        std::printf("FAILED: %s\n", what);
        ++failures;
    }
}

//! Writes a Float32 GeoTIFF whose pixel (x, y) holds 100 + 4y + x.
bool writeElevation(const char *path)
{
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!driver)
        return false;
    GDALDataset *dataset = driver->Create(path, 4, 3, 1, GDT_Float32, nullptr);
    if (!dataset)
        return false;
    double transform[6] = {512000.0, 10.0, 0.0, 4512015.0, 0.0, -5.0};
    dataset->SetGeoTransform(transform);
    std::array<float, 12> values{};
    for (int i = 0; i < 12; ++i)
        values[static_cast<size_t>(i)] = 100.0f + static_cast<float>(i);
    const CPLErr written = dataset->GetRasterBand(1)->RasterIO(
        GF_Write, 0, 0, 4, 3, values.data(), 4, 3, GDT_Float32, 0, 0);
    dataset->GetRasterBand(1)->SetNoDataValue(-9999.0);
    GDALClose(dataset);
    return written == CE_None;
}

} // namespace

int main()
{
    //! [open]
    GDALAllRegister(); // the SDK does not register GDAL's drivers itself
    if (!writeElevation("elevation.tif"))
    {
        std::printf("FAILED: could not write elevation.tif\n");
        return 1;
    }

    Raster dem;
    std::string message;
    if (!dem.open("elevation.tif", message))
    {
        std::printf("FAILED: %s\n", message.c_str());
        return 1;
    }
    std::vector<double> transform;
    dem.getGeoTransform(transform);
    RasterBand band = dem.firstBand();
    double noData = 0.0;
    const bool hasNoData = band.getNoDataValue(noData);
    //! [open]

    std::printf("raster: driver %s, %d x %d pixels, %d band(s); origin (%.0f, %.0f), "
                "pixel %.0f x %.0f m\n",
                dem.driverName().c_str(), dem.xSize(), dem.ySize(), dem.bandCount(),
                transform[0], transform[3], transform[1], -transform[5]);
    std::printf("band 1: type %s, nodata %d (%.0f), pixel (2, 1) = %.0f, outside = %.0f\n",
                band.dataTypeName().c_str(), hasNoData, noData, band.getValue(2, 1),
                band.getValue(9, 9));
    check(dem.xSize() == 4 && dem.ySize() == 3 && dem.bandCount() == 1, "raster size");
    check(band.getValue(2, 1) == 106.0 && band.getValue(9, 9) == 0.0, "pixel reads");
    check(hasNoData && noData == -9999.0, "nodata value");

    //! [item]
    HydroCouple::SDK::Dimension rows("rows"), columns("columns");
    RasterComponentDataItem elevation("elevation", {&rows, &columns}, nullptr, nullptr, &dem, 1);

    // A 2 x 2 window: rows 1..2, columns 1..2.
    std::array<double, 4> window{};
    const std::array<int64_t, 2> start{1, 1}, count{2, 2};
    const bool read = elevation.getValuesInto(
        HydroCouple::SDK::detail::hostDescriptor(window.data(), HydroCouple::DataKind::Float64,
                                                 count),
        start, count, &message);

    // Raster::open() opens read only, so writing back is refused.
    std::string refused;
    const bool written = elevation.setValuesFrom(
        HydroCouple::SDK::detail::hostDescriptor(window.data(), HydroCouple::DataKind::Float64,
                                                 count),
        start, count, &refused);
    //! [item]

    std::printf("item: shape {%lld, %lld}, window %.0f %.0f / %.0f %.0f, write %d (%s)\n",
                static_cast<long long>(elevation.shape()[0]),
                static_cast<long long>(elevation.shape()[1]), window[0], window[1], window[2],
                window[3], written, refused.c_str());
    check(read, message.c_str());
    check(elevation.shape() == std::vector<int64_t>{3, 4}, "shape is { rows, columns }");
    check(window[0] == 105.0 && window[3] == 110.0, "window values");
    check(!written && refused == "Raster band write failed", "read-only dataset");

    std::printf("%s\n", failures == 0 ? "all checks passed" : "checks FAILED");
    return failures == 0 ? 0 : 1;
}
