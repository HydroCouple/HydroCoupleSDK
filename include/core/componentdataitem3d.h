#ifndef COMPONENTDATAITEM3D_H
#define COMPONENTDATAITEM3D_H

#include "abstractcomponentdataitem.h"

class Dimension;
class ValueDefinition;

/*!
 * \brief The ComponentDataItem3D class
 */
template<class T>
class HYDROCOUPLESDK_EXPORT ComponentDataItem3D :  public virtual HydroCouple::IComponentDataItem
{

  public:

    ComponentDataItem3D(const QList<Dimension*> &dimensions, ValueDefinition* valueDefinition);

    virtual ~ComponentDataItem3D();

    QList<HydroCouple::IDimension*> dimensions() const override;

    HydroCouple::IValueDefinition* valueDefinition() const override;

    void getValue(int dimensionIndexes[], QVariant & data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    virtual void resetDataArray();

  private:

    void createData();

    void deleteData();

  private:
    QList<Dimension*> m_dimensions;
    ValueDefinition* m_valueDefinition;
    T*** m_data;
};


class HYDROCOUPLESDK_EXPORT ComponentDataItem3DInt: public AbstractComponentDataItem,  public virtual ComponentDataItem3D<int>
{

    Q_OBJECT
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)

  public:

    ComponentDataItem3DInt(const QString &id, const QList<Dimension*> &dimensions, ValueDefinition *valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem3DInt();

};

class HYDROCOUPLESDK_EXPORT ComponentDataItem3DDouble: public AbstractComponentDataItem,  public virtual ComponentDataItem3D<double>
{

    Q_OBJECT
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)

  public:

    ComponentDataItem3DDouble(const QString &id, const QList<Dimension*> &dimensions, ValueDefinition *valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem3DDouble();

};

class HYDROCOUPLESDK_EXPORT ComponentDataItem3DString: public AbstractComponentDataItem,  public virtual ComponentDataItem3D<QString>
{
    Q_OBJECT
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)

  public:

    ComponentDataItem3DString(const QString &id, const QList<Dimension*> &dimensions, ValueDefinition *valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem3DString();

};

Q_DECLARE_METATYPE(ComponentDataItem3DInt*)
Q_DECLARE_METATYPE(ComponentDataItem3DDouble*)
Q_DECLARE_METATYPE(ComponentDataItem3DString*)

#endif // COMPONENTDATAITEM3D_H

