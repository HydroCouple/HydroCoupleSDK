// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/rastercomponentdataitem.h"

namespace HydroCouple::Spatial {

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

int RasterComponentDataItem::dimensionLength(const std::initializer_list<int> &dimensionIndexes) const {
    if (!m_raster) return 0;

    auto band = m_raster->band(m_bandIndex);

    if (dimensionIndexes.size() == 0) return 1;

    int dim = *dimensionIndexes.begin();
    if (dim == 0) return band.ySize();
    if (dim == 1) return band.xSize();
    return 0;
}

void RasterComponentDataItem::getValue(hydrocouple_variant &data,
                                       const std::initializer_list<int> &dimensionIndexes) const {
    if (!m_raster || dimensionIndexes.size() != 2) return;

    auto it = dimensionIndexes.begin();
    int y = *it;
    ++it;
    int x = *it;

    auto band = m_raster->band(m_bandIndex);
    double value = band.getValue(x, y);
    data = hydrocouple_variant(value);
}

void RasterComponentDataItem::getValues(hydrocouple_variant *data,
                                        const std::initializer_list<int> &dimensionIndexes,
                                        const std::initializer_list<int> &dimensionLengths) const {
    if (!m_raster || dimensionIndexes.size() < 2 || !data) return;

    auto it = dimensionIndexes.begin();
    int yOff = *it;
    ++it;
    int xOff = *it;

    auto band = m_raster->band(m_bandIndex);

    int ySize = band.ySize();
    int xSize = band.xSize();

    if (dimensionLengths.size() > 0) {
        auto lit = dimensionLengths.begin();
        ySize = *lit;
        ++lit;
        if (lit != dimensionLengths.end()) {
            xSize = *lit;
        }
    }

    std::vector<double> buffer;
    if (!band.readBlock(xOff, yOff, xSize, ySize, buffer)) {
        return;
    }

    for (int i = 0; i < (int)buffer.size(); ++i) {
        data[i] = hydrocouple_variant(buffer[i]);
    }
}

void RasterComponentDataItem::setValue(const hydrocouple_variant &data,
                                       const std::initializer_list<int> &dimensionIndexes) {
    if (!m_raster || dimensionIndexes.size() != 2) return;

    auto it = dimensionIndexes.begin();
    int y = *it;
    ++it;
    int x = *it;

    auto band = m_raster->band(m_bandIndex);
    if (data.index() == 11) {
        band.setValue(x, y, std::get<double>(data));
    } else if (data.index() == 4) {
        band.setValue(x, y, static_cast<double>(std::get<int>(data)));
    }
}

void RasterComponentDataItem::setValues(const hydrocouple_variant *data,
                                        const std::initializer_list<int> &dimensionIndexes,
                                        const std::initializer_list<int> &dimensionLengths) {
    if (!m_raster || dimensionIndexes.size() < 2 || !data) return;

    auto it = dimensionIndexes.begin();
    int yOff = *it;
    ++it;
    int xOff = *it;

    auto band = m_raster->band(m_bandIndex);

    int ySize = band.ySize();
    int xSize = band.xSize();

    if (dimensionLengths.size() > 0) {
        auto lit = dimensionLengths.begin();
        ySize = *lit;
        ++lit;
        if (lit != dimensionLengths.end()) {
            xSize = *lit;
        }
    }

    int count = xSize * ySize;
    std::vector<double> buffer(count);

    for (int i = 0; i < count; ++i) {
        if (data[i].index() == 11) {
            buffer[i] = std::get<double>(data[i]);
        } else if (data[i].index() == 4) {
            buffer[i] = static_cast<double>(std::get<int>(data[i]));
        }
    }

    band.writeBlock(xOff, yOff, xSize, ySize, buffer);
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

} // namespace HydroCouple::Spatial
