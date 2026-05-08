// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/dimension.h"

namespace HydroCouple {

Dimension::Dimension(std::string_view id)
    : Identity(id)
{
}

Dimension::Dimension(std::string_view id, std::string_view caption)
    : Identity(id, caption)
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

Dimension *Dimension::copy(const HydroCouple::IDimension *src)
{
    if (!src) return nullptr;
    auto *d = new Dimension(src->id(), src->caption());
    d->setDescription(src->description());
    d->setLengthType(src->lengthType());
    return d;
}

Dimension *Dimension::copy() const
{
    return Dimension::copy(this);
}

} // namespace HydroCouple
