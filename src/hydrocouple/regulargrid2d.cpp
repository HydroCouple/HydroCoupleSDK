// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/regulargrid2d.h"

#include <cmath>
#include <algorithm>

namespace HydroCouple::Spatial {

RegularGrid2D::RegularGrid2D() : Identity() {
}

std::shared_ptr<RegularGrid2D> RegularGrid2D::cartesian(double x0, double y0,
                                                         double dx, double dy,
                                                         int nx, int ny) {
    auto grid = std::make_shared<RegularGrid2D>();
    grid->m_gridType = Cartesian;
    grid->m_nx = nx;
    grid->m_ny = ny;
    grid->m_nodeX.resize(nx * ny);
    grid->m_nodeY.resize(nx * ny);

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            int idx = j * nx + i;
            grid->m_nodeX[idx] = x0 + i * dx;
            grid->m_nodeY[idx] = y0 + j * dy;
        }
    }

    return grid;
}

std::shared_ptr<RegularGrid2D> RegularGrid2D::rectilinear(const std::vector<double> &nodeX,
                                                           const std::vector<double> &nodeY,
                                                           int nx, int ny) {
    if ((int)nodeX.size() != nx || (int)nodeY.size() != ny) {
        return nullptr;
    }

    auto grid = std::make_shared<RegularGrid2D>();
    grid->m_gridType = Rectilinear;
    grid->m_nx = nx;
    grid->m_ny = ny;
    grid->m_nodeX.resize(nx * ny);
    grid->m_nodeY.resize(nx * ny);

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            int idx = j * nx + i;
            grid->m_nodeX[idx] = nodeX[i];
            grid->m_nodeY[idx] = nodeY[j];
        }
    }

    return grid;
}

std::shared_ptr<RegularGrid2D> RegularGrid2D::curvilinear(const std::vector<double> &nodeX,
                                                           const std::vector<double> &nodeY,
                                                           int nx, int ny) {
    if ((int)nodeX.size() != nx * ny || (int)nodeY.size() != nx * ny) {
        return nullptr;
    }

    auto grid = std::make_shared<RegularGrid2D>();
    grid->m_gridType = Curvilinear;
    grid->m_nx = nx;
    grid->m_ny = ny;
    grid->m_nodeX = nodeX;
    grid->m_nodeY = nodeY;

    return grid;
}

double RegularGrid2D::nodeX(int i, int j) const {
    if (i < 0 || j < 0 || i >= m_nx || j >= m_ny) return 0.0;
    return m_nodeX[j * m_nx + i];
}

double RegularGrid2D::nodeY(int i, int j) const {
    if (i < 0 || j < 0 || i >= m_nx || j >= m_ny) return 0.0;
    return m_nodeY[j * m_nx + i];
}

void RegularGrid2D::setNodeX(int i, int j, double x) {
    if (i < 0 || j < 0 || i >= m_nx || j >= m_ny) return;
    m_nodeX[j * m_nx + i] = x;
}

void RegularGrid2D::setNodeY(int i, int j, double y) {
    if (i < 0 || j < 0 || i >= m_nx || j >= m_ny) return;
    m_nodeY[j * m_nx + i] = y;
}

void RegularGrid2D::cellCenter(int i, int j, double &x, double &y) const {
    if (i < 0 || j < 0 || i >= numXCells() || j >= numYCells()) {
        x = y = 0.0;
        return;
    }

    // Four corners of cell (i, j): (i,j), (i+1,j), (i,j+1), (i+1,j+1)
    double x00 = nodeX(i, j);
    double y00 = nodeY(i, j);
    double x10 = nodeX(i + 1, j);
    double y10 = nodeY(i + 1, j);
    double x01 = nodeX(i, j + 1);
    double y01 = nodeY(i, j + 1);
    double x11 = nodeX(i + 1, j + 1);
    double y11 = nodeY(i + 1, j + 1);

    x = 0.25 * (x00 + x10 + x01 + x11);
    y = 0.25 * (y00 + y10 + y01 + y11);
}

double RegularGrid2D::cellArea(int i, int j) const {
    if (i < 0 || j < 0 || i >= numXCells() || j >= numYCells()) {
        return 0.0;
    }

    // Shoelace formula for quadrilateral
    double x00 = nodeX(i, j);
    double y00 = nodeY(i, j);
    double x10 = nodeX(i + 1, j);
    double y10 = nodeY(i + 1, j);
    double x11 = nodeX(i + 1, j + 1);
    double y11 = nodeY(i + 1, j + 1);
    double x01 = nodeX(i, j + 1);
    double y01 = nodeY(i, j + 1);

    double area = std::abs(
        (x10 - x00) * (y01 - y00) - (x01 - x00) * (y10 - y00) +
        (x11 - x10) * (y01 - y10) - (x01 - x10) * (y11 - y10)) * 0.5;

    return area;
}

Envelope RegularGrid2D::envelope() const {
    Envelope env;

    if (isEmpty()) return env;

    double minX = m_nodeX[0], maxX = m_nodeX[0];
    double minY = m_nodeY[0], maxY = m_nodeY[0];

#ifdef _OPENMP
#pragma omp simd reduction(min : minX, minY) reduction(max : maxX, maxY)
#endif
    for (int i = 1; i < (int)m_nodeX.size(); ++i) {
        minX = std::min(minX, m_nodeX[i]);
        maxX = std::max(maxX, m_nodeX[i]);
        minY = std::min(minY, m_nodeY[i]);
        maxY = std::max(maxY, m_nodeY[i]);
    }

    env = Envelope(minX, maxX, minY, maxY);
    return env;
}

} // namespace HydroCouple::Spatial
