<<<<<<< HEAD
/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#include "stdafx.h"
#include "core/abstractmodelcomponent.h"
#include "core/abstractmodelcomponentinfo.h"
#include "core/componentstatuschangeeventargs.h"
#include "core/abstractargument.h"
#include "core/abstractinput.h"
#include "core/abstractoutput.h"
#include "core/abstractadaptedoutputfactory.h"
#include "core/idbasedargument.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "progresschecker.h"

#include <QSharedPointer>
#include <QDataStream>
#include <math.h>
#include <QThread>
using namespace HydroCouple;

AbstractModelComponent::AbstractModelComponent(const QString &id, AbstractModelComponentInfo *modelComponentInfo)
  : Identity(id, modelComponentInfo->thread() == QThread::currentThread() ? modelComponentInfo : nullptr),
    m_status(IModelComponent::Created),
    m_modelComponentInfo(modelComponentInfo),
    m_initialized(false),
    m_prepared(false),
    m_index(0),
    m_mpiProcess(0)
{
  m_workflowComponent = nullptr;
  m_progressChecker = new ProgressChecker(0,100,100,this);
  m_referenceDir = QFileInfo(m_modelComponentInfo->libraryFilePath()).dir();
  emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(IModelComponent::Created,
                                                                                                                 IModelComponent::Created,
                                                                                                                 "Created SWMM Model" , 0 , this)));

#ifdef USE_MPI
  MPI_Comm_group(MPI_COMM_WORLD, &m_worldGroup);
  int worldGroupSize = 0;
  MPI_Group_size(m_worldGroup, &worldGroupSize);
//  printf("World Group Size: %i\n", worldGroupSize);
#endif

  createIdentifierArguments();
}

AbstractModelComponent::AbstractModelComponent(const QString &id, const QString &caption, AbstractModelComponentInfo *modelComponentInfo)
  : Identity(id,caption,modelComponentInfo->thread() == QThread::currentThread() ? modelComponentInfo : nullptr),
    m_status(HydroCouple::IModelComponent::Created),
    m_modelComponentInfo(modelComponentInfo),
    m_initialized(false),
    m_prepared(false),
    m_index(0),
    m_mpiProcess(0)
{
  m_workflowComponent = nullptr;
  m_progressChecker = new ProgressChecker(0,100,100,this);
  m_referenceDir = QFileInfo(m_modelComponentInfo->libraryFilePath()).dir();
  emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(IModelComponent::Created,
                                                                                                                 IModelComponent::Created,
                                                                                                                 "Created SWMM Model",
                                                                                                                 0,this)));
#ifdef USE_MPI
  MPI_Comm_group(MPI_COMM_WORLD, &m_worldGroup);
  int worldGroupSize = 0;
  MPI_Group_size(m_worldGroup, &worldGroupSize);
//  printf("World Group Size: %i\n", worldGroupSize);
#endif

  createIdentifierArguments();
}

AbstractModelComponent::~AbstractModelComponent()
{
  qDeleteAll(m_inputs.values());
  m_inputs.clear();
  m_orderedInputs.clear();

  qDeleteAll(m_outputs.values());
  m_outputs.clear();
  m_orderedOutputs.clear();

  qDeleteAll(m_arguments.values());
  m_arguments.clear();
  m_argumentsInsertionOrdered.clear();

#ifdef USE_MPI

  if(m_mpiResourcesInitialized)
  {
    MPI_Group_free(&m_ComponentMPIGroup);
    MPI_Comm_free(&m_ComponentMPIComm);
  }

  MPI_Group_free(&m_worldGroup);

#endif

}

int AbstractModelComponent::index() const
{
  return m_index;
}

void AbstractModelComponent::setIndex(int index)
{
  m_index = index;
  emit propertyChanged("Index");
}

IModelComponentInfo* AbstractModelComponent::componentInfo() const
{
  return m_modelComponentInfo;
}

QList<IArgument*>  AbstractModelComponent::arguments() const
{
  QList<IArgument*> arguments;

  for(AbstractArgument *argument : m_argumentsInsertionOrdered)
  {
    arguments.append(argument);
  }

  return arguments;
}

IModelComponent::ComponentStatus AbstractModelComponent::status() const
{
  return m_status;
}

QList<IInput*> AbstractModelComponent::inputs() const
{
  return m_orderedInputs;
}

void AbstractModelComponent::createInputs()
{

}

QList<IOutput*> AbstractModelComponent::outputs() const
{
  return m_orderedOutputs;
}

void AbstractModelComponent::createOutputs()
{

}

void AbstractModelComponent::initialize()
{
  if(status() == IModelComponent::Created ||
     status() == IModelComponent::Initialized ||
     status() == IModelComponent::Failed)
  {
    setStatus(IModelComponent::Initializing , "" );

    QString message;

    clearInputs();
    clearOutputs();

    if(initializeIdentifierArguments(message) &&
       initializeMPIResources(message) &&
       initializeArguments(message))
    {
      createInputs();
      createOutputs();

      setStatus(IModelComponent::Initialized, "", 0);
      m_initialized = true;
    }
    else
    {
      initializeFailureCleanUp();
      setStatus(IModelComponent::Failed, message);
      m_initialized = false;
      m_prepared = false;
    }
  }
}

const HydroCouple::IWorkflowComponent *AbstractModelComponent::workflow() const
{
  return m_workflowComponent;
}

void AbstractModelComponent::setWorkflow(const HydroCouple::IWorkflowComponent *workflow)
{
  m_workflowComponent = workflow;
}

bool AbstractModelComponent::isInitialized() const
{
  return m_initialized;
}

bool AbstractModelComponent::isPrepared() const
{
  return m_prepared;
}

void AbstractModelComponent::applyInputValues()
{
  //#ifdef USE_OPENMP
  //#pragma omp parallel for
  //#endif
  for(int i = 0; i < (int)m_orderedInputs.size(); i++)
  {
    AbstractInput *input = dynamic_cast<AbstractInput*>(m_orderedInputs[i]);
    AbstractMultiInput* mInput = nullptr;

    if(((mInput = dynamic_cast<AbstractMultiInput*>(input)) != nullptr && mInput->providers().length() > 0) ||
       input->provider() != nullptr)
    {
      input->retrieveValuesFromProvider();
      input->applyData();
    }
  }
}

void AbstractModelComponent::updateOutputValues(const QList<IOutput *> &requiredOutputs)
{
  if(requiredOutputs.length())
  {

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
    for(int i = 0; i < requiredOutputs.size(); i++)
    {
      IOutput *output = requiredOutputs[i];
      AbstractOutput *abstractOutput = dynamic_cast<AbstractOutput*>(output);

      if(abstractOutput && (abstractOutput->consumers().length() || abstractOutput->adaptedOutputs().length() ))
      {
        abstractOutput->updateValues();
      }
    }
  }
  else
  {

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
    for(int i = 0; i < (int)m_orderedOutputs.size(); i++)
    {
      IOutput *output = m_orderedOutputs[i];
      AbstractOutput *abstractOutput = dynamic_cast<AbstractOutput*>(output);

      if(abstractOutput && (abstractOutput->consumers().length() || abstractOutput->adaptedOutputs().length() ))
      {
        abstractOutput->updateValues();
      }
    }
  }
}

void AbstractModelComponent::initializeAdaptedOutputs()
{
  for(IOutput* output  : m_orderedOutputs)
  {
    AbstractOutput *abstractOutput = dynamic_cast<AbstractOutput*>(output);
    abstractOutput->initializeAdaptedOutputs();
  }
}

int AbstractModelComponent::mpiNumOfProcesses() const
{
  return m_mpiAllocatedProcesses.size();
}

int AbstractModelComponent::mpiProcessRank() const
{
  return m_mpiProcess;
}

void AbstractModelComponent::mpiSetProcessRank(int processRank)
{
  m_mpiProcess = processRank;
}

QSet<int> AbstractModelComponent::mpiAllocatedProcesses() const
{
  return m_mpiAllocatedProcesses;
}

void AbstractModelComponent::mpiAllocateProcesses(const QSet<int> &mpiProcessesToAllocate)
{
  m_mpiAllocatedProcesses = mpiProcessesToAllocate;
}

void AbstractModelComponent::mpiClearAllocatedProcesses()
{
  m_mpiAllocatedProcesses.clear();
}

int AbstractModelComponent::gpuPlatform(int processor) const
{
  if(m_gpuAllocation.contains(processor))
  {
    std::tuple<int,int,int> gpuPlatformAndDevices = m_gpuAllocation[processor];
    return std::get<0>(gpuPlatformAndDevices);
  }

  return 0;
}

int AbstractModelComponent::gpuDevice(int processor) const
{
  if(m_gpuAllocation.contains(processor))
  {
    std::tuple<int,int,int> gpuPlatformAndDevices = m_gpuAllocation[processor];
    return std::get<1>(gpuPlatformAndDevices);
  }


  return -1;
}

int AbstractModelComponent::gpuMaxNumBlocksOrWorkGrps(int processor) const
{
  if(m_gpuAllocation.contains(processor))
  {
    std::tuple<int,int,int> gpuPlatformAndDevices = m_gpuAllocation[processor];
    return std::get<2>(gpuPlatformAndDevices);
  }

  return 0;
}

void AbstractModelComponent::gpuAllocatedResources(int mpiProcess, int gpuPlatform, int gpuDevice, int maxNumGPUBlocksOrWorkGrps)
{
  if(m_gpuAllocation.contains(mpiProcess))
  {
    m_gpuAllocation[mpiProcess] = std::make_tuple(gpuPlatform, gpuDevice, maxNumGPUBlocksOrWorkGrps);
  }
}

void AbstractModelComponent::gpuClearAllocatedResources()
{
  m_gpuAllocation.clear();
}

QString AbstractModelComponent::referenceDirectory() const
{
  return m_referenceDir.absolutePath();
}

void AbstractModelComponent::setReferenceDirectory(const QString &referenceDir)
{
  QDir d(referenceDir);

  if(d.exists())
  {
    m_referenceDir = d;
    emit propertyChanged("ReferenceDirectory");
  }
}

QFileInfo AbstractModelComponent::getAbsoluteFilePath(const QString &filePath) const
{
  QFileInfo outFile(filePath.trimmed());

  if(outFile.isRelative())
  {
    outFile = m_referenceDir.absoluteFilePath(filePath.trimmed());
  }

  return outFile;
}

QFileInfo AbstractModelComponent::getRelativeFilePath(const QString &filePath) const
{
  QFileInfo outFile(filePath.trimmed());

  if(outFile.isAbsolute())
  {
    outFile = m_referenceDir.relativeFilePath(filePath.trimmed());
  }

  return outFile;
}

void AbstractModelComponent::readArguments(QXmlStreamReader &xmlReader)
{
  while (!xmlReader.isEndDocument() && !xmlReader.hasError())
  {
    if(!xmlReader.name().toString().compare("Arguments", Qt::CaseInsensitive) && !xmlReader.hasError()
       && xmlReader.tokenType() == QXmlStreamReader::StartElement)
    {
      while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Arguments", Qt::CaseInsensitive)) && !xmlReader.hasError())
      {
        if(!xmlReader.name().toString().compare("Argument", Qt::CaseInsensitive) && !xmlReader.hasError()  && xmlReader.tokenType() == QXmlStreamReader::StartElement)
        {
          readArgument(xmlReader);

          while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Argument", Qt::CaseInsensitive)) && !xmlReader.hasError())
          {
            xmlReader.readNext();
          }
        }
        xmlReader.readNext();
      }

      this->initialize();
    }
    xmlReader.readNext();
  }
}

void AbstractModelComponent::readArgument(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("Argument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
  {
    QXmlStreamAttributes attributes = xmlReader.attributes();

    if(attributes.hasAttribute("Id") && attributes.hasAttribute("ArgumentIOType"))
    {
      QStringRef argumentId = attributes.value("Id");
      QStringRef argIOType = attributes.value("ArgumentIOType");
      IArgument* targument = nullptr;


      for(IArgument* argument : m_arguments)
      {
        if(!argumentId.toString().compare(argument->id() , Qt::CaseInsensitive))
        {
          targument = argument;

          QString value;
          QXmlStreamWriter writer(&value);

          while(!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Argument", Qt::CaseInsensitive)) && !xmlReader.hasError())
          {
            xmlReader.readNext();
            writer.writeCurrentToken(xmlReader);
          }

          QString message;

          if(!argIOType.toString().compare("File", Qt::CaseInsensitive))
          {
            targument->readValues(value, message, true);
          }
          else
          {
            targument->readValues(value, message);
          }

          break;
        }
      }
    }
  }
}

void AbstractModelComponent::setInitialized(bool initialized)
{
  m_initialized = initialized;
}

void AbstractModelComponent::setPrepared(bool prepared)
{
  m_prepared = prepared;
}

IdBasedArgumentString *AbstractModelComponent::identifierArgument() const
{
  return m_identifiersArgument;
}

void AbstractModelComponent::addInput(AbstractInput *input)
{
  if(m_inputs.find(input->id()) == m_inputs.end())
  {
    m_inputs[input->id()] = input;
    m_orderedInputs.push_back(input);
    emit propertyChanged("Inputs");
  }
}

bool AbstractModelComponent::removeInput(AbstractInput *input)
{
  if(m_inputs.contains(input->id()))
  {
    m_inputs.remove(input->id());
    auto it = std::find(m_orderedInputs.begin(), m_orderedInputs.end(), input);

    if(it != m_orderedInputs.end())
    {
      m_orderedInputs.erase(it);
    }

    emit propertyChanged("Inputs");
    return true;
  }

  return false;
}

void AbstractModelComponent::clearInputs()
{

  qDeleteAll(m_inputs.values());
  m_inputs.clear();
  m_orderedInputs.clear();

  emit propertyChanged("Inputs");
}

QHash<QString,AbstractInput*> AbstractModelComponent::inputsInternal() const
{
  return m_inputs;
}

void AbstractModelComponent::addOutput(AbstractOutput *output)
{
  if(m_outputs.contains(output->id()) == false)
  {
    m_outputs[output->id()] = output;
    m_orderedOutputs.push_back(output);
    emit propertyChanged("Outputs");
  }
}

bool AbstractModelComponent::removeOutput(AbstractOutput *output)
{
  if(m_outputs.contains(output->id()))
  {
    m_outputs.remove(output->id());

    auto it = std::find(m_orderedOutputs.begin() , m_orderedOutputs.end() , output);

    if(it != m_orderedOutputs.end())
    {
      m_orderedOutputs.erase(it);
    }

    emit propertyChanged("Outputs");
    return true;
  }

  return false;
}

void AbstractModelComponent::clearOutputs()
{
  qDeleteAll(m_outputs.values());
  m_outputs.clear();
  m_orderedOutputs.clear();

  emit propertyChanged("Outputs");
}

QHash<QString,AbstractOutput*> AbstractModelComponent::outputsInternal() const
{
  return m_outputs;
}

void AbstractModelComponent::addArgument(AbstractArgument *argument)
{
  if(!m_arguments.contains(argument->id()))
  {
    m_arguments[argument->id()] = argument;
    m_argumentsInsertionOrdered.append(argument);
    emit propertyChanged("Arguments");
  }
}

bool AbstractModelComponent::removeArgument(AbstractArgument *argument)
{
  if(m_arguments.contains(argument->id()))
  {
    m_arguments.remove(argument->id());
    m_argumentsInsertionOrdered.removeOne(argument);
    emit propertyChanged("Arguments");
    return true;
  }

  return false;
}

void AbstractModelComponent::clearArguments()
{
  qDeleteAll(m_arguments.values());
  m_arguments.clear();

  emit propertyChanged("Arguments");
}

QHash<QString,AbstractArgument*> AbstractModelComponent::argumentsInternal() const
{
  return m_arguments;
}

void AbstractModelComponent::setStatus(HydroCouple::IModelComponent::ComponentStatus status)
{
  m_status = status;
}

void AbstractModelComponent::setStatus(IModelComponent::ComponentStatus status, const QString &message)
{

  IModelComponent::ComponentStatus oldStatus = m_status;
  m_status = status;


  QString cStatus = statusToString(status);
  //  QString oldStat = statusToString(oldStatus);

  QString outMessage = "Component Type: " + componentInfo()->caption() + " | " +
                       "Component Id: " + id() + " | " +
                       "Status: " +  cStatus ;

  if(!message.isEmpty() && !message.isNull())
    outMessage +=" | Message: " + message;

  emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(m_status, oldStatus, outMessage, this)));


}

void AbstractModelComponent::setStatus(IModelComponent::ComponentStatus status, const QString &message, int progress)
{
  IModelComponent::ComponentStatus oldStatus = m_status;
  m_status = status;

  QString cStatus = statusToString(status);
  //  QString oldStat = statusToString(oldStatus);

  QString outMessage = "Component Type: " + componentInfo()->caption() + " | " +
                       "Component Id: " + id() + " | " +
                       "Status: " +  cStatus +
                       " | Message: " + message;

  emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(m_status , oldStatus, outMessage, progress, this)));
}

ProgressChecker *AbstractModelComponent::progressChecker() const
{
  return m_progressChecker;
}

QString AbstractModelComponent::statusToString(IModelComponent::ComponentStatus status)
{
  switch (status)
  {
    case ComponentStatus::Created:
      return "Created";
      break;
    case ComponentStatus::Initializing:
      return "Initializing";
      break;
    case ComponentStatus::Initialized:
      return "Initialized";
      break;
    case ComponentStatus::Validating:
      return "Validating";
      break;
    case ComponentStatus::Valid:
      return "Valid";
      break;
    case ComponentStatus::WaitingForData:
      return "WaitingForData";
      break;
    case ComponentStatus::Invalid:
      return "Invalid";
      break;
    case ComponentStatus::Preparing:
      return "Preparing";
      break;
    case ComponentStatus::Updating:
      return "Updating";
      break;
    case ComponentStatus::Updated:
      return "Updated";
      break;
    case ComponentStatus::Done:
      return "Done";
      break;
    case ComponentStatus::Finishing:
      return "Finishing";
      break;
    case ComponentStatus::Finished:
      return "Finished";
      break;
    case ComponentStatus::Failed:
      return "Failed";
      break;
    default:
      return	QString();
      break;
  }
}

MPI_Comm AbstractModelComponent::mpiCommunicator() const
{
  return m_ComponentMPIComm;
}

void AbstractModelComponent::createIdentifierArguments()
{
  Dimension *identifierDimension = new Dimension("IdentifierDimension","Dimension for identifiers", this);
  Quantity* quantity = Quantity::unitLessValues("IdentifiersQuantity", QVariant::String , this);

  QStringList identifiers;
  identifiers.append("Id");
  identifiers.append("Caption");
  identifiers.append("Description");

  m_identifiersArgument = new IdBasedArgumentString("Identifiers", identifiers,identifierDimension,quantity,this);
  m_identifiersArgument->setMatchIdentifiersWhenReading(true);
  m_identifiersArgument->setCaption("Model Identifiers");

  QString descript = "<h1>Model Identifiers</h1>"
                     "<hr>"
                     "<p>Identifiers for this model instance</p>";

  m_identifiersArgument->setDescription(descript);


  (*m_identifiersArgument)["Id"] = id();
  (*m_identifiersArgument)["Caption"] = caption();
  (*m_identifiersArgument)["Description"] = description();


  m_identifiersArgument->addFileFilter("Input XML File (*.xml)");
  m_identifiersArgument->setMatchIdentifiersWhenReading(true);

  addArgument(m_identifiersArgument);
}

bool AbstractModelComponent::initializeIdentifierArguments(QString &message)
{
  QString identifier = (*m_identifiersArgument)["Id"];

  if(identifier.isNull() || identifier.isEmpty())
  {
    message = "The id provided is invalid!";
    return false;
  }
  else
  {
    setId(identifier);
  }

  setCaption((*m_identifiersArgument)["Caption"]);
  setDescription((*m_identifiersArgument)["Description"]);

  return true;
}

bool AbstractModelComponent::initializeMPIResources(QString &message)
{

  message = "";

#ifdef USE_MPI

  if(m_mpiProcess == 0)
  {
    if(m_mpiResourcesInitialized)
    {
      MPI_Group_free(&m_ComponentMPIGroup);
      MPI_Comm_free(&m_ComponentMPIComm);
      m_mpiResourcesInitialized = false;
    }

    printf("Preparing MPI Communicator on MPI Processor: %i for Component: %s\n", mpiProcessRank(), qPrintable(id()));

    m_mpiAllocatedProcessesArray.clear();

    m_mpiAllocatedProcessesArray.push_back(0);

    for(int proc : m_mpiAllocatedProcesses)
    {
      if(proc != 0)
      {
        m_mpiAllocatedProcessesArray.push_back(proc);
      }
    }

    if(m_mpiAllocatedProcessesArray.size() > 1)
    {
      //Send message to children
      for(int proc : m_mpiAllocatedProcessesArray)
      {
        if(proc != 0)
        {
          printf("Slave to worker: %i to initialize communicator\n", proc);
          MPI_Send(&m_mpiAllocatedProcessesArray[0], m_mpiAllocatedProcessesArray.size(), MPI_INT, proc, 1000, MPI_COMM_WORLD);
        }
      }

      printf("Initializing comm group\n");
      MPI_Group_incl(m_worldGroup, m_mpiAllocatedProcessesArray.size(), &m_mpiAllocatedProcessesArray[0], &m_ComponentMPIGroup);

#ifdef _WIN32
      MPI_Comm_create(MPI_COMM_WORLD, m_ComponentMPIGroup, &m_ComponentMPIComm);
#else
      MPI_Comm_create_group(MPI_COMM_WORLD, m_ComponentMPIGroup, 0, &m_ComponentMPIComm);
#endif
      printf("Initializing comm group\n");

      m_mpiResourcesInitialized = true;
    }
    else
    {
      m_ComponentMPIComm = MPI_COMM_SELF;
    }

    return true;

  }
  else
  {
    MPI_Status status;
    int result = 0;

    printf("worker receiving communicator\n");

    if((result = MPI_Probe(MPI_ANY_SOURCE, 1000, MPI_COMM_WORLD, &status)) == MPI_SUCCESS)
    {
      int dataSize = 0;
      MPI_Get_count(&status, MPI_INT, &dataSize);

      if(dataSize)
      {
        if(m_mpiResourcesInitialized)
        {
          MPI_Group_free(&m_ComponentMPIGroup);
          MPI_Comm_free(&m_ComponentMPIComm);
          m_mpiResourcesInitialized = false;
        }

        printf("Preparing MPI Communicator on MPI Processor: %i for Component: %s\n", mpiProcessRank(), qPrintable(id()));
        int *procs = new int[dataSize];

        MPI_Recv(procs,dataSize,MPI_INT,status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD,&status);
        MPI_Group_incl(m_worldGroup, dataSize, procs, &m_ComponentMPIGroup);

#ifdef _WIN32
        MPI_Comm_create(MPI_COMM_WORLD, m_ComponentMPIGroup, &m_ComponentMPIComm);
#else
        MPI_Comm_create_group(MPI_COMM_WORLD, m_ComponentMPIGroup, 0, &m_ComponentMPIComm);
#endif

        m_mpiResourcesInitialized = true;

        delete[] procs;
        printf("Finished Preparing MPI Communicator on MPI Processor: %i for Component: %s\n", mpiProcessRank(), qPrintable(id()));
      }
    }

    return true;
  }
#else
  {
    m_ComponentMPIComm = 0;
    return true;
  }
#endif

  return false;
}
=======
/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#include "stdafx.h"
#include "core/abstractmodelcomponent.h"
#include "core/abstractmodelcomponentinfo.h"
#include "core/componentstatuschangeeventargs.h"
#include "core/abstractargument.h"
#include "core/abstractinput.h"
#include "core/abstractoutput.h"
#include "core/abstractadaptedoutputfactory.h"
#include "core/idbasedargument.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "progresschecker.h"

#include <QSharedPointer>
#include <QDataStream>
#include <math.h>
#include <QThread>
using namespace HydroCouple;

AbstractModelComponent::AbstractModelComponent(const QString &id, AbstractModelComponentInfo *modelComponentInfo)
  : Identity(id, modelComponentInfo->thread() == QThread::currentThread() ? modelComponentInfo : nullptr),
    m_status(IModelComponent::Created),
    m_modelComponentInfo(modelComponentInfo),
    m_initialized(false),
    m_prepared(false),
    m_index(0),
    m_mpiProcess(0)
{
  m_workflowComponent = nullptr;
  m_progressChecker = new ProgressChecker(0,100,100,this);
  m_referenceDir = QFileInfo(m_modelComponentInfo->libraryFilePath()).dir();
  emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(IModelComponent::Created,
                                                                                                                 IModelComponent::Created,
                                                                                                                 "Created SWMM Model" , 0 , this)));

#ifdef USE_MPI
  MPI_Comm_group(MPI_COMM_WORLD, &m_worldGroup);
  int worldGroupSize = 0;
  MPI_Group_size(m_worldGroup, &worldGroupSize);
  printf("World Group Size: %i\n", worldGroupSize);
#endif

  createIdentifierArguments();
}

AbstractModelComponent::AbstractModelComponent(const QString &id, const QString &caption, AbstractModelComponentInfo *modelComponentInfo)
  : Identity(id,caption,modelComponentInfo->thread() == QThread::currentThread() ? modelComponentInfo : nullptr),
    m_status(HydroCouple::IModelComponent::Created),
    m_modelComponentInfo(modelComponentInfo),
    m_initialized(false),
    m_prepared(false),
    m_index(0),
    m_mpiProcess(0)
{
  m_workflowComponent = nullptr;
  m_progressChecker = new ProgressChecker(0,100,100,this);
  m_referenceDir = QFileInfo(m_modelComponentInfo->libraryFilePath()).dir();
  emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(IModelComponent::Created,
                                                                                                                 IModelComponent::Created,
                                                                                                                 "Created SWMM Model",
                                                                                                                 0,this)));
#ifdef USE_MPI
  MPI_Comm_group(MPI_COMM_WORLD, &m_worldGroup);
  int worldGroupSize = 0;
  MPI_Group_size(m_worldGroup, &worldGroupSize);
  printf("World Group Size: %i\n", worldGroupSize);
#endif

  createIdentifierArguments();
}

AbstractModelComponent::~AbstractModelComponent()
{
  qDeleteAll(m_inputs.values());
  m_inputs.clear();
  m_orderedInputs.clear();

  qDeleteAll(m_outputs.values());
  m_outputs.clear();
  m_orderedOutputs.clear();

  qDeleteAll(m_arguments.values());
  m_arguments.clear();
  m_argumentsInsertionOrdered.clear();

#ifdef USE_MPI

  if(m_mpiResourcesInitialized)
  {
    MPI_Group_free(&m_ComponentMPIGroup);
    MPI_Comm_free(&m_ComponentMPIComm);
  }

  MPI_Group_free(&m_worldGroup);

#endif

}

int AbstractModelComponent::index() const
{
  return m_index;
}

void AbstractModelComponent::setIndex(int index)
{
  m_index = index;
  emit propertyChanged("Index");
}

IModelComponentInfo* AbstractModelComponent::componentInfo() const
{
  return m_modelComponentInfo;
}

QList<IArgument*>  AbstractModelComponent::arguments() const
{
  QList<IArgument*> arguments;

  for(AbstractArgument *argument : m_argumentsInsertionOrdered)
  {
    arguments.append(argument);
  }

  return arguments;
}

IModelComponent::ComponentStatus AbstractModelComponent::status() const
{
  return m_status;
}

QList<IInput*> AbstractModelComponent::inputs() const
{
  return m_orderedInputs;
}

void AbstractModelComponent::createInputs()
{

}

QList<IOutput*> AbstractModelComponent::outputs() const
{
  return m_orderedOutputs;
}

void AbstractModelComponent::createOutputs()
{

}

void AbstractModelComponent::initialize()
{
  if(status() == IModelComponent::Created ||
     status() == IModelComponent::Initialized ||
     status() == IModelComponent::Failed)
  {
    setStatus(IModelComponent::Initializing , "" );

    QString message;

    clearInputs();
    clearOutputs();

    if(initializeIdentifierArguments(message) &&
       initializeMPIResources(message) &&
       initializeArguments(message))
    {
      createInputs();
      createOutputs();

      setStatus(IModelComponent::Initialized, "", 0);
      m_initialized = true;
    }
    else
    {
      initializeFailureCleanUp();
      setStatus(IModelComponent::Failed, message);
      m_initialized = false;
      m_prepared = false;
    }
  }
}

const HydroCouple::IWorkflowComponent *AbstractModelComponent::workflow() const
{
  return m_workflowComponent;
}

void AbstractModelComponent::setWorkflow(const HydroCouple::IWorkflowComponent *workflow)
{
  m_workflowComponent = workflow;
}

bool AbstractModelComponent::isInitialized() const
{
  return m_initialized;
}

bool AbstractModelComponent::isPrepared() const
{
  return m_prepared;
}

void AbstractModelComponent::applyInputValues()
{
  //#ifdef USE_OPENMP
  //#pragma omp parallel for
  //#endif
  for(int i = 0; i < (int)m_orderedInputs.size(); i++)
  {
    AbstractInput *input = dynamic_cast<AbstractInput*>(m_orderedInputs[i]);
    AbstractMultiInput* mInput = nullptr;

    if(((mInput = dynamic_cast<AbstractMultiInput*>(input)) != nullptr && mInput->providers().length() > 0) ||
       input->provider() != nullptr)
    {
      input->retrieveValuesFromProvider();
      input->applyData();
    }
  }
}

void AbstractModelComponent::updateOutputValues(const QList<IOutput *> &requiredOutputs)
{
  if(requiredOutputs.length())
  {

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
    for(int i = 0; i < requiredOutputs.size(); i++)
    {
      IOutput *output = requiredOutputs[i];
      AbstractOutput *abstractOutput = dynamic_cast<AbstractOutput*>(output);

      if(abstractOutput && (abstractOutput->consumers().length() || abstractOutput->adaptedOutputs().length() ))
      {
        abstractOutput->updateValues();
      }
    }
  }
  else
  {

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
    for(int i = 0; i < (int)m_orderedOutputs.size(); i++)
    {
      IOutput *output = m_orderedOutputs[i];
      AbstractOutput *abstractOutput = dynamic_cast<AbstractOutput*>(output);

      if(abstractOutput && (abstractOutput->consumers().length() || abstractOutput->adaptedOutputs().length() ))
      {
        abstractOutput->updateValues();
      }
    }
  }
}

void AbstractModelComponent::initializeAdaptedOutputs()
{
  for(IOutput* output  : m_orderedOutputs)
  {
    AbstractOutput *abstractOutput = dynamic_cast<AbstractOutput*>(output);
    abstractOutput->initializeAdaptedOutputs();
  }
}

int AbstractModelComponent::mpiNumOfProcesses() const
{
  return m_mpiAllocatedProcesses.size();
}

int AbstractModelComponent::mpiProcessRank() const
{
  return m_mpiProcess;
}

void AbstractModelComponent::mpiSetProcessRank(int processRank)
{
  m_mpiProcess = processRank;
}

QSet<int> AbstractModelComponent::mpiAllocatedProcesses() const
{
  return m_mpiAllocatedProcesses;
}

void AbstractModelComponent::mpiAllocateProcesses(const QSet<int> &mpiProcessesToAllocate)
{
  m_mpiAllocatedProcesses = mpiProcessesToAllocate;
}

void AbstractModelComponent::mpiClearAllocatedProcesses()
{
  m_mpiAllocatedProcesses.clear();
}

int AbstractModelComponent::gpuPlatform(int processor) const
{
  if(m_gpuAllocation.contains(processor))
  {
    std::tuple<int,int,int> gpuPlatformAndDevices = m_gpuAllocation[processor];
    return std::get<0>(gpuPlatformAndDevices);
  }

  return 0;
}

int AbstractModelComponent::gpuDevice(int processor) const
{
  if(m_gpuAllocation.contains(processor))
  {
    std::tuple<int,int,int> gpuPlatformAndDevices = m_gpuAllocation[processor];
    return std::get<1>(gpuPlatformAndDevices);
  }


  return -1;
}

int AbstractModelComponent::gpuMaxNumBlocksOrWorkGrps(int processor) const
{
  if(m_gpuAllocation.contains(processor))
  {
    std::tuple<int,int,int> gpuPlatformAndDevices = m_gpuAllocation[processor];
    return std::get<2>(gpuPlatformAndDevices);
  }

  return 0;
}

void AbstractModelComponent::gpuAllocatedResources(int mpiProcess, int gpuPlatform, int gpuDevice, int maxNumGPUBlocksOrWorkGrps)
{
  if(m_gpuAllocation.contains(mpiProcess))
  {
    m_gpuAllocation[mpiProcess] = std::make_tuple(gpuPlatform, gpuDevice, maxNumGPUBlocksOrWorkGrps);
  }
}

void AbstractModelComponent::gpuClearAllocatedResources()
{
  m_gpuAllocation.clear();
}

QString AbstractModelComponent::referenceDirectory() const
{
  return m_referenceDir.absolutePath();
}

void AbstractModelComponent::setReferenceDirectory(const QString &referenceDir)
{
  QDir d(referenceDir);

  if(d.exists())
  {
    m_referenceDir = d;
    emit propertyChanged("ReferenceDirectory");
  }
}

QFileInfo AbstractModelComponent::getAbsoluteFilePath(const QString &filePath) const
{
  QFileInfo outFile(filePath.trimmed());

  if(outFile.isRelative())
  {
    outFile = m_referenceDir.absoluteFilePath(filePath.trimmed());
  }

  return outFile;
}

QFileInfo AbstractModelComponent::getRelativeFilePath(const QString &filePath) const
{
  QFileInfo outFile(filePath.trimmed());

  if(outFile.isAbsolute())
  {
    outFile = m_referenceDir.relativeFilePath(filePath.trimmed());
  }

  return outFile;
}

void AbstractModelComponent::readArguments(QXmlStreamReader &xmlReader)
{
  while (!xmlReader.isEndDocument() && !xmlReader.hasError())
  {
    if(!xmlReader.name().toString().compare("Arguments", Qt::CaseInsensitive) && !xmlReader.hasError()
       && xmlReader.tokenType() == QXmlStreamReader::StartElement)
    {
      while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Arguments", Qt::CaseInsensitive)) && !xmlReader.hasError())
      {
        if(!xmlReader.name().toString().compare("Argument", Qt::CaseInsensitive) && !xmlReader.hasError()  && xmlReader.tokenType() == QXmlStreamReader::StartElement)
        {
          readArgument(xmlReader);

          while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Argument", Qt::CaseInsensitive)) && !xmlReader.hasError())
          {
            xmlReader.readNext();
          }
        }
        xmlReader.readNext();
      }

      this->initialize();
    }
    xmlReader.readNext();
  }
}

void AbstractModelComponent::readArgument(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("Argument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
  {
    QXmlStreamAttributes attributes = xmlReader.attributes();

    if(attributes.hasAttribute("Id") && attributes.hasAttribute("ArgumentIOType"))
    {
      QStringRef argumentId = attributes.value("Id");
      QStringRef argIOType = attributes.value("ArgumentIOType");
      IArgument* targument = nullptr;


      for(IArgument* argument : m_arguments)
      {
        if(!argumentId.toString().compare(argument->id() , Qt::CaseInsensitive))
        {
          targument = argument;

          QString value;
          QXmlStreamWriter writer(&value);

          while(!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Argument", Qt::CaseInsensitive)) && !xmlReader.hasError())
          {
            xmlReader.readNext();
            writer.writeCurrentToken(xmlReader);
          }

          QString message;

          if(!argIOType.toString().compare("File", Qt::CaseInsensitive))
          {
            targument->readValues(value, message, true);
          }
          else
          {
            targument->readValues(value, message);
          }

          break;
        }
      }
    }
  }
}

void AbstractModelComponent::setInitialized(bool initialized)
{
  m_initialized = initialized;
}

void AbstractModelComponent::setPrepared(bool prepared)
{
  m_prepared = prepared;
}

IdBasedArgumentString *AbstractModelComponent::identifierArgument() const
{
  return m_identifiersArgument;
}

void AbstractModelComponent::addInput(AbstractInput *input)
{
  if(m_inputs.find(input->id()) == m_inputs.end())
  {
    m_inputs[input->id()] = input;
    m_orderedInputs.push_back(input);
    emit propertyChanged("Inputs");
  }
}

bool AbstractModelComponent::removeInput(AbstractInput *input)
{
  if(m_inputs.contains(input->id()))
  {
    m_inputs.remove(input->id());
    auto it = std::find(m_orderedInputs.begin(), m_orderedInputs.end(), input);

    if(it != m_orderedInputs.end())
    {
      m_orderedInputs.erase(it);
    }

    emit propertyChanged("Inputs");
    return true;
  }

  return false;
}

void AbstractModelComponent::clearInputs()
{

  qDeleteAll(m_inputs.values());
  m_inputs.clear();
  m_orderedInputs.clear();

  emit propertyChanged("Inputs");
}

QHash<QString,AbstractInput*> AbstractModelComponent::inputsInternal() const
{
  return m_inputs;
}

void AbstractModelComponent::addOutput(AbstractOutput *output)
{
  if(m_outputs.contains(output->id()) == false)
  {
    m_outputs[output->id()] = output;
    m_orderedOutputs.push_back(output);
    emit propertyChanged("Outputs");
  }
}

bool AbstractModelComponent::removeOutput(AbstractOutput *output)
{
  if(m_outputs.contains(output->id()))
  {
    m_outputs.remove(output->id());

    auto it = std::find(m_orderedOutputs.begin() , m_orderedOutputs.end() , output);

    if(it != m_orderedOutputs.end())
    {
      m_orderedOutputs.erase(it);
    }

    emit propertyChanged("Outputs");
    return true;
  }

  return false;
}

void AbstractModelComponent::clearOutputs()
{
  qDeleteAll(m_outputs.values());
  m_outputs.clear();
  m_orderedOutputs.clear();

  emit propertyChanged("Outputs");
}

QHash<QString,AbstractOutput*> AbstractModelComponent::outputsInternal() const
{
  return m_outputs;
}

void AbstractModelComponent::addArgument(AbstractArgument *argument)
{
  if(!m_arguments.contains(argument->id()))
  {
    m_arguments[argument->id()] = argument;
    m_argumentsInsertionOrdered.append(argument);
    emit propertyChanged("Arguments");
  }
}

bool AbstractModelComponent::removeArgument(AbstractArgument *argument)
{
  if(m_arguments.contains(argument->id()))
  {
    m_arguments.remove(argument->id());
    m_argumentsInsertionOrdered.removeOne(argument);
    emit propertyChanged("Arguments");
    return true;
  }

  return false;
}

void AbstractModelComponent::clearArguments()
{
  qDeleteAll(m_arguments.values());
  m_arguments.clear();

  emit propertyChanged("Arguments");
}

QHash<QString,AbstractArgument*> AbstractModelComponent::argumentsInternal() const
{
  return m_arguments;
}

void AbstractModelComponent::setStatus(HydroCouple::IModelComponent::ComponentStatus status)
{
  m_status = status;
}

void AbstractModelComponent::setStatus(IModelComponent::ComponentStatus status, const QString &message)
{

  IModelComponent::ComponentStatus oldStatus = m_status;
  m_status = status;


  QString cStatus = statusToString(status);
  //  QString oldStat = statusToString(oldStatus);

  QString outMessage = "Component Type: " + componentInfo()->caption() + " | " +
                       "Component Id: " + id() + " | " +
                       "Status: " +  cStatus ;

  if(!message.isEmpty() && !message.isNull())
    outMessage +=" | Message: " + message;

  emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(m_status, oldStatus, outMessage, this)));


}

void AbstractModelComponent::setStatus(IModelComponent::ComponentStatus status, const QString &message, int progress)
{
  IModelComponent::ComponentStatus oldStatus = m_status;
  m_status = status;

  QString cStatus = statusToString(status);
  //  QString oldStat = statusToString(oldStatus);

  QString outMessage = "Component Type: " + componentInfo()->caption() + " | " +
                       "Component Id: " + id() + " | " +
                       "Status: " +  cStatus +
                       " | Message: " + message;

  emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(m_status , oldStatus, outMessage, progress, this)));
}

ProgressChecker *AbstractModelComponent::progressChecker() const
{
  return m_progressChecker;
}

QString AbstractModelComponent::statusToString(IModelComponent::ComponentStatus status)
{
  switch (status)
  {
    case ComponentStatus::Created:
      return "Created";
      break;
    case ComponentStatus::Initializing:
      return "Initializing";
      break;
    case ComponentStatus::Initialized:
      return "Initialized";
      break;
    case ComponentStatus::Validating:
      return "Validating";
      break;
    case ComponentStatus::Valid:
      return "Valid";
      break;
    case ComponentStatus::WaitingForData:
      return "WaitingForData";
      break;
    case ComponentStatus::Invalid:
      return "Invalid";
      break;
    case ComponentStatus::Preparing:
      return "Preparing";
      break;
    case ComponentStatus::Updating:
      return "Updating";
      break;
    case ComponentStatus::Updated:
      return "Updated";
      break;
    case ComponentStatus::Done:
      return "Done";
      break;
    case ComponentStatus::Finishing:
      return "Finishing";
      break;
    case ComponentStatus::Finished:
      return "Finished";
      break;
    case ComponentStatus::Failed:
      return "Failed";
      break;
    default:
      return	QString();
      break;
  }
}

MPI_Comm AbstractModelComponent::mpiCommunicator() const
{
  return m_ComponentMPIComm;
}

void AbstractModelComponent::createIdentifierArguments()
{
  Dimension *identifierDimension = new Dimension("IdentifierDimension","Dimension for identifiers", this);
  Quantity* quantity = Quantity::unitLessValues("IdentifiersQuantity", QVariant::String , this);

  QStringList identifiers;
  identifiers.append("Id");
  identifiers.append("Caption");
  identifiers.append("Description");

  m_identifiersArgument = new IdBasedArgumentString("Identifiers", identifiers,identifierDimension,quantity,this);
  m_identifiersArgument->setMatchIdentifiersWhenReading(true);
  m_identifiersArgument->setCaption("Model Identifiers");

  QString descript = "<h1>Model Identifiers</h1>"
                     "<hr>"
                     "<p>Identifiers for this model instance</p>";

  m_identifiersArgument->setDescription(descript);


  (*m_identifiersArgument)["Id"] = id();
  (*m_identifiersArgument)["Caption"] = caption();
  (*m_identifiersArgument)["Description"] = description();


  m_identifiersArgument->addFileFilter("Input XML File (*.xml)");
  m_identifiersArgument->setMatchIdentifiersWhenReading(true);

  addArgument(m_identifiersArgument);
}

bool AbstractModelComponent::initializeIdentifierArguments(QString &message)
{
  QString identifier = (*m_identifiersArgument)["Id"];

  if(identifier.isNull() || identifier.isEmpty())
  {
    message = "The id provided is invalid!";
    return false;
  }
  else
  {
    setId(identifier);
  }

  setCaption((*m_identifiersArgument)["Caption"]);
  setDescription((*m_identifiersArgument)["Description"]);

  return true;
}

bool AbstractModelComponent::initializeMPIResources(QString &message)
{

  message = "";

#ifdef USE_MPI

  if(m_mpiProcess == 0)
  {
    if(m_mpiResourcesInitialized)
    {
      MPI_Group_free(&m_ComponentMPIGroup);
      MPI_Comm_free(&m_ComponentMPIComm);
      m_mpiResourcesInitialized = false;
    }

    printf("Preparing MPI Communicator on MPI Processor: %i for Component: %s\n", mpiProcessRank(), qPrintable(id()));

    m_mpiAllocatedProcessesArray.clear();

    m_mpiAllocatedProcessesArray.push_back(0);

    for(int proc : m_mpiAllocatedProcesses)
    {
      if(proc != 0)
      {
        m_mpiAllocatedProcessesArray.push_back(proc);
      }
    }

    if(m_mpiAllocatedProcessesArray.size() > 1)
    {
      //Send message to children
      for(int proc : m_mpiAllocatedProcessesArray)
      {
        if(proc != 0)
        {
          printf("Slave to worker: %i to initialize communicator\n", proc);
          MPI_Send(&m_mpiAllocatedProcessesArray[0], m_mpiAllocatedProcessesArray.size(), MPI_INT, proc, 1000, MPI_COMM_WORLD);
        }
      }

      printf("Initializing comm group\n");
      MPI_Group_incl(m_worldGroup, m_mpiAllocatedProcessesArray.size(), &m_mpiAllocatedProcessesArray[0], &m_ComponentMPIGroup);

#ifdef _WIN32
      MPI_Comm_create(MPI_COMM_WORLD, m_ComponentMPIGroup, &m_ComponentMPIComm);
#else
      MPI_Comm_create_group(MPI_COMM_WORLD, m_ComponentMPIGroup, 0, &m_ComponentMPIComm);
#endif
      printf("Initializing comm group\n");

      m_mpiResourcesInitialized = true;
    }
    else
    {
      m_ComponentMPIComm = MPI_COMM_SELF;
    }

    return true;

  }
  else
  {
    MPI_Status status;
    int result = 0;

    printf("worker receiving communicator\n");

    if((result = MPI_Probe(MPI_ANY_SOURCE, 1000, MPI_COMM_WORLD, &status)) == MPI_SUCCESS)
    {
      int dataSize = 0;
      MPI_Get_count(&status, MPI_INT, &dataSize);

      if(dataSize)
      {
        if(m_mpiResourcesInitialized)
        {
          MPI_Group_free(&m_ComponentMPIGroup);
          MPI_Comm_free(&m_ComponentMPIComm);
          m_mpiResourcesInitialized = false;
        }

        printf("Preparing MPI Communicator on MPI Processor: %i for Component: %s\n", mpiProcessRank(), qPrintable(id()));
        int *procs = new int[dataSize];

        MPI_Recv(procs,dataSize,MPI_INT,status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD,&status);
        MPI_Group_incl(m_worldGroup, dataSize, procs, &m_ComponentMPIGroup);

#ifdef _WIN32
        MPI_Comm_create(MPI_COMM_WORLD, m_ComponentMPIGroup, &m_ComponentMPIComm);
#else
        MPI_Comm_create_group(MPI_COMM_WORLD, m_ComponentMPIGroup, 0, &m_ComponentMPIComm);
#endif

        m_mpiResourcesInitialized = true;

        delete[] procs;
        printf("Finished Preparing MPI Communicator on MPI Processor: %i for Component: %s\n", mpiProcessRank(), qPrintable(id()));
      }
    }

    return true;
  }
#else
  {
    m_ComponentMPIComm = 0;
    return true;
  }
#endif

  return false;
}
>>>>>>> 39d903328ddf73cd5eb24ad1fe279d85b568b1eb
