#include "stdafx.h"
#include "core/abstractadaptedoutput.h"
#include "core/valuedefinition.h"
#include "temporal/timeseriesinterpolationadaptedoutput.h"
#include "core/idbasedargument.h"
#include "core/dimension.h"
#include "core/idbasedargument.h"
#include "temporal/temporalinterpolationfactory.h"
#include "temporal/timedata.h"
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Temporal;

TimeSeriesInterpolationAdaptedOutput::TimeSeriesInterpolationAdaptedOutput(const QString& id,
                                                                           Quantity* valueDefinition,
                                                                           HydroCouple::Temporal::ITimeExchangeItem* adaptee,
                                                                           AbstractAdaptedOutputFactory* temporalInterpolationFactory)
  : AbstractAdaptedOutput(id,QList<Dimension*>({new Dimension("timeDimension" , "Time dimension for adapted output", temporalInterpolationFactory )}),valueDefinition,dynamic_cast<HydroCouple::IOutput*>(adaptee),temporalInterpolationFactory),
    TimeSeriesComponentDataItem<double>(QList<SDKTemporal::Time*>({new SDKTemporal::Time(temporalInterpolationFactory)}),valueDefinition->defaultValue().toDouble()),
    m_polynomialOrderArgument(nullptr),
    m_polynomialOrder(2),
    m_adaptee(adaptee)
{
  m_timeDimension = dimensionsInternal()[0];

  Dimension *identifierDimension = new Dimension("PolynomialOrderDimension","Dimension for the order of the polynomial",this);
  QStringList identifiers;
  identifiers.append("PolynomialOrder");
  Quantity* quantity = Quantity::unitLessValues("IdentifiersUnit","", QVariant::Int , this);

  m_polynomialOrderArgument = new IdBasedArgumentInt("PolynomialOrder", identifiers, identifierDimension, quantity, nullptr);
  m_polynomialOrderArgument->setCaption("Interpolation polynomial order");
  m_polynomialOrderArgument->addComment("Interpolation polynomila order must be at least 2 for Linear, 3 for Quadratic, 4 for Cubic, etc.");
  m_polynomialOrderArgument->setValue(0, m_polynomialOrder);

  addArgument(m_polynomialOrderArgument);
  m_timeSeriesBuffer[-1000000000000000000000.00000] = 0;
}


TimeSeriesInterpolationAdaptedOutput::~TimeSeriesInterpolationAdaptedOutput()
{
  delete m_polynomialOrderArgument;
}

int TimeSeriesInterpolationAdaptedOutput::polynomialOrder() const
{
  return m_polynomialOrder;
}

void TimeSeriesInterpolationAdaptedOutput::setPolynomialOrder(int polynomialOrder)
{
  if(polynomialOrder >= 2)
  {
    m_polynomialOrder = polynomialOrder;
  }
}

void TimeSeriesInterpolationAdaptedOutput::initialize()
{

    m_polynomialOrderArgument->getValues(0,1,&m_polynomialOrder);

    if(m_polynomialOrder < 2)
      m_polynomialOrder = 2;

    initializeAdaptedOutputs();
}

void TimeSeriesInterpolationAdaptedOutput::refresh()
{
  //retrieve data into buffer and trim appropriately.

  int timeDimIndex = m_adaptee->dimensionLength(nullptr,0) - 1;
  double time = m_adaptee->times()[timeDimIndex]->dateTime();

  QVariant value(0.0);
  m_adaptee->getValue(timeDimIndex,value);

  m_timeSeriesBuffer[time] = value.toDouble();

  while (m_timeSeriesBuffer.count() > m_polynomialOrder)
  {
     m_timeSeriesBuffer.remove(m_timeSeriesBuffer.keys()[0]);
  }

  refreshAdaptedOutputs();
}

QList<ITime*> TimeSeriesInterpolationAdaptedOutput::times() const
{
  QList<ITime*> otimes;
  QList<SDKTemporal::Time*> itimes = TimeSeriesComponentDataItem<double>::timesInternal();

  for(SDKTemporal::Time* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

ITimeSpan* TimeSeriesInterpolationAdaptedOutput::timeSpan() const
{
  return TimeSeriesComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesInterpolationAdaptedOutput::timeDimension() const
{
  return m_timeDimension;
}

void TimeSeriesInterpolationAdaptedOutput::update(IInput *querySpecifier)
{
  ITimeExchangeItem* timeExchangeItem = dynamic_cast<ITimeExchangeItem*>(querySpecifier);

  if(timeExchangeItem)
  {
    QList<HydroCouple::Temporal::ITime*> inpTimes = timeExchangeItem->times();
    HydroCouple::Temporal::ITime *queryTime = inpTimes[inpTimes.length() -1];

    while (m_timeSeriesBuffer.keys()[m_timeSeriesBuffer.keys().length() - 1] < queryTime->dateTime() &&
           modelComponent()->status() == HydroCouple::Updated)
    {
      modelComponent()->update();
    }

    int timeDimLength = length();

    QList<SDKTemporal::Time*> ctimes = timesInternal();
    SDKTemporal::Time* lastTime = ctimes[timeDimLength -1];

    double currentTime = m_timeSeriesBuffer.keys()[m_timeSeriesBuffer.keys().length() - 1];

    if(currentTime > lastTime->dateTime())
    {
      if(timeDimLength > 1)
      {
        double values[timeDimLength -1];
        getValues(1,timeDimLength - 1,values);
        setValues(0,timeDimLength - 1,values);

        for(int i = 0 ; i < timeDimLength -1; i++)
        {
          ctimes[i]->setDateTime(ctimes[i+1]->qDateTime());
        }
      }

      lastTime->setDateTime(currentTime);
    }

    double value = interpolate(queryTime->dateTime(), m_timeSeriesBuffer.keys() , m_timeSeriesBuffer.values());
    setValueT(timeDimLength - 1, value);
  }
  else
  {
    while (modelComponent()->status() != HydroCouple::Done &&
           modelComponent()->status() != HydroCouple::Failed &&
           modelComponent()->status() != HydroCouple::Finished)
    {
      modelComponent()->update();
    }
  }

  QList<HydroCouple::IAdaptedOutput*> tadaptedOutputs = adaptedOutputs();

  for(HydroCouple::IAdaptedOutput* adaptedOutput :tadaptedOutputs)
  {
    adaptedOutput->refresh();
  }
}

int TimeSeriesInterpolationAdaptedOutput::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void TimeSeriesInterpolationAdaptedOutput::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesInterpolationAdaptedOutput::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesInterpolationAdaptedOutput::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesInterpolationAdaptedOutput::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesInterpolationAdaptedOutput::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesInterpolationAdaptedOutput::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesInterpolationAdaptedOutput::getValue(int timeIndex, QVariant &data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(timeIndex,data);
}

void TimeSeriesInterpolationAdaptedOutput::getValues(int timeIndex, int stride, QVariant data[]) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesInterpolationAdaptedOutput::getValues(int timeIndex, int stride, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesInterpolationAdaptedOutput::setValue(int timeIndex, const QVariant &data)
{
  TimeSeriesComponentDataItem<double>::setValueT(timeIndex,data);
}

void TimeSeriesInterpolationAdaptedOutput::setValues(int timeIndex, int stride, const QVariant data[])
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex,stride,data);
}

void TimeSeriesInterpolationAdaptedOutput::setValues(int timeIndex, int stride, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex,stride,data);
}

double TimeSeriesInterpolationAdaptedOutput::interpolate(double t, const QList<double> &ts, const QList<double> &ys)
{
  double value = 0;

  for(int i = 0 ; i < ts.length() ; i++)
  {
    value += ys[i] * basis(i,t,ts);
  }

  return value;
}

double TimeSeriesInterpolationAdaptedOutput::basis(int tIndex, double t, const QList<double>& ts)
{
  double tsj = ts[tIndex];
  double value = 1.0;

  for(int i = 0 ; i < ts.length() ; i++)
  {
    if(tIndex != i)
    {
      value *= (t - ts[i])/(tsj-ts[i]);
    }
  }

  return value;
}


