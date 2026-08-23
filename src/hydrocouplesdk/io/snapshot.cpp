// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/io/snapshot.h"
#include "hydrocouplehelpers.h"

namespace HydroCouple::SDK::IO {

Snapshot::Snapshot(double time)
    : m_time(time)
{
}

bool Snapshot::capture(const HydroCouple::IComponentDataItem &item,
                       std::string &message)
{
    const DataKind kind = item.dataKind();
    const int64_t  elementSize = HydroCouple::Helpers::dataKindSize(kind);
    if (elementSize <= 0)
    {
        message = "Snapshot::capture: item '" + item.id() +
                  "' has a non-fixed-width DataKind (String/Opaque) that "
                  "cannot be captured as raw bytes";
        return false;
    }

    FieldSlice field;
    field.itemId = item.id();
    field.kind   = kind;
    field.shape  = item.shape();

    const int64_t count = field.elementCount();
    field.data.resize(static_cast<size_t>(count * elementSize));

    BufferDescriptor destination;
    destination.data         = field.data.data();
    destination.kind         = kind;
    destination.rank         = static_cast<int32_t>(field.shape.size());
    destination.shape        = field.shape.data();
    destination.stridesBytes = nullptr; // C-contiguous
    destination.space        = MemorySpace::Host;

    const std::vector<int64_t> start(field.shape.size(), 0);
    if (!item.getValuesInto(destination, start, field.shape, &message))
    {
        message = "Snapshot::capture: item '" + item.id() + "': " + message;
        return false;
    }

    m_fields.push_back(std::move(field));
    message.clear();
    return true;
}

void Snapshot::addField(FieldSlice field)
{
    m_fields.push_back(std::move(field));
}

} // namespace HydroCouple::SDK::IO
