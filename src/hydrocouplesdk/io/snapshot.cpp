// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/io/snapshot.h"
#include "hydrocouplehelpers.h"
#include "hydrocouplespatial.h"
#include <cstring>
#include <cmath>

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

std::string verticalFieldId(const HydroCouple::IComponentDataItem &item)
{
    const auto *layering=dynamic_cast<const HydroCouple::Spatial::ILayering *>(&item);
    if (!layering || !layering->verticalCoordinate()) return {};
    std::string first=item.id();
    if (auto *owner=item.modelComponent()) for (auto *output:owner->outputs()) {
        const auto *candidate=dynamic_cast<const HydroCouple::Spatial::ILayering *>(output);
        if (candidate && candidate->verticalCoordinate()==layering->verticalCoordinate()) { first=output->id(); break; }
    }
    const std::string identity=(item.modelComponent() ? item.modelComponent()->id() : "")+"/"+first;
    const char *digits="0123456789abcdef"; std::string id="__hc_interfaces_";
    for (unsigned char c:identity) { id+=digits[c>>4]; id+=digits[c&15]; }
    return id;
}

bool Snapshot::captureGeometry(const HydroCouple::IComponentDataItem &item,std::string &message)
{
    auto *layering=dynamic_cast<const HydroCouple::Spatial::ILayering *>(&item);
    if (!layering) { message.clear(); return true; }
    const auto *v=layering->verticalCoordinate();
    if (!v || v->columnCount()<1 || v->layerCount()<1) { message="layered item has no prepared vertical coordinate"; return false; }
    const std::string id=verticalFieldId(item);
    const std::vector<int64_t> shape={v->columnCount(),v->layerCount()+1};
    for (const auto &f:m_fields) if (f.itemId==id) {
        if (f.shape!=shape) {message="shared recorded vertical coordinate changed shape";return false;}
        message.clear(); return true;
    }
    const auto elevations=v->interfaceElevations();
    if (elevations.size()!=size_t(shape[0]*shape[1])) {message="vertical interface shape mismatch";return false;}
    for (double z:elevations) if (!std::isfinite(z)) {message="nonfinite recorded interface elevation";return false;}
    FieldSlice f;f.itemId=id;f.kind=DataKind::Float64;f.shape=shape;f.data.resize(elevations.size_bytes());
    std::memcpy(f.data.data(),elevations.data(),elevations.size_bytes());m_fields.push_back(std::move(f));
    message.clear();return true;
}

void Snapshot::addField(FieldSlice field)
{
    m_fields.push_back(std::move(field));
}

} // namespace HydroCouple::SDK::IO
