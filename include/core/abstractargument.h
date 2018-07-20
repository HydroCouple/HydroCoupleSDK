/*!
 * \file  AbstractAdaptedOutputFactoryComponentInfo.h
 * \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
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

#ifndef ABSTRACTARGUMENT_H
#define ABSTRACTARGUMENT_H

#include "abstractcomponentdataitem.h"

#include <QXmlStreamReader>

class Dimension;
class AbstractComponentDataItem;

class HYDROCOUPLESDK_EXPORT AbstractArgument : public AbstractComponentDataItem,
    public virtual HydroCouple::IArgument
{
    Q_OBJECT

    Q_INTERFACES(HydroCouple::IArgument)

    Q_PROPERTY(bool Optional READ isOptional NOTIFY propertyChanged)
    Q_PROPERTY(bool ReadOnly READ isReadOnly NOTIFY propertyChanged)
    Q_PROPERTY(QStringList FileFilters READ fileFilters NOTIFY propertyChanged)
    Q_PROPERTY(bool CanReadFromFile READ canReadFromFile NOTIFY propertyChanged)
    Q_PROPERTY(bool CanReadFromString READ canReadFromString NOTIFY propertyChanged)

  public:

    AbstractArgument(const QString& id,
                     const QList<Dimension*> &dimensions,
                     ValueDefinition *valueDefinition,
                     AbstractModelComponent *modelComponent);

    AbstractArgument(const QString &id,
                     const QString &caption,
                     const QList<Dimension*> &dimensions,
                     ValueDefinition *valueDefinition,
                     AbstractModelComponent *modelComponent);

    virtual ~AbstractArgument();

    bool isOptional() const override;

    bool isReadOnly() const override;

    QStringList fileFilters() const override;

    bool canReadFromFile() const override;

    bool canReadFromString() const override;

    HydroCouple::IArgument::ArgumentIOType currentArgumentIOType() const override;

    void setIsOptional(bool isOptional);

    void setIsReadOnly(bool isReadOnly);

    void addFileFilter(const QString &extension);

    virtual AbstractArgument *clone() const;

  protected:

    void setCanReadFromFile(bool canReadFromFile);

    void setCanReadFromString(bool canReadFromString);

    void setArgumentIOType(HydroCouple::IArgument::ArgumentIOType argumentIOType);

  signals:

    void propertyChanged(const QString &propertyName) override;

  private:

    QStringList m_fileFilters;
    bool m_isOptional , m_isReadOnly, m_canReadFromFile, m_canReadFromString;
    HydroCouple::IArgument::ArgumentIOType m_argumentIOType;
};

Q_DECLARE_METATYPE(AbstractArgument*)
Q_DECLARE_METATYPE(QList<AbstractArgument*>)

#endif // ABSTRACTARGUMENT_H

