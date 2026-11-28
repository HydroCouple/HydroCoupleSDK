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

HydroCouple::ValueKind ValueDefinition::valueKind() const
{
    return m_valueKind;
}

void ValueDefinition::setValueKind(HydroCouple::ValueKind kind)
{
    m_valueKind = kind;
    emitPropertyChanged("ValueKind");
}

std::string ValueDefinition::standardName() const
{
    return m_standardName;
}

void ValueDefinition::setStandardName(std::string_view name)
{
    m_standardName = name;
    emitPropertyChanged("StandardName");
}

std::string ValueDefinition::conceptURI() const
{
    return m_conceptURI;
}

void ValueDefinition::setConceptURI(std::string_view uri)
{
    m_conceptURI = uri;
    emitPropertyChanged("ConceptURI");
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
    j["valueKind"]    = static_cast<int>(m_valueKind);
    // Written only when declared, so documents of undeclared definitions
    // read exactly as before.
    if (!m_standardName.empty())
        j["standardName"] = m_standardName;
    if (!m_conceptURI.empty())
        j["conceptURI"] = m_conceptURI;
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
    if (j.contains("valueKind"))
        m_valueKind = static_cast<HydroCouple::ValueKind>(j["valueKind"].get<int>());
    if (j.contains("standardName"))
        m_standardName = j["standardName"].get<std::string>();
    if (j.contains("conceptURI"))
        m_conceptURI = j["conceptURI"].get<std::string>();
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
    const auto *sdk = dynamic_cast<const ValueDefinition *>(src);
    auto *result = new ValueDefinition(sdk ? sdk->type() : typeid(double));
    result->setCaption(src->caption());
    result->setDescription(src->description());
    result->setMissingValue(src->missingValue());
    result->setDefaultValue(src->defaultValue());
    result->setValueKind(src->valueKind());
    result->setStandardName(src->standardName());
    result->setConceptURI(src->conceptURI());
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
    result->setValueKind(src->valueKind());
    result->setStandardName(src->standardName());
    result->setConceptURI(src->conceptURI());
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
        if (!m_unit->symbol().empty())
            j["unit"]["symbol"] = m_unit->symbol();
        if (!m_unit->ucumCode().empty())
            j["unit"]["ucumCode"] = m_unit->ucumCode();
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

namespace {

//! Declares a factory quantity's QUDT quantity kind as its concept URI. The
//! kind is what these generic quantities honestly know; a component that
//! means something narrower (channel discharge, not any volume flow rate)
//! sets its own conceptURI() and standardName().
//! \param[in,out] quantity          Quantity to label; must not be null.
//! \param[in]     qudtQuantityKind  QUDT quantity-kind local name, appended
//!                                  to http://qudt.org/vocab/quantitykind/.
//! \return \p quantity, for chaining.
Quantity *ofKind(Quantity *quantity, const char *qudtQuantityKind)
{
    quantity->setConceptURI(std::string("http://qudt.org/vocab/quantitykind/") +
                            qudtQuantityKind);
    return quantity;
}

} // namespace

Quantity *Quantity::flowInCMS()
{
    return ofKind(new Quantity("Flow (m³/s)", typeid(double), Unit::flowInCMS()),
                  "VolumeFlowRate");
}

Quantity *Quantity::flowInCFS()
{
    return ofKind(new Quantity("Flow (ft³/s)", typeid(double), Unit::flowInCFS()),
                  "VolumeFlowRate");
}

Quantity *Quantity::lengthInMeters()
{
    return ofKind(new Quantity("Length (m)", typeid(double), Unit::lengthInMeters()),
                  "Length");
}

Quantity *Quantity::lengthInFeet()
{
    return ofKind(new Quantity("Length (ft)", typeid(double), Unit::lengthInFeet()),
                  "Length");
}

Quantity *Quantity::areaInSquareMeters()
{
    return ofKind(new Quantity("Area (m²)", typeid(double), Unit::areaInSquareMeters()),
                  "Area");
}

Quantity *Quantity::volumeInCubicMeters()
{
    return ofKind(new Quantity("Volume (m³)", typeid(double), Unit::volumeInCubicMeters()),
                  "Volume");
}

Quantity *Quantity::timeInDays()
{
    return ofKind(new Quantity("Time (days)", typeid(double), Unit::timeInDays()), "Time");
}

Quantity *Quantity::timeInSeconds()
{
    return ofKind(new Quantity("Time (s)", typeid(double), Unit::timeInSeconds()), "Time");
}

Quantity *Quantity::unitLess(std::string_view caption)
{
    return new Quantity(caption, typeid(double), Unit::unitlessCoefficient());
}

Quantity *Quantity::copy(const HydroCouple::IQuantity *src)
{
    if (!src)
        return nullptr;
    const auto *sdk = dynamic_cast<const ValueDefinition *>(src);
    auto *result = new Quantity(sdk ? sdk->type() : typeid(double), Unit::copy(src->unit()));
    result->setCaption(src->caption());
    result->setDescription(src->description());
    result->setMissingValue(src->missingValue());
    result->setDefaultValue(src->defaultValue());
    result->setValueKind(src->valueKind());
    result->setStandardName(src->standardName());
    result->setConceptURI(src->conceptURI());
    result->setMinValue(src->minValue());
    result->setMaxValue(src->maxValue());
    return result;
}

} // namespace HydroCouple::SDK
