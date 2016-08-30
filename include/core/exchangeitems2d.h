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

#ifndef COMPONENTDATAITEM2D_H
#define COMPONENTDATAITEM2D_H

#include "abstractinput.h"

class Dimension;
class ValueDefinition;

/*!
 * \brief The ComponentDataItem2D class
 */
template<class T>
class HYDROCOUPLESDK_EXPORT ComponentDataItem2D
{

  public:

    ComponentDataItem2D(int iLength, int jLength, const T& defaultValue);

    virtual ~ComponentDataItem2D();

    void getValueT(int dimensionIndexes[], QVariant &data) const ;

    void getValuesT(int dimensionIndexes[], int stride[],  QVariant data[]) const ;

    void getValuesT(int dimensionIndexes[], int stride[],  void *data) const ;

    void setValueT(int dimensionIndexes[], const QVariant &data) ;

    void setValuesT(int dimensionIndexes[], int stride[], const QVariant data[]) ;

    void setValuesT(int dimensionIndexes[], int stride[], const void *data) ;

    virtual void resetDataArray();

    virtual void resetDataArray(int iLength, int jLength);

    T defaultValue() const;

  protected:

    void setDefaultValue(const T& defaultValue);

    int iLength() const;

    void setILength(int iLength);

    int jLength() const;

    void setJLength(int jLength);

  private:

    void createData();//= 0;

    void deleteData(); //=0;

  private:
    int m_iLength, m_jLength;
    T m_defaultValue;
    T** m_data;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT Input2DInt: public AbstractInput,
    public ComponentDataItem2D<int>
{
    Q_OBJECT

  public:

    Input2DInt(const QString& id,
               Dimension* iDimension, Dimension* jDimension,
               int iLength, int jLength,
               ValueDefinition* valueDefinition,
               AbstractModelComponent* modelComponent);

    virtual ~Input2DInt();

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT Input2DDouble: public AbstractInput,
    public ComponentDataItem2D<double>
{
    Q_OBJECT

  public:

    Input2DDouble(const QString& id,
                  Dimension* iDimension, Dimension* jDimension,
                  int iDimensionLength, int jDimensionLength,
                  ValueDefinition* valueDefinition,
                  AbstractModelComponent* modelComponent);

    virtual ~Input2DDouble();

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT Input2DString: public AbstractInput,
    public ComponentDataItem2D<QString>
{
    Q_OBJECT

  public:

    Input2DString(const QString& id,
                  Dimension* iDimension, Dimension* jDimension,
                  int iDimensionLength, int jDimensionLength,
                  ValueDefinition* valueDefinition,
                  AbstractModelComponent* modelComponent);

    virtual ~Input2DString();

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

};

Q_DECLARE_METATYPE(Input2DInt*)
Q_DECLARE_METATYPE(Input2DDouble*)
Q_DECLARE_METATYPE(Input2DString*)

#endif // COMPONENTDATAITEM2D_H

