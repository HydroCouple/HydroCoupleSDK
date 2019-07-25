/*!
 * \file exchangeitems2d.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */


#ifndef COMPONENTDATAITEM2D_H
#define COMPONENTDATAITEM2D_H

#include "componentdataitem.h"
#include "abstractinput.h"
#include <QDateTime>

class Dimension;
class ValueDefinition;


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

    virtual ~Input2DInt() override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes,  void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

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

    virtual ~Input2DDouble() override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes,  void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

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

    virtual ~Input2DString() override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes,  void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

};

Q_DECLARE_METATYPE(Input2DInt*)
Q_DECLARE_METATYPE(Input2DDouble*)
Q_DECLARE_METATYPE(Input2DString*)

#endif // COMPONENTDATAITEM2D_H

