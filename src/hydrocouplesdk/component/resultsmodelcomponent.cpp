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
#include <fstream>
#include <cmath>
#include "hydrocouplesdk/core/unit.h"

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
            if ((m_entry.time->units=="elapsed_s" || m_entry.time->units=="seconds_since_start")) {
                if (!m_entry.time->epochJulianDay || !std::isfinite(*m_entry.time->epochJulianDay))
                    throw std::runtime_error("elapsed_s results require a declared Julian-day epoch");
                for (double &t:m_times) t=*m_entry.time->epochJulianDay+t/86400.0;
            } else if (m_entry.time->units!="julian_day")
                throw std::runtime_error("unsupported recorded calendar: "+m_entry.time->units);
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
//! A spatial reference system read back from a recorded geometry file's
//! "reference" object (authority, id, text, units and their vertical
//! counterparts). distanceUnits() and verticalDistanceUnits() throw
//! nlohmann::json::out_of_range when the record lacks "units" or
//! "vertical_units".
class RecordedReference final : public HydroCouple::Spatial::ISpatialReferenceSystem {
public:
    explicit RecordedReference(nlohmann::json j):m_json(std::move(j)),a(m_json.value("authority","")),
        text(m_json.value("text","")),va(m_json.value("vertical_authority","")),vt(m_json.value("vertical_text","")) {}
    int authSRID() const override {return m_json.value("id",0);}
    const std::string &authName() const override {return a;}
    const std::string &srText() const override {return text;}
    IUnit::DistanceUnits distanceUnits() const override {return static_cast<IUnit::DistanceUnits>(m_json.at("units").get<int>());}
    const std::string &verticalAuthName() const override {return va;}
    int verticalAuthSRID() const override {return m_json.value("vertical_id",0);}
    const std::string &verticalSrText() const override {return vt;}
    IUnit::DistanceUnits verticalDistanceUnits() const override {return static_cast<IUnit::DistanceUnits>(m_json.at("vertical_units").get<int>());}
private:
    nlohmann::json m_json;std::string a,text,va,vt;
};

//! A recorded mesh, its reference system and the polyhedral-surface view
//! over them, shared by every item recorded on the same geometry file.
struct RecordedSurface {
    IO::MeshDefinition mesh;
    std::unique_ptr<RecordedReference> reference;
    std::unique_ptr<Spatial::PolyhedralSurfaceAdapter> surface;
    RecordedSurface(const IO::MeshDefinition &m,const nlohmann::json &r):mesh(m),
        reference(r.is_object()?std::make_unique<RecordedReference>(r):nullptr),
        surface(std::make_unique<Spatial::PolyhedralSurfaceAdapter>(Spatial::polyhedralSurfaceFromMesh(mesh),mesh,reference.get())) {}
};

class SpatialRecordedItem
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
                        const IO::MeshDefinition &mesh, const nlohmann::json &reference = {}, std::shared_ptr<RecordedSurface> geometry = {})
        : RecordedItem(entry, reader, dimensions, valueDefinition, component),
          m_geometry(geometry ? std::move(geometry) : std::make_shared<RecordedSurface>(mesh,reference))
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
        return m_geometry->surface.get();
    }

    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override
    {
        // Scalars only (see meshDataType()).
        return HydroCouple::Spatial::VectorBasis::Unknown;
    }

    //! The dimension the values run along: the last, time being the first.
    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        const auto axis=m_entry.geometry.is_object() ? m_entry.geometry.value("entity_axis",int(dimensions().size())-1) : int(dimensions().size())-1;
        return axis>=0 && size_t(axis)<dimensions().size() ? dimensions()[axis] : nullptr;
    }

private:
    std::shared_ptr<RecordedSurface> m_geometry;
    MeshLocation m_objectType = MeshLocation::Node;
};

//! Recorded interface elevations of a layered item: a Float64 entry of
//! shape { time, column, interface } with at least two interfaces. Holds
//! one time level at a time; select() loads a level and checks that the
//! elevations are finite and strictly decrease downward. The constructor
//! loads the last level and throws std::runtime_error on a bad shape or
//! bad values.
class RecordedVertical final : public HydroCouple::Spatial::IVerticalCoordinate {
public:
    RecordedVertical(ResultEntry entry,IResultReader *reader,int kind):m_entry(std::move(entry)),m_reader(reader),m_kind(kind) {
        if (m_entry.kind!=DataKind::Float64 || m_entry.shape.size()!=3 || m_entry.shape[0]<1 || m_entry.shape[1]<1 || m_entry.shape[2]<2)
            throw std::runtime_error("invalid recorded interface-elevation shape");
        m_values.resize(size_t(m_entry.shape[1]*m_entry.shape[2]));
        std::string message;if (!select(m_entry.shape[0]-1,message)) throw std::runtime_error(message);
    }
    bool select(int64_t time,std::string &message) const {
        if (time==m_time) return true;
        const std::vector<int64_t> start={time,0,0},count={1,m_entry.shape[1],m_entry.shape[2]};
        std::vector<double> values(m_values.size());
        BufferDescriptor b;b.data=values.data();b.kind=DataKind::Float64;b.rank=3;b.shape=count.data();b.space=MemorySpace::Host;
        if (!m_reader->read(m_entry,start,count,b,message)) return false;
        for (int64_t c=0;c<columnCount();++c) for (int64_t k=0;k<=layerCount();++k) {
            const auto i=size_t(c*(layerCount()+1)+k);
            if (!std::isfinite(values[i]) || (k && values[i]>=values[i-1])) {message="invalid recorded layer elevations";return false;}
        }
        m_values=std::move(values);m_time=time;++m_epoch;return true;
    }
    HydroCouple::Spatial::VerticalCoordinateKind kind() const override {return static_cast<HydroCouple::Spatial::VerticalCoordinateKind>(m_kind);}
    int64_t columnCount() const override {return m_entry.shape[1];}
    int64_t layerCount() const override {return m_entry.shape[2]-1;}
    bool isTimeVarying() const override {return m_entry.shape[0]>1;}
    uint64_t geometryEpoch() const override {return m_epoch;}
    double interfaceElevation(int64_t c,int64_t k) const override {return m_values.at(size_t(c*(layerCount()+1)+k));}
    std::span<const double> interfaceElevations() const override {return m_values;}
private:
    ResultEntry m_entry;IResultReader *m_reader;int m_kind;
    mutable std::vector<double> m_values;mutable int64_t m_time=-1;mutable uint64_t m_epoch=0;
};

//! A recorded item on a layered mesh. Reading a single time level (count[0]
//! equal to 1 over the full rank) also selects that level of the vertical
//! coordinate, so verticalCoordinate() describes the values just read.
//! The constructor throws std::runtime_error when the layer or entity axis
//! disagrees with the vertical coordinate.
class LayeredRecordedItem final : public SpatialRecordedItem,
    public virtual HydroCouple::SpatioTemporal::ITimeLayeredMeshComponentDataItem {
public:
    LayeredRecordedItem(const ResultEntry &e,IResultReader *reader,const std::vector<Dimension *> &dims,
        ValueDefinition *q,HydroCouple::IModelComponent *mc,const IO::MeshDefinition &mesh,
        const nlohmann::json &ref,ResultEntry vertical,IResultReader *verticalReader,std::shared_ptr<RecordedSurface> geometry)
        :SpatialRecordedItem(e,reader,dims,q,mc,mesh,ref,std::move(geometry)),
         m_vertical(std::move(vertical),verticalReader,e.geometry.at("vertical_kind").get<int>()),
         m_layerAxis(e.geometry.at("layer_axis").get<size_t>()) {
        const auto entity=e.geometry.at("entity_axis").get<size_t>();
        if (entity>=e.shape.size() || m_layerAxis>=e.shape.size() || entity==m_layerAxis ||
            e.shape[entity]!=m_vertical.columnCount() || e.shape[m_layerAxis]!=m_vertical.layerCount())
            throw std::runtime_error("recorded layer/mesh dimensions disagree");
    }
    HydroCouple::IDimension *layerDimension() const override {return dimensions().at(m_layerAxis);}
    HydroCouple::Spatial::IVerticalCoordinate *verticalCoordinate() const override {return const_cast<RecordedVertical *>(&m_vertical);}
    bool getValuesInto(const BufferDescriptor &b,std::span<const int64_t> start,std::span<const int64_t> count,
        std::string *message=nullptr) const override {
        if (!SpatialRecordedItem::getValuesInto(b,start,count,message)) return false;
        if (count.size()==m_entry.shape.size() && count[0]==1) {
            std::string detail;if (!m_vertical.select(start[0],detail)) {if(message)*message=detail;return false;}
        }
        return true;
    }
private:
    RecordedVertical m_vertical;size_t m_layerAxis;
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

//! Private state of a ResultsModelComponent.
struct ResultsModelComponent::Impl
{
    IO::RunManifest manifest;    //!< The manifest, copied at construction.
    std::string     componentId; //!< Component whose entries are exposed; "" = all.

    //! One reader per artifact, shared by the items reading from it.
    std::map<std::string, std::unique_ptr<IResultReader>> readers;

    std::vector<std::unique_ptr<Dimension>>       dimensions; //!< Axes of every item, in creation order.
    std::map<std::string,std::shared_ptr<RecordedSurface>> geometries; //!< Recorded meshes by geometry file path, shared by the items on them.
    std::vector<std::unique_ptr<Unit>> units; //!< Units rebuilt from the entries' quantity records.
    std::vector<std::unique_ptr<ValueDefinition>> valueDefinitions; //!< One per item.
    //! The recorded items, in catalog order. Created with new and
    //! registered as outputs; neither this struct nor the base class deletes
    //! them.
    std::vector<RecordedItem *>                   items;
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

        try {
        if (entry->quantity.is_object()) {
            const auto &j=entry->quantity;
            auto unit=std::make_unique<Unit>(entry->units,j.at("scale").get<double>(),j.at("offset").get<double>());
            const auto powers=j.at("powers").get<std::vector<double>>();
            if (powers.size()!=UnitDimensions::kDimensionCount || !std::isfinite(unit->conversionFactorToSI()) || unit->conversionFactorToSI()<=0 || !std::isfinite(unit->offsetToSI()))
                throw std::runtime_error("invalid recorded physical unit");
            for (size_t d=0;d<powers.size();++d) {if (!std::isfinite(powers[d])) throw std::runtime_error("invalid unit powers");unit->dimensionsInternal()->setPower(static_cast<IUnitDimensions::FundamentalUnitDimension>(d),powers[d]);}
            auto q=std::make_unique<Quantity>(entry->description,typeid(double),unit.get());
            q->setValueKind(static_cast<ValueKind>(j.at("value_kind").get<int>()));
            m_impl->units.push_back(std::move(unit));m_impl->valueDefinitions.push_back(std::move(q));
        } else m_impl->valueDefinitions.emplace_back(Quantity::unitLess(entry->units.empty()?entry->itemId:entry->units));
        IResultReader *reader=m_impl->readers.at(entry->artifact).get();
        IO::MeshDefinition mesh;std::string detail;nlohmann::json ref;
        bool spatial=false;std::shared_ptr<RecordedSurface> geometry;
        if (entry->geometry.is_object()) {
            const auto geometryPath=m_impl->manifest.directory/entry->geometry.at("artifact").get<std::string>();
            const std::string geometryKey=geometryPath.string();
            const auto cached=m_impl->geometries.find(geometryKey);
            if (cached!=m_impl->geometries.end()) {geometry=cached->second;mesh=geometry->mesh;}
            else {
            std::ifstream input(geometryPath);if (!input) throw std::runtime_error("missing recorded geometry: "+geometryPath.string());
            nlohmann::json g;input>>g;
            if (g.at("schema")!="hc.recorded-mesh/1.0") throw std::runtime_error("unsupported recorded geometry schema");
            mesh.nodeX=g.at("node_x").get<std::vector<double>>();mesh.nodeY=g.at("node_y").get<std::vector<double>>();mesh.nodeZ=g.at("node_z").get<std::vector<double>>();
            mesh.faceNodeOffsets=g.at("face_offsets").get<std::vector<int64_t>>();mesh.faceNodes=g.at("face_nodes").get<std::vector<int64_t>>();
            const auto edges=g.at("edge_nodes").get<std::vector<int64_t>>();if (edges.size()%2) throw std::runtime_error("invalid recorded edge connectivity");
            for (size_t e=0;e<edges.size();e+=2) mesh.edgeNodes.push_back({edges[e],edges[e+1]});
            if (!mesh.validate(detail)) throw std::runtime_error(detail);
            for (double x:mesh.nodeX) if(!std::isfinite(x)) throw std::runtime_error("invalid recorded coordinates");
            for (double y:mesh.nodeY) if(!std::isfinite(y)) throw std::runtime_error("invalid recorded coordinates");
            if (g.contains("reference")) ref=g["reference"];
                geometry=std::make_shared<RecordedSurface>(mesh,ref);
                m_impl->geometries.emplace(geometryKey,geometry);
            }
            const auto axis=entry->geometry.at("entity_axis").get<size_t>();
            const auto n=entry->location==IO::MeshLocation::Face?mesh.faceCount():entry->location==IO::MeshLocation::Edge?mesh.edgeCount():mesh.nodeCount();
            if(axis>=entry->shape.size() || entry->shape[axis]!=n) throw std::runtime_error("recorded entity/mesh dimensions disagree");
            spatial=true;
        } else spatial=reader->readMesh(*entry,mesh,detail);
        RecordedItem *item=nullptr;
        if (spatial && entry->geometry.contains("vertical")) {
            ResultEntry v;if (!ResultEntry::fromJson(entry->geometry.at("vertical"),v,detail)) throw std::runtime_error(detail);
            if (v.shape.size()!=3 || v.shape[0]!=entry->shape[0]) throw std::runtime_error("recorded field/interface times disagree");
            if (!m_impl->readers.count(v.artifact)) {
                auto vr=IO::createResultReader(v.format,detail);if (!vr || !vr->open(m_impl->manifest.artifactPath(v),detail)) throw std::runtime_error(detail);
                m_impl->readers.emplace(v.artifact,std::move(vr));
            }
            item=new LayeredRecordedItem(*entry,reader,dimensions,m_impl->valueDefinitions.back().get(),this,mesh,ref,v,m_impl->readers.at(v.artifact).get(),geometry);
        } else if(spatial) item=new SpatialRecordedItem(*entry,reader,dimensions,m_impl->valueDefinitions.back().get(),this,mesh,ref,geometry);
        else item=new RecordedItem(*entry,reader,dimensions,m_impl->valueDefinitions.back().get(),this);
        addOutput(item);
        m_impl->items.push_back(item);
        } catch (const std::exception &e) {
            const std::string detail="item "+entry->itemId+": "+e.what();
            reportError(ErrorEntry::Severity::Fatal,5,detail);setStatus(ComponentStatus::Failed,detail);return;
        }
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
