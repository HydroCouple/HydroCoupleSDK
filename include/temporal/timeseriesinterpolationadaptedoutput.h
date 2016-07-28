#ifndef TIMESERIESINTERPOLATIONADAPTEDOUTPUT_H
#define TIMESERIESINTERPOLATIONADAPTEDOUTPUT_H

#include "hydrocoupletemporal.h"
#include "core/abstractadaptedoutput.h"
#include "temporal/timeseriescomponentdataitem.h"

class TemporalInterpolationFactory;
class AbstractModelComponent;
class Quantity;
class IdBasedArgumentInt;

class HYDROCOUPLESDK_EXPORT TimeSeriesInterpolationAdaptedOutput : public AbstractAdaptedOutput,
    public TimeSeriesComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeExchangeItem
{
    Q_OBJECT

    Q_INTERFACES(HydroCouple::IAdaptedOutput
                 HydroCouple::Temporal::ITimeExchangeItem)

    Q_PROPERTY(int PolynomialOrder READ polynomialOrder WRITE setPolynomialOrder NOTIFY propertyChanged)


  public:


    TimeSeriesInterpolationAdaptedOutput(const QString& id,
                                         Quantity* valueDefinition,
                                         HydroCouple::Temporal::ITimeExchangeItem* adaptee,
                                         AbstractAdaptedOutputFactory* timeSeriesInterpolationFactory);

    virtual ~TimeSeriesInterpolationAdaptedOutput();

    int polynomialOrder() const;

    void setPolynomialOrder(int polynomialOrder);

    void initialize() override;

    void refresh() override;

    QList<HydroCouple::Temporal::ITime*> times() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    void update(HydroCouple::IInput* querySpecifier) override;

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int timeIndex, QVariant &data) const override;

    void getValues(int timeIndex, int stride, QVariant data[]) const override;

    void getValues(int timeIndex, int stride, void *data) const override;

    void setValue(int timeIndex, const QVariant &data) override;

    void setValues(int timeIndex, int stride, const QVariant data[]) override;

    void setValues(int timeIndex, int stride, const void *data) override;

  signals:

    void propertyChanged(const QString& propertyName);

  private:

    static double interpolate(double t, const QList<double>& ts , const QList<double>& ys);

    static double basis(int tIndex, double t, const QList<double>& ts);

  private:

    IdBasedArgumentInt* m_polynomialOrderArgument;
    int m_polynomialOrder;
    Dimension* m_timeDimension;
    QMap<double,double> m_timeSeriesBuffer;
    HydroCouple::Temporal::ITimeExchangeItem* m_adaptee;

};

Q_DECLARE_METATYPE(TimeSeriesInterpolationAdaptedOutput*)

#endif // TIMESERIESINTERPOLATIONADAPTEDOUTPUT_H

