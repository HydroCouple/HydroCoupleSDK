// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/spatial/regulargrid3d.h"

namespace HydroCouple::Spatial {

RegularGrid3D::RegularGrid3D() : Identity() {
}

std::shared_ptr<RegularGrid3D> RegularGrid3D::cartesian(double x0, double y0, double z0,
                                                         double dx, double dy, double dz,
                                                         int nx, int ny, int nz) {
    auto grid = std::make_shared<RegularGrid3D>();
    grid->m_nx = nx;
    grid->m_ny = ny;
    grid->m_nz = nz;
    grid->m_nodeX.resize(nx * ny * nz);
    grid->m_nodeY.resize(nx * ny * nz);
    grid->m_nodeZ.resize(nx * ny * nz);

    for (int k = 0; k < nz; ++k) {
        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                int idx = k * (nx * ny) + j * nx + i;
                grid->m_nodeX[idx] = x0 + i * dx;
                grid->m_nodeY[idx] = y0 + j * dy;
                grid->m_nodeZ[idx] = z0 + k * dz;
            }
        }
    }

    return grid;
}

std::shared_ptr<RegularGrid3D> RegularGrid3D::curvilinear(const std::vector<double> &nodeX,
                                                           const std::vector<double> &nodeY,
                                                           const std::vector<double> &nodeZ,
                                                           int nx, int ny, int nz) {
    int totalNodes = nx * ny * nz;
    if ((int)nodeX.size() != totalNodes || (int)nodeY.size() != totalNodes ||
        (int)nodeZ.size() != totalNodes) {
        return nullptr;
    }

    auto grid = std::make_shared<RegularGrid3D>();
    grid->m_nx = nx;
    grid->m_ny = ny;
    grid->m_nz = nz;
    grid->m_nodeX = nodeX;
    grid->m_nodeY = nodeY;
    grid->m_nodeZ = nodeZ;

    return grid;
}

double RegularGrid3D::nodeX(int i, int j, int k) const {
    if (i < 0 || j < 0 || k < 0 || i >= m_nx || j >= m_ny || k >= m_nz) return 0.0;
    return m_nodeX[k * (m_nx * m_ny) + j * m_nx + i];
}

double RegularGrid3D::nodeY(int i, int j, int k) const {
    if (i < 0 || j < 0 || k < 0 || i >= m_nx || j >= m_ny || k >= m_nz) return 0.0;
    return m_nodeY[k * (m_nx * m_ny) + j * m_nx + i];
}

double RegularGrid3D::nodeZ(int i, int j, int k) const {
    if (i < 0 || j < 0 || k < 0 || i >= m_nx || j >= m_ny || k >= m_nz) return 0.0;
    return m_nodeZ[k * (m_nx * m_ny) + j * m_nx + i];
}

void RegularGrid3D::setNodeX(int i, int j, int k, double x) {
    if (i < 0 || j < 0 || k < 0 || i >= m_nx || j >= m_ny || k >= m_nz) return;
    m_nodeX[k * (m_nx * m_ny) + j * m_nx + i] = x;
}

void RegularGrid3D::setNodeY(int i, int j, int k, double y) {
    if (i < 0 || j < 0 || k < 0 || i >= m_nx || j >= m_ny || k >= m_nz) return;
    m_nodeY[k * (m_nx * m_ny) + j * m_nx + i] = y;
}

void RegularGrid3D::setNodeZ(int i, int j, int k, double z) {
    if (i < 0 || j < 0 || k < 0 || i >= m_nx || j >= m_ny || k >= m_nz) return;
    m_nodeZ[k * (m_nx * m_ny) + j * m_nx + i] = z;
}

} // namespace HydroCouple::Spatial
