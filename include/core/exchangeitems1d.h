/*!
 *  \file    AbstractAdaptedOutputFactoryComponentInfo.h
 *  \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version 1.0.0.0
 *  \section Description
 *  \section License
 *  AbstractAdaptedOutputFactoryComponentInfo.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  AbstractAdaptedOutputFactoryComponentInfo.h its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2016
 *  \pre
 *  \bug
 *  \todo finish writing corresponding outputs.
 *  \warning
 */

#ifndef EXCHANGEITEMS1D_H
#define EXCHANGEITEMS1D_H

#include  "abstractinput.h"

/*!
 * \brief The ComponentDataItem1D class
 */
template<class T>
class HYDROCOUPLESDK_EXPORT ComponentDataItem1D
{

  public:

    ComponentDataItem1D(int length, const T& defaultValue);

    virtual ~ComponentDataItem1D();

    void getValueT(int dimensionIndexes[], QVariant &data) const ;

    void getValueT(int dimensionIndex, QVariant &data) const ;

    void getValuesT(int dimensionIndexes[], int stride[],  QVariant data[]) const ;

    void getValuesT(int dimensionIndexes, int stride,  QVariant data[]) const ;

    void getValuesT(int dimensionIndexes[], int stride[],  void *data) const ;

    void getValuesT(int dimensionIndex, int stride,  void *data) const ;

    void setValueT(int dimensionIndexes[], const QVariant &data);

    void setValueT(int dimensionIndex, const QVariant &data);

    void setValuesT(int dimensionIndexes[], int stride[], const QVariant data[]);

    void setValuesT(int dimensionIndex, int stride, const QVariant data[]);

    void setValuesT(int dimensionIndexes[], int stride[], const void *data);

    void setValuesT(int dimensionIndex, int stride, const void *data) ;

    virtual void resetDataArray();

    virtual void resetDataArray(int length);

    T defaultValue() const;

    T &operator[](int index);

  protected:

    void setDefaultValue(const T& defaultValue);

    int length() const;

    void setLength(int length);

  private:

    void createData();

    void deleteData();

  private:
    int m_length;
    T m_defaultValue;
    T* m_data;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT Input1DInt: public AbstractInput,
    public ComponentDataItem1D<int>
{
    Q_OBJECT

  public:

    Input1DInt(const QString& id,
               Dimension* dimension,
               int length,
               ValueDefinition *valueDefinition,
               AbstractModelComponent* modelComponent);

    virtual ~Input1DInt();

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;
};

//==============================================================================================================================


class HYDROCOUPLESDK_EXPORT Input1DDouble: public AbstractInput,
    public ComponentDataItem1D<double>
{
    Q_OBJECT

  public:

    Input1DDouble(const QString& id,
                  Dimension* dimension,
                  int length,
                  ValueDefinition* valueDefinition,
                  AbstractModelComponent *modelComponent);

    virtual ~Input1DDouble();

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT Input1DString: public AbstractInput,
    public ComponentDataItem1D<QString>
{
    Q_OBJECT

  public:

    Input1DString(const QString &id,
                  Dimension* dimension,
                  int length,
                  ValueDefinition* valueDefinition,
                  AbstractModelComponent* modelComponent);

    virtual ~Input1DString();

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

};

Q_DECLARE_METATYPE(Input1DInt*)
Q_DECLARE_METATYPE(Input1DDouble*)
Q_DECLARE_METATYPE(Input1DString*)

#endif // EXCHANGEITEMS1D_H

