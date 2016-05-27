#ifndef COMPONENTDATAITEM2D_H
#define COMPONENTDATAITEM2D_H

#include "abstractcomponentdataitem.h"

class Dimension;
class ValueDefinition;

/*!
 * \brief The ComponentDataItem2D class
 */
template<class T>
class HYDROCOUPLESDK_EXPORT ComponentDataItem2D :  public virtual HydroCouple::IComponentDataItem
{

  public:

    ComponentDataItem2D(const QList<Dimension*> &dimensions, ValueDefinition* valueDefinition);

    virtual ~ComponentDataItem2D();

    QList<HydroCouple::IDimension*> dimensions() const override;

    HydroCouple::IValueDefinition* valueDefinition() const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    virtual void resetDataArray();

  private:
    void createData();//= 0;

    void deleteData(); //=0;

  private:
    QList<Dimension*> m_dimensions;
    ValueDefinition* m_valueDefinition;
    T** m_data;
};


class HYDROCOUPLESDK_EXPORT ComponentDataItem2DInt: public AbstractComponentDataItem,  public virtual ComponentDataItem2D<int>
{
    Q_OBJECT
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)

  public:

    ComponentDataItem2DInt(const QString &id, const QList<Dimension*> &dimensions, ValueDefinition *valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem2DInt();

};

class HYDROCOUPLESDK_EXPORT ComponentDataItem2DDouble: public AbstractComponentDataItem,  public virtual ComponentDataItem2D<double>
{
    Q_OBJECT
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
  public:

    ComponentDataItem2DDouble(const QString &id, const QList<Dimension*> &dimensions, ValueDefinition *valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem2DDouble();

};

class HYDROCOUPLESDK_EXPORT ComponentDataItem2DString: public AbstractComponentDataItem,  public virtual ComponentDataItem2D<QString>
{
    Q_OBJECT
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)

  public:

    ComponentDataItem2DString(const QString &id, const QList<Dimension*> &dimensions, ValueDefinition *valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem2DString();

};

Q_DECLARE_METATYPE(ComponentDataItem2DInt*)
Q_DECLARE_METATYPE(ComponentDataItem2DDouble*)
Q_DECLARE_METATYPE(ComponentDataItem2DString*)

#endif // COMPONENTDATAITEM2D_H

