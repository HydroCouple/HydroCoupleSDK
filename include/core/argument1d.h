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

#ifndef ARGUMENT1D_H
#define ARGUMENT1D_H

#include "abstractargument.h"
#include "componentdataitem1d.h"
#include <QFileInfo>

class HYDROCOUPLESDK_EXPORT Argument1DInt : public AbstractArgument,
    public ComponentDataItem1D<int>
{

  public:
    Argument1DInt(const QString& id,
                  Dimension* dimension,
                  int length,
                  ValueDefinition *valueDefinition,
                  AbstractModelComponent *modelComponent);

    virtual ~Argument1DInt(){}

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

    QString toString() const override;

    bool readValues(const QString &value, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem) override;

  private:
    Dimension* m_dimension;
    QFileInfo m_inputFile;
};

class HYDROCOUPLESDK_EXPORT Argument1DDouble : public AbstractArgument,
    public ComponentDataItem1D<double>
{
  public:
    Argument1DDouble(const QString& id,
                     Dimension* dimension,
                     int length,
                     ValueDefinition *valueDefinition,
                     AbstractModelComponent *modelComponent);

    virtual ~Argument1DDouble(){}

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

    QString toString() const override;

    bool readValues(const QString &value, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem) override;

  private:
    Dimension* m_dimension;
    QFileInfo m_inputFile;

};

class HYDROCOUPLESDK_EXPORT Argument1DString : public AbstractArgument,
    public ComponentDataItem1D<QString>
{
  public:
    Argument1DString(const QString& id,
                     Dimension* dimension,
                     int length,
                     ValueDefinition* valueDefinition,
                     AbstractModelComponent* modelComponent);

    virtual ~Argument1DString(){}

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

    QString toString() const override;

    bool readValues(const QString &value, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem) override;

  private:
    Dimension* m_dimension;
    QFileInfo m_inputFile;
};


Q_DECLARE_METATYPE(Argument1DInt*)
Q_DECLARE_METATYPE(Argument1DDouble*)
Q_DECLARE_METATYPE(Argument1DString*)

#endif // ARGUMENT1D_H

