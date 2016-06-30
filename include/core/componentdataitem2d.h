#ifndef COMPONENTDATAITEM2D_H
#define COMPONENTDATAITEM2D_H

#include "abstractcomponentdataitem.h"

class Dimension;
class ValueDefinition;

/*!
 * \brief The ComponentDataItem2D class
 */
template<class T>
class HYDROCOUPLESDK_EXPORT ComponentDataItem2D
{

  public:

    ComponentDataItem2D(const QList<Dimension*> &dimensions, const T& defaultValue);

    virtual ~ComponentDataItem2D();

    void getValueT(int dimensionIndexes[], QVariant &data) const ;

    void getValuesT(int dimensionIndexes[], int stride[],  QVariant data[]) const ;

    void getValuesT(int dimensionIndexes[], int stride[],  void *data) const ;

    void setValueT(int dimensionIndexes[], const QVariant &data) ;

    void setValuesT(int dimensionIndexes[], int stride[], const QVariant data[]) ;

    void setValuesT(int dimensionIndexes[], int stride[], const void *data) ;

    virtual void resetDataArray();

    T defaultValue() const;

  protected:

    void setDefaultValue(const T& defaultValue);

  private:

    void createData();//= 0;

    void deleteData(); //=0;

  private:
    QList<Dimension*> m_dimensions;
    T m_defaultValue;
    T** m_data;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT ComponentDataItem2DInt: public AbstractComponentDataItem,
    public virtual ComponentDataItem2D<int>
{
    Q_OBJECT

  public:

    ComponentDataItem2DInt(const QString& id,
                           const QList<Dimension*>& dimensions,
                           ValueDefinition* valueDefinition,
                           AbstractModelComponent* modelComponent);

    virtual ~ComponentDataItem2DInt();

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT ComponentDataItem2DDouble: public AbstractComponentDataItem,
    public virtual ComponentDataItem2D<double>
{
    Q_OBJECT

  public:

    ComponentDataItem2DDouble(const QString& id,
                              const QList<Dimension*>& dimensions,
                              ValueDefinition* valueDefinition,
                              AbstractModelComponent* modelComponent);

    virtual ~ComponentDataItem2DDouble();

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT ComponentDataItem2DString: public AbstractComponentDataItem,
    public virtual ComponentDataItem2D<QString>
{
    Q_OBJECT

  public:

    ComponentDataItem2DString(const QString& id,
                              const QList<Dimension*>& dimensions,
                              ValueDefinition* valueDefinition,
                              AbstractModelComponent* modelComponent);

    virtual ~ComponentDataItem2DString();

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

};

Q_DECLARE_METATYPE(ComponentDataItem2DInt*)
Q_DECLARE_METATYPE(ComponentDataItem2DDouble*)
Q_DECLARE_METATYPE(ComponentDataItem2DString*)

#endif // COMPONENTDATAITEM2D_H

