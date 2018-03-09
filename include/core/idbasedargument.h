/*!
 * \file exchangeitems2d.h
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

#ifndef IDBASEDARGUMENT_H
#define IDBASEDARGUMENT_H

#include "abstractargument.h"
#include "idbasedexchangeitems.h"
#include <QFileInfo>

class AbstractArgument;

class HYDROCOUPLESDK_EXPORT IdBasedArgumentInt : public AbstractArgument,
    public IdBasedComponentDataItem<int>,
    public virtual HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT

    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)


  public:

    IdBasedArgumentInt(const QString& id,
                       const QStringList& identifiers,
                       Dimension* identifierDimension,
                       ValueDefinition* valueDefinition,
                       AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedArgumentInt();

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

    void saveData() override;

    QString toString() const override;

    bool readValues(const QString &value, QString &message, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem, QString &message) override;

    bool matchIdentifiersWhenReading() const;

    void setMatchIdentifiersWhenReading(bool match);

  private:
    QFileInfo m_inputFile;
    bool m_matchIdentifiersDuringRead;
    Dimension* m_identifierDimension;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedArgumentDouble : public AbstractArgument,
    public IdBasedComponentDataItem<double>,
    public virtual HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:
    IdBasedArgumentDouble(const QString& id,
                          const QStringList& identifiers,
                          Dimension* identifierDimension,
                          ValueDefinition* valueDefinition,
                          AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedArgumentDouble();

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

    void saveData() override;

    QString toString() const override;

    bool readValues(const QString &value, QString &message, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem, QString &message) override;

    bool matchIdentifiersWhenReading() const;

    void setMatchIdentifiersWhenReading(bool match);

  private:
    QFileInfo m_inputFile;
    bool m_matchIdentifiersDuringRead;
    Dimension* m_identifierDimension;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedArgumentString : public AbstractArgument,
    public IdBasedComponentDataItem<QString>,
    public virtual HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:
    IdBasedArgumentString(const QString& id,
                           const QStringList& identifiers,
                           Dimension* identifierDimension,
                           ValueDefinition* valueDefinition,
                           AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedArgumentString();

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

    void saveData() override;

    QString toString() const override;

    bool readValues(const QString &value, QString &message, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem, QString &message) override;

    bool matchIdentifiersWhenReading() const;

    void setMatchIdentifiersWhenReading(bool match);

  private:
    QFileInfo m_inputFile;
    bool m_matchIdentifiersDuringRead;
    Dimension* m_identifierDimension;
};


//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedArgumentDateTime: public AbstractArgument,
    public IdBasedComponentDataItem<QDateTime>,
    public virtual HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:

    IdBasedArgumentDateTime(const QString& id,
                           const QStringList& identifiers,
                           Dimension* identifierDimension,
                           ValueDefinition* valueDefinition,
                           AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedArgumentDateTime();

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

    void saveData() override;

    QString toString() const override;

    bool readValues(const QString &value, QString &message, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem, QString &message) override;

    bool matchIdentifiersWhenReading() const;

    void setMatchIdentifiersWhenReading(bool match);

  private:
    QFileInfo m_inputFile;
    bool m_matchIdentifiersDuringRead;
    Dimension* m_identifierDimension;
};

Q_DECLARE_METATYPE(IdBasedArgumentInt*)
Q_DECLARE_METATYPE(IdBasedArgumentDouble*)
Q_DECLARE_METATYPE(IdBasedArgumentString*)
Q_DECLARE_METATYPE(IdBasedArgumentDateTime*)


#endif // IDBASEDARGUMENT_H

