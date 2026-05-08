// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/identity.h"

namespace HydroCouple {

Identity::Identity()
    : m_id(generateUUID())
{
}

Identity::Identity(std::string_view id)
    : m_id(id.empty() ? generateUUID() : std::string(id))
{
}

Identity::Identity(std::string_view id, std::string_view caption)
    : Description(caption),
      m_id(id.empty() ? generateUUID() : std::string(id))
{
}

Identity::Identity(std::string_view id,
                   std::string_view caption,
                   std::string_view description)
    : Description(caption, description),
      m_id(id.empty() ? generateUUID() : std::string(id))
{
}

std::string Identity::id() const
{
    return m_id;
}

void Identity::setId(std::string_view id)
{
    if (id.empty()) return;
    m_id = id;
    emitPropertyChanged("Id");
}

} // namespace HydroCouple
