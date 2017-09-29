#ifndef TIMEGEOMETRYMULTIINPUT_H
#define TIMEGEOMETRYMULTIINPUT_H

#include "core/abstractinput.h"
#include "spatiotemporal/timegeometrycomponentdataitem.h"

class HYDROCOUPLESDK_EXPORT TimeGeometryMultiInputDouble : public AbstractMultiInput,
    public TimeGeometryComponentDataItem<double>,
    public virtual HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                 HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem)

  public:

    TimeGeometryMultiInputDouble(const QString &id,
                                 HydroCouple::Spatial::IGeometry::GeometryType geometryType,
                                 Dimension *timeDimension,
                                 Dimension *geometryDimension,
                                 ValueDefinition *valueDefinition,
                                 AbstractModelComponent *modelComponent);

    virtual ~TimeGeometryMultiInputDouble();

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    int timeCount() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    int geometryCount() const override;

    HydroCouple::Spatial::IGeometry *geometry(int geometryIndex) const override;

    HydroCouple::IDimension *geometryDimension() const override;

    HydroCouple::Spatial::IEnvelope *envelope() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, int geometryIndex, void *data) const override;

    void getValues(int timeIndex, int geometryIndex, int timeStride, int geomStride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int timeIndex, int geometryIndex, const void *data) override;

    void setValues(int timeIndex, int geometryIndex, int timeStride, int geomStride, const void *data) override;

  private:

    Dimension *m_timeDimension, *m_geometryDimension;

};


#endif // TIMEGEOMETRYMULTIINPUT_H
