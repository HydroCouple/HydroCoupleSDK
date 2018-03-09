/*!
 * \file abstractadaptedoutputfactory.h
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

#ifndef ABSTRACTADAPTEDOUTPUTFACTORY_H
#define ABSTRACTADAPTEDOUTPUTFACTORY_H

#include "identity.h"
#include "abstractadaptedoutputfactorycomponentinfo.h"

class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactory : public Identity,
    public virtual HydroCouple::IAdaptedOutputFactory
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IAdaptedOutputFactory)

  public:

    AbstractAdaptedOutputFactory(const QString& id, QObject* parent);

    virtual ~AbstractAdaptedOutputFactory();
};

class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactoryComponent : public AbstractAdaptedOutputFactory,
    public virtual HydroCouple::IAdaptedOutputFactoryComponent
{

    friend class AbstractAdaptedOutputFactoryComponentInfo;
    friend class TemporalInterpolationFactoryComponentInfo;

    Q_OBJECT
    Q_INTERFACES(HydroCouple::IAdaptedOutputFactoryComponent)
    Q_PROPERTY(HydroCouple::IAdaptedOutputFactoryComponentInfo* ComponentInfo READ componentInfo NOTIFY propertyChanged)

  public:

    AbstractAdaptedOutputFactoryComponent(const QString& id, QObject* parent);

    virtual ~AbstractAdaptedOutputFactoryComponent();

    HydroCouple::IAdaptedOutputFactoryComponentInfo* componentInfo() const override;

  signals:

    void propertyChanged(const QString &propertyName) override;

  protected:
    void setComponentInfo(AbstractAdaptedOutputFactoryComponentInfo* componentInfo);

  private:
    AbstractAdaptedOutputFactoryComponentInfo* m_componentInfo;

};


Q_DECLARE_METATYPE(AbstractAdaptedOutputFactory*)
Q_DECLARE_METATYPE(AbstractAdaptedOutputFactoryComponent*)

#endif // ABSTRACTADAPTEDOUTPUTFACTORY_H

