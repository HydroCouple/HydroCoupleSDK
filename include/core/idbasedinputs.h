/*!
 * \file idbasedexchangeitems.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
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

#ifndef IDBASEDINPUTS_H
#define IDBASEDINPUTS_H

#include "abstractinput.h"
#include "idbasedcomponentdataitem.h"

class HYDROCOUPLESDK_EXPORT IdBasedInputInt : public AbstractInput,
    public IdBasedComponentDataItem<int>,
    public virtual HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

    using IComponentDataItem::getValue;
    using IComponentDataItem::setValue;

  public:

    IdBasedInputInt(const QString& id,
                    const QStringList& identifiers,
                    Dimension *identifierDimension,
                    ValueDefinition* valueDefinition,
                    AbstractModelComponent* modelComponent);

    virtual ~IdBasedInputInt();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int idIndex, void* data) const override;

    void getValues(int idIndex, int stride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int idIndex, const void * data) override;

    void setValues(int idIndex, int stride, const void *data) override;

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) const override;

  private:

    Dimension* m_identifierDimension;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedInputDouble : public AbstractInput,
    public IdBasedComponentDataItem<double>,
    public virtual  HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:

    IdBasedInputDouble(const QString& id,
                       const QStringList& identifiers,
                       Dimension *identifierDimension,
                       ValueDefinition* valueDefinition,
                       AbstractModelComponent* modelComponent);

    virtual ~IdBasedInputDouble();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int idIndex, void* data) const override;

    void getValues(int idIndex, int stride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int idIndex, const void * data) override;

    void setValues(int idIndex, int stride, const void *data) override;

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) const override;

  private:
    Dimension* m_identifierDimension;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedInputString : public AbstractInput,
    public IdBasedComponentDataItem<QString>,
    public virtual HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:

    IdBasedInputString(const QString& id,
                       const QStringList& identifiers,
                       Dimension *identifierDimension,
                       ValueDefinition* valueDefinition,
                       AbstractModelComponent* modelComponent);

    virtual ~IdBasedInputString();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int idIndex, void* data) const override;

    void getValues(int idIndex, int stride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int idIndex, const void * data) override;

    void setValues(int idIndex, int stride, const void *data) override;

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) const override;

  private:
    Dimension* m_identifierDimension;
};

Q_DECLARE_METATYPE(IdBasedInputInt*)
Q_DECLARE_METATYPE(IdBasedInputDouble*)
Q_DECLARE_METATYPE(IdBasedInputString*)

#endif // IDBASEDINPUTS_H

