// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/component/abstractworkflowcomponent.h"
#include "hydrocouplesdk/component/workflowstatuschangeeventargs.h"
#include "hydrocouplesdk/core/diagnosticcodes.h"
#include "hydrocouplehelpers.h"

#include <algorithm>
#include <iterator>

namespace HydroCouple::SDK {

using WorkflowStatus = HydroCouple::IWorkflowComponent::WorkflowStatus;
using ComponentStatus = HydroCouple::IModelComponent::ComponentStatus;

AbstractWorkflowComponent::AbstractWorkflowComponent(
    std::string_view id, HydroCouple::IWorkflowComponentInfo *info)
    : Identity(id), m_componentInfo(info)
{
}

AbstractWorkflowComponent::~AbstractWorkflowComponent()
{
    for (auto *component : m_components)
    {
        if (component->workflow() == this)
            component->setWorkflow(nullptr);
    }
}

// ── IWorkflowComponent ────────────────────────────────────────────────────

HydroCouple::IWorkflowComponentInfo *
AbstractWorkflowComponent::componentInfo() const
{
    return m_componentInfo;
}

std::vector<HydroCouple::IIdentity *>
AbstractWorkflowComponent::modelComponentLabels() const
{
    std::vector<HydroCouple::IIdentity *> labels;
    labels.reserve(m_roles.size());
    for (const auto &role : m_roles)
        labels.push_back(role.label.get());
    return labels;
}

bool AbstractWorkflowComponent::isRequiredModelComponent(
    const HydroCouple::IIdentity *modelComponentLabel) const
{
    if (!modelComponentLabel)
        return false;
    for (const auto &role : m_roles)
    {
        if (role.label->id() == modelComponentLabel->id())
            return role.required;
    }
    return false;
}

void AbstractWorkflowComponent::initialize()
{
    if (m_status != WorkflowStatus::Created &&
        m_status != WorkflowStatus::Failed)
    {
        reportError(ErrorEntry::Severity::Warning, 0,
                    "initialize() ignored in state " + statusToString(m_status));
        return;
    }

    m_stopRequested  = false;
    m_pauseRequested = false;
    setStatus(WorkflowStatus::Initializing);

    for (auto *component : m_components)
    {
        if (component->status() == ComponentStatus::Created ||
            component->status() == ComponentStatus::Failed)
            component->initialize();

        if (component->status() == ComponentStatus::Failed)
        {
            setStatus(WorkflowStatus::Failed,
                      "component '" + component->id() +
                          "' failed to initialize");
            return;
        }
    }

    std::string message;
    if (!onInitialize(message))
    {
        setStatus(WorkflowStatus::Failed, message);
        return;
    }
    setStatus(WorkflowStatus::Initialized);
}

std::vector<std::string> AbstractWorkflowComponent::validate()
{
    if (m_status != WorkflowStatus::Initialized &&
        m_status != WorkflowStatus::Validated)
        return {"validate() requires the Initialized state (current: " +
                statusToString(m_status) + ")"};

    setStatus(WorkflowStatus::Validating);
    std::vector<std::string> problems;

    for (const auto &role : m_roles)
    {
        if (role.required && role.component == nullptr)
            problems.push_back("required role '" + role.label->id() +
                               "' is not filled");
    }

    for (auto *component : m_components)
    {
        for (auto &problem : component->validate())
            problems.push_back("component '" + component->id() +
                               "': " + problem);
    }

    const std::vector<ConnectionFinding> links = validateConnections();

    validateComposition(problems);

    // Link warnings are worth seeing whatever the outcome; they fail nothing.
    for (const auto &finding : links)
        if (finding.severity != ErrorEntry::Severity::Error)
            reportError(finding.severity, finding.code, finding.message);

    for (const auto &problem : problems)
        reportError(ErrorEntry::Severity::Error, 0, problem);
    for (const auto &finding : links)
    {
        if (finding.severity != ErrorEntry::Severity::Error)
            continue;
        reportError(ErrorEntry::Severity::Error, finding.code, finding.message);
        problems.push_back(finding.message);
    }

    if (problems.empty())
        setStatus(WorkflowStatus::Validated);
    else
        setStatus(WorkflowStatus::Failed,
                  "composition validation found " +
                      std::to_string(problems.size()) + " problem(s)");
    return problems;
}

void AbstractWorkflowComponent::prepare()
{
    if (m_status != WorkflowStatus::Validated)
    {
        setStatus(WorkflowStatus::Failed,
                  "prepare() requires the Validated state (current: " +
                      statusToString(m_status) + ")");
        return;
    }

    setStatus(WorkflowStatus::Preparing);

    for (auto *component : m_components)
    {
        component->prepare();
        if (component->status() == ComponentStatus::Failed)
        {
            setStatus(WorkflowStatus::Failed,
                      "component '" + component->id() + "' failed to prepare");
            return;
        }
    }

    std::string message;
    if (!onPrepare(message))
    {
        setStatus(WorkflowStatus::Failed, message);
        return;
    }
    setStatus(WorkflowStatus::Prepared);
}

void AbstractWorkflowComponent::update()
{
    if (m_status != WorkflowStatus::Prepared &&
        m_status != WorkflowStatus::Updated)
    {
        setStatus(WorkflowStatus::Failed,
                  "update() requires the Prepared or Updated state (current: " +
                      statusToString(m_status) + ")");
        return;
    }

    setStatus(WorkflowStatus::Updating);

    std::string message;
    if (!performStep(message))
    {
        setStatus(WorkflowStatus::Failed, message);
        return;
    }

    // The step completed: this is a synchronization point — honor control
    // requests, then Done detection.
    if (m_stopRequested)
    {
        setStatus(WorkflowStatus::Done, "stopped on request");
        return;
    }
    if (isCompositionDone())
    {
        setStatus(WorkflowStatus::Done);
        return;
    }
    if (m_pauseRequested.exchange(false))
    {
        setStatus(WorkflowStatus::Paused, "paused on request");
        return;
    }
    setStatus(WorkflowStatus::Updated);
}

void AbstractWorkflowComponent::finish()
{
    if (m_status == WorkflowStatus::Finished)
        return;

    setStatus(WorkflowStatus::Finishing);

    for (auto *component : m_components)
    {
        if (component->status() != ComponentStatus::Finished &&
            component->status() != ComponentStatus::Created)
            component->finish();
    }

    onFinish();
    setStatus(WorkflowStatus::Finished);
}

void AbstractWorkflowComponent::requestStop()
{
    m_stopRequested = true;
}

void AbstractWorkflowComponent::requestPause()
{
    m_pauseRequested = true;
}

void AbstractWorkflowComponent::resume()
{
    if (m_status == WorkflowStatus::Paused)
    {
        m_pauseRequested = false;
        setStatus(WorkflowStatus::Updated, "resumed");
    }
}

WorkflowStatus AbstractWorkflowComponent::status() const
{
    return m_status;
}

std::vector<HydroCouple::IModelComponent *>
AbstractWorkflowComponent::modelComponents() const
{
    return m_components;
}

bool AbstractWorkflowComponent::addModelComponent(
    HydroCouple::IModelComponent *component,
    const HydroCouple::IIdentity *modelRoleIdentifier, std::string *message)
{
    if (!component)
    {
        if (message)
            *message = "component is null";
        return false;
    }
    if (std::find(m_components.begin(), m_components.end(), component) !=
        m_components.end())
    {
        if (message)
            *message = "component '" + component->id() + "' is already managed";
        return false;
    }

    if (modelRoleIdentifier)
    {
        RoleSlot *slot = nullptr;
        for (auto &role : m_roles)
        {
            if (role.label->id() == modelRoleIdentifier->id())
            {
                slot = &role;
                break;
            }
        }
        if (!slot)
        {
            if (message)
                *message = "unknown role '" + modelRoleIdentifier->id() + "'";
            return false;
        }
        if (slot->component)
        {
            if (message)
                *message = "role '" + modelRoleIdentifier->id() +
                           "' is already filled by '" +
                           slot->component->id() + "'";
            return false;
        }
        slot->component = component;
    }

    m_components.push_back(component);
    component->setWorkflow(this);
    if (message)
        message->clear();
    return true;
}

bool AbstractWorkflowComponent::removeModelComponent(
    HydroCouple::IModelComponent *component)
{
    auto found = std::find(m_components.begin(), m_components.end(), component);
    if (found == m_components.end())
        return false;

    for (auto &role : m_roles)
    {
        if (role.component == component)
            role.component = nullptr;
    }
    m_components.erase(found);
    if (component->workflow() == this)
        component->setWorkflow(nullptr);
    return true;
}

std::vector<HydroCouple::ErrorEntry>
AbstractWorkflowComponent::errors(bool clearAfterRead)
{
    std::scoped_lock lock(m_errorMutex);
    std::vector<ErrorEntry> drained = m_errorQueue;
    if (clearAfterRead)
        m_errorQueue.clear();
    return drained;
}

// ── ISignal ───────────────────────────────────────────────────────────────

void AbstractWorkflowComponent::connect(
    const std::shared_ptr<ISlot<
        const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &>> &slot)
{
    m_statusChanged.connect(slot);
}

void AbstractWorkflowComponent::disconnect(
    const std::shared_ptr<ISlot<
        const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &>> &slot)
{
    m_statusChanged.disconnect(slot);
}

void AbstractWorkflowComponent::blockSignals(bool block)
{
    Description::blockSignals(block);
    m_statusChanged.blockSignals(block);
}

void AbstractWorkflowComponent::emit(
    const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &args)
{
    m_statusChanged.emit(args);
}

// ── Non-interface helpers ─────────────────────────────────────────────────

HydroCouple::IModelComponent *
AbstractWorkflowComponent::componentForRole(const std::string &roleId) const
{
    for (const auto &role : m_roles)
    {
        if (role.label->id() == roleId)
            return role.component;
    }
    return nullptr;
}

std::string AbstractWorkflowComponent::statusToString(WorkflowStatus status)
{
    switch (status)
    {
        case WorkflowStatus::Created:      return "Created";
        case WorkflowStatus::Initializing: return "Initializing";
        case WorkflowStatus::Initialized:  return "Initialized";
        case WorkflowStatus::Validating:   return "Validating";
        case WorkflowStatus::Validated:    return "Validated";
        case WorkflowStatus::Preparing:    return "Preparing";
        case WorkflowStatus::Prepared:     return "Prepared";
        case WorkflowStatus::Updating:     return "Updating";
        case WorkflowStatus::Updated:      return "Updated";
        case WorkflowStatus::Paused:       return "Paused";
        case WorkflowStatus::Done:         return "Done";
        case WorkflowStatus::Finishing:    return "Finishing";
        case WorkflowStatus::Finished:     return "Finished";
        case WorkflowStatus::Failed:       return "Failed";
    }
    return "Unknown";
}

// ── Strategy hook defaults ────────────────────────────────────────────────

bool AbstractWorkflowComponent::onInitialize(std::string &)
{
    return true;
}

std::vector<ConnectionFinding> AbstractWorkflowComponent::validateConnections()
{
    std::vector<ConnectionFinding> findings;
    for (auto *component : m_components)
    {
        if (!component)
            continue;
        for (HydroCouple::IInput *input : component->inputs())
        {
            if (!input)
                continue;
            std::vector<HydroCouple::IOutput *> providers;
            if (auto *multi = dynamic_cast<HydroCouple::IMultiInput *>(input))
                providers = multi->providers();
            else if (HydroCouple::IOutput *provider = input->provider())
                providers.push_back(provider);

            for (HydroCouple::IOutput *provider : providers)
            {
                if (!provider)
                    continue;
                auto link = validateConnection(*provider, *input);
                findings.insert(findings.end(), std::make_move_iterator(link.begin()),
                                std::make_move_iterator(link.end()));
            }
        }
    }
    return findings;
}

void AbstractWorkflowComponent::validateComposition(std::vector<std::string> &)
{
}

bool AbstractWorkflowComponent::onPrepare(std::string &)
{
    return true;
}

void AbstractWorkflowComponent::onFinish()
{
}

bool AbstractWorkflowComponent::isCompositionDone() const
{
    if (m_components.empty())
        return true;
    for (const auto *component : m_components)
    {
        const ComponentStatus status = component->status();
        if (status != ComponentStatus::Done &&
            status != ComponentStatus::Finished)
            return false;
    }
    return true;
}

// ── Role registration ─────────────────────────────────────────────────────

void AbstractWorkflowComponent::addModelComponentLabel(
    std::unique_ptr<Identity> label, bool required)
{
    RoleSlot slot;
    slot.label    = std::move(label);
    slot.required = required;
    m_roles.push_back(std::move(slot));
}

// ── Diagnostics ───────────────────────────────────────────────────────────

void AbstractWorkflowComponent::reportError(ErrorEntry::Severity severity,
                                            int32_t code,
                                            const std::string &message)
{
    ErrorEntry entry;
    entry.severity = severity;
    entry.code     = code;
    entry.source   = id();
    entry.message  = message;
    std::scoped_lock lock(m_errorMutex);
    m_errorQueue.push_back(std::move(entry));
}

// ── Status management ─────────────────────────────────────────────────────

void AbstractWorkflowComponent::setStatus(WorkflowStatus status)
{
    setStatus(status, {}, 0.0f);
}

void AbstractWorkflowComponent::setStatus(WorkflowStatus status,
                                          const std::string &message)
{
    setStatus(status, message, 0.0f);
}

void AbstractWorkflowComponent::setStatus(WorkflowStatus status,
                                          const std::string &message,
                                          float progress)
{
    const WorkflowStatus previous = m_status.exchange(status);

    // Checked against the normative table and reported -- once per distinct
    // (from, to) -- rather than refused, for the same reason as components:
    // a refused Done would strand whoever waits for it.
    if (previous != status &&
        !HydroCouple::Helpers::isValidWorkflowStatusTransition(previous, status))
    {
        bool firstTime = false;
        {
            std::scoped_lock lock(m_errorMutex);
            firstTime = m_reportedIllegalTransitions
                            .emplace(static_cast<int>(previous), static_cast<int>(status))
                            .second;
        }
        if (firstTime)
            reportError(ErrorEntry::Severity::Error,
                        Diagnostic::IllegalStatusTransition,
                        "illegal workflow status transition " +
                            statusToString(previous) + " -> " + statusToString(status) +
                            " (Helpers::isValidWorkflowStatusTransition rejects it)");
    }

    if (status == WorkflowStatus::Failed)
        reportError(ErrorEntry::Severity::Fatal, 0,
                    message.empty() ? "workflow failed" : message);

    auto args = std::make_shared<WorkflowStatusChangeEventArgs>(
        this, status, previous, message, progress);
    emit(args);
}

} // namespace HydroCouple::SDK
