// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/component/abstractmodelcomponentinfo.h"
#include "hydrocouplesdk/data/abstractinput.h"
#include "hydrocouplesdk/data/abstractoutput.h"
#include "hydrocouplesdk/data/abstractargument.h"
#include "hydrocouplesdk/component/componentstatuschangeeventargs.h"

#ifdef USE_MPI
#include <mpi.h>
#endif

namespace HydroCouple::SDK {

AbstractModelComponent::AbstractModelComponent(std::string_view id,
                                                AbstractModelComponentInfo *info)
    : Identity(id)
    , m_componentInfo(info)
{
}

AbstractModelComponent::AbstractModelComponent(std::string_view id,
                                                std::string_view caption,
                                                AbstractModelComponentInfo *info)
    : Identity(id, caption)
    , m_componentInfo(info)
{
}

// ── IModelComponent ───────────────────────────────────────────────────────

HydroCouple::IModelComponentInfo *AbstractModelComponent::componentInfo() const
{
    return m_componentInfo;
}

HydroCouple::IModelComponent::ComponentStatus AbstractModelComponent::status() const
{
    return m_status;
}

std::vector<HydroCouple::IArgument *> AbstractModelComponent::arguments() const
{
    // The v2 lifecycle contract allows arguments() in the Created state, so
    // the argument set is materialized lazily on first access (initialize()
    // reuses the same guard and will not create them twice).
    const_cast<AbstractModelComponent *>(this)->ensureArgumentsCreated();
    std::vector<HydroCouple::IArgument *> result;
    result.reserve(m_orderedArguments.size());
    for (auto *a : m_orderedArguments)
        result.push_back(a);
    return result;
}

void AbstractModelComponent::ensureArgumentsCreated()
{
    if (!m_argumentsCreated)
    {
        m_argumentsCreated = true;
        createArguments();
    }
}

std::vector<HydroCouple::IInput *> AbstractModelComponent::inputs() const
{
    std::vector<HydroCouple::IInput *> result;
    result.reserve(m_orderedInputs.size());
    for (auto *i : m_orderedInputs)
        result.push_back(i);
    return result;
}

std::vector<HydroCouple::IOutput *> AbstractModelComponent::outputs() const
{
    std::vector<HydroCouple::IOutput *> result;
    result.reserve(m_orderedOutputs.size());
    for (auto *o : m_orderedOutputs)
        result.push_back(o);
    return result;
}

std::vector<HydroCouple::IComponentDataItem *> AbstractModelComponent::results() const
{
    return {};
}

void AbstractModelComponent::initialize()
{
    auto prevStatus = m_status;
    if (prevStatus != ComponentStatus::Created   &&
        prevStatus != ComponentStatus::Failed    &&
        prevStatus != ComponentStatus::Initialized)
        return;

    setStatus(ComponentStatus::Initializing, "Initializing component");

    // Populate arguments if not yet done
    if (m_orderedArguments.empty())
        ensureArgumentsCreated();

    std::string msg;
    if (!initializeArguments(msg)) {
        setStatus(ComponentStatus::Failed, "Argument initialization failed: " + msg);
        initializeFailureCleanUp();
        return;
    }

    clearInputs();
    clearOutputs();
    createInputs();
    createOutputs();
    initializeAdaptedOutputs();

    m_initialized = true;
    setStatus(ComponentStatus::Initialized, "Component initialized successfully");
}

// ── Workflow ──────────────────────────────────────────────────────────────

const HydroCouple::IWorkflowComponent *AbstractModelComponent::workflow() const
{
    return m_workflowComponent;
}

void AbstractModelComponent::setWorkflow(const HydroCouple::IWorkflowComponent *wf)
{
    m_workflowComponent = wf;
    emitPropertyChanged("Workflow");
}

// ── Capabilities and diagnostics (v2 contracts) ───────────────────────────

std::set<Capability> AbstractModelComponent::capabilities() const
{
    return {};
}

std::vector<ErrorEntry> AbstractModelComponent::errors(bool clearAfterRead)
{
    std::lock_guard lock(m_errorMutex);
    std::vector<ErrorEntry> result = m_errorQueue;
    if (clearAfterRead)
        m_errorQueue.clear();
    return result;
}

void AbstractModelComponent::reportError(ErrorEntry::Severity severity,
                                          int32_t code,
                                          const std::string &message)
{
    std::lock_guard lock(m_errorMutex);
    m_errorQueue.push_back(ErrorEntry{severity, code, id(), message});
}

// ── MPI (SDK transport machinery) ─────────────────────────────────────────

int AbstractModelComponent::mpiNumOfProcesses() const
{
#ifdef USE_MPI
    int n = 1;
    if (m_ComponentMPIComm != MPI_COMM_NULL)
        MPI_Comm_size(m_ComponentMPIComm, &n);
    return n;
#else
    return 1;
#endif
}

int AbstractModelComponent::mpiProcessRank() const { return m_mpiProcess; }

void AbstractModelComponent::mpiSetProcessRank(int rank) { m_mpiProcess = rank; }

std::set<int> AbstractModelComponent::mpiAllocatedProcesses() const
{
    return m_mpiAllocatedProcesses;
}

void AbstractModelComponent::mpiAllocateProcesses(const std::set<int> &procs)
{
    m_mpiAllocatedProcesses = procs;
    m_mpiAllocatedProcessesArray.assign(procs.begin(), procs.end());
    emitPropertyChanged("MpiAllocatedProcesses");
}

void AbstractModelComponent::mpiClearAllocatedProcesses()
{
    m_mpiAllocatedProcesses.clear();
    m_mpiAllocatedProcessesArray.clear();
    emitPropertyChanged("MpiAllocatedProcesses");
}

// ── GPU ───────────────────────────────────────────────────────────────────

int AbstractModelComponent::gpuPlatform(int processor) const
{
    auto it = m_gpuAllocation.find(processor);
    return it != m_gpuAllocation.end() ? std::get<0>(it->second) : -1;
}

int AbstractModelComponent::gpuDevice(int processor) const
{
    auto it = m_gpuAllocation.find(processor);
    return it != m_gpuAllocation.end() ? std::get<1>(it->second) : -1;
}

int AbstractModelComponent::gpuMaxNumBlocksOrWorkGrps(int processor) const
{
    auto it = m_gpuAllocation.find(processor);
    return it != m_gpuAllocation.end() ? std::get<2>(it->second) : 0;
}

void AbstractModelComponent::gpuAllocatedResources(int mpiProcess, int gpuPlatform,
                                                     int gpuDevice, int maxBlocks)
{
    m_gpuAllocation[mpiProcess] = {gpuPlatform, gpuDevice, maxBlocks};
}

void AbstractModelComponent::gpuClearAllocatedResources()
{
    m_gpuAllocation.clear();
}

// ── Reference directory ───────────────────────────────────────────────────

std::string AbstractModelComponent::referenceDirectory() const
{
    return m_referenceDir.string();
}

void AbstractModelComponent::setReferenceDirectory(const std::string &dir)
{
    m_referenceDir = dir;
    emitPropertyChanged("ReferenceDirectory");
}

std::filesystem::path AbstractModelComponent::getAbsoluteFilePath(
    const std::filesystem::path &filePath) const
{
    if (filePath.is_absolute()) return filePath;
    return m_referenceDir / filePath;
}

std::filesystem::path AbstractModelComponent::getRelativeFilePath(
    const std::filesystem::path &filePath) const
{
    if (m_referenceDir.empty()) return filePath;
    return std::filesystem::relative(filePath, m_referenceDir);
}

// ── ISignal<const shared_ptr<IComponentStatusChangeEventArgs>&> ───────────

void AbstractModelComponent::connect(
    const std::shared_ptr<ISlot<const std::shared_ptr<IComponentStatusChangeEventArgs> &>> &slot)
{
    m_statusChanged.connect(slot);
}

void AbstractModelComponent::disconnect(
    const std::shared_ptr<ISlot<const std::shared_ptr<IComponentStatusChangeEventArgs> &>> &slot)
{
    m_statusChanged.disconnect(slot);
}

void AbstractModelComponent::blockSignals(bool block)
{
    Description::blockSignals(block);
    m_statusChanged.blockSignals(block);
}

void AbstractModelComponent::emit(
    const std::shared_ptr<IComponentStatusChangeEventArgs> &args)
{
    m_statusChanged.emit(args);
}

// ── Non-interface helpers ─────────────────────────────────────────────────

int  AbstractModelComponent::index()        const { return m_index; }
void AbstractModelComponent::setIndex(int i)      { m_index = i; }
bool AbstractModelComponent::isInitialized() const { return m_initialized; }
bool AbstractModelComponent::isPrepared()    const { return m_prepared; }

void AbstractModelComponent::applyInputValues()
{
    for (auto *input : m_orderedInputs) {
        if (auto *ai = dynamic_cast<AbstractInput *>(input)) {
            ai->retrieveValuesFromProvider();
            ai->applyData();
        }
    }
}

void AbstractModelComponent::updateOutputValues(
    const std::vector<HydroCouple::IOutput *> &requiredOutputs)
{
    if (requiredOutputs.empty()) {
        for (auto *o : m_orderedOutputs)
            if (auto *ao = dynamic_cast<AbstractOutput *>(o))
                ao->updateValues();
    } else {
        for (auto *o : requiredOutputs)
            if (auto *ao = dynamic_cast<AbstractOutput *>(o))
                ao->updateValues();
    }
}

void AbstractModelComponent::initializeAdaptedOutputs()
{
    for (auto *output : m_orderedOutputs)
        if (auto *ao = dynamic_cast<AbstractOutput *>(output))
            ao->initializeAdaptedOutputs();
}

// ── Status management ─────────────────────────────────────────────────────

void AbstractModelComponent::setStatus(
    HydroCouple::IModelComponent::ComponentStatus status)
{
    auto prev = m_status;
    m_status  = status;
    auto args = std::make_shared<ComponentStatusChangeEventArgs>(
        this, status, prev);
    m_statusChanged.emit(args);
    emitPropertyChanged("Status");
}

void AbstractModelComponent::setStatus(
    HydroCouple::IModelComponent::ComponentStatus status,
    const std::string &message)
{
    auto prev = m_status;
    m_status  = status;
    if (status == ComponentStatus::Failed)
        reportError(ErrorEntry::Severity::Fatal, 0, message);
    auto args = std::shared_ptr<ComponentStatusChangeEventArgs>(
        new ComponentStatusChangeEventArgs(this, status, prev, message));
    m_statusChanged.emit(args);
    emitPropertyChanged("Status");
}

void AbstractModelComponent::setStatus(
    HydroCouple::IModelComponent::ComponentStatus status,
    const std::string &message, float progress)
{
    auto prev = m_status;
    m_status  = status;
    auto args = std::shared_ptr<ComponentStatusChangeEventArgs>(
        new ComponentStatusChangeEventArgs(this, status, prev, message, progress));
    m_statusChanged.emit(args);
    emitPropertyChanged("Status");
}

// ── Registry mutators ─────────────────────────────────────────────────────

void AbstractModelComponent::addInput(AbstractInput *input)
{
    if (!input) return;
    auto [it, inserted] = m_inputs.emplace(input->id(), input);
    if (inserted) m_orderedInputs.push_back(input);
    emitPropertyChanged("Inputs");
}

bool AbstractModelComponent::removeInput(AbstractInput *input)
{
    if (!input) return false;
    auto it = m_inputs.find(input->id());
    if (it == m_inputs.end() || it->second != input) return false;
    m_inputs.erase(it);
    m_orderedInputs.erase(
        std::remove(m_orderedInputs.begin(), m_orderedInputs.end(), input),
        m_orderedInputs.end());
    emitPropertyChanged("Inputs");
    return true;
}

void AbstractModelComponent::clearInputs()
{
    m_inputs.clear();
    m_orderedInputs.clear();
}

std::unordered_map<std::string, AbstractInput *>
AbstractModelComponent::inputsInternal() const { return m_inputs; }

void AbstractModelComponent::addOutput(AbstractOutput *output)
{
    if (!output) return;
    auto [it, inserted] = m_outputs.emplace(output->id(), output);
    if (inserted) m_orderedOutputs.push_back(output);
    emitPropertyChanged("Outputs");
}

bool AbstractModelComponent::removeOutput(AbstractOutput *output)
{
    if (!output) return false;
    auto it = m_outputs.find(output->id());
    if (it == m_outputs.end() || it->second != output) return false;
    m_outputs.erase(it);
    m_orderedOutputs.erase(
        std::remove(m_orderedOutputs.begin(), m_orderedOutputs.end(), output),
        m_orderedOutputs.end());
    emitPropertyChanged("Outputs");
    return true;
}

void AbstractModelComponent::clearOutputs()
{
    m_outputs.clear();
    m_orderedOutputs.clear();
}

std::unordered_map<std::string, AbstractOutput *>
AbstractModelComponent::outputsInternal() const { return m_outputs; }

void AbstractModelComponent::addArgument(AbstractArgument *argument)
{
    if (!argument) return;
    auto [it, inserted] = m_arguments.emplace(argument->id(), argument);
    if (inserted) m_orderedArguments.push_back(argument);
    emitPropertyChanged("Arguments");
}

bool AbstractModelComponent::removeArgument(AbstractArgument *argument)
{
    if (!argument) return false;
    auto it = m_arguments.find(argument->id());
    if (it == m_arguments.end() || it->second != argument) return false;
    m_arguments.erase(it);
    m_orderedArguments.erase(
        std::remove(m_orderedArguments.begin(), m_orderedArguments.end(), argument),
        m_orderedArguments.end());
    emitPropertyChanged("Arguments");
    return true;
}

void AbstractModelComponent::clearArguments()
{
    m_arguments.clear();
    m_orderedArguments.clear();
}

std::unordered_map<std::string, AbstractArgument *>
AbstractModelComponent::argumentsInternal() const { return m_arguments; }

MPI_Comm AbstractModelComponent::mpiCommunicator() const
{
    return m_ComponentMPIComm;
}

// ── Status to string ──────────────────────────────────────────────────────

std::string AbstractModelComponent::statusToString(
    HydroCouple::IModelComponent::ComponentStatus status)
{
    switch (status) {
    case ComponentStatus::Created:       return "Created";
    case ComponentStatus::Initializing:  return "Initializing";
    case ComponentStatus::Initialized:   return "Initialized";
    case ComponentStatus::Validating:    return "Validating";
    case ComponentStatus::Valid:         return "Valid";
    case ComponentStatus::WaitingForData:return "WaitingForData";
    case ComponentStatus::Invalid:       return "Invalid";
    case ComponentStatus::Preparing:     return "Preparing";
    case ComponentStatus::Updating:      return "Updating";
    case ComponentStatus::Updated:       return "Updated";
    case ComponentStatus::Checkpointing: return "Checkpointing";
    case ComponentStatus::Done:          return "Done";
    case ComponentStatus::Finishing:     return "Finishing";
    case ComponentStatus::Finished:      return "Finished";
    case ComponentStatus::Failed:        return "Failed";
    default:                             return "Unknown";
    }
}

} // namespace HydroCouple::SDK
