// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/core/unit.h"

namespace HydroCouple::SDK {

// ── ValueDefinition ───────────────────────────────────────────────────────

ValueDefinition::ValueDefinition(const std::type_info &type)
    : m_type(&type)
{
}

ValueDefinition::ValueDefinition(std::string_view caption,
                                 const std::type_info &type)
    : Description(caption), m_type(&type)
{
}

const std::type_info &ValueDefinition::type() const
{
    return *m_type;
}

double ValueDefinition::missingValue() const
{
    return m_missingValue;
}

void ValueDefinition::setMissingValue(double value)
{
    m_missingValue = value;
    emitPropertyChanged("MissingValue");
}

double ValueDefinition::defaultValue() const
{
    return m_defaultValue;
}

void ValueDefinition::setDefaultValue(double value)
{
    m_defaultValue = value;
    emitPropertyChanged("DefaultValue");
}

nlohmann::json ValueDefinition::toJson() const
{
    nlohmann::json j;
    j["caption"]      = caption();
    j["description"]  = description();
    j["missingValue"] = m_missingValue;
    j["defaultValue"] = m_defaultValue;
    return j;
}

void ValueDefinition::fromJson(const nlohmann::json &j)
{
    if (j.contains("caption"))
        setCaption(j["caption"].get<std::string>());
    if (j.contains("description"))
        setDescription(j["description"].get<std::string>());
    if (j.contains("missingValue"))
        m_missingValue = j["missingValue"].get<double>();
    if (j.contains("defaultValue"))
        m_defaultValue = j["defaultValue"].get<double>();
}

DataKind ValueDefinition::dataKindForType(const std::type_info &type)
{
    if (type == typeid(double))       return DataKind::Float64;
    if (type == typeid(float))        return DataKind::Float32;
    if (type == typeid(int32_t))      return DataKind::Int32;
    if (type == typeid(uint32_t))     return DataKind::UInt32;
    if (type == typeid(int64_t))      return DataKind::Int64;
    if (type == typeid(uint64_t))     return DataKind::UInt64;
    if (type == typeid(int16_t))      return DataKind::Int16;
    if (type == typeid(uint16_t))     return DataKind::UInt16;
    if (type == typeid(int8_t))       return DataKind::Int8;
    if (type == typeid(uint8_t))      return DataKind::UInt8;
    if (type == typeid(bool))         return DataKind::Boolean;
    if (type == typeid(std::string))  return DataKind::String;
    return DataKind::Opaque;
}

ValueDefinition *ValueDefinition::copy(const HydroCouple::IValueDefinition *src)
{
    if (!src)
        return nullptr;
    auto *result = new ValueDefinition(src->type());
    result->setCaption(src->caption());
    result->setDescription(src->description());
    result->setMissingValue(src->missingValue());
    result->setDefaultValue(src->defaultValue());
    return result;
}

// ── Quality ───────────────────────────────────────────────────────────────

Quality::Quality(const std::vector<std::string> &categories)
    : ValueDefinition(typeid(int32_t)), m_categories(categories)
{
}

Quality::Quality(std::string_view caption,
                 const std::vector<std::string> &categories)
    : ValueDefinition(caption, typeid(int32_t)), m_categories(categories)
{
}

std::vector<std::string> Quality::categories() const
{
    return m_categories;
}

void Quality::setCategories(const std::vector<std::string> &categories)
{
    m_categories = categories;
    emitPropertyChanged("Categories");
}

void Quality::addCategory(const std::string &category)
{
    m_categories.push_back(category);
    emitPropertyChanged("Categories");
}

bool Quality::isOrdered() const
{
    return m_isOrdered;
}

void Quality::setIsOrdered(bool ordered)
{
    m_isOrdered = ordered;
    emitPropertyChanged("IsOrdered");
}

nlohmann::json Quality::toJson() const
{
    nlohmann::json j = ValueDefinition::toJson();
    j["categories"] = m_categories;
    j["isOrdered"]  = m_isOrdered;
    return j;
}

void Quality::fromJson(const nlohmann::json &j)
{
    ValueDefinition::fromJson(j);
    if (j.contains("categories"))
        m_categories = j["categories"].get<std::vector<std::string>>();
    if (j.contains("isOrdered"))
        m_isOrdered = j["isOrdered"].get<bool>();
}

Quality *Quality::copy(const HydroCouple::IQuality *src)
{
    if (!src)
        return nullptr;
    auto *result = new Quality(src->categories());
    result->setCaption(src->caption());
    result->setDescription(src->description());
    result->setMissingValue(src->missingValue());
    result->setDefaultValue(src->defaultValue());
    result->setIsOrdered(src->isOrdered());
    return result;
}

// ── Quantity ──────────────────────────────────────────────────────────────

Quantity::Quantity(const std::type_info &type, Unit *unit)
    : ValueDefinition(type), m_unit(unit)
{
}

Quantity::Quantity(std::string_view caption, const std::type_info &type,
                   Unit *unit)
    : ValueDefinition(caption, type), m_unit(unit)
{
}

HydroCouple::IUnit *Quantity::unit() const
{
    return m_unit;
}

double Quantity::minValue() const
{
    return m_minValue;
}

void Quantity::setMinValue(double min)
{
    m_minValue = min;
    emitPropertyChanged("MinValue");
}

double Quantity::maxValue() const
{
    return m_maxValue;
}

void Quantity::setMaxValue(double max)
{
    m_maxValue = max;
    emitPropertyChanged("MaxValue");
}

nlohmann::json Quantity::toJson() const
{
    nlohmann::json j = ValueDefinition::toJson();
    j["minValue"] = m_minValue;
    j["maxValue"] = m_maxValue;
    if (m_unit)
    {
        j["unit"] = {
            {"caption", m_unit->caption()},
            {"conversionFactorToSI", m_unit->conversionFactorToSI()},
            {"offsetToSI", m_unit->offsetToSI()},
        };
    }
    return j;
}

void Quantity::fromJson(const nlohmann::json &j)
{
    ValueDefinition::fromJson(j);
    if (j.contains("minValue"))
        m_minValue = j["minValue"].get<double>();
    if (j.contains("maxValue"))
        m_maxValue = j["maxValue"].get<double>();
}

// ── Factory helpers ───────────────────────────────────────────────────────

Quantity *Quantity::flowInCMS()
{
    return new Quantity("Flow (m³/s)", typeid(double), Unit::flowInCMS());
}

Quantity *Quantity::flowInCFS()
{
    return new Quantity("Flow (ft³/s)", typeid(double), Unit::flowInCFS());
}

Quantity *Quantity::lengthInMeters()
{
    return new Quantity("Length (m)", typeid(double), Unit::lengthInMeters());
}

Quantity *Quantity::lengthInFeet()
{
    return new Quantity("Length (ft)", typeid(double), Unit::lengthInFeet());
}

Quantity *Quantity::areaInSquareMeters()
{
    return new Quantity("Area (m²)", typeid(double), Unit::areaInSquareMeters());
}

Quantity *Quantity::volumeInCubicMeters()
{
    return new Quantity("Volume (m³)", typeid(double), Unit::volumeInCubicMeters());
}

Quantity *Quantity::timeInDays()
{
    return new Quantity("Time (days)", typeid(double), Unit::timeInDays());
}

Quantity *Quantity::timeInSeconds()
{
    return new Quantity("Time (s)", typeid(double), Unit::timeInSeconds());
}

Quantity *Quantity::unitLess(std::string_view caption)
{
    return new Quantity(caption, typeid(double), Unit::unitlessCoefficient());
}

Quantity *Quantity::copy(const HydroCouple::IQuantity *src)
{
    if (!src)
        return nullptr;
    auto *result = new Quantity(src->type(), Unit::copy(src->unit()));
    result->setCaption(src->caption());
    result->setDescription(src->description());
    result->setMissingValue(src->missingValue());
    result->setDefaultValue(src->defaultValue());
    result->setMinValue(src->minValue());
    result->setMaxValue(src->maxValue());
    return result;
}

} // namespace HydroCouple::SDK
