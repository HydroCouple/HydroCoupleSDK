// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   abstractmodelcomponent.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Abstract base implementation of HydroCouple::IModelComponent.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTMODELCOMPONENT_H
#define HYDROCOUPLESDK_ABSTRACTMODELCOMPONENT_H

#include "identity.h"
#include "signal.h"
#include "hydrocouple.h"

#include <filesystem>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#ifdef USE_MPI
#include <mpi.h>
#else
using MPI_Comm  = int;
using MPI_Group = int;
#endif

namespace HydroCouple {

class AbstractModelComponentInfo;
class AbstractOutput;
class AbstractInput;
class AbstractArgument;
class ComponentStatusChangeEventArgs;

/*!
 * \brief Abstract base class for HydroCouple::IModelComponent.
 *
 * Provides:
 * - Status-change signal (ISignal<const shared_ptr<IComponentStatusChangeEventArgs>&>)
 * - Ordered argument / input / output registries
 * - Reference directory management (std::filesystem::path)
 * - MPI resource tracking (process rank, allocated processes)
 * - GPU allocation table (mpiProcess → (gpuPlatform, gpuDevice, maxBlocks))
 * - Component ordering index (used by composition managers)
 *
 * Lifecycle
 * ---------
 * 1. Created → createArguments() is called (by constructor or lazily)
 * 2. initialize() → Initializing → calls initializeArguments()
 *    → createInputs(), createOutputs()
 *    → Initialized or Failed
 * 3. validate() → Validating → Valid or Invalid
 * 4. prepare()  → Preparing  → Updated
 * 5. update()   → Updating   → Updated / Done / Failed
 * 6. finish()   → Finishing  → Finished or Created
 *
 * Subclasses must implement createArguments(), initializeArguments(),
 * createInputs(), createOutputs(), validate(), prepare(), update(),
 * finish(), and initializeFailureCleanUp().
 *
 * Thread safety
 * -------------
 * The status-change signal is thread-safe (std::shared_mutex).
 * The argument/input/output maps are NOT: mutate them only from the
 * owning thread during the initialize/prepare lifecycle phases.
 */
class HYDROCOUPLESDK_EXPORT AbstractModelComponent
    : public Identity,
      public virtual HydroCouple::IModelComponent
{
    friend class AbstractModelComponentInfo;

public:
    /*!
     * \brief Construct with an id and optional component-info pointer.
     */
    explicit AbstractModelComponent(std::string_view id,
                                     AbstractModelComponentInfo *info = nullptr);

    /*!
     * \brief Construct with id, caption, and optional component-info.
     */
    AbstractModelComponent(std::string_view id,
                            std::string_view caption,
                            AbstractModelComponentInfo *info = nullptr);

    ~AbstractModelComponent() override = default;

    // ── IModelComponent ───────────────────────────────────────────────────

    [[nodiscard]] HydroCouple::IModelComponentInfo *componentInfo() const override final;

    [[nodiscard]] HydroCouple::IModelComponent::ComponentStatus status() const override final;

    [[nodiscard]] std::vector<HydroCouple::IArgument *>       arguments() const override final;
    [[nodiscard]] std::vector<HydroCouple::IInput *>          inputs()    const override final;
    [[nodiscard]] std::vector<HydroCouple::IOutput *>         outputs()   const override final;
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> results() const override;

    /*!
     * \brief Default initialize(): transitions Created/Failed → Initialized.
     *
     * Calls in order: createArguments(), initializeArguments(),
     * createInputs(), createOutputs(), initializeAdaptedOutputs().
     * Sets status to Initializing before and Initialized/Failed after.
     */
    void initialize() override;

    /*!
     * \brief Validates the component's current configuration.
     * \details Sets status to Validating, calls the subclass implementation.
     * Pure virtual — subclasses set status to Valid/Invalid.
     */
    std::vector<std::string> validate() override = 0;

    /*!
     * \brief Prepares the component for time stepping.
     * Pure virtual — subclasses implement domain-specific preparation.
     */
    void prepare() override = 0;

    /*!
     * \brief Advances the component by one update step.
     * Pure virtual — subclasses implement time-stepping logic.
     */
    void update(const std::initializer_list<HydroCouple::IOutput *> &requiredOutputs = {})
        override = 0;

    /*!
     * \brief Finalises the component.
     * Pure virtual — subclasses write output files and free resources.
     */
    void finish() override = 0;

    // ── Workflow ──────────────────────────────────────────────────────────

    [[nodiscard]] const HydroCouple::IWorkflowComponent *workflow() const override;
    void setWorkflow(const HydroCouple::IWorkflowComponent *workflow) override;

    // ── MPI ───────────────────────────────────────────────────────────────

    [[nodiscard]] int             mpiNumOfProcesses()  const override;
    [[nodiscard]] int             mpiProcessRank()     const override;
    void mpiSetProcessRank(int rank)                         override;
    [[nodiscard]] std::set<int>   mpiAllocatedProcesses()    const override;
    void mpiAllocateProcesses(const std::set<int> &procs)    override;
    void mpiClearAllocatedProcesses()                        override;

    // ── GPU ───────────────────────────────────────────────────────────────

    [[nodiscard]] int gpuPlatform(int processor)            const;
    [[nodiscard]] int gpuDevice(int processor)              const;
    [[nodiscard]] int gpuMaxNumBlocksOrWorkGrps(int processor) const;
    void gpuAllocatedResources(int mpiProcess, int gpuPlatform,
                                int gpuDevice, int maxBlocks);
    void gpuClearAllocatedResources();

    // ── Reference directory ───────────────────────────────────────────────

    [[nodiscard]] std::string referenceDirectory() const override;
    void setReferenceDirectory(const std::string &dir) override;

    // ── UI stubs ──────────────────────────────────────────────────────────

    [[nodiscard]] bool hasEditor() const override { return false; }
    void showEditor(void * = nullptr) override {}
    [[nodiscard]] bool hasViewer() const override { return false; }
    void showViewer(void * = nullptr) override {}

    // ── ISignal<const shared_ptr<IComponentStatusChangeEventArgs>&> ───────

    void connect(const std::shared_ptr<
        ISlot<const std::shared_ptr<IComponentStatusChangeEventArgs> &>> &slot) override;

    void disconnect(const std::shared_ptr<
        ISlot<const std::shared_ptr<IComponentStatusChangeEventArgs> &>> &slot) override;

    /*!
     * \brief Gates both the propertyChanged and componentStatusChanged signals.
     */
    void blockSignals(bool block) override;

    // ── Non-interface helpers ─────────────────────────────────────────────

    [[nodiscard]] int  index() const;
    void setIndex(int index);

    [[nodiscard]] bool isInitialized() const;
    [[nodiscard]] bool isPrepared()    const;

    /*!
     * \brief Applies values from all connected inputs (calls retrieveValuesFromProvider()
     *        + applyData() on each AbstractInput).
     */
    virtual void applyInputValues();

    /*!
     * \brief Triggers updateValues on each required output (or all outputs if empty).
     */
    virtual void updateOutputValues(
        const std::vector<HydroCouple::IOutput *> &requiredOutputs = {});

    /*!
     * \brief Calls initialize() on all adapted outputs registered on outputs.
     */
    virtual void initializeAdaptedOutputs();

    /*!
     * \brief Converts a filesystem path relative to the reference directory to absolute.
     */
    [[nodiscard]] std::filesystem::path getAbsoluteFilePath(
        const std::filesystem::path &filePath) const;

    /*!
     * \brief Converts an absolute path to one relative to the reference directory.
     */
    [[nodiscard]] std::filesystem::path getRelativeFilePath(
        const std::filesystem::path &filePath) const;

    /*!
     * \brief Returns a human-readable label for a ComponentStatus value.
     */
    static std::string statusToString(HydroCouple::IModelComponent::ComponentStatus status);

protected:
    // ── Pure-virtual lifecycle hooks ──────────────────────────────────────

    virtual void createArguments()  = 0;
    virtual bool initializeArguments(std::string &message) = 0;
    virtual void createInputs()     = 0;
    virtual void createOutputs()    = 0;

    /*!
     * \brief Called when initialize() fails; subclass releases partial state.
     */
    virtual void initializeFailureCleanUp() = 0;

    // ── Status management ─────────────────────────────────────────────────

    void setStatus(HydroCouple::IModelComponent::ComponentStatus status);

    virtual void setStatus(HydroCouple::IModelComponent::ComponentStatus status,
                            const std::string &message);

    virtual void setStatus(HydroCouple::IModelComponent::ComponentStatus status,
                            const std::string &message, float progress);

    // ── Registry mutators (called by subclasses) ──────────────────────────

    void addInput(AbstractInput *input);
    bool removeInput(AbstractInput *input);
    void clearInputs();
    [[nodiscard]] std::unordered_map<std::string, AbstractInput *> inputsInternal() const;

    void addOutput(AbstractOutput *output);
    bool removeOutput(AbstractOutput *output);
    void clearOutputs();
    [[nodiscard]] std::unordered_map<std::string, AbstractOutput *> outputsInternal() const;

    void addArgument(AbstractArgument *argument);
    bool removeArgument(AbstractArgument *argument);
    void clearArguments();
    [[nodiscard]] std::unordered_map<std::string, AbstractArgument *> argumentsInternal() const;

    // ── MPI communicator ─────────────────────────────────────────────────

    [[nodiscard]] MPI_Comm mpiCommunicator() const;

    // ── Protected ISignal emit (required pure-virtual) ────────────────────

    void emit(const std::shared_ptr<IComponentStatusChangeEventArgs> &args) override;

    // MPI state exposed for subclasses
    int                   m_mpiProcess  = 0;
    MPI_Comm              m_ComponentMPIComm{};
    std::set<int>         m_mpiAllocatedProcesses;
    std::vector<int>      m_mpiAllocatedProcessesArray;

private:
    void initializeIdentifierArgument();

    HydroCouple::IModelComponent::ComponentStatus m_status =
        HydroCouple::IModelComponent::ComponentStatus::Created;

    // Ordered insertion lists for deterministic iteration
    std::vector<AbstractInput *>    m_orderedInputs;
    std::vector<AbstractOutput *>   m_orderedOutputs;
    std::vector<AbstractArgument *> m_orderedArguments;

    // Fast O(1) lookup by id
    std::unordered_map<std::string, AbstractInput *>    m_inputs;
    std::unordered_map<std::string, AbstractOutput *>   m_outputs;
    std::unordered_map<std::string, AbstractArgument *> m_arguments;

    AbstractModelComponentInfo              *m_componentInfo  = nullptr;
    const HydroCouple::IWorkflowComponent  *m_workflowComponent = nullptr;

    bool m_initialized = false;
    bool m_prepared    = false;
    int  m_index       = 0;

    std::filesystem::path m_referenceDir;

    // GPU allocation: mpiProcess → (gpuPlatform, gpuDevice, maxBlocks)
    std::unordered_map<int, std::tuple<int, int, int>> m_gpuAllocation;

    // MPI group handles (non-owning when USE_MPI is off)
    MPI_Group m_worldGroup{};
    MPI_Group m_componentMPIGroup{};
    bool      m_mpiResourcesInitialized = false;

    Signal<const std::shared_ptr<IComponentStatusChangeEventArgs> &> m_statusChanged;
};

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_ABSTRACTMODELCOMPONENT_H
