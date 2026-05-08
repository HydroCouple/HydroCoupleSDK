// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/argument2d.h"
#include "hydrocouple/dimension.h"
#include "hydrocouple/valuedefinition.h"

#include <fstream>
#include <sstream>

namespace HydroCouple {

// ── Argument2DInt ─────────────────────────────────────────────────────────

Argument2DInt::Argument2DInt(std::string_view id,
                             Dimension *dimensionX, Dimension *dimensionY,
                             int lengthX, int lengthY,
                             ValueDefinition *valueDef,
                             HydroCouple::IModelComponent *mc)
    : AbstractArgument(id, {dimensionX, dimensionY}, valueDef, mc)
    , ComponentDataItem2D<int>(lengthX, lengthY)
    , m_dimensionX(dimensionX)
    , m_dimensionY(dimensionY)
{}

int Argument2DInt::dimensionLength(const std::initializer_list<int> &dimIndexes) const {
    if (dimIndexes.size() == 0) return iLength();
    auto it = dimIndexes.begin();
    int idx = *it;
    return (idx == 0) ? iLength() : jLength();
}

void Argument2DInt::getValue(hydrocouple_variant &data,
                             const std::initializer_list<int> &dimIndexes) const {
    ComponentDataItem2D<int>::getValueT(dimIndexes, data);
}

void Argument2DInt::getValues(hydrocouple_variant *data,
                              const std::initializer_list<int> &dimIndexes,
                              const std::initializer_list<int> &dimLengths) const {
    ComponentDataItem2D<int>::getValuesT(dimIndexes, dimLengths, data);
}

void Argument2DInt::setValue(const hydrocouple_variant &data,
                             const std::initializer_list<int> &dimIndexes) {
    ComponentDataItem2D<int>::setValueT(data, dimIndexes);
    emitValueChanged(std::vector<int>(dimIndexes));
}

void Argument2DInt::setValues(const hydrocouple_variant *data,
                              const std::initializer_list<int> &dimIndexes,
                              const std::initializer_list<int> &dimLengths) {
    ComponentDataItem2D<int>::setValuesT(data, dimIndexes, dimLengths);
    emitValueChanged(std::vector<int>(dimIndexes), std::vector<int>(dimLengths));
}

std::string Argument2DInt::toString() const {
    auto j = writeData();
    return j.dump();
}

void Argument2DInt::saveData() {
    if (!m_inputFile.empty()) {
        std::ofstream ofs(m_inputFile);
        ofs << toString();
    }
}

bool Argument2DInt::initialize(const std::string &value,
                               HydroCouple::IArgument::ArgumentInputType argType,
                               std::string &message) {
    try {
        if (argType == HydroCouple::IArgument::ArgumentInputType::File) {
            m_inputFile = value;
            std::ifstream ifs(value);
            if (!ifs) {
                message = "Failed to open file: " + value;
                return false;
            }
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            auto j = nlohmann::json::parse(buffer.str());
            readData(j);
        } else {
            auto j = nlohmann::json::parse(value);
            readData(j);
        }
        return true;
    } catch (const std::exception &e) {
        message = std::string(e.what());
        return false;
    }
}

bool Argument2DInt::initialize(const HydroCouple::IComponentDataItem &src,
                               std::string &message) {
    try {
        int nx = src.dimensionLength({0});
        int ny = src.dimensionLength({1});
        resizeDataArray(nx, ny);
        hydrocouple_variant var;
        for (int i = 0; i < nx; ++i) {
            for (int j = 0; j < ny; ++j) {
                src.getValue(var, {i, j});
                (*this)(i, j) = std::get<int>(var);
            }
        }
        return true;
    } catch (const std::exception &e) {
        message = std::string(e.what());
        return false;
    }
}

void Argument2DInt::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values") && j.contains("iLength") && j.contains("jLength")) {
        int ni = j["iLength"].get<int>();
        int nj = j["jLength"].get<int>();
        resizeDataArray(ni, nj);
        auto &arr = j["values"];
        for (int i = 0; i < ni; ++i) {
            for (int jj = 0; jj < nj; ++jj) {
                (*this)(i, jj) = arr[i * nj + jj].get<int>();
            }
        }
    }
}

nlohmann::json Argument2DInt::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < iLength(); ++i) {
        for (int jj = 0; jj < jLength(); ++jj) {
            arr.push_back((*this)(i, jj));
        }
    }
    j["values"] = arr;
    j["iLength"] = iLength();
    j["jLength"] = jLength();
    return j;
}

// ── Argument2DDouble ──────────────────────────────────────────────────────

Argument2DDouble::Argument2DDouble(std::string_view id,
                                   Dimension *dimensionX, Dimension *dimensionY,
                                   int lengthX, int lengthY,
                                   ValueDefinition *valueDef,
                                   HydroCouple::IModelComponent *mc)
    : AbstractArgument(id, {dimensionX, dimensionY}, valueDef, mc)
    , ComponentDataItem2D<double>(lengthX, lengthY)
    , m_dimensionX(dimensionX)
    , m_dimensionY(dimensionY)
{}

int Argument2DDouble::dimensionLength(const std::initializer_list<int> &dimIndexes) const {
    if (dimIndexes.size() == 0) return iLength();
    auto it = dimIndexes.begin();
    int idx = *it;
    return (idx == 0) ? iLength() : jLength();
}

void Argument2DDouble::getValue(hydrocouple_variant &data,
                                const std::initializer_list<int> &dimIndexes) const {
    ComponentDataItem2D<double>::getValueT(dimIndexes, data);
}

void Argument2DDouble::getValues(hydrocouple_variant *data,
                                 const std::initializer_list<int> &dimIndexes,
                                 const std::initializer_list<int> &dimLengths) const {
    ComponentDataItem2D<double>::getValuesT(dimIndexes, dimLengths, data);
}

void Argument2DDouble::setValue(const hydrocouple_variant &data,
                                const std::initializer_list<int> &dimIndexes) {
    ComponentDataItem2D<double>::setValueT(data, dimIndexes);
    emitValueChanged(std::vector<int>(dimIndexes));
}

void Argument2DDouble::setValues(const hydrocouple_variant *data,
                                 const std::initializer_list<int> &dimIndexes,
                                 const std::initializer_list<int> &dimLengths) {
    ComponentDataItem2D<double>::setValuesT(data, dimIndexes, dimLengths);
    emitValueChanged(std::vector<int>(dimIndexes), std::vector<int>(dimLengths));
}

std::string Argument2DDouble::toString() const {
    auto j = writeData();
    return j.dump();
}

void Argument2DDouble::saveData() {
    if (!m_inputFile.empty()) {
        std::ofstream ofs(m_inputFile);
        ofs << toString();
    }
}

bool Argument2DDouble::initialize(const std::string &value,
                                  HydroCouple::IArgument::ArgumentInputType argType,
                                  std::string &message) {
    try {
        if (argType == HydroCouple::IArgument::ArgumentInputType::File) {
            m_inputFile = value;
            std::ifstream ifs(value);
            if (!ifs) {
                message = "Failed to open file: " + value;
                return false;
            }
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            auto j = nlohmann::json::parse(buffer.str());
            readData(j);
        } else {
            auto j = nlohmann::json::parse(value);
            readData(j);
        }
        return true;
    } catch (const std::exception &e) {
        message = std::string(e.what());
        return false;
    }
}

bool Argument2DDouble::initialize(const HydroCouple::IComponentDataItem &src,
                                  std::string &message) {
    try {
        int nx = src.dimensionLength({0});
        int ny = src.dimensionLength({1});
        resizeDataArray(nx, ny);
        hydrocouple_variant var;
        for (int i = 0; i < nx; ++i) {
            for (int j = 0; j < ny; ++j) {
                src.getValue(var, {i, j});
                (*this)(i, j) = std::get<double>(var);
            }
        }
        return true;
    } catch (const std::exception &e) {
        message = std::string(e.what());
        return false;
    }
}

void Argument2DDouble::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values") && j.contains("iLength") && j.contains("jLength")) {
        int ni = j["iLength"].get<int>();
        int nj = j["jLength"].get<int>();
        resizeDataArray(ni, nj);
        auto &arr = j["values"];
        for (int i = 0; i < ni; ++i) {
            for (int jj = 0; jj < nj; ++jj) {
                (*this)(i, jj) = arr[i * nj + jj].get<double>();
            }
        }
    }
}

nlohmann::json Argument2DDouble::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < iLength(); ++i) {
        for (int jj = 0; jj < jLength(); ++jj) {
            arr.push_back((*this)(i, jj));
        }
    }
    j["values"] = arr;
    j["iLength"] = iLength();
    j["jLength"] = jLength();
    return j;
}

// ── Argument2DString ──────────────────────────────────────────────────────

Argument2DString::Argument2DString(std::string_view id,
                                   Dimension *dimensionX, Dimension *dimensionY,
                                   int lengthX, int lengthY,
                                   ValueDefinition *valueDef,
                                   HydroCouple::IModelComponent *mc)
    : AbstractArgument(id, {dimensionX, dimensionY}, valueDef, mc)
    , ComponentDataItem2D<std::string>(lengthX, lengthY)
    , m_dimensionX(dimensionX)
    , m_dimensionY(dimensionY)
{}

int Argument2DString::dimensionLength(const std::initializer_list<int> &dimIndexes) const {
    if (dimIndexes.size() == 0) return iLength();
    auto it = dimIndexes.begin();
    int idx = *it;
    return (idx == 0) ? iLength() : jLength();
}

void Argument2DString::getValue(hydrocouple_variant &data,
                                const std::initializer_list<int> &dimIndexes) const {
    ComponentDataItem2D<std::string>::getValueT(dimIndexes, data);
}

void Argument2DString::getValues(hydrocouple_variant *data,
                                 const std::initializer_list<int> &dimIndexes,
                                 const std::initializer_list<int> &dimLengths) const {
    ComponentDataItem2D<std::string>::getValuesT(dimIndexes, dimLengths, data);
}

void Argument2DString::setValue(const hydrocouple_variant &data,
                                const std::initializer_list<int> &dimIndexes) {
    ComponentDataItem2D<std::string>::setValueT(data, dimIndexes);
    emitValueChanged(std::vector<int>(dimIndexes));
}

void Argument2DString::setValues(const hydrocouple_variant *data,
                                 const std::initializer_list<int> &dimIndexes,
                                 const std::initializer_list<int> &dimLengths) {
    ComponentDataItem2D<std::string>::setValuesT(data, dimIndexes, dimLengths);
    emitValueChanged(std::vector<int>(dimIndexes), std::vector<int>(dimLengths));
}

std::string Argument2DString::toString() const {
    auto j = writeData();
    return j.dump();
}

void Argument2DString::saveData() {
    if (!m_inputFile.empty()) {
        std::ofstream ofs(m_inputFile);
        ofs << toString();
    }
}

bool Argument2DString::initialize(const std::string &value,
                                  HydroCouple::IArgument::ArgumentInputType argType,
                                  std::string &message) {
    try {
        if (argType == HydroCouple::IArgument::ArgumentInputType::File) {
            m_inputFile = value;
            std::ifstream ifs(value);
            if (!ifs) {
                message = "Failed to open file: " + value;
                return false;
            }
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            auto j = nlohmann::json::parse(buffer.str());
            readData(j);
        } else {
            auto j = nlohmann::json::parse(value);
            readData(j);
        }
        return true;
    } catch (const std::exception &e) {
        message = std::string(e.what());
        return false;
    }
}

bool Argument2DString::initialize(const HydroCouple::IComponentDataItem &src,
                                  std::string &message) {
    try {
        int nx = src.dimensionLength({0});
        int ny = src.dimensionLength({1});
        resizeDataArray(nx, ny);
        hydrocouple_variant var;
        for (int i = 0; i < nx; ++i) {
            for (int j = 0; j < ny; ++j) {
                src.getValue(var, {i, j});
                (*this)(i, j) = std::get<std::string>(var);
            }
        }
        return true;
    } catch (const std::exception &e) {
        message = std::string(e.what());
        return false;
    }
}

void Argument2DString::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values") && j.contains("iLength") && j.contains("jLength")) {
        int ni = j["iLength"].get<int>();
        int nj = j["jLength"].get<int>();
        resizeDataArray(ni, nj);
        auto &arr = j["values"];
        for (int i = 0; i < ni; ++i) {
            for (int jj = 0; jj < nj; ++jj) {
                (*this)(i, jj) = arr[i * nj + jj].get<std::string>();
            }
        }
    }
}

nlohmann::json Argument2DString::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < iLength(); ++i) {
        for (int jj = 0; jj < jLength(); ++jj) {
            arr.push_back((*this)(i, jj));
        }
    }
    j["values"] = arr;
    j["iLength"] = iLength();
    j["jLength"] = jLength();
    return j;
}

} // namespace HydroCouple
