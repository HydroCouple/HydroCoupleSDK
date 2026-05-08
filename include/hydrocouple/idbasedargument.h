// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   idbasedargument.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Concrete ID-based typed IArgument implementations.
 *
 * Each class combines AbstractArgument, IdBasedComponentDataItem<T>,
 * and IIdBasedComponentDataItem.
 */

#ifndef HYDROCOUPLESDK_IDBASEDARGUMENT_H
#define HYDROCOUPLESDK_IDBASEDARGUMENT_H

#include "abstractargument.h"
#include "idbasedcomponentdataitem.h"
#include "hydrocouple.h"

#include <filesystem>
#include <string>
#include <vector>

namespace HydroCouple {

#define HYDROCOUPLESDK_IDBASED_ARG(ClassName, T)                                          \
class HYDROCOUPLESDK_EXPORT ClassName                                                      \
    : public AbstractArgument,                                                             \
      public IdBasedComponentDataItem<T>,                                                  \
      public virtual HydroCouple::IIdBasedComponentDataItem                                \
{                                                                                          \
public:                                                                                    \
    using IComponentDataItem::getValue;                                                    \
    using IComponentDataItem::getValues;                                                   \
    using IComponentDataItem::setValue;                                                    \
    using IComponentDataItem::setValues;                                                   \
                                                                                           \
    ClassName(std::string_view id,                                                         \
              const std::vector<std::string> &identifiers,                                 \
              Dimension *identifierDimension,                                               \
              ValueDefinition *valueDef,                                                    \
              HydroCouple::IModelComponent *mc);                                           \
    ~ClassName() override = default;                                                       \
                                                                                           \
    /* IIdBasedComponentDataItem */                                                         \
    std::vector<std::string>   identifiers()        const override;                        \
    HydroCouple::IDimension   *identifierDimension() const override;                       \
                                                                                           \
    /* IIdBasedComponentDataItem typed get/set */                                           \
    void getValue(hydrocouple_variant &data, int idIndex,                                   \
                  const std::initializer_list<int> &dimIndexes = {}) const override;       \
    void getValues(hydrocouple_variant *data,                                               \
                   const std::initializer_list<int> &idIndexes,                            \
                   const std::initializer_list<int> &dimIndexes = {},                      \
                   const std::initializer_list<int> &dimLengths = {}) const override;      \
    void getValues(hydrocouple_variant *data, int idIndex,                                  \
                   const std::initializer_list<int> &dimIndexes = {},                      \
                   int idIndexLength = 1,                                                   \
                   const std::initializer_list<int> &dimLengths = {}) const override;      \
    void setValue(const hydrocouple_variant &data, int idIndex,                             \
                  const std::initializer_list<int> &dimIndexes = {}) override;             \
    void setValues(const hydrocouple_variant *data,                                         \
                   const std::initializer_list<int> &idIndexes,                            \
                   const std::initializer_list<int> &dimIndexes = {},                      \
                   const std::initializer_list<int> &dimLengths = {}) override;            \
    void setValues(const hydrocouple_variant *data, int idIndex,                            \
                   const std::initializer_list<int> &dimIndexes = {},                      \
                   int idIndexLength = 1,                                                   \
                   const std::initializer_list<int> &dimLengths = {}) override;            \
                                                                                           \
    /* IComponentDataItem (dispatches to id-based above) */                                \
    int  dimensionLength(const std::initializer_list<int> &dimIndexes = {}) const override;\
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
                                                                                           \
    /* IArgument */                                                                         \
    std::string toString() const override;                                                  \
    void saveData() override;                                                               \
    bool initialize(const std::string &value,                                               \
                    HydroCouple::IArgument::ArgumentInputType argType,                      \
                    std::string &message) override;                                         \
    bool initialize(const HydroCouple::IComponentDataItem &src,                            \
                    std::string &message) override;                                         \
    void readData(const nlohmann::json &j) override;                                        \
    nlohmann::json writeData() const override;                                              \
                                                                                           \
    bool matchIdentifiersWhenReading() const;                                               \
    void setMatchIdentifiersWhenReading(bool match);                                        \
                                                                                           \
private:                                                                                   \
    Dimension            *m_identifierDimension;                                           \
    std::filesystem::path m_inputFile;                                                     \
    bool                  m_matchIdentifiers = false;                                      \
}

HYDROCOUPLESDK_IDBASED_ARG(IdBasedArgumentInt,    int);
HYDROCOUPLESDK_IDBASED_ARG(IdBasedArgumentDouble, double);
HYDROCOUPLESDK_IDBASED_ARG(IdBasedArgumentString, std::string);

#undef HYDROCOUPLESDK_IDBASED_ARG

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_IDBASEDARGUMENT_H
