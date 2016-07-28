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
 *  \todo
 *  \warning
 */

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

    ComponentDataItem3D(int iLength, int jLength, int kLength, const T& defaultValue);

    virtual ~ComponentDataItem3D();

    void getValueT(int dimensionIndexes[], QVariant & data) const;

    void getValuesT(int dimensionIndexes[], int stride[],  QVariant data[]) const;

    void getValuesT(int dimensionIndexes[], int stride[],  void *data) const;

    void setValueT(int dimensionIndexes[], const QVariant &data);

    void setValuesT(int dimensionIndexes[], int stride[], const QVariant data[]);

    void setValuesT(int dimensionIndexes[], int stride[], const void *data);

    virtual void resetDataArray();

    virtual void resetDataArray(int iLength, int jLength, int kLength);

    T defaultValue() const;

  protected:

    void setDefaultValue(const T& defaultValue);

    int iLength() const;

    void setILength(int ilength);

    int jLength() const;

    void setJLength(int jlength);

    int kLength() const;

    void setKLength(int klength);

  private:

    void createData();

    void deleteData();

  private:
    int m_iLength, m_jLength, m_kLength;
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
                           Dimension* iDimension, Dimension* jDimension, Dimension* kDimension,
                           int iLength, int jLength, int kLength,
                           ValueDefinition *valueDefinition,
                           AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem3DInt();

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

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

    ComponentDataItem3DDouble(const QString &id,
                              Dimension* iDimension, Dimension* jDimension, Dimension* kDimension,
                              int iLength, int jLength, int kLength,
                              ValueDefinition *valueDefinition,
                              AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem3DDouble();

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

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

    ComponentDataItem3DString(const QString &id,
                              Dimension* iDimension, Dimension* jDimension, Dimension* kDimension,
                              int iLength, int jLength, int kLength,
                              ValueDefinition *valueDefinition,
                              AbstractModelComponent* parentModelComponent);

    virtual ~ComponentDataItem3DString();

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

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

