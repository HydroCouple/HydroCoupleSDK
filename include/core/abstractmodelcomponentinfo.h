/*!
 * \file  abstractmodelcomponentinfo.h
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


#ifndef MODELCOMPONENTINFO_H
#define MODELCOMPONENTINFO_H

#include "hydrocouplesdk.h"
#include "componentinfo.h"

class AbstractAdaptedOutputFactory;

/*!
 * \brief The ModelComponentInfo class.
 */
class HYDROCOUPLESDK_EXPORT AbstractModelComponentInfo : public ComponentInfo,
    public virtual HydroCouple::IModelComponentInfo
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IModelComponentInfo)

    Q_PROPERTY(QList<HydroCouple::IAdaptedOutputFactory*> AdaptedOutputFactories READ adaptedOutputFactories NOTIFY propertyChanged)

  public:

    AbstractModelComponentInfo(QObject* parent = nullptr);

    virtual ~AbstractModelComponentInfo() override;

    QList<HydroCouple::IAdaptedOutputFactory*> adaptedOutputFactories() const override final;

  signals:

    void propertyChanged(const QString &propertyName) override;

  protected:

    QHash<QString,AbstractAdaptedOutputFactory*> adaptedOutputFactoriesInternal() const;

    void addAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory);

    bool removeAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory);

    void clearAdaptedOutputFactories();

  private:

    QHash<QString, AbstractAdaptedOutputFactory*> m_adaptedOutputFactories;
};

Q_DECLARE_METATYPE(AbstractModelComponentInfo*)

#endif // MODELCOMPONENTINFO_H
