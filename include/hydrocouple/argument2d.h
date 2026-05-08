// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   argument2d.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Concrete 2-D typed IArgument implementations.
 *
 * Each class combines AbstractArgument with ComponentDataItem2D<T>
 * for 2D grid-based argument storage and JSON serialization.
 */

#ifndef HYDROCOUPLESDK_ARGUMENT2D_H
#define HYDROCOUPLESDK_ARGUMENT2D_H

#include "abstractargument.h"
#include "componentdataitem.h"

#include <string>

namespace HydroCouple {

#define HYDROCOUPLESDK_ARGUMENT2D(ClassName, T)                                     \
class HYDROCOUPLESDK_EXPORT ClassName                                                \
    : public AbstractArgument,                                                      \
      public ComponentDataItem2D<T>                                                  \
{                                                                                    \
public:                                                                              \
    ClassName(std::string_view id,                                                   \
              Dimension *dimensionX, Dimension *dimensionY,                          \
              int lengthX, int lengthY,                                              \
              ValueDefinition *valueDef,                                             \
              HydroCouple::IModelComponent *mc);                                     \
    ~ClassName() override = default;                                                 \
                                                                                    \
    int dimensionLength(const std::initializer_list<int> &dimIndexes = {}) const override;\
    void getValue(hydrocouple_variant &data,                                          \
                  const std::initializer_list<int> &dimIndexes) const override;      \
    void getValues(hydrocouple_variant *data,                                         \
                   const std::initializer_list<int> &dimIndexes,                     \
                   const std::initializer_list<int> &dimLengths = {}) const override;\
    void setValue(const hydrocouple_variant &data,                                    \
                  const std::initializer_list<int> &dimIndexes) override;            \
    void setValues(const hydrocouple_variant *data,                                   \
                   const std::initializer_list<int> &dimIndexes,                     \
                   const std::initializer_list<int> &dimLengths = {}) override;      \
                                                                                    \
    std::string toString() const override;                                           \
    void saveData() override;                                                        \
    bool initialize(const std::string &value,                                         \
                    HydroCouple::IArgument::ArgumentInputType argType,              \
                    std::string &message) override;                                  \
    bool initialize(const HydroCouple::IComponentDataItem &src,                      \
                    std::string &message) override;                                  \
    void readData(const nlohmann::json &j) override;                                 \
    nlohmann::json writeData() const override;                                       \
                                                                                    \
private:                                                                             \
    Dimension            *m_dimensionX, *m_dimensionY;                              \
    std::filesystem::path m_inputFile;                                              \
}

HYDROCOUPLESDK_ARGUMENT2D(Argument2DInt,    int);
HYDROCOUPLESDK_ARGUMENT2D(Argument2DDouble, double);
HYDROCOUPLESDK_ARGUMENT2D(Argument2DString, std::string);

#undef HYDROCOUPLESDK_ARGUMENT2D

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_ARGUMENT2D_H
