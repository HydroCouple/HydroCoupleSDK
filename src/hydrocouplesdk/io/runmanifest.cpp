// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/io/runmanifest.h"
#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/core/uuid.h"
#include "hydrocouplesdk/version.h"
#include "hydrocouplesdk/io/snapshot.h"
#include "hydrocouplesdk/core/unit.h"
#include "hydrocouplespatial.h"
#include "hydrocoupletemporal.h"
#include <map>

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace HydroCouple::SDK::IO {

namespace {

/*!
 * ISO-8601 UTC stamp for "now".
 */
std::string utcNow()
{
    const std::time_t now = std::time(nullptr);
    std::tm           utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    std::ostringstream stream;
    stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

/*!
 * Status spelling shared with AbstractModelComponent's diagnostics.
 */
std::string componentStatusName(
    HydroCouple::IModelComponent::ComponentStatus status)
{
    return HydroCouple::SDK::AbstractModelComponent::statusToString(status);
}

} // namespace

// ── RunStatus ─────────────────────────────────────────────────────────────

std::string runStatusName(RunStatus status)
{
    switch (status)
    {
        case RunStatus::Completed: return "completed";
        case RunStatus::Failed:    return "failed";
        case RunStatus::Stopped:   return "stopped";
    }
    return "completed";
}

bool runStatusFromName(const std::string &name, RunStatus &status)
{
    if (name == "completed") { status = RunStatus::Completed; return true; }
    if (name == "failed")    { status = RunStatus::Failed;    return true; }
    if (name == "stopped")   { status = RunStatus::Stopped;   return true; }
    return false;
}

// ── RunManifest ───────────────────────────────────────────────────────────

std::vector<const ResultEntry *> RunManifest::entriesFor(
    const std::string &componentId) const
{
    std::vector<const ResultEntry *> found;
    for (const ResultEntry &entry : results)
    {
        if (entry.componentId == componentId)
            found.push_back(&entry);
    }
    return found;
}

const ResultEntry *RunManifest::entry(const std::string &componentId,
                                      const std::string &itemId) const
{
    for (const ResultEntry &entry : results)
    {
        if (entry.componentId == componentId && entry.itemId == itemId)
            return &entry;
    }
    return nullptr;
}

std::filesystem::path RunManifest::artifactPath(const ResultEntry &e) const
{
    const std::filesystem::path artifact(e.artifact);
    if (artifact.is_absolute() || directory.empty())
        return artifact;
    return directory / artifact;
}

nlohmann::json RunManifest::toJson() const
{
    nlohmann::json document;
    document["schema_version"] = schemaVersion;

    nlohmann::json run;
    run["id"] = id;
    if (!caption.empty())  run["caption"]  = caption;
    if (!started.empty())  run["started"]  = started;
    if (!finished.empty()) run["finished"] = finished;
    if (wallSeconds > 0.0) run["wall_seconds"] = wallSeconds;
    run["status"] = runStatusName(status);
    if (!host.empty()) run["host"] = host;
    document["run"] = std::move(run);

    if (!sdkVersion.empty() || !interfaceVersion.empty())
    {
        nlohmann::json versions;
        if (!sdkVersion.empty())       versions["sdk"] = sdkVersion;
        if (!interfaceVersion.empty()) versions["interface"] = interfaceVersion;
        document["versions"] = std::move(versions);
    }

    if (!compositionPath.empty() || !compositionSha256.empty() ||
        !compositionEmbedded.is_null())
    {
        nlohmann::json composition;
        if (!compositionPath.empty())
            composition["path"] = compositionPath;
        if (!compositionSha256.empty())
            composition["sha256"] = compositionSha256;
        if (!compositionEmbedded.is_null())
            composition["embedded"] = compositionEmbedded;
        document["composition"] = std::move(composition);
    }

    if (!components.empty())
    {
        document["components"] = nlohmann::json::array();
        for (const ComponentOutcome &outcome : components)
        {
            nlohmann::json block;
            block["id"] = outcome.id;
            if (!outcome.caption.empty())
                block["caption"] = outcome.caption;
            if (!outcome.finalStatus.empty())
                block["final_status"] = outcome.finalStatus;
            if (!outcome.errors.empty())
                block["errors"] = outcome.errors;
            document["components"].push_back(std::move(block));
        }
    }

    document["results"] = nlohmann::json::array();
    for (const ResultEntry &entry : results)
        document["results"].push_back(entry.toJson());

    return document;
}

bool RunManifest::fromJson(const nlohmann::json &json, RunManifest &manifest,
                           std::string &message)
{
    manifest = RunManifest{};

    if (!json.is_object())
    {
        message = "Run manifest must be a JSON object";
        return false;
    }
    if (json.contains("schema_version"))
    {
        if (!json["schema_version"].is_string())
        {
            message = "\"schema_version\" must be a string";
            return false;
        }
        manifest.schemaVersion = json["schema_version"].get<std::string>();
        const std::string major =
            manifest.schemaVersion.substr(0, manifest.schemaVersion.find('.'));
        if (major != "1")
        {
            message = "Unsupported run manifest schema_version \"" +
                      manifest.schemaVersion + "\"; this SDK reads version 1.x";
            return false;
        }
    }

    if (!json.contains("run") || !json["run"].is_object())
    {
        message = "Run manifest must carry a \"run\" object";
        return false;
    }
    const auto &run = json["run"];
    manifest.id          = run.value("id", "");
    manifest.caption     = run.value("caption", "");
    manifest.started     = run.value("started", "");
    manifest.finished    = run.value("finished", "");
    manifest.wallSeconds = run.value("wall_seconds", 0.0);
    manifest.host        = run.value("host", "");
    if (run.contains("status") &&
        !runStatusFromName(run["status"].get<std::string>(), manifest.status))
    {
        message = "run.status must be \"completed\", \"failed\" or "
                  "\"stopped\"";
        return false;
    }

    if (json.contains("versions") && json["versions"].is_object())
    {
        manifest.sdkVersion = json["versions"].value("sdk", "");
        manifest.interfaceVersion = json["versions"].value("interface", "");
    }

    if (json.contains("composition") && json["composition"].is_object())
    {
        manifest.compositionPath = json["composition"].value("path", "");
        manifest.compositionSha256 = json["composition"].value("sha256", "");
        if (json["composition"].contains("embedded"))
            manifest.compositionEmbedded = json["composition"]["embedded"];
    }

    if (json.contains("components"))
    {
        if (!json["components"].is_array())
        {
            message = "\"components\" must be an array";
            return false;
        }
        for (const auto &block : json["components"])
        {
            ComponentOutcome outcome;
            outcome.id          = block.value("id", "");
            outcome.caption     = block.value("caption", "");
            outcome.finalStatus = block.value("final_status", "");
            if (block.contains("errors") && block["errors"].is_array())
            {
                for (const auto &text : block["errors"])
                    outcome.errors.push_back(text.get<std::string>());
            }
            manifest.components.push_back(std::move(outcome));
        }
    }

    if (!json.contains("results") || !json["results"].is_array())
    {
        message = "Run manifest must carry a \"results\" array (it may be "
                  "empty, but the catalog is what makes a run reopenable)";
        return false;
    }
    for (const auto &block : json["results"])
    {
        ResultEntry entry;
        if (!ResultEntry::fromJson(block, entry, message))
            return false;
        manifest.results.push_back(std::move(entry));
    }

    message.clear();
    return true;
}

bool RunManifest::read(const std::filesystem::path &path,
                       RunManifest &manifest, std::string &message)
{
    std::ifstream stream(path);
    if (!stream)
    {
        message = "Cannot open run manifest \"" + path.string() + "\"";
        return false;
    }
    nlohmann::json document;
    try
    {
        stream >> document;
    }
    catch (const std::exception &exception)
    {
        message = "Run manifest \"" + path.string() +
                  "\" is not valid JSON: " + exception.what();
        return false;
    }
    if (!fromJson(document, manifest, message))
    {
        message = "Run manifest \"" + path.string() + "\": " + message;
        return false;
    }
    // Artifact paths in results[] are relative to the manifest.
    manifest.directory = path.has_parent_path() ? path.parent_path()
                                                : std::filesystem::path(".");
    return true;
}

bool RunManifest::write(const std::filesystem::path &path,
                        std::string &message) const
{
    std::error_code code;
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path(), code);

    std::ofstream stream(path);
    if (!stream)
    {
        message = "Cannot write run manifest \"" + path.string() + "\"";
        return false;
    }
    stream << toJson().dump(2) << '\n';
    if (!stream)
    {
        message = "Failed while writing run manifest \"" + path.string() + "\"";
        return false;
    }
    message.clear();
    return true;
}

// ── RunRecorder ───────────────────────────────────────────────────────────

RunRecorder::RunRecorder(std::string runId)
    : m_start(std::chrono::steady_clock::now())
{
    m_manifest.id = runId.empty() ? generateUUID() : std::move(runId);
    m_manifest.started    = utcNow();
    m_manifest.sdkVersion = HYDROCOUPLESDK_VERSION;
    m_manifest.interfaceVersion = HYDROCOUPLESDK_VERSION; // versions lockstep
}

void RunRecorder::setCaption(std::string caption)
{
    m_manifest.caption = std::move(caption);
}

void RunRecorder::setComposition(const std::filesystem::path &path,
                                 const nlohmann::json &document, bool embed)
{
    m_manifest.compositionPath = path.string();
    if (embed && !document.is_null())
        m_manifest.compositionEmbedded = document;
}

void RunRecorder::addWriter(std::shared_ptr<IOutputWriter> writer)
{
    if (writer)
        m_writers.push_back(std::move(writer));
}

void RunRecorder::addComponent(HydroCouple::IModelComponent *component)
{
    if (component)
        m_components.push_back(component);
}

RunManifest RunRecorder::finish(RunStatus status)
{
    m_manifest.finished = utcNow();
    m_manifest.wallSeconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                      m_start)
            .count();
    m_manifest.status = status;

    for (HydroCouple::IModelComponent *component : m_components)
    {
        ComponentOutcome outcome;
        outcome.id          = component->id();
        outcome.caption     = component->caption();
        outcome.finalStatus = componentStatusName(component->status());
        for (const ErrorEntry &error : component->errors())
            outcome.errors.push_back(error.message);
        m_manifest.components.push_back(std::move(outcome));
    }

    // Writer catalogs are only valid after finalize(); the caller is
    // responsible for having drained the IOThread first.
    for (const std::shared_ptr<IOutputWriter> &writer : m_writers)
    {
        for (ResultEntry entry : writer->catalog())
        {
            // Attribute the entry to its producing component when the item
            // id is unambiguous across the registered components.
            if (entry.componentId.empty())
            {
                for (HydroCouple::IModelComponent *component : m_components)
                {
                    bool owns = false;
                    for (HydroCouple::IOutput *output : component->outputs())
                        owns = owns || output->id() == entry.itemId;
                    for (HydroCouple::IComponentDataItem *item :
                         component->results())
                        owns = owns || item->id() == entry.itemId;
                    if (owns)
                    {
                        entry.componentId = component->id();
                        break;
                    }
                }
            }
            m_manifest.results.push_back(std::move(entry));
        }
    }

    return m_manifest;
}

bool RunRecorder::finishToFile(const std::filesystem::path &path,
                               std::string &message, RunStatus status)
{
    RunManifest manifest = finish(status);
    try {
        std::filesystem::create_directories(path.parent_path());
        std::map<HydroCouple::Spatial::IPolyhedralSurface *,std::string> geometries;
        for (auto &entry:manifest.results) {
            if (entry.itemId.starts_with("__hc_interfaces_")) continue;
            HydroCouple::IComponentDataItem *item=nullptr;
            for (auto *component:m_components) {
                if (!entry.componentId.empty() && component->id()!=entry.componentId) continue;
                for (auto *candidate:component->outputs()) if (candidate->id()==entry.itemId) {
                    if (item) {message="ambiguous recorded output identity: "+entry.itemId;return false;}
                    item=candidate;
                }
            }
            if (!item) continue;
            entry.description=item->caption();
            if (entry.time && (entry.time->units=="elapsed_s" || entry.time->units=="seconds_since_start")) {
                const auto *clock=dynamic_cast<const HydroCouple::Temporal::ITimeModelComponent *>(item->modelComponent());
                if (clock && clock->simulationPeriod()) entry.time->epochJulianDay=clock->simulationPeriod()->julianDay();
            }
            entry.dimensions={"time"};
            for (auto *d:item->dimensions()) entry.dimensions.push_back(d->id());
            if (entry.dimensions.size()!=entry.shape.size()) {message="recorded dimension rank mismatch";return false;}
            if (const auto *q=dynamic_cast<const HydroCouple::IQuantity *>(item->valueDefinition());q && q->unit() &&
                (entry.units.empty() || entry.units==q->unit()->caption())) {
                const auto *u=q->unit();entry.units=u->caption();
                std::vector<double> powers;
                for (size_t i=0;i<UnitDimensions::kDimensionCount;++i)
                    powers.push_back(u->dimensions()->power(static_cast<IUnitDimensions::FundamentalUnitDimension>(i)));
                entry.quantity={{"scale",u->conversionFactorToSI()},{"offset",u->offsetToSI()},
                    {"powers",powers},{"value_kind",int(q->valueKind())}};
            }
            auto *spatial=dynamic_cast<HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem *>(item);
            if (!spatial || !spatial->polyhedralSurface()) continue;
            auto *surface=spatial->polyhedralSurface();auto *view=surface->meshView();
            if (!view) {message="recorded surface has no mesh view";return false;}
            if (!geometries.count(surface)) {
                nlohmann::json g={{"schema","hc.recorded-mesh/1.0"},
                    {"node_x",view->nodeX()},{"node_y",view->nodeY()},{"node_z",view->nodeZ()},
                    {"face_offsets",view->faceNodeOffsets()},{"face_nodes",view->faceNodes()},
                    {"edge_nodes",view->edgeNodes()}};
                if (auto *ref=surface->spatialReferenceSystem())
                    g["reference"]={{"authority",ref->authName()},{"id",ref->authSRID()},
                      {"text",ref->srText()},{"units",int(ref->distanceUnits())},
                      {"vertical_authority",ref->verticalAuthName()},{"vertical_id",ref->verticalAuthSRID()},
                      {"vertical_text",ref->verticalSrText()},{"vertical_units",int(ref->verticalDistanceUnits())}};
                const std::string name="geometry_"+generateUUID()+"_"+std::to_string(geometries.size())+".json";
                std::ofstream out(path.parent_path()/name);out<<g.dump();out.close();
                if (!out) {message="could not write recorded geometry";return false;}
                geometries.emplace(surface,name);
            }
            entry.geometry={{"artifact",geometries.at(surface)}};
            const auto dims=item->dimensions();
            const auto it=std::find(dims.begin(),dims.end(),spatial->entityDimension());
            if (it==dims.end()) {message="recorded mesh entity dimension is absent";return false;}
            entry.geometry["entity_axis"]=1+std::distance(dims.begin(),it);
            switch (spatial->location()) {
                case HydroCouple::Spatial::MeshLocation::Face:entry.location=MeshLocation::Face;break;
                case HydroCouple::Spatial::MeshLocation::Edge:entry.location=MeshLocation::Edge;break;
                case HydroCouple::Spatial::MeshLocation::Node:entry.location=MeshLocation::Node;break;
                default:message="unsupported recorded mesh location";return false;
            }
            if (auto *layered=dynamic_cast<HydroCouple::Spatial::ILayering *>(item)) {
                const auto v=std::find_if(manifest.results.begin(),manifest.results.end(),[&](const ResultEntry &e){return e.itemId==verticalFieldId(*item) && e.format==entry.format;});
                if (v!=manifest.results.end()) {
                    entry.geometry["vertical"]=v->toJson();
                    entry.geometry["vertical_kind"]=int(layered->verticalCoordinate()->kind());
                    const auto l=std::find(dims.begin(),dims.end(),layered->layerDimension());
                    if (l==dims.end()) {message="recorded layer dimension is absent";return false;}
                    entry.geometry["layer_axis"]=1+std::distance(dims.begin(),l);
                }
            }
        }
        std::erase_if(manifest.results,[](const ResultEntry &e){return e.itemId.starts_with("__hc_interfaces_");});
        return manifest.write(path,message);
    } catch (const std::exception &e) {message=std::string("recording result metadata: ")+e.what();return false;}
}

CompositionSpec compositionForRun(const RunManifest &manifest,
                                  const std::string &manifestPath)
{
    CompositionSpec spec;
    spec.schemaVersion       = kCompositionSchemaVersion;
    spec.metadata.id         = manifest.id;
    spec.metadata.caption    = manifest.caption.empty()
                                   ? ("Reopened run " + manifest.id)
                                   : manifest.caption;
    spec.metadata.created    = manifest.finished;
    spec.metadata.sdkVersion = manifest.sdkVersion;
    spec.metadata.interfaceVersion = manifest.interfaceVersion;

    // One open-mode block per component that contributed results, in
    // catalog order.
    std::vector<std::string> seen;
    for (const ResultEntry &entry : manifest.results)
    {
        if (std::find(seen.begin(), seen.end(), entry.componentId) !=
            seen.end())
            continue;
        seen.push_back(entry.componentId);

        ComponentSpec block;
        block.id              = entry.componentId;
        block.mode            = ExecutionMode::Open;
        block.resultsManifest = manifestPath;
        for (const ComponentOutcome &outcome : manifest.components)
        {
            if (outcome.id == entry.componentId)
            {
                block.caption = outcome.caption;
                break;
            }
        }
        spec.components.push_back(std::move(block));
    }

    return spec;
}

} // namespace HydroCouple::SDK::IO
