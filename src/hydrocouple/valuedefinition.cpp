// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/valuedefinition.h"
#include "hydrocouple/unit.h"

#include <format>
#include <stdexcept>

namespace HydroCouple {

// ── ValueDefinition ───────────────────────────────────────────────────────

ValueDefinition::ValueDefinition(const std::type_info &type)
    : m_type(&type)
{
}

ValueDefinition::ValueDefinition(std::string_view caption, const std::type_info &type)
    : Description(caption),
      m_type(&type)
{
}

const std::type_info &ValueDefinition::type() const
{
    return *m_type;
}

hydrocouple_variant ValueDefinition::missingValue() const
{
    return m_missingValue;
}

void ValueDefinition::setMissingValue(const hydrocouple_variant &value)
{
    m_missingValue = value;
    emitPropertyChanged("MissingValue");
}

hydrocouple_variant ValueDefinition::defaultValue() const
{
    return m_defaultValue;
}

void ValueDefinition::setDefaultValue(const hydrocouple_variant &value)
{
    m_defaultValue = value;
    emitPropertyChanged("DefaultValue");
}

nlohmann::json ValueDefinition::toJson() const
{
    return {
        {"caption",      m_caption},
        {"description",  m_description},
        {"type",         m_type->name()},
    };
}

void ValueDefinition::fromJson(const nlohmann::json &j)
{
    if (j.contains("caption"))
        m_caption = j["caption"].get<std::string>();
    if (j.contains("description"))
        m_description = j["description"].get<std::string>();
}

std::string ValueDefinition::serializeVariant(const hydrocouple_variant &value)
{
    return std::visit([](const auto &v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>)
            return v;
        else if constexpr (std::is_arithmetic_v<T>)
            return std::to_string(v);
        else
            return {};
    }, value);
}

hydrocouple_variant ValueDefinition::deserializeVariant(
    std::string_view text, const std::type_info &type)
{
    if (type == typeid(double))       return std::stod(std::string(text));
    if (type == typeid(float))        return static_cast<float>(std::stod(std::string(text)));
    if (type == typeid(int))          return std::stoi(std::string(text));
    if (type == typeid(long))         return std::stol(std::string(text));
    if (type == typeid(bool))         return text == "1" || text == "true";
    if (type == typeid(std::string))  return std::string(text);
    return {};
}

bool ValueDefinition::validateValue(const hydrocouple_variant &value) const
{
    return std::visit([&](const auto &v) -> bool {
        return typeid(v) == *m_type;
    }, value);
}

ValueDefinition *ValueDefinition::copy(const HydroCouple::IValueDefinition *src)
{
    if (!src) return nullptr;
    if (auto *q = dynamic_cast<const HydroCouple::IQuality *>(src))
        return Quality::copy(q);
    if (auto *q = dynamic_cast<const HydroCouple::IQuantity *>(src))
        return Quantity::copy(q);
    return nullptr;
}


// ── Quality ───────────────────────────────────────────────────────────────

Quality::Quality(const std::type_info &type,
                 const std::set<hydrocouple_variant> &categories)
    : ValueDefinition(type),
      m_categories(categories)
{
}

Quality::Quality(std::string_view caption,
                 const std::type_info &type,
                 const std::set<hydrocouple_variant> &categories)
    : ValueDefinition(caption, type),
      m_categories(categories)
{
}

std::set<hydrocouple_variant> Quality::categories() const
{
    return m_categories;
}

void Quality::setCategories(const std::set<hydrocouple_variant> &categories)
{
    m_categories = categories;
    emitPropertyChanged("Categories");
}

void Quality::addCategory(const hydrocouple_variant &category)
{
    m_categories.insert(category);
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
    auto j = ValueDefinition::toJson();
    j["isOrdered"] = m_isOrdered;
    auto cats = nlohmann::json::array();
    for (const auto &c : m_categories)
        cats.push_back(serializeVariant(c));
    j["categories"] = cats;
    return j;
}

void Quality::fromJson(const nlohmann::json &j)
{
    ValueDefinition::fromJson(j);
    if (j.contains("isOrdered"))
        m_isOrdered = j["isOrdered"].get<bool>();
    if (j.contains("categories")) {
        m_categories.clear();
        for (const auto &c : j["categories"])
            m_categories.insert(deserializeVariant(c.get<std::string>(), *m_type));
    }
}

Quality *Quality::copy(const HydroCouple::IQuality *src)
{
    if (!src) return nullptr;
    auto *q = new Quality(src->type(), src->categories());
    q->setCaption(src->caption());
    q->setDescription(src->description());
    q->setIsOrdered(src->isOrdered());
    q->setMissingValue(src->missingValue());
    q->setDefaultValue(src->defaultValue());
    return q;
}


// ── Quantity ──────────────────────────────────────────────────────────────

Quantity::Quantity(const std::type_info &type, Unit *unit)
    : ValueDefinition(type),
      m_unit(unit)
{
}

Quantity::Quantity(std::string_view caption, const std::type_info &type, Unit *unit)
    : ValueDefinition(caption, type),
      m_unit(unit)
{
}

HydroCouple::IUnit *Quantity::unit() const
{
    return m_unit;
}

hydrocouple_variant Quantity::minValue() const
{
    return m_minValue;
}

void Quantity::setMinValue(const hydrocouple_variant &min)
{
    m_minValue = min;
    emitPropertyChanged("MinValue");
}

hydrocouple_variant Quantity::maxValue() const
{
    return m_maxValue;
}

void Quantity::setMaxValue(const hydrocouple_variant &max)
{
    m_maxValue = max;
    emitPropertyChanged("MaxValue");
}

nlohmann::json Quantity::toJson() const
{
    auto j = ValueDefinition::toJson();
    j["minValue"] = serializeVariant(m_minValue);
    j["maxValue"] = serializeVariant(m_maxValue);
    return j;
}

void Quantity::fromJson(const nlohmann::json &j)
{
    ValueDefinition::fromJson(j);
    if (j.contains("minValue"))
        m_minValue = deserializeVariant(j["minValue"].get<std::string>(), *m_type);
    if (j.contains("maxValue"))
        m_maxValue = deserializeVariant(j["maxValue"].get<std::string>(), *m_type);
}

// Static factory helpers — units are created with default construction; callers
// may replace them with richer Unit objects after the fact.
Quantity *Quantity::flowInCMS()         { return new Quantity("Flow (m³/s)",  typeid(double), nullptr); }
Quantity *Quantity::flowInCFS()         { return new Quantity("Flow (ft³/s)", typeid(double), nullptr); }
Quantity *Quantity::lengthInMeters()    { return new Quantity("Length (m)",   typeid(double), nullptr); }
Quantity *Quantity::lengthInFeet()      { return new Quantity("Length (ft)",  typeid(double), nullptr); }
Quantity *Quantity::areaInSquareMeters(){ return new Quantity("Area (m²)",    typeid(double), nullptr); }
Quantity *Quantity::volumeInCubicMeters(){ return new Quantity("Volume (m³)", typeid(double), nullptr); }
Quantity *Quantity::timeInDays()        { return new Quantity("Time (days)",  typeid(double), nullptr); }
Quantity *Quantity::timeInSeconds()     { return new Quantity("Time (s)",     typeid(double), nullptr); }
Quantity *Quantity::unitLess(std::string_view caption)
{
    return new Quantity(caption, typeid(double), nullptr);
}

Quantity *Quantity::copy(const HydroCouple::IQuantity *src)
{
    if (!src) return nullptr;
    auto *q = new Quantity(src->type(), nullptr);
    q->setCaption(src->caption());
    q->setDescription(src->description());
    q->setMinValue(src->minValue());
    q->setMaxValue(src->maxValue());
    q->setMissingValue(src->missingValue());
    q->setDefaultValue(src->defaultValue());
    return q;
}

} // namespace HydroCouple
