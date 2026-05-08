// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   argument1d.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Concrete 1-D typed IArgument implementations.
 *
 * Each class combines AbstractArgument with ComponentDataItem1D<T>.
 * initialize(string, argType, message) parses CSV / JSON / file as appropriate.
 * saveData() writes the current values back to the associated file if one was
 * set via File or URL initialisation.
 */

#ifndef HYDROCOUPLESDK_ARGUMENT1D_H
#define HYDROCOUPLESDK_ARGUMENT1D_H

#include "abstractargument.h"
#include "componentdataitem.h"

#include <filesystem>
#include <string>

namespace HydroCouple {

#define HYDROCOUPLESDK_ARGUMENT1D(ClassName, T)                                           \
class HYDROCOUPLESDK_EXPORT ClassName                                                      \
    : public AbstractArgument,                                                             \
      public ComponentDataItem1D<T>                                                        \
{                                                                                          \
public:                                                                                    \
    ClassName(std::string_view id, Dimension *dimension, int length,                       \
              ValueDefinition *valueDef, HydroCouple::IModelComponent *mc);                \
    ~ClassName() override = default;                                                       \
    int dimensionLength(const std::initializer_list<int> &dimIndexes = {}) const override; \
    void getValue(hydrocouple_variant &data,                                                \
                  const std::initializer_list<int> &dimIndexes) const override;            \
    void getValues(hydrocouple_variant *data,                                               \
                   const std::initializer_list<int> &dimIndexes,                           \
                   const std::initializer_list<int> &dimLengths = {}) const override;      \
    void setValue(const hydrocouple_variant &data,                                          \
                  const std::initializer_list<int> &dimIndexes) override;                  \
    void setValues(const hydrocouple_variant *data,                                         \
                   const std::initializer_list<int> &dimIndexes,                           \
                   const std::initializer_list<int> &dimLengths = {}) override;            \
    std::string toString() const override;                                                  \
    void saveData() override;                                                               \
    bool initialize(const std::string &value,                                               \
                    HydroCouple::IArgument::ArgumentInputType argType,                      \
                    std::string &message) override;                                         \
    bool initialize(const HydroCouple::IComponentDataItem &componentDataItem,               \
                    std::string &message) override;                                         \
    void readData(const nlohmann::json &j) override;                                        \
    nlohmann::json writeData() const override;                                              \
private:                                                                                    \
    Dimension              *m_dimension;                                                    \
    std::filesystem::path   m_inputFile;                                                    \
}

HYDROCOUPLESDK_ARGUMENT1D(Argument1DInt,    int);
HYDROCOUPLESDK_ARGUMENT1D(Argument1DDouble, double);
HYDROCOUPLESDK_ARGUMENT1D(Argument1DString, std::string);

#undef HYDROCOUPLESDK_ARGUMENT1D

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_ARGUMENT1D_H
