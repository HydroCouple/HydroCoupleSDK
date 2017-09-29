/*! \file   abstractmodelcomponent.h
 *  \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version   1.0.0.0
 *  \section   Description
 *  This header files contains the the SDK interface definitions for the
 *  HydroCouple Component-Based modeling framework.
 *  \section License
 *  hydrocouple.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  hydrocouple.h its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2016
 *  \pre
 *  \bug
 *  \warning
 *  \todo
 */

#ifndef ABSTRACTMODELCOMPONENT_H
#define ABSTRACTMODELCOMPONENT_H

#include "identity.h"
#include <QMutex>
#include <QDir>
#include <QString>
#include <QSet>
#include <QHash>
#include <QXmlStreamReader>
#include <tuple>

class ModelComponentInfo;
class AbstractOutput;
class AbstractInput;
class AbstractArgument;
class ComponentStatusChangeEventArgs;
class AbstractAdaptedOutputFactory;
class IdBasedArgumentString;
class ProgressChecker;
struct SerializedData;

/*!
 * \brief  AbstractModelComponent class.
 */
class HYDROCOUPLESDK_EXPORT AbstractModelComponent : public Identity,
    public virtual HydroCouple::IModelComponent
{

    friend class ModelComponentInfo;

    Q_OBJECT

    Q_INTERFACES(HydroCouple::IModelComponent)
    Q_PROPERTY(HydroCouple::IModelComponentInfo* ComponentInfo READ componentInfo NOTIFY propertyChanged)
    Q_PROPERTY(QList<HydroCouple::IArgument*> Arguments READ arguments NOTIFY propertyChanged)
    Q_PROPERTY(QList<HydroCouple::IInput*> Inputs READ inputs NOTIFY propertyChanged)
    Q_PROPERTY(QList<HydroCouple::IOutput*> Outputs READ outputs NOTIFY propertyChanged)
    Q_PROPERTY(QList<HydroCouple::IAdaptedOutputFactory*> AdaptedOutputFactories READ adaptedOutputFactories NOTIFY propertyChanged)
    Q_PROPERTY(QString ReferenceDirectory READ referenceDirectory WRITE setReferenceDirectory NOTIFY propertyChanged)
    Q_PROPERTY(int Index READ index NOTIFY propertyChanged)

  public:

    AbstractModelComponent(const QString &id, ModelComponentInfo *modelComponentInfo = nullptr);

    AbstractModelComponent(const QString &id, const QString &caption, ModelComponentInfo *modelComponentInfo = nullptr);

    virtual ~AbstractModelComponent();

    int index() const override final;

    void setIndex(int index) override final;

    HydroCouple::IModelComponentInfo* componentInfo() const override final;

    QList<HydroCouple::IArgument*> arguments() const override final;

    virtual void createArguments() = 0;

    virtual bool initializeArguments(QString &message) = 0;

    IModelComponent::ComponentStatus status() const override final;

    QList<HydroCouple::IInput*> inputs() const override final;

    virtual void createInputs() = 0;

    QList<HydroCouple::IOutput*> outputs() const override final;

    virtual void createOutputs() = 0;

    QList<HydroCouple::IAdaptedOutputFactory*> adaptedOutputFactories() const override final;

    virtual void createAdaptedOutputFactories() = 0;

    void initialize() override;

    bool isInitialized() const;

    bool isPrepared() const;

    virtual void applyInputValues();

    virtual void updateOutputValues(const QList<HydroCouple::IOutput*>& requiredOutputs);

    virtual void initializeAdaptedOutputs();

    int mpiProcessRank() const override;

    void mpiSetProcessRank(int processRank) override;

    QSet<int> mpiAllocatedProcesses() const override;

    void mpiAllocateProcesses(const QSet<int> &allocatedProcessors) override;

    void mpiClearAllocatedProcesses() override;

#ifdef QT_DEBUG
    void mpiProcessMessage(const SerializableData &data) override;
#endif

    int gpuPlatform(int processor) const override;

    int gpuDevice(int processor) const override;

    int gpuMaxNumBlocksOrWorkGrps(int processor) const override;

    void gpuAllocatedResources(int mpiProcess, int gpuPlatform, int gpuDevice, int maxNumGPUBlocksOrWorkGrps) override;

    void gpuClearAllocatedResources() override;

    QString referenceDirectory() const override;

    void setReferenceDirectory(const QString &referenceDir) override;

    QFileInfo getAbsoluteFilePath(const QString &filePath) const;

    QFileInfo getRelativeFilePath(const QString &filePath) const;

    void readArguments(QXmlStreamReader &xmlReader);

    static void serializedDataToRawBytes(SerializableData &serializedData, char* &rawBytesData, int &length);

  signals:

    void componentStatusChanged(const QSharedPointer<HydroCouple::IComponentStatusChangeEventArgs> &statusChangedEvent) override ;

    void propertyChanged(const QString &propertyName) override;

  protected:

    void readArgument(QXmlStreamReader &xmlReader);

    void setInitialized(bool initialized);

    void setPrepared(bool prepared);

    virtual void intializeFailureCleanUp() = 0;

    void addInput(AbstractInput *input);

    bool removeInput(AbstractInput *input);

    void clearInputs();

    QHash<QString,AbstractInput*> inputsInternal() const;

    void addOutput(AbstractOutput *output);

    bool removeOutput(AbstractOutput *output);

    void clearOutputs();

    QHash<QString,AbstractOutput*> outputsInternal() const;


    void addAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory);

    bool removeAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory);

    void clearAdaptedOutputFactories();

    QHash<QString,AbstractAdaptedOutputFactory*> adaptedOutputFactoriesInternal() const;


    void addArgument(AbstractArgument *argument);

    bool removeArgument(AbstractArgument *argument);

    void clearArguments();

    QHash<QString,AbstractArgument*> argumentsInternal() const;


    void setStatus(HydroCouple::IModelComponent::ComponentStatus status);

    virtual void setStatus(HydroCouple::IModelComponent::ComponentStatus status, const QString &message);

    virtual void setStatus(HydroCouple::IModelComponent::ComponentStatus status, const QString &message, int progress);

    ProgressChecker *progressChecker() const;

    static QString statusToString(HydroCouple::IModelComponent::ComponentStatus status);

  private:

    void createIdentifierArguments();

    bool initializeIdentifierArguments(QString &message);

  private:

    HydroCouple::IModelComponent::ComponentStatus m_status;
    QHash<QString, AbstractInput*> m_inputs;
    QHash<QString, AbstractOutput*> m_outputs;
    QHash<QString, AbstractAdaptedOutputFactory*> m_adaptedOutputFactories;
    QHash<QString, AbstractArgument*> m_arguments;
    QList<AbstractArgument*> m_argumentsInsertionOrdered;
    ModelComponentInfo *m_modelComponentInfo;
    bool m_initialized, m_prepared;
    IdBasedArgumentString *m_identifiersArgument = nullptr;
    QDir m_referenceDir;
    ProgressChecker *m_progressChecker;
    int m_mpiProcess, m_index;
    QHash<int,std::tuple<int,int,int>>  m_gpuAllocation;
    QSet<int> m_mpiAllocatedProcesses;

};

Q_DECLARE_METATYPE(AbstractModelComponent*)

#endif // ABSTRACTMODELCOMPONENT_H
