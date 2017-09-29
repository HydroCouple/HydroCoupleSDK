#include "stdafx.h"
#include "core/abstractmodelcomponent.h"
#include "core/modelcomponentinfo.h"
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

using namespace HydroCouple;


AbstractModelComponent::AbstractModelComponent(const QString &id, ModelComponentInfo *modelComponentInfo)
  : Identity(id, modelComponentInfo),
    m_status(IModelComponent::Created),
    m_modelComponentInfo(modelComponentInfo),
    m_initialized(false),
    m_prepared(false),
    m_mpiProcess(0),
    m_index(0)
{
  m_progressChecker = new ProgressChecker(0,100,100,this);
  m_referenceDir = QFileInfo(m_modelComponentInfo->libraryFilePath()).dir();
  emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(IModelComponent::Created,
                                                                                                                 IModelComponent::Created,
                                                                                                                 "Created SWMM Model" , 0 , this)));
  createIdentifierArguments();

}

AbstractModelComponent::AbstractModelComponent(const QString &id, const QString &caption, ModelComponentInfo *modelComponentInfo)
  : Identity(id,caption,modelComponentInfo),
    m_status(HydroCouple::IModelComponent::Created),
    m_modelComponentInfo(modelComponentInfo),
    m_initialized(false),
    m_prepared(false),
    m_mpiProcess(0),
    m_index(0)
{
  m_progressChecker = new ProgressChecker(0,100,100,this);
  m_referenceDir = QFileInfo(m_modelComponentInfo->libraryFilePath()).dir();
  emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(IModelComponent::Created,
                                                                                                                 IModelComponent::Created,
                                                                                                                 "Created SWMM Model",
                                                                                                                 0,this)));
  createIdentifierArguments();
}

AbstractModelComponent::~AbstractModelComponent()
{
  qDeleteAll(m_inputs.values());
  m_inputs.clear();

  qDeleteAll(m_outputs.values());
  m_outputs.clear();

  qDeleteAll(m_adaptedOutputFactories.values());
  m_adaptedOutputFactories.clear();

  qDeleteAll(m_arguments.values());
  m_arguments.clear();
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
  QList<IInput*> inputs;

  for(AbstractInput* input : m_inputs)
  {
    inputs.append(input);
  }

  return inputs;
}

void AbstractModelComponent::createInputs()
{

}

QList<IOutput*> AbstractModelComponent::outputs() const
{
  QList<IOutput*> outputs;

  for(AbstractOutput* output : m_outputs)
  {
    outputs.append(output);
  }

  return outputs;
}

void AbstractModelComponent::createOutputs()
{

}

QList<IAdaptedOutputFactory*> AbstractModelComponent::adaptedOutputFactories() const
{

  QList<IAdaptedOutputFactory*> factories;

  for(AbstractAdaptedOutputFactory* factory : m_adaptedOutputFactories)
  {
    factories.append(factory);
  }

  return factories;
}

void AbstractModelComponent::initialize()
{
  if(status() == IModelComponent::Created || status() == IModelComponent::Initialized || status() == IModelComponent::Failed)
  {
    setStatus(IModelComponent::Initializing , "" );

    QString message;

    clearInputs();
    clearOutputs();

    if(initializeIdentifierArguments(message) &&
       initializeArguments(message))
    {
      createInputs();
      createOutputs();

      setStatus(IModelComponent::Initialized, "", 0);
      m_initialized = true;
    }
    else
    {
      intializeFailureCleanUp();
      setStatus(IModelComponent::Failed, message);
      m_initialized = false;
      m_prepared = false;
    }
  }
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
  AbstractMultiInput* mInput = nullptr;

  for(AbstractInput* input : m_inputs)
  {
    if(input->provider() != nullptr || ((mInput = dynamic_cast<AbstractMultiInput*>(input))
                                        && mInput->providers().length()))
    {
      AbstractInput *tinput = input;
      tinput->retrieveValuesFromProvider();
      tinput->applyData();
    }
  }
}

void AbstractModelComponent::updateOutputValues(const QList<IOutput *> &requiredOutputs)
{
  if(requiredOutputs.length())
  {
    for(IOutput* output : requiredOutputs)
    {
      AbstractOutput *abstractOutput = dynamic_cast<AbstractOutput*>(output);

      if(abstractOutput && (abstractOutput->consumers().length() || abstractOutput->adaptedOutputs().length() ))
      {
        abstractOutput->updateValues();
      }
    }
  }
  else
  {
    for(AbstractOutput* abstractOutput : m_outputs)
    {
      if(abstractOutput && (abstractOutput->consumers().length() || abstractOutput->adaptedOutputs().length() ))
      {
        abstractOutput->updateValues();
      }
    }
  }
}

void AbstractModelComponent::initializeAdaptedOutputs()
{
  for(AbstractOutput* abstractOutput : m_outputs)
  {
    abstractOutput->initializeAdaptedOutputs();
  }
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

#ifdef QT_DEBUG
void AbstractModelComponent::mpiProcessMessage(const SerializableData &data)
{

}
#endif

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
    if(!xmlReader.name().compare("Arguments", Qt::CaseInsensitive) && !xmlReader.hasError()
       && xmlReader.tokenType() == QXmlStreamReader::StartElement)
    {
      while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Arguments", Qt::CaseInsensitive)) && !xmlReader.hasError())
      {
        if(!xmlReader.name().compare("Argument", Qt::CaseInsensitive) && !xmlReader.hasError()  && xmlReader.tokenType() == QXmlStreamReader::StartElement)
        {
          readArgument(xmlReader);

          while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Argument", Qt::CaseInsensitive)) && !xmlReader.hasError())
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
  if(!xmlReader.name().compare("Argument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
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

          while(!(xmlReader.isEndElement() && !xmlReader.name().compare("Argument", Qt::CaseInsensitive)) && !xmlReader.hasError())
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

void AbstractModelComponent::serializedDataToRawBytes(SerializableData &serializedData, char *&rawBytesData, int &length)
{
  QDataStream stream;
  stream.setByteOrder((QDataStream::ByteOrder)serializedData.byteOrder);
  stream << (qint32)serializedData.byteOrder;
  stream << (qint64)serializedData.parentMPIProcessRank;
  stream << (qint64)serializedData.componentIndex;
  stream << (qint64)serializedData.numAllocatedMPIProcesses;

  if(serializedData.numAllocatedMPIProcesses > 0)
  {
    for(int i = 0; i < serializedData.numAllocatedMPIProcesses ; i++)
    {
      stream <<  (qint64)serializedData.allocatedMPIProcesses[i];
    }
  }

  stream << (qint64)serializedData.dataSize;

  if(serializedData.dataSize > 0 && serializedData.data != nullptr)
  {
    stream.writeRawData(serializedData.data, serializedData.dataSize);
  }
  //  else
  //  {
  //    serializedData.size = 0;
  //    stream << serializedData.size;
  //  }

  char* tempRaw = nullptr;
  uint tempLength = 0;

  stream.readBytes(tempRaw, tempLength);

  rawBytesData = new char[tempLength - 4];

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
  for(int i = 3; i < tempLength; i++)
  {
    rawBytesData[i - 3] = tempRaw[i];
  }

  delete[] tempRaw;
}

void AbstractModelComponent::setInitialized(bool initialized)
{
  m_initialized = initialized;
}

void AbstractModelComponent::setPrepared(bool prepared)
{
  m_prepared = prepared;
}

void AbstractModelComponent::addInput(AbstractInput *input)
{
  if(m_inputs.find(input->id()) == m_inputs.end())
  {
    m_inputs[input->id()] = input;
    emit propertyChanged("Inputs");
  }
}

bool AbstractModelComponent::removeInput(AbstractInput *input)
{
  if(m_inputs.contains(input->id()))
  {
    m_inputs.remove(input->id());
    emit propertyChanged("Inputs");
    return true;
  }

  return false;
}

void AbstractModelComponent::clearInputs()
{

  qDeleteAll(m_inputs.values());
  m_inputs.clear();
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
    emit propertyChanged("Outputs");
  }
}

bool AbstractModelComponent::removeOutput(AbstractOutput *output)
{
  if(m_outputs.contains(output->id()))
  {
    m_outputs.remove(output->id());
    emit propertyChanged("Outputs");
    return true;
  }

  return false;
}

void AbstractModelComponent::clearOutputs()
{
  qDeleteAll(m_outputs.values());
  m_outputs.clear();

  emit propertyChanged("Outputs");
}

QHash<QString,AbstractAdaptedOutputFactory*> AbstractModelComponent::adaptedOutputFactoriesInternal() const
{
  return m_adaptedOutputFactories;
}

void AbstractModelComponent::addAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory)
{
  if(m_adaptedOutputFactories.find(adaptedOutputFactory->id()) == m_adaptedOutputFactories.end())
  {
    m_adaptedOutputFactories[adaptedOutputFactory->id()] = adaptedOutputFactory;
    emit propertyChanged("AdaptedOutputFactories");
  }
}

bool AbstractModelComponent::removeAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory)
{
  if(m_adaptedOutputFactories.contains(adaptedOutputFactory->id()) == true)
  {
    m_adaptedOutputFactories.remove(adaptedOutputFactory->id());
    emit propertyChanged("AdaptedOutputFactories");
    return true;
  }

  return false;
}

void AbstractModelComponent::clearAdaptedOutputFactories()
{
  qDeleteAll(m_adaptedOutputFactories);
  m_adaptedOutputFactories.clear();

  emit propertyChanged("AdaptedOutputFactories");
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
  //  HydroCouple::ComponentStatus oldStatus = m_status;
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

  //  printf("%s \n" , outMessage.toStdString().c_str());
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
