#ifndef COMPONENTDATAITEM1D_H
#define COMPONENTDATAITEM1D_H

#include "abstractcomponentdataitem.h"

/*!
 * \brief The ComponentDataItem1D class
 */
template<class T>
class HYDROCOUPLESDK_EXPORT ComponentDataItem1D
{

  public:

    ComponentDataItem1D(Dimension *dimension, const T& defaultValue);

    virtual ~ComponentDataItem1D();

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

    Dimension* dimensionInternal() const;

  private:

    void createData();

    void deleteData();

  private:
    Dimension* m_dimension;
    T m_defaultValue;
    T* m_data;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT ComponentDataItem1DInt: public AbstractComponentDataItem,
    public ComponentDataItem1D<int>
{
    Q_OBJECT

  public:

    ComponentDataItem1DInt(const QString& id,
                           Dimension* dimension,
                           ValueDefinition *valueDefinition,
                           AbstractModelComponent* modelComponent);

    virtual ~ComponentDataItem1DInt();

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

  private:
    Dimension* m_dimension;
};

//==============================================================================================================================


class HYDROCOUPLESDK_EXPORT ComponentDataItem1DDouble: public AbstractComponentDataItem,
    public ComponentDataItem1D<double>
{
    Q_OBJECT

  public:

    ComponentDataItem1DDouble(const QString& id,
                              Dimension* dimension,
                              ValueDefinition* valueDefinition,
                              AbstractModelComponent *modelComponent);

    virtual ~ComponentDataItem1DDouble();

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

  private:
    Dimension* m_dimension;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT ComponentDataItem1DString: public AbstractComponentDataItem,
    public ComponentDataItem1D<QString>
{
    Q_OBJECT

  public:

    ComponentDataItem1DString(const QString &id,
                              Dimension* dimension,
                              ValueDefinition* valueDefinition,
                              AbstractModelComponent* modelComponent);

    virtual ~ComponentDataItem1DString();

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

  private:
    Dimension* m_dimension;
};

Q_DECLARE_METATYPE(ComponentDataItem1DInt*)
Q_DECLARE_METATYPE(ComponentDataItem1DDouble*)
Q_DECLARE_METATYPE(ComponentDataItem1DString*)

#endif // COMPONENTDATAITEM1D_H

