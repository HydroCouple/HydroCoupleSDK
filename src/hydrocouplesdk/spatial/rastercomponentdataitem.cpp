// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/spatial/rastercomponentdataitem.h"
#include "hydrocouplesdk/data/bufferio.h"

#include <array>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

RasterComponentDataItem::RasterComponentDataItem(std::string_view id,
                                                 const std::vector<Dimension *> &dimensions,
                                                 ValueDefinition *valueDefinition,
                                                 IModelComponent *modelComponent,
                                                 Raster *raster,
                                                 int bandIndex)
    : AbstractComponentDataItem(id, dimensions, valueDefinition, modelComponent),
      m_raster(raster),
      m_bandIndex(bandIndex) {
}

std::vector<int64_t> RasterComponentDataItem::shape() const {
    if (!m_raster) return {0, 0};
    auto band = m_raster->band(m_bandIndex);
    return {static_cast<int64_t>(band.ySize()),
            static_cast<int64_t>(band.xSize())};
}

DataKind RasterComponentDataItem::dataKind() const {
    return DataKind::Float64;
}

bool RasterComponentDataItem::getValuesInto(const BufferDescriptor &destination,
                                            std::span<const int64_t> start,
                                            std::span<const int64_t> count,
                                            std::string *message) const {
    if (!m_raster) {
        if (message) *message = "No raster attached";
        return false;
    }
    const auto itemShape = shape();
    if (std::string error = HydroCouple::SDK::detail::validateSelection(
            itemShape, destination, DataKind::Float64, start, count);
        !error.empty()) {
        if (message) *message = std::move(error);
        return false;
    }
    // Read the selected block into a dense staging buffer, then place it
    // into the (possibly strided) destination via the hyperslab engine.
    std::vector<double> staging(static_cast<size_t>(count[0] * count[1]));
    auto band = m_raster->band(m_bandIndex);
    if (!band.readBlock(static_cast<int>(start[1]), static_cast<int>(start[0]),
                        static_cast<int>(count[1]), static_cast<int>(count[0]),
                        staging)) {
        if (message) *message = "Raster band read failed";
        return false;
    }
    const std::array<int64_t, 2> zero{0, 0};
    return HydroCouple::SDK::detail::readHyperslab<double>(
        staging.data(), count, destination,
        std::span<const int64_t>(zero.data(), 2), count, message);
}

bool RasterComponentDataItem::setValuesFrom(const BufferDescriptor &source,
                                            std::span<const int64_t> start,
                                            std::span<const int64_t> count,
                                            std::string *message) {
    if (!m_raster) {
        if (message) *message = "No raster attached";
        return false;
    }
    const auto itemShape = shape();
    if (std::string error = HydroCouple::SDK::detail::validateSelection(
            itemShape, source, DataKind::Float64, start, count);
        !error.empty()) {
        if (message) *message = std::move(error);
        return false;
    }
    // Gather the (possibly strided) source into a dense staging buffer,
    // then write the block to the band.
    std::vector<double> staging(static_cast<size_t>(count[0] * count[1]));
    const std::array<int64_t, 2> zero{0, 0};
    if (!HydroCouple::SDK::detail::writeHyperslab<double>(
            staging.data(), count, source,
            std::span<const int64_t>(zero.data(), 2), count, message))
        return false;
    auto band = m_raster->band(m_bandIndex);
    if (!band.writeBlock(static_cast<int>(start[1]), static_cast<int>(start[0]),
                         static_cast<int>(count[1]), static_cast<int>(count[0]),
                         staging)) {
        if (message) *message = "Raster band write failed";
        return false;
    }
    emitValueChanged({start.begin(), start.end()}, {count.begin(), count.end()});
    return true;
}

void RasterComponentDataItem::getBandDimensions(int &xSize, int &ySize) const {
    if (!m_raster) {
        xSize = ySize = 0;
        return;
    }
    auto band = m_raster->band(m_bandIndex);
    xSize = band.xSize();
    ySize = band.ySize();
}

bool RasterComponentDataItem::readBlock(int xOff, int yOff, int xSize, int ySize,
                                        std::vector<double> &buffer) const {
    if (!m_raster) return false;
    auto band = m_raster->band(m_bandIndex);
    return band.readBlock(xOff, yOff, xSize, ySize, buffer);
}

bool RasterComponentDataItem::writeBlock(int xOff, int yOff, int xSize, int ySize,
                                         const std::vector<double> &buffer) {
    if (!m_raster) return false;
    auto band = m_raster->band(m_bandIndex);
    return band.writeBlock(xOff, yOff, xSize, ySize, buffer);
}

} // namespace HydroCouple::SDK::Spatial
