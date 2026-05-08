// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   exchangeitems2d.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Concrete 2-D typed IInput and IOutput implementations.
 *
 * Each class combines AbstractInput/AbstractOutput (for IExchangeItem semantics)
 * with ComponentDataItem2D<T> (for 2D grid-based typed storage).
 *
 * canConsume() is left pure virtual in the Input variants so subclasses can
 * apply model-specific compatibility rules. A default no-op implementation
 * is provided for Output variants (outputs don't consume).
 */

#ifndef HYDROCOUPLESDK_EXCHANGEITEMS2D_H
#define HYDROCOUPLESDK_EXCHANGEITEMS2D_H

#include "abstractinput.h"
#include "abstractoutput.h"
#include "componentdataitem.h"

#include <string>

namespace HydroCouple {

// ── 2-D typed inputs ──────────────────────────────────────────────────────

#define HYDROCOUPLESDK_INPUT2D(ClassName, T)                                        \
class HYDROCOUPLESDK_EXPORT ClassName                                                \
    : public AbstractInput,                                                          \
      public ComponentDataItem2D<T>                                                  \
{                                                                                    \
public:                                                                              \
    ClassName(std::string_view id, Dimension *dimensionX, Dimension *dimensionY,    \
              int lengthX, int lengthY,                                              \
              ValueDefinition *valueDef, HydroCouple::IModelComponent *mc);          \
    ~ClassName() override = default;                                                 \
    int dimensionLength(const std::initializer_list<int> &dimIndexes = {}) const override; \
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
    void readData(const nlohmann::json &j) override;                                 \
    nlohmann::json writeData() const override;                                       \
private:                                                                             \
    Dimension *m_dimensionX, *m_dimensionY;                                         \
}

HYDROCOUPLESDK_INPUT2D(Input2DInt,    int);
HYDROCOUPLESDK_INPUT2D(Input2DDouble, double);
HYDROCOUPLESDK_INPUT2D(Input2DString, std::string);

#undef HYDROCOUPLESDK_INPUT2D

// ── 2-D typed outputs ─────────────────────────────────────────────────────

#define HYDROCOUPLESDK_OUTPUT2D(ClassName, T)                                        \
class HYDROCOUPLESDK_EXPORT ClassName                                                 \
    : public AbstractOutput,                                                          \
      public ComponentDataItem2D<T>                                                   \
{                                                                                     \
public:                                                                               \
    ClassName(std::string_view id, Dimension *dimensionX, Dimension *dimensionY,     \
              int lengthX, int lengthY,                                               \
              ValueDefinition *valueDef, HydroCouple::IModelComponent *mc);           \
    ~ClassName() override = default;                                                  \
    int dimensionLength(const std::initializer_list<int> &dimIndexes = {}) const override;\
    void getValue(hydrocouple_variant &data,                                           \
                  const std::initializer_list<int> &dimIndexes) const override;       \
    void getValues(hydrocouple_variant *data,                                          \
                   const std::initializer_list<int> &dimIndexes,                      \
                   const std::initializer_list<int> &dimLengths = {}) const override; \
    void setValue(const hydrocouple_variant &data,                                     \
                  const std::initializer_list<int> &dimIndexes) override;             \
    void setValues(const hydrocouple_variant *data,                                    \
                   const std::initializer_list<int> &dimIndexes,                      \
                   const std::initializer_list<int> &dimLengths = {}) override;       \
    void updateValues() override {}                                                    \
    void readData(const nlohmann::json &j) override;                                  \
    nlohmann::json writeData() const override;                                        \
private:                                                                              \
    Dimension *m_dimensionX, *m_dimensionY;                                         \
}

HYDROCOUPLESDK_OUTPUT2D(Output2DInt,    int);
HYDROCOUPLESDK_OUTPUT2D(Output2DDouble, double);
HYDROCOUPLESDK_OUTPUT2D(Output2DString, std::string);

#undef HYDROCOUPLESDK_OUTPUT2D

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_EXCHANGEITEMS2D_H
