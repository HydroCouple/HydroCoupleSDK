// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/component/resultsmodelcomponent.h"
#include "hydrocouplesdk/data/abstractoutput.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/spatial/meshadapters.h"
#include "hydrocouplesdk/temporal/timedata.h"
#include "hydrocouplehelpers.h"
#include "hydrocouplespatiotemporal.h"
#include "hydrocoupletemporal.h"

#include <algorithm>
#include <map>

namespace HydroCouple::SDK {

namespace {

using HydroCouple::SDK::IO::ResultEntry;
using HydroCouple::SDK::IO::IResultReader;

/*!
 * \brief One recorded item: an output whose values are read on demand from
 *        a run artifact.
 *
 * Implements the v2 data plane by delegating getValuesInto() to the
 * reader, and ITimeSeriesComponentDataItem when the catalog entry records
 * a time axis. setValuesFrom() is refused — a recorded run is history, and
 * quietly accepting writes would invite a UI to believe it had changed
 * something.
 */
class RecordedItem
    : public AbstractOutput,
      public virtual HydroCouple::Temporal::ITimeSeriesComponentDataItem
{
public:
    RecordedItem(const ResultEntry &entry, IResultReader *reader,
                 const std::vector<Dimension *> &dimensions,
                 ValueDefinition *valueDefinition,
                 HydroCouple::IModelComponent *component)
        : AbstractOutput(entry.itemId, entry.description, dimensions,
                         valueDefinition, component),
          m_entry(entry),
          m_reader(reader)
    {
        std::string message;
        if (m_entry.time.has_value() &&
            m_reader->readTimes(m_entry, m_times, message))
        {
            m_timeObjects.reserve(m_times.size());
            for (size_t i = 0; i < m_times.size(); ++i)
                m_timeObjects.push_back(std::make_shared<Temporal::TimeData>(
                    "t" + std::to_string(i), m_times[i]));
            if (!m_times.empty())
                m_timeSpan = std::make_shared<Temporal::TimeSpan>(
                    "TimeSpan", m_times.front(),
                    m_times.back() - m_times.front());
        }
    }

    ~RecordedItem() override = default;

    [[nodiscard]] const ResultEntry &entry() const noexcept { return m_entry; }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return m_entry.shape;
    }

    [[nodiscard]] DataKind dataKind() const override { return m_entry.kind; }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override
    {
        std::string readMessage;
        if (!m_reader->read(m_entry, start, count, destination, readMessage))
        {
            if (message)
                *message = readMessage;
            return false;
        }
        if (message)
            message->clear();
        return true;
    }

    /*!
     * \brief Refused: a recorded run is history, not a writable store.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &,
                                     std::span<const int64_t>,
                                     std::span<const int64_t>,
                                     std::string *message = nullptr) override
    {
        if (message)
            *message = "item \"" + m_entry.itemId +
                       "\" belongs to a recorded run and is read-only";
        return false;
    }

    // ── IOutput ───────────────────────────────────────────────────────────

    /*!
     * \brief Nothing to compute: values already exist in the artifact.
     */
    void updateValues() override {}

    // ── ITimeSeriesComponentDataItem ─────────────────────────────────────

    [[nodiscard]] const HydroCouple::Temporal::IDateTime *
    time(int64_t timeIndex) const override
    {
        if (timeIndex < 0 ||
            timeIndex >= static_cast<int64_t>(m_timeObjects.size()))
            return nullptr;
        return m_timeObjects[static_cast<size_t>(timeIndex)].get();
    }

    [[nodiscard]] int64_t timeCount() const override
    {
        return static_cast<int64_t>(m_times.size());
    }

    [[nodiscard]] std::span<const double> times() const override
    {
        return {m_times.data(), m_times.size()};
    }

    [[nodiscard]] HydroCouple::Temporal::ITimeSpan *timeSpan() const override
    {
        return m_timeSpan.get();
    }

    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override
    {
        return dimensions().empty() ? nullptr : dimensions().front();
    }

    // What a recorded value's timestamp means is whatever the producer
    // declared when it was written; the result catalogue does not (yet)
    // carry that, so a reopened item answers honestly: not declared. A
    // recorded series is served exactly at the instants it holds.
    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const override
    {
        return HydroCouple::Temporal::TimeKind::Unknown;
    }

    [[nodiscard]] double intervalLength() const override { return 0.0; }

    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const override
    {
        return HydroCouple::Temporal::TimeInterpolation::None;
    }

    [[nodiscard]] HydroCouple::Temporal::TimeExtrapolation timeExtrapolation() const override
    {
        return HydroCouple::Temporal::TimeExtrapolation::Refuse;
    }

protected:
    ResultEntry    m_entry;
    IResultReader *m_reader;

private:

    std::vector<double> m_times;
    std::vector<std::shared_ptr<Temporal::TimeData>> m_timeObjects;
    std::shared_ptr<Temporal::TimeSpan>              m_timeSpan;
};

/*!
 * A recorded item whose artifact also carried the mesh its values are
 * attached to.
 *
 * A separate class rather than a nullable surface on RecordedItem, because
 * the interface is the answer to "can this be mapped?". An item that carried
 * IPolyhedralSurfaceComponentDataItem with nothing behind it would say yes
 * and then hand back nothing, which is worse for a consumer than a plain
 * honest no — it turns a question that can be asked once into one that has
 * to be asked twice.
 */
class SpatialRecordedItem final
    : public RecordedItem,
      public virtual HydroCouple::SpatioTemporal::
          ITimeSeriesPolyhedralSurfaceComponentDataItem
{
    using MeshLocation = HydroCouple::Spatial::MeshLocation;

public:
    SpatialRecordedItem(const ResultEntry &entry, IResultReader *reader,
                        const std::vector<Dimension *> &dimensions,
                        ValueDefinition *valueDefinition,
                        HydroCouple::IModelComponent *component,
                        const IO::MeshDefinition &mesh)
        : RecordedItem(entry, reader, dimensions, valueDefinition, component),
          m_surface(std::make_unique<Spatial::PolyhedralSurfaceAdapter>(
              Spatial::polyhedralSurfaceFromMesh(mesh), mesh))
    {
        switch (entry.location.value_or(IO::MeshLocation::Node))
        {
            case IO::MeshLocation::Edge:
                m_objectType = MeshLocation::Edge;
                break;
            case IO::MeshLocation::Face:
                m_objectType = MeshLocation::Face;
                break;
            default:
                m_objectType = MeshLocation::Node;
                break;
        }
    }

    ~SpatialRecordedItem() override = default;

    // ── IPolyhedralSurfaceComponentDataItem ──────────────────────────────

    [[nodiscard]] MeshLocation location() const override
    {
        return m_objectType;
    }

    [[nodiscard]] HydroCouple::Spatial::SpatialDataType meshDataType()
        const override
    {
        // One number per entity is what a result catalog records; a vector
        // field is written as separate items.
        return HydroCouple::Spatial::SpatialDataType::Scalar;
    }

    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *polyhedralSurface()
        const override
    {
        return m_surface.get();
    }

    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override
    {
        // Scalars only (see meshDataType()).
        return HydroCouple::Spatial::VectorBasis::Unknown;
    }

    //! The dimension the values run along: the last, time being the first.
    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        return dimensions().empty() ? nullptr : dimensions().back();
    }

private:
    std::unique_ptr<Spatial::PolyhedralSurfaceAdapter> m_surface;
    MeshLocation m_objectType = MeshLocation::Node;
};

/*!
 * Preference order when the same item was recorded by several writers:
 * random-access, natively typed formats first, long-format last. Lower
 * ranks win; unreadable formats are excluded before ranking.
 */
int formatRank(const std::string &format)
{
    if (format == "hdf5_ugrid")   return 0;
    if (format == "netcdf_ugrid") return 1;
    if (format == "geopackage")   return 2;
    if (format == "csv")          return 3;
    return 99;
}

} // namespace

// ── Impl ──────────────────────────────────────────────────────────────────

struct ResultsModelComponent::Impl
{
    IO::RunManifest manifest;
    std::string     componentId;

    //! One reader per artifact, shared by the items reading from it.
    std::map<std::string, std::unique_ptr<IResultReader>> readers;

    std::vector<std::unique_ptr<Dimension>>       dimensions;
    std::vector<std::unique_ptr<ValueDefinition>> valueDefinitions;
    std::vector<RecordedItem *>                   items; //!< Owned by base.
};

// ── Construction ──────────────────────────────────────────────────────────

ResultsModelComponent::ResultsModelComponent(IO::RunManifest manifest,
                                             std::string componentId,
                                             std::string_view id)
    : AbstractModelComponent(id.empty() ? std::string_view(componentId) : id,
                             "Recorded results"),
      m_impl(std::make_unique<Impl>())
{
    m_impl->manifest    = std::move(manifest);
    m_impl->componentId = std::move(componentId);
}

ResultsModelComponent::~ResultsModelComponent() = default;

std::unique_ptr<ResultsModelComponent> ResultsModelComponent::open(
    const std::filesystem::path &manifestPath, const std::string &componentId,
    std::string &message)
{
    IO::RunManifest manifest;
    if (!IO::RunManifest::read(manifestPath, manifest, message))
        return nullptr;
    message.clear();
    return std::make_unique<ResultsModelComponent>(std::move(manifest),
                                                   componentId);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────

void ResultsModelComponent::initialize()
{
    setStatus(ComponentStatus::Initializing);

    // Which catalog entries belong to this component (all, when no id).
    std::vector<const IO::ResultEntry *> entries;
    if (m_impl->componentId.empty())
    {
        for (const IO::ResultEntry &entry : m_impl->manifest.results)
            entries.push_back(&entry);
    }
    else
    {
        entries = m_impl->manifest.entriesFor(m_impl->componentId);
    }

    if (entries.empty())
    {
        const std::string detail =
            "run \"" + m_impl->manifest.id + "\" records no results for " +
            (m_impl->componentId.empty()
                 ? std::string("any component")
                 : ("component \"" + m_impl->componentId + "\""));
        reportError(ErrorEntry::Severity::Fatal, 1, detail);
        setStatus(ComponentStatus::Failed, detail);
        return;
    }

    // The same item may have been recorded by several writers. It is still
    // one logical variable, so expose it once, served by the most capable
    // format this build can actually read.
    std::map<std::string, const IO::ResultEntry *> chosen;
    std::map<std::string, std::vector<std::string>> rejected;
    for (const IO::ResultEntry *entry : entries)
    {
        if (!IO::resultFormatSupported(entry->format))
        {
            rejected[entry->itemId].push_back(entry->format);
            continue;
        }
        const auto existing = chosen.find(entry->itemId);
        if (existing == chosen.end() ||
            formatRank(entry->format) < formatRank(existing->second->format))
            chosen[entry->itemId] = entry;
    }

    if (chosen.empty() && !rejected.empty())
    {
        std::string detail = "no readable artifact for the recorded items (";
        bool first = true;
        for (const auto &[itemId, formats] : rejected)
        {
            for (const std::string &format : formats)
            {
                if (!first)
                    detail += ", ";
                detail += itemId + " as " + format;
                first = false;
            }
        }
        detail += ") — rebuild with the matching features enabled";
        reportError(ErrorEntry::Severity::Fatal, 4, detail);
        setStatus(ComponentStatus::Failed, detail);
        return;
    }

    // Items appear in catalog order for determinism.
    std::vector<const IO::ResultEntry *> selected;
    for (const IO::ResultEntry *entry : entries)
    {
        const auto pick = chosen.find(entry->itemId);
        if (pick != chosen.end() && pick->second == entry)
            selected.push_back(entry);
    }

    for (const IO::ResultEntry *entry : selected)
    {
        // One reader per artifact, opened once.
        if (!m_impl->readers.count(entry->artifact))
        {
            std::string message;
            std::unique_ptr<IResultReader> reader =
                IO::createResultReader(entry->format, message);
            if (!reader)
            {
                const std::string detail = "item \"" + entry->itemId +
                                           "\": " + message;
                reportError(ErrorEntry::Severity::Fatal, 2, detail);
                setStatus(ComponentStatus::Failed, detail);
                return;
            }
            const std::filesystem::path path =
                m_impl->manifest.artifactPath(*entry);
            if (!reader->open(path, message))
            {
                const std::string detail = "item \"" + entry->itemId +
                                           "\": " + message;
                reportError(ErrorEntry::Severity::Fatal, 3, detail);
                setStatus(ComponentStatus::Failed, detail);
                return;
            }
            m_impl->readers.emplace(entry->artifact, std::move(reader));
        }

        // Dimensions mirror the recorded ones so a UI can label axes.
        std::vector<Dimension *> dimensions;
        for (size_t d = 0; d < entry->shape.size(); ++d)
        {
            const std::string name =
                d < entry->dimensions.size() ? entry->dimensions[d]
                                             : "dim" + std::to_string(d);
            m_impl->dimensions.push_back(
                std::make_unique<Dimension>(name, name));
            dimensions.push_back(m_impl->dimensions.back().get());
        }

        m_impl->valueDefinitions.emplace_back(
            Quantity::unitLess(entry->units.empty() ? entry->itemId
                                                    : entry->units));

        IResultReader *reader = m_impl->readers.at(entry->artifact).get();

        // Geometry when the artifact carried it. Asked for once, here, rather
        // than on every read: a mesh does not move between time steps, and an
        // item that re-read it per step would pay for the whole topology to
        // draw one frame.
        IO::MeshDefinition mesh;
        std::string        meshMessage;
        const bool         spatial = reader->readMesh(*entry, mesh, meshMessage);

        RecordedItem *item =
            spatial ? new SpatialRecordedItem(
                          *entry, reader, dimensions,
                          m_impl->valueDefinitions.back().get(), this, mesh)
                    : new RecordedItem(*entry, reader, dimensions,
                                       m_impl->valueDefinitions.back().get(),
                                       this);
        addOutput(item);
        m_impl->items.push_back(item);
    }

    setStatus(ComponentStatus::Initialized,
              "results loaded from run \"" + m_impl->manifest.id + "\"");

    // A recorded run has already computed its values: it is finished by
    // construction. The documented sequence is still walked so listeners
    // observe a lawful lifecycle.
    setStatus(ComponentStatus::Finishing);
    setStatus(ComponentStatus::Finished,
              "recorded run \"" + m_impl->manifest.id + "\" ready for reading");
}

std::vector<std::string> ResultsModelComponent::validate()
{
    return {};
}

void ResultsModelComponent::prepare() {}

void ResultsModelComponent::update(
    const std::vector<HydroCouple::IOutput *> &)
{
}

void ResultsModelComponent::finish()
{
    m_impl->readers.clear();
    if (status() != ComponentStatus::Finished)
        setStatus(ComponentStatus::Finished);
}

std::vector<HydroCouple::IComponentDataItem *>
ResultsModelComponent::results() const
{
    std::vector<HydroCouple::IComponentDataItem *> items;
    items.reserve(m_impl->items.size());
    for (RecordedItem *item : m_impl->items)
        items.push_back(item);
    return items;
}

// ── Recorded-run accessors ────────────────────────────────────────────────

const IO::RunManifest &ResultsModelComponent::manifest() const noexcept
{
    return m_impl->manifest;
}

const std::string &ResultsModelComponent::sourceComponentId() const noexcept
{
    return m_impl->componentId;
}

const IO::ResultEntry *ResultsModelComponent::entryFor(
    const std::string &itemId) const
{
    for (RecordedItem *item : m_impl->items)
    {
        if (item->id() == itemId)
            return &item->entry();
    }
    return nullptr;
}

bool ResultsModelComponent::times(const std::string &itemId,
                                  std::vector<double> &values,
                                  std::string &message) const
{
    for (RecordedItem *item : m_impl->items)
    {
        if (item->id() != itemId)
            continue;
        const std::span<const double> stored = item->times();
        values.assign(stored.begin(), stored.end());
        message.clear();
        return true;
    }
    message = "no recorded item \"" + itemId + "\"";
    return false;
}

// ── Unused AbstractModelComponent hooks ───────────────────────────────────

void ResultsModelComponent::createArguments() {}

bool ResultsModelComponent::initializeArguments(std::string &message)
{
    message.clear();
    return true;
}

void ResultsModelComponent::createInputs() {}

void ResultsModelComponent::createOutputs() {}

void ResultsModelComponent::initializeFailureCleanUp()
{
    m_impl->readers.clear();
}

} // namespace HydroCouple::SDK
