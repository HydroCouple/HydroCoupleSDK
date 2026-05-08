// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/argument1d.h"
#include "hydrocouple/dimension.h"
#include "hydrocouple/valuedefinition.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace HydroCouple {

// ── Shared helper — parse a JSON array string into a vector<T> ────────────

template<typename T>
static bool parseJsonArray(const std::string &text, std::vector<T> &out, std::string &msg)
{
    try {
        auto j = nlohmann::json::parse(text);
        if (!j.is_array()) { msg = "Expected JSON array"; return false; }
        out.clear();
        for (const auto &el : j)
            out.push_back(el.get<T>());
        return true;
    } catch (const std::exception &e) {
        msg = e.what();
        return false;
    }
}

// ── Macro-driven implementation ───────────────────────────────────────────

#define IMPL_ARGUMENT1D(ClassName, T)                                                     \
                                                                                           \
ClassName::ClassName(std::string_view id, Dimension *dimension, int length,                \
                     ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)          \
    : AbstractArgument(id, {dimension}, valueDef, mc)                                      \
    , ComponentDataItem1D<T>(length)                                                       \
    , m_dimension(dimension)                                                               \
{}                                                                                         \
                                                                                           \
int ClassName::dimensionLength(const std::initializer_list<int> &) const                   \
{                                                                                          \
    return ComponentDataItem1D<T>::length();                                               \
}                                                                                          \
                                                                                           \
void ClassName::getValue(hydrocouple_variant &data,                                         \
                          const std::initializer_list<int> &dimIndexes) const              \
{                                                                                          \
    ComponentDataItem1D<T>::getValueT(dimIndexes, data);                                   \
}                                                                                          \
                                                                                           \
void ClassName::getValues(hydrocouple_variant *data,                                        \
                           const std::initializer_list<int> &dimIndexes,                   \
                           const std::initializer_list<int> &dimLengths) const             \
{                                                                                          \
    ComponentDataItem1D<T>::getValuesT(dimIndexes, dimLengths, data);                      \
}                                                                                          \
                                                                                           \
void ClassName::setValue(const hydrocouple_variant &data,                                   \
                          const std::initializer_list<int> &dimIndexes)                    \
{                                                                                          \
    ComponentDataItem1D<T>::setValueT(data, dimIndexes);                                   \
}                                                                                          \
                                                                                           \
void ClassName::setValues(const hydrocouple_variant *data,                                  \
                           const std::initializer_list<int> &dimIndexes,                   \
                           const std::initializer_list<int> &dimLengths)                   \
{                                                                                          \
    ComponentDataItem1D<T>::setValuesT(data, dimIndexes, dimLengths);                      \
}                                                                                          \
                                                                                           \
std::string ClassName::toString() const                                                    \
{                                                                                          \
    nlohmann::json arr = nlohmann::json::array();                                          \
    for (int i = 0; i < length(); ++i) arr.push_back((*this)[i]);                         \
    return arr.dump();                                                                     \
}                                                                                          \
                                                                                           \
void ClassName::saveData()                                                                 \
{                                                                                          \
    if (m_inputFile.empty()) return;                                                       \
    std::ofstream f(m_inputFile);                                                          \
    if (f) f << toString();                                                                \
}                                                                                          \
                                                                                           \
bool ClassName::initialize(const std::string &value,                                       \
                            HydroCouple::IArgument::ArgumentInputType argType,             \
                            std::string &message)                                          \
{                                                                                          \
    using AT = HydroCouple::IArgument::ArgumentInputType;                                  \
    try {                                                                                   \
        if (argType == AT::File || argType == AT::URL) {                                   \
            m_inputFile = value;                                                            \
            std::ifstream f(value);                                                        \
            if (!f) { message = "Cannot open file: " + value; return false; }             \
            std::ostringstream ss; ss << f.rdbuf();                                        \
            std::vector<T> parsed;                                                         \
            if (!parseJsonArray(ss.str(), parsed, message)) return false;                  \
            resizeDataArray((int)parsed.size());                                            \
            for (int i = 0; i < (int)parsed.size(); ++i) (*this)[i] = parsed[i];          \
        } else {                                                                            \
            std::vector<T> parsed;                                                          \
            if (!parseJsonArray(value, parsed, message)) return false;                      \
            resizeDataArray((int)parsed.size());                                            \
            for (int i = 0; i < (int)parsed.size(); ++i) (*this)[i] = parsed[i];          \
        }                                                                                   \
        setCurrentArgumentInputType(argType);                                               \
        return true;                                                                        \
    } catch (const std::exception &e) {                                                    \
        message = e.what(); return false;                                                   \
    }                                                                                      \
}                                                                                          \
                                                                                           \
bool ClassName::initialize(const HydroCouple::IComponentDataItem &src, std::string &msg)  \
{                                                                                          \
    if (src.dimensionLength() != length()) {                                               \
        resizeDataArray(src.dimensionLength());                                             \
    }                                                                                      \
    for (int i = 0; i < length(); ++i) {                                                   \
        hydrocouple_variant v;                                                              \
        src.getValue(v, {i});                                                               \
        (*this)[i] = std::get<T>(v);                                                       \
    }                                                                                      \
    msg.clear();                                                                            \
    return true;                                                                            \
}                                                                                          \
                                                                                           \
void ClassName::readData(const nlohmann::json &j)                                          \
{                                                                                          \
    AbstractArgument::readData(j);                                                          \
    if (j.contains("values")) {                                                             \
        auto &arr = j["values"];                                                            \
        resizeDataArray((int)arr.size());                                                   \
        for (int i = 0; i < (int)arr.size(); ++i) (*this)[i] = arr[i].get<T>();           \
    }                                                                                      \
}                                                                                          \
                                                                                           \
nlohmann::json ClassName::writeData() const                                                \
{                                                                                          \
    auto j = AbstractArgument::writeData();                                                \
    auto arr = nlohmann::json::array();                                                    \
    for (int i = 0; i < length(); ++i) arr.push_back((*this)[i]);                         \
    j["values"] = arr;                                                                     \
    return j;                                                                              \
}

IMPL_ARGUMENT1D(Argument1DInt,    int)
IMPL_ARGUMENT1D(Argument1DDouble, double)
IMPL_ARGUMENT1D(Argument1DString, std::string)

#undef IMPL_ARGUMENT1D

} // namespace HydroCouple
