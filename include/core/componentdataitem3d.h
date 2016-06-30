#ifndef COMPONENTDATAITEM3D_H
#define COMPONENTDATAITEM3D_H

#include "abstractcomponentdataitem.h"

class Dimension;
class ValueDefinition;

/*!
 * \brief The ComponentDataItem3D class
 */
template<class T>
class HYDROCOUPLESDK_EXPORT ComponentDataItem3D
{

  public:

    ComponentDataItem3D(const QList<Dimension*> &dimensions, const T& defaultValue);

    virtual ~ComponentDataItem3D();

    void getValueT(int dimensionIndexes[], QVariant & data) const;

    void getValuesT(int dimensionIndexes[], int stride[],  QVariant data[]) const;

    void getValuesT(int dimensionIndexes[], int stride[],  void *data) const;

    void setValueT(int dimensionIndexes[], const QVariant &data);

    void setValuesT(int dimensionIndexes[], int stride[], const QVariant data[]);

    void setValuesT(int dimensionIndexes[], int stride[], const void *data);

    virtual void resetDataArray();

    T defaultValue() const;

  protected:

    void setDefaultValue(const T& defaultValue);

  private:

    void createData();

    void deleteData();

  private:

    QList<Dimension*> m_dimensions;
    T m_defaultValue;
    T*** m_data;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT ComponentDataItem3DInt: public AbstractComponentDataItem,
    public ComponentDataItem3D<int>
{

    Q_OBJECT


  public:

    ComponentDataItem3DInt(const QString& id,
                           const QList<Dimension*>& dimensions,
                           ValueDefinition *valueDefinition,
                           AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem3DInt();

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT ComponentDataItem3DDouble: public AbstractComponentDataItem,
    public ComponentDataItem3D<double>
{

    Q_OBJECT

  public:

    ComponentDataItem3DDouble(const QString &id, const QList<Dimension*> &dimensions, ValueDefinition *valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem3DDouble();

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT ComponentDataItem3DString: public AbstractComponentDataItem,
    public ComponentDataItem3D<QString>
{
    Q_OBJECT

  public:

    ComponentDataItem3DString(const QString &id, const QList<Dimension*> &dimensions, ValueDefinition *valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem3DString();

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

};

Q_DECLARE_METATYPE(ComponentDataItem3DInt*)
Q_DECLARE_METATYPE(ComponentDataItem3DDouble*)
Q_DECLARE_METATYPE(ComponentDataItem3DString*)

#endif // COMPONENTDATAITEM3D_H

