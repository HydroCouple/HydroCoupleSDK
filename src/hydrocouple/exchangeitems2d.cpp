// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/exchangeitems2d.h"
#include "hydrocouple/dimension.h"
#include "hydrocouple/valuedefinition.h"

#include <nlohmann/json.hpp>

namespace HydroCouple {

// ── Input2DInt ────────────────────────────────────────────────────────────

Input2DInt::Input2DInt(std::string_view id, Dimension *dx, Dimension *dy,
                       int lx, int ly, ValueDefinition *vd, IModelComponent *mc)
    : AbstractInput(id, {dx, dy}, vd, mc), ComponentDataItem2D<int>(lx, ly),
      m_dimensionX(dx), m_dimensionY(dy) {}

int Input2DInt::dimensionLength(const std::initializer_list<int> &di) const {
    return di.size() == 0 ? iLength() : (*di.begin() == 0 ? iLength() : jLength());
}

void Input2DInt::getValue(hydrocouple_variant &d, const std::initializer_list<int> &di) const {
    ComponentDataItem2D<int>::getValueT(di, d);
}

void Input2DInt::getValues(hydrocouple_variant *d, const std::initializer_list<int> &di,
                           const std::initializer_list<int> &dl) const {
    ComponentDataItem2D<int>::getValuesT(di, dl, d);
}

void Input2DInt::setValue(const hydrocouple_variant &d, const std::initializer_list<int> &di) {
    ComponentDataItem2D<int>::setValueT(d, di);
    emitValueChanged(std::vector<int>(di));
}

void Input2DInt::setValues(const hydrocouple_variant *d, const std::initializer_list<int> &di,
                           const std::initializer_list<int> &dl) {
    ComponentDataItem2D<int>::setValuesT(d, di, dl);
    emitValueChanged(std::vector<int>(di), std::vector<int>(dl));
}

void Input2DInt::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values") && j.contains("iLength") && j.contains("jLength")) {
        int ni = j["iLength"].get<int>(), nj = j["jLength"].get<int>();
        resizeDataArray(ni, nj);
        auto &arr = j["values"];
        for (int i = 0; i < ni; ++i)
            for (int jj = 0; jj < nj; ++jj)
                (*this)(i, jj) = arr[i * nj + jj].get<int>();
    }
}

nlohmann::json Input2DInt::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < iLength(); ++i)
        for (int jj = 0; jj < jLength(); ++jj)
            arr.push_back((*this)(i, jj));
    j["values"] = arr;
    j["iLength"] = iLength();
    j["jLength"] = jLength();
    return j;
}

// ── Input2DDouble ────────────────────────────────────────────────────────

Input2DDouble::Input2DDouble(std::string_view id, Dimension *dx, Dimension *dy,
                             int lx, int ly, ValueDefinition *vd, IModelComponent *mc)
    : AbstractInput(id, {dx, dy}, vd, mc), ComponentDataItem2D<double>(lx, ly),
      m_dimensionX(dx), m_dimensionY(dy) {}

int Input2DDouble::dimensionLength(const std::initializer_list<int> &di) const {
    return di.size() == 0 ? iLength() : (*di.begin() == 0 ? iLength() : jLength());
}

void Input2DDouble::getValue(hydrocouple_variant &d, const std::initializer_list<int> &di) const {
    ComponentDataItem2D<double>::getValueT(di, d);
}

void Input2DDouble::getValues(hydrocouple_variant *d, const std::initializer_list<int> &di,
                              const std::initializer_list<int> &dl) const {
    ComponentDataItem2D<double>::getValuesT(di, dl, d);
}

void Input2DDouble::setValue(const hydrocouple_variant &d, const std::initializer_list<int> &di) {
    ComponentDataItem2D<double>::setValueT(d, di);
    emitValueChanged(std::vector<int>(di));
}

void Input2DDouble::setValues(const hydrocouple_variant *d, const std::initializer_list<int> &di,
                              const std::initializer_list<int> &dl) {
    ComponentDataItem2D<double>::setValuesT(d, di, dl);
    emitValueChanged(std::vector<int>(di), std::vector<int>(dl));
}

void Input2DDouble::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values") && j.contains("iLength") && j.contains("jLength")) {
        int ni = j["iLength"].get<int>(), nj = j["jLength"].get<int>();
        resizeDataArray(ni, nj);
        auto &arr = j["values"];
        for (int i = 0; i < ni; ++i)
            for (int jj = 0; jj < nj; ++jj)
                (*this)(i, jj) = arr[i * nj + jj].get<double>();
    }
}

nlohmann::json Input2DDouble::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < iLength(); ++i)
        for (int jj = 0; jj < jLength(); ++jj)
            arr.push_back((*this)(i, jj));
    j["values"] = arr;
    j["iLength"] = iLength();
    j["jLength"] = jLength();
    return j;
}

// ── Input2DString ────────────────────────────────────────────────────────

Input2DString::Input2DString(std::string_view id, Dimension *dx, Dimension *dy,
                             int lx, int ly, ValueDefinition *vd, IModelComponent *mc)
    : AbstractInput(id, {dx, dy}, vd, mc), ComponentDataItem2D<std::string>(lx, ly),
      m_dimensionX(dx), m_dimensionY(dy) {}

int Input2DString::dimensionLength(const std::initializer_list<int> &di) const {
    return di.size() == 0 ? iLength() : (*di.begin() == 0 ? iLength() : jLength());
}

void Input2DString::getValue(hydrocouple_variant &d, const std::initializer_list<int> &di) const {
    ComponentDataItem2D<std::string>::getValueT(di, d);
}

void Input2DString::getValues(hydrocouple_variant *d, const std::initializer_list<int> &di,
                              const std::initializer_list<int> &dl) const {
    ComponentDataItem2D<std::string>::getValuesT(di, dl, d);
}

void Input2DString::setValue(const hydrocouple_variant &d, const std::initializer_list<int> &di) {
    ComponentDataItem2D<std::string>::setValueT(d, di);
    emitValueChanged(std::vector<int>(di));
}

void Input2DString::setValues(const hydrocouple_variant *d, const std::initializer_list<int> &di,
                              const std::initializer_list<int> &dl) {
    ComponentDataItem2D<std::string>::setValuesT(d, di, dl);
    emitValueChanged(std::vector<int>(di), std::vector<int>(dl));
}

void Input2DString::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values") && j.contains("iLength") && j.contains("jLength")) {
        int ni = j["iLength"].get<int>(), nj = j["jLength"].get<int>();
        resizeDataArray(ni, nj);
        auto &arr = j["values"];
        for (int i = 0; i < ni; ++i)
            for (int jj = 0; jj < nj; ++jj)
                (*this)(i, jj) = arr[i * nj + jj].get<std::string>();
    }
}

nlohmann::json Input2DString::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < iLength(); ++i)
        for (int jj = 0; jj < jLength(); ++jj)
            arr.push_back((*this)(i, jj));
    j["values"] = arr;
    j["iLength"] = iLength();
    j["jLength"] = jLength();
    return j;
}

// ── Output2DInt ───────────────────────────────────────────────────────────

Output2DInt::Output2DInt(std::string_view id, Dimension *dx, Dimension *dy,
                         int lx, int ly, ValueDefinition *vd, IModelComponent *mc)
    : AbstractOutput(id, {dx, dy}, vd, mc), ComponentDataItem2D<int>(lx, ly),
      m_dimensionX(dx), m_dimensionY(dy) {}

int Output2DInt::dimensionLength(const std::initializer_list<int> &di) const {
    return di.size() == 0 ? iLength() : (*di.begin() == 0 ? iLength() : jLength());
}

void Output2DInt::getValue(hydrocouple_variant &d, const std::initializer_list<int> &di) const {
    ComponentDataItem2D<int>::getValueT(di, d);
}

void Output2DInt::getValues(hydrocouple_variant *d, const std::initializer_list<int> &di,
                            const std::initializer_list<int> &dl) const {
    ComponentDataItem2D<int>::getValuesT(di, dl, d);
}

void Output2DInt::setValue(const hydrocouple_variant &d, const std::initializer_list<int> &di) {
    ComponentDataItem2D<int>::setValueT(d, di);
    emitValueChanged(std::vector<int>(di));
}

void Output2DInt::setValues(const hydrocouple_variant *d, const std::initializer_list<int> &di,
                            const std::initializer_list<int> &dl) {
    ComponentDataItem2D<int>::setValuesT(d, di, dl);
    emitValueChanged(std::vector<int>(di), std::vector<int>(dl));
}

void Output2DInt::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values") && j.contains("iLength") && j.contains("jLength")) {
        int ni = j["iLength"].get<int>(), nj = j["jLength"].get<int>();
        resizeDataArray(ni, nj);
        auto &arr = j["values"];
        for (int i = 0; i < ni; ++i)
            for (int jj = 0; jj < nj; ++jj)
                (*this)(i, jj) = arr[i * nj + jj].get<int>();
    }
}

nlohmann::json Output2DInt::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < iLength(); ++i)
        for (int jj = 0; jj < jLength(); ++jj)
            arr.push_back((*this)(i, jj));
    j["values"] = arr;
    j["iLength"] = iLength();
    j["jLength"] = jLength();
    return j;
}

// ── Output2DDouble ────────────────────────────────────────────────────────

Output2DDouble::Output2DDouble(std::string_view id, Dimension *dx, Dimension *dy,
                               int lx, int ly, ValueDefinition *vd, IModelComponent *mc)
    : AbstractOutput(id, {dx, dy}, vd, mc), ComponentDataItem2D<double>(lx, ly),
      m_dimensionX(dx), m_dimensionY(dy) {}

int Output2DDouble::dimensionLength(const std::initializer_list<int> &di) const {
    return di.size() == 0 ? iLength() : (*di.begin() == 0 ? iLength() : jLength());
}

void Output2DDouble::getValue(hydrocouple_variant &d, const std::initializer_list<int> &di) const {
    ComponentDataItem2D<double>::getValueT(di, d);
}

void Output2DDouble::getValues(hydrocouple_variant *d, const std::initializer_list<int> &di,
                               const std::initializer_list<int> &dl) const {
    ComponentDataItem2D<double>::getValuesT(di, dl, d);
}

void Output2DDouble::setValue(const hydrocouple_variant &d, const std::initializer_list<int> &di) {
    ComponentDataItem2D<double>::setValueT(d, di);
    emitValueChanged(std::vector<int>(di));
}

void Output2DDouble::setValues(const hydrocouple_variant *d, const std::initializer_list<int> &di,
                               const std::initializer_list<int> &dl) {
    ComponentDataItem2D<double>::setValuesT(d, di, dl);
    emitValueChanged(std::vector<int>(di), std::vector<int>(dl));
}

void Output2DDouble::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values") && j.contains("iLength") && j.contains("jLength")) {
        int ni = j["iLength"].get<int>(), nj = j["jLength"].get<int>();
        resizeDataArray(ni, nj);
        auto &arr = j["values"];
        for (int i = 0; i < ni; ++i)
            for (int jj = 0; jj < nj; ++jj)
                (*this)(i, jj) = arr[i * nj + jj].get<double>();
    }
}

nlohmann::json Output2DDouble::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < iLength(); ++i)
        for (int jj = 0; jj < jLength(); ++jj)
            arr.push_back((*this)(i, jj));
    j["values"] = arr;
    j["iLength"] = iLength();
    j["jLength"] = jLength();
    return j;
}

// ── Output2DString ────────────────────────────────────────────────────────

Output2DString::Output2DString(std::string_view id, Dimension *dx, Dimension *dy,
                               int lx, int ly, ValueDefinition *vd, IModelComponent *mc)
    : AbstractOutput(id, {dx, dy}, vd, mc), ComponentDataItem2D<std::string>(lx, ly),
      m_dimensionX(dx), m_dimensionY(dy) {}

int Output2DString::dimensionLength(const std::initializer_list<int> &di) const {
    return di.size() == 0 ? iLength() : (*di.begin() == 0 ? iLength() : jLength());
}

void Output2DString::getValue(hydrocouple_variant &d, const std::initializer_list<int> &di) const {
    ComponentDataItem2D<std::string>::getValueT(di, d);
}

void Output2DString::getValues(hydrocouple_variant *d, const std::initializer_list<int> &di,
                               const std::initializer_list<int> &dl) const {
    ComponentDataItem2D<std::string>::getValuesT(di, dl, d);
}

void Output2DString::setValue(const hydrocouple_variant &d, const std::initializer_list<int> &di) {
    ComponentDataItem2D<std::string>::setValueT(d, di);
    emitValueChanged(std::vector<int>(di));
}

void Output2DString::setValues(const hydrocouple_variant *d, const std::initializer_list<int> &di,
                               const std::initializer_list<int> &dl) {
    ComponentDataItem2D<std::string>::setValuesT(d, di, dl);
    emitValueChanged(std::vector<int>(di), std::vector<int>(dl));
}

void Output2DString::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values") && j.contains("iLength") && j.contains("jLength")) {
        int ni = j["iLength"].get<int>(), nj = j["jLength"].get<int>();
        resizeDataArray(ni, nj);
        auto &arr = j["values"];
        for (int i = 0; i < ni; ++i)
            for (int jj = 0; jj < nj; ++jj)
                (*this)(i, jj) = arr[i * nj + jj].get<std::string>();
    }
}

nlohmann::json Output2DString::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < iLength(); ++i)
        for (int jj = 0; jj < jLength(); ++jj)
            arr.push_back((*this)(i, jj));
    j["values"] = arr;
    j["iLength"] = iLength();
    j["jLength"] = jLength();
    return j;
}

} // namespace HydroCouple
