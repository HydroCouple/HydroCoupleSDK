// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/core/dimension.h"

namespace HydroCouple::SDK {

namespace {

// The SDK's own data items name their axes with a small, stable vocabulary;
// mapping those names to IDimension::DimensionRole here means every item gets
// a correct role() without each construction site repeating it. Anything
// else stays Unknown until setRole() is called.
//
// id: the dimension id, compared exactly and case-sensitively.
// Returns the role for a well-known id, otherwise DimensionRole::Unknown.
HydroCouple::IDimension::DimensionRole roleForId(std::string_view id)
{
    using Role = HydroCouple::IDimension::DimensionRole;
    if (id == "time" || id == "times")                                   return Role::Time;
    if (id == "layers" || id == "layer")                                 return Role::Layer;
    if (id == "band" || id == "bands")                                   return Role::Band;
    if (id == "y_cells" || id == "rows" || id == "row" || id == "y")     return Role::Row;
    if (id == "x_cells" || id == "columns" || id == "column" || id == "x") return Role::Column;
    if (id == "z_cells" || id == "z")                                    return Role::Depth;
    if (id == "coordinate" || id == "component" || id == "components")   return Role::Component;
    if (id == "identifiers" || id == "identifier" || id == "geometries" ||
        id == "nodes" || id == "node" || id == "edges" || id == "edge" ||
        id == "faces" || id == "face" || id == "volumes" || id == "patches" ||
        id == "vertices" || id == "entities" || id == "element" || id == "elements")
        return Role::Entity;
    return Role::Unknown;
}

} // namespace

Dimension::Dimension(std::string_view id)
    : Identity(id), m_role(roleForId(id))
{
}

Dimension::Dimension(std::string_view id, std::string_view caption)
    : Identity(id, caption), m_role(roleForId(id))
{
}

HydroCouple::IDimension::LengthType Dimension::lengthType() const
{
    return m_lengthType;
}

void Dimension::setLengthType(HydroCouple::IDimension::LengthType lt)
{
    m_lengthType = lt;
    emitPropertyChanged("LengthType");
}

HydroCouple::IDimension::DimensionRole Dimension::role() const
{
    return m_role;
}

void Dimension::setRole(HydroCouple::IDimension::DimensionRole role)
{
    m_role = role;
    emitPropertyChanged("Role");
}

Dimension *Dimension::copy(const HydroCouple::IDimension *src)
{
    if (!src) return nullptr;
    auto *d = new Dimension(src->id(), src->caption());
    d->setDescription(src->description());
    d->setLengthType(src->lengthType());
    d->setRole(src->role());
    return d;
}

Dimension *Dimension::copy() const
{
    return Dimension::copy(this);
}

} // namespace HydroCouple::SDK
