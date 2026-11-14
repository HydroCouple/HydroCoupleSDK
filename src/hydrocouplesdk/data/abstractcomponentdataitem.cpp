// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

namespace HydroCouple::SDK {

// ── ComponentDataItemValueChanged ─────────────────────────────────────────

ComponentDataItemValueChanged::ComponentDataItemValueChanged(
    HydroCouple::IComponentDataItem *item,
    std::vector<int64_t> start,
    std::vector<int64_t> count)
    : m_item(item)
    , m_start(std::move(start))
    , m_count(std::move(count))
{
}

HydroCouple::IComponentDataItem *ComponentDataItemValueChanged::componentDataItem() const
{
    return m_item;
}

std::vector<int64_t> ComponentDataItemValueChanged::start() const
{
    return m_start;
}

std::vector<int64_t> ComponentDataItemValueChanged::count() const
{
    return m_count;
}


// ── AbstractComponentDataItem ─────────────────────────────────────────────

AbstractComponentDataItem::AbstractComponentDataItem(
    std::string_view id,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : Identity(id)
    , m_dimensions(dimensions)
    , m_valueDefinition(valueDefinition)
    , m_modelComponent(modelComponent)
{
}

AbstractComponentDataItem::AbstractComponentDataItem(
    std::string_view id,
    std::string_view caption,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : Identity(id, caption)
    , m_dimensions(dimensions)
    , m_valueDefinition(valueDefinition)
    , m_modelComponent(modelComponent)
{
}

HydroCouple::IModelComponent *AbstractComponentDataItem::modelComponent() const
{
    return m_modelComponent;
}

std::vector<HydroCouple::IDimension *> AbstractComponentDataItem::dimensions() const
{
    std::vector<HydroCouple::IDimension *> result;
    result.reserve(m_dimensions.size());
    for (auto *d : m_dimensions)
        result.push_back(d);
    return result;
}

HydroCouple::IValueDefinition *AbstractComponentDataItem::valueDefinition() const
{
    return m_valueDefinition;
}

// ── ISignal<const shared_ptr<IComponentDataItemValueChanged>&> ────────────

void AbstractComponentDataItem::connect(
    const std::shared_ptr<ISlot<const std::shared_ptr<IComponentDataItemValueChanged> &>> &slot)
{
    m_valueChanged.connect(slot);
}

void AbstractComponentDataItem::disconnect(
    const std::shared_ptr<ISlot<const std::shared_ptr<IComponentDataItemValueChanged> &>> &slot)
{
    m_valueChanged.disconnect(slot);
}

void AbstractComponentDataItem::blockSignals(bool block)
{
    Description::blockSignals(block);
    m_valueChanged.blockSignals(block);
}

void AbstractComponentDataItem::emit(
    const std::shared_ptr<IComponentDataItemValueChanged> &args)
{
    m_valueChanged.emit(args);
}

// ── Comments ──────────────────────────────────────────────────────────────

std::vector<std::string> AbstractComponentDataItem::comments() const
{
    return m_comments;
}

void AbstractComponentDataItem::setComments(const std::vector<std::string> &comments)
{
    m_comments = comments;
    emitPropertyChanged("Comments");
}

void AbstractComponentDataItem::addComment(const std::string &comment)
{
    m_comments.push_back(comment);
    emitPropertyChanged("Comments");
}

// ── Serialisation ─────────────────────────────────────────────────────────

void AbstractComponentDataItem::readData(const nlohmann::json &j)
{
    if (j.contains("caption"))
        setCaption(j["caption"].get<std::string>());
    if (j.contains("description"))
        setDescription(j["description"].get<std::string>());
    if (j.contains("comments")) {
        m_comments.clear();
        for (const auto &c : j["comments"])
            m_comments.push_back(c.get<std::string>());
    }
}

nlohmann::json AbstractComponentDataItem::writeData() const
{
    nlohmann::json j;
    j["caption"]     = caption();
    j["description"] = description();
    auto arr = nlohmann::json::array();
    for (const auto &c : m_comments)
        arr.push_back(c);
    j["comments"] = arr;
    return j;
}

// ── Protected internal accessors ─────────────────────────────────────────

std::vector<Dimension *> AbstractComponentDataItem::dimensionsInternal() const
{
    return m_dimensions;
}

ValueDefinition *AbstractComponentDataItem::valueDefinitionInternal() const
{
    return m_valueDefinition;
}

HydroCouple::IModelComponent *AbstractComponentDataItem::modelComponentInternal() const
{
    return m_modelComponent;
}

void AbstractComponentDataItem::emitValueChanged(
    const std::shared_ptr<IComponentDataItemValueChanged> &args)
{
    m_valueChanged.emit(args);
}

void AbstractComponentDataItem::emitValueChanged(std::vector<int64_t> start,
                                                  std::vector<int64_t> count)
{
    auto args = std::make_shared<ComponentDataItemValueChanged>(
        this, std::move(start), std::move(count));
    m_valueChanged.emit(args);
}

} // namespace HydroCouple::SDK
