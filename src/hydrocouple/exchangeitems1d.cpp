// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/exchangeitems1d.h"
#include "hydrocouple/dimension.h"
#include "hydrocouple/valuedefinition.h"

#include <nlohmann/json.hpp>

namespace HydroCouple {

// ── Input1DInt ────────────────────────────────────────────────────────────

Input1DInt::Input1DInt(std::string_view id, Dimension *dimension, int length,
                       ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
    : AbstractInput(id, {dimension}, valueDef, mc)
    , ComponentDataItem1D<int>(length)
    , m_dimension(dimension)
{}

int Input1DInt::dimensionLength(const std::initializer_list<int> &) const {
    return ComponentDataItem1D<int>::length();
}

void Input1DInt::getValue(hydrocouple_variant &data,
                          const std::initializer_list<int> &dimIndexes) const {
    ComponentDataItem1D<int>::getValueT(dimIndexes, data);
}

void Input1DInt::getValues(hydrocouple_variant *data,
                           const std::initializer_list<int> &dimIndexes,
                           const std::initializer_list<int> &dimLengths) const {
    ComponentDataItem1D<int>::getValuesT(dimIndexes, dimLengths, data);
}

void Input1DInt::setValue(const hydrocouple_variant &data,
                          const std::initializer_list<int> &dimIndexes) {
    ComponentDataItem1D<int>::setValueT(data, dimIndexes);
    emitValueChanged(std::vector<int>(dimIndexes));
}

void Input1DInt::setValues(const hydrocouple_variant *data,
                           const std::initializer_list<int> &dimIndexes,
                           const std::initializer_list<int> &dimLengths) {
    ComponentDataItem1D<int>::setValuesT(data, dimIndexes, dimLengths);
    emitValueChanged(std::vector<int>(dimIndexes), std::vector<int>(dimLengths));
}

void Input1DInt::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values")) {
        auto &arr = j["values"];
        resizeDataArray((int)arr.size());
        for (int i = 0; i < (int)arr.size(); ++i)
            (*this)[i] = arr[i].get<int>();
    }
}

nlohmann::json Input1DInt::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < length(); ++i)
        arr.push_back((*this)[i]);
    j["values"] = arr;
    return j;
}

// ── Input1DDouble ─────────────────────────────────────────────────────────

Input1DDouble::Input1DDouble(std::string_view id, Dimension *dimension, int length,
                             ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
    : AbstractInput(id, {dimension}, valueDef, mc)
    , ComponentDataItem1D<double>(length)
    , m_dimension(dimension)
{}

int Input1DDouble::dimensionLength(const std::initializer_list<int> &) const {
    return ComponentDataItem1D<double>::length();
}

void Input1DDouble::getValue(hydrocouple_variant &data,
                             const std::initializer_list<int> &dimIndexes) const {
    ComponentDataItem1D<double>::getValueT(dimIndexes, data);
}

void Input1DDouble::getValues(hydrocouple_variant *data,
                              const std::initializer_list<int> &dimIndexes,
                              const std::initializer_list<int> &dimLengths) const {
    ComponentDataItem1D<double>::getValuesT(dimIndexes, dimLengths, data);
}

void Input1DDouble::setValue(const hydrocouple_variant &data,
                             const std::initializer_list<int> &dimIndexes) {
    ComponentDataItem1D<double>::setValueT(data, dimIndexes);
    emitValueChanged(std::vector<int>(dimIndexes));
}

void Input1DDouble::setValues(const hydrocouple_variant *data,
                              const std::initializer_list<int> &dimIndexes,
                              const std::initializer_list<int> &dimLengths) {
    ComponentDataItem1D<double>::setValuesT(data, dimIndexes, dimLengths);
    emitValueChanged(std::vector<int>(dimIndexes), std::vector<int>(dimLengths));
}

void Input1DDouble::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values")) {
        auto &arr = j["values"];
        resizeDataArray((int)arr.size());
        for (int i = 0; i < (int)arr.size(); ++i)
            (*this)[i] = arr[i].get<double>();
    }
}

nlohmann::json Input1DDouble::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < length(); ++i)
        arr.push_back((*this)[i]);
    j["values"] = arr;
    return j;
}

// ── Input1DString ─────────────────────────────────────────────────────────

Input1DString::Input1DString(std::string_view id, Dimension *dimension, int length,
                             ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
    : AbstractInput(id, {dimension}, valueDef, mc)
    , ComponentDataItem1D<std::string>(length)
    , m_dimension(dimension)
{}

int Input1DString::dimensionLength(const std::initializer_list<int> &) const {
    return ComponentDataItem1D<std::string>::length();
}

void Input1DString::getValue(hydrocouple_variant &data,
                             const std::initializer_list<int> &dimIndexes) const {
    ComponentDataItem1D<std::string>::getValueT(dimIndexes, data);
}

void Input1DString::getValues(hydrocouple_variant *data,
                              const std::initializer_list<int> &dimIndexes,
                              const std::initializer_list<int> &dimLengths) const {
    ComponentDataItem1D<std::string>::getValuesT(dimIndexes, dimLengths, data);
}

void Input1DString::setValue(const hydrocouple_variant &data,
                             const std::initializer_list<int> &dimIndexes) {
    ComponentDataItem1D<std::string>::setValueT(data, dimIndexes);
    emitValueChanged(std::vector<int>(dimIndexes));
}

void Input1DString::setValues(const hydrocouple_variant *data,
                              const std::initializer_list<int> &dimIndexes,
                              const std::initializer_list<int> &dimLengths) {
    ComponentDataItem1D<std::string>::setValuesT(data, dimIndexes, dimLengths);
    emitValueChanged(std::vector<int>(dimIndexes), std::vector<int>(dimLengths));
}

void Input1DString::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values")) {
        auto &arr = j["values"];
        resizeDataArray((int)arr.size());
        for (int i = 0; i < (int)arr.size(); ++i)
            (*this)[i] = arr[i].get<std::string>();
    }
}

nlohmann::json Input1DString::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < length(); ++i)
        arr.push_back((*this)[i]);
    j["values"] = arr;
    return j;
}

// ── Output1DInt ───────────────────────────────────────────────────────────

Output1DInt::Output1DInt(std::string_view id, Dimension *dimension, int length,
                         ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
    : AbstractOutput(id, {dimension}, valueDef, mc)
    , ComponentDataItem1D<int>(length)
    , m_dimension(dimension)
{}

int Output1DInt::dimensionLength(const std::initializer_list<int> &) const {
    return ComponentDataItem1D<int>::length();
}

void Output1DInt::getValue(hydrocouple_variant &data,
                           const std::initializer_list<int> &dimIndexes) const {
    ComponentDataItem1D<int>::getValueT(dimIndexes, data);
}

void Output1DInt::getValues(hydrocouple_variant *data,
                            const std::initializer_list<int> &dimIndexes,
                            const std::initializer_list<int> &dimLengths) const {
    ComponentDataItem1D<int>::getValuesT(dimIndexes, dimLengths, data);
}

void Output1DInt::setValue(const hydrocouple_variant &data,
                           const std::initializer_list<int> &dimIndexes) {
    ComponentDataItem1D<int>::setValueT(data, dimIndexes);
    emitValueChanged(std::vector<int>(dimIndexes));
}

void Output1DInt::setValues(const hydrocouple_variant *data,
                            const std::initializer_list<int> &dimIndexes,
                            const std::initializer_list<int> &dimLengths) {
    ComponentDataItem1D<int>::setValuesT(data, dimIndexes, dimLengths);
    emitValueChanged(std::vector<int>(dimIndexes), std::vector<int>(dimLengths));
}

void Output1DInt::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values")) {
        auto &arr = j["values"];
        resizeDataArray((int)arr.size());
        for (int i = 0; i < (int)arr.size(); ++i)
            (*this)[i] = arr[i].get<int>();
    }
}

nlohmann::json Output1DInt::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < length(); ++i)
        arr.push_back((*this)[i]);
    j["values"] = arr;
    return j;
}

// ── Output1DDouble ────────────────────────────────────────────────────────

Output1DDouble::Output1DDouble(std::string_view id, Dimension *dimension, int length,
                               ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
    : AbstractOutput(id, {dimension}, valueDef, mc)
    , ComponentDataItem1D<double>(length)
    , m_dimension(dimension)
{}

int Output1DDouble::dimensionLength(const std::initializer_list<int> &) const {
    return ComponentDataItem1D<double>::length();
}

void Output1DDouble::getValue(hydrocouple_variant &data,
                              const std::initializer_list<int> &dimIndexes) const {
    ComponentDataItem1D<double>::getValueT(dimIndexes, data);
}

void Output1DDouble::getValues(hydrocouple_variant *data,
                               const std::initializer_list<int> &dimIndexes,
                               const std::initializer_list<int> &dimLengths) const {
    ComponentDataItem1D<double>::getValuesT(dimIndexes, dimLengths, data);
}

void Output1DDouble::setValue(const hydrocouple_variant &data,
                              const std::initializer_list<int> &dimIndexes) {
    ComponentDataItem1D<double>::setValueT(data, dimIndexes);
    emitValueChanged(std::vector<int>(dimIndexes));
}

void Output1DDouble::setValues(const hydrocouple_variant *data,
                               const std::initializer_list<int> &dimIndexes,
                               const std::initializer_list<int> &dimLengths) {
    ComponentDataItem1D<double>::setValuesT(data, dimIndexes, dimLengths);
    emitValueChanged(std::vector<int>(dimIndexes), std::vector<int>(dimLengths));
}

void Output1DDouble::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values")) {
        auto &arr = j["values"];
        resizeDataArray((int)arr.size());
        for (int i = 0; i < (int)arr.size(); ++i)
            (*this)[i] = arr[i].get<double>();
    }
}

nlohmann::json Output1DDouble::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < length(); ++i)
        arr.push_back((*this)[i]);
    j["values"] = arr;
    return j;
}

// ── Output1DString ────────────────────────────────────────────────────────

Output1DString::Output1DString(std::string_view id, Dimension *dimension, int length,
                               ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
    : AbstractOutput(id, {dimension}, valueDef, mc)
    , ComponentDataItem1D<std::string>(length)
    , m_dimension(dimension)
{}

int Output1DString::dimensionLength(const std::initializer_list<int> &) const {
    return ComponentDataItem1D<std::string>::length();
}

void Output1DString::getValue(hydrocouple_variant &data,
                              const std::initializer_list<int> &dimIndexes) const {
    ComponentDataItem1D<std::string>::getValueT(dimIndexes, data);
}

void Output1DString::getValues(hydrocouple_variant *data,
                               const std::initializer_list<int> &dimIndexes,
                               const std::initializer_list<int> &dimLengths) const {
    ComponentDataItem1D<std::string>::getValuesT(dimIndexes, dimLengths, data);
}

void Output1DString::setValue(const hydrocouple_variant &data,
                              const std::initializer_list<int> &dimIndexes) {
    ComponentDataItem1D<std::string>::setValueT(data, dimIndexes);
    emitValueChanged(std::vector<int>(dimIndexes));
}

void Output1DString::setValues(const hydrocouple_variant *data,
                               const std::initializer_list<int> &dimIndexes,
                               const std::initializer_list<int> &dimLengths) {
    ComponentDataItem1D<std::string>::setValuesT(data, dimIndexes, dimLengths);
    emitValueChanged(std::vector<int>(dimIndexes), std::vector<int>(dimLengths));
}

void Output1DString::readData(const nlohmann::json &j) {
    AbstractComponentDataItem::readData(j);
    if (j.contains("values")) {
        auto &arr = j["values"];
        resizeDataArray((int)arr.size());
        for (int i = 0; i < (int)arr.size(); ++i)
            (*this)[i] = arr[i].get<std::string>();
    }
}

nlohmann::json Output1DString::writeData() const {
    auto j = AbstractComponentDataItem::writeData();
    auto arr = nlohmann::json::array();
    for (int i = 0; i < length(); ++i)
        arr.push_back((*this)[i]);
    j["values"] = arr;
    return j;
}

} // namespace HydroCouple
