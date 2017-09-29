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

#include "componentdataitem.h"
#include  "abstractinput.h"
#include <QDateTime>


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

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes,  void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) const override;
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

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes,  void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) const override;

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

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes,  void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) const override;

};

Q_DECLARE_METATYPE(Input1DInt*)
Q_DECLARE_METATYPE(Input1DDouble*)
Q_DECLARE_METATYPE(Input1DString*)

#endif // EXCHANGEITEMS1D_H

