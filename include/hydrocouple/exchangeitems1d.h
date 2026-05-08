// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   exchangeitems1d.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Concrete 1-D typed IInput and IOutput implementations.
 *
 * Each class combines AbstractInput/AbstractOutput (for IExchangeItem semantics)
 * with ComponentDataItem1D<T> (for flat typed storage).
 *
 * canConsume() is left pure virtual in the Input variants so subclasses can
 * apply model-specific compatibility rules. A default no-op implementation
 * is provided for Output variants (outputs don't consume).
 */

#ifndef HYDROCOUPLESDK_EXCHANGEITEMS1D_H
#define HYDROCOUPLESDK_EXCHANGEITEMS1D_H

#include "abstractinput.h"
#include "abstractoutput.h"
#include "componentdataitem.h"

#include <string>

namespace HydroCouple {

// ── 1-D typed inputs ──────────────────────────────────────────────────────

#define HYDROCOUPLESDK_INPUT1D(ClassName, T)                                        \
class HYDROCOUPLESDK_EXPORT ClassName                                                \
    : public AbstractInput,                                                          \
      public ComponentDataItem1D<T>                                                  \
{                                                                                    \
public:                                                                              \
    ClassName(std::string_view id, Dimension *dimension, int length,                 \
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
    Dimension *m_dimension;                                                          \
}

HYDROCOUPLESDK_INPUT1D(Input1DInt,    int);
HYDROCOUPLESDK_INPUT1D(Input1DDouble, double);
HYDROCOUPLESDK_INPUT1D(Input1DString, std::string);

#undef HYDROCOUPLESDK_INPUT1D

// ── 1-D typed outputs ─────────────────────────────────────────────────────

#define HYDROCOUPLESDK_OUTPUT1D(ClassName, T)                                        \
class HYDROCOUPLESDK_EXPORT ClassName                                                 \
    : public AbstractOutput,                                                          \
      public ComponentDataItem1D<T>                                                   \
{                                                                                     \
public:                                                                               \
    ClassName(std::string_view id, Dimension *dimension, int length,                  \
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
    Dimension *m_dimension;                                                           \
}

HYDROCOUPLESDK_OUTPUT1D(Output1DInt,    int);
HYDROCOUPLESDK_OUTPUT1D(Output1DDouble, double);
HYDROCOUPLESDK_OUTPUT1D(Output1DString, std::string);

#undef HYDROCOUPLESDK_OUTPUT1D

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_EXCHANGEITEMS1D_H
