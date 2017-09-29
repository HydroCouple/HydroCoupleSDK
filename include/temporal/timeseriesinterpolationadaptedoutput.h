#ifndef TIMESERIESINTERPOLATIONADAPTEDOUTPUT_H
#define TIMESERIESINTERPOLATIONADAPTEDOUTPUT_H

#include "hydrocoupletemporal.h"
#include "core/abstractadaptedoutput.h"
#include "temporal/timeseriescomponentdataitem.h"
#include "splineinterpolator.h"

#include <vector>

class TemporalInterpolationFactory;
class AbstractModelComponent;
class Quantity;
class IdBasedArgumentInt;

class HYDROCOUPLESDK_EXPORT TimeSeriesInterpolationAdaptedOutput : public AbstractAdaptedOutput,
    public TimeSeriesComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeSeriesComponentDataItem
{
    Q_OBJECT

    Q_INTERFACES(HydroCouple::IAdaptedOutput
                 HydroCouple::Temporal::ITimeComponentDataItem
                 HydroCouple::Temporal::ITimeSeriesComponentDataItem)

  public:
    TimeSeriesInterpolationAdaptedOutput(const QString &id,
                                         Quantity *valueDefinition,
                                         HydroCouple::Temporal::ITimeSeriesComponentDataItem *adaptee,
                                         AbstractAdaptedOutputFactory *timeSeriesInterpolationFactory);

    virtual ~TimeSeriesInterpolationAdaptedOutput();

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    int timeCount() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    void updateValues(HydroCouple::IInput* querySpecifier) override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, void *data) const override;

    void getValues(int timeIndex, int stride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int timeIndex, const void *data) override;

    void setValues(int timeIndex, int stride, const void *data) override;

    void initialize() override;

    void refresh() override;

    SplineInterpolator::InterpolationMethod interpolationMethod() const;

    void setInterpolationMethod(SplineInterpolator::InterpolationMethod interpolationMethod);

  signals:

    void propertyChanged(const QString& propertyName) override;

  private:

     SplineInterpolator::InterpolationMethod getInterpolationMode(int count);

  private:

    IdBasedArgumentInt* m_interpolationOptionsArgument;
    int m_polynomialOrder ;
    Dimension* m_timeDimension;
    std::vector<double> m_timesBuffer;
    std::vector<double> m_valuesBuffer;
    HydroCouple::Temporal::ITimeSeriesComponentDataItem* m_adaptee;
    SplineInterpolator::InterpolationMethod m_interpolationMode;

};

Q_DECLARE_METATYPE(TimeSeriesInterpolationAdaptedOutput*)

#endif // TIMESERIESINTERPOLATIONADAPTEDOUTPUT_H

