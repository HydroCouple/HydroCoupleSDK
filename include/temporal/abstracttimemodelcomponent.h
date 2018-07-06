/*!
 * \file abstracttimemodelcomponent.h
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

#ifndef ABSTRACTTIMEMODELCOMPONENT_H
#define ABSTRACTTIMEMODELCOMPONENT_H

#include "hydrocoupletemporal.h"
#include "core/abstractmodelcomponent.h"

namespace SDKTemporal
{
  class DateTime;
  class TimeSpan;
}

class HYDROCOUPLESDK_EXPORT AbstractTimeModelComponent : public AbstractModelComponent,
    public virtual HydroCouple::Temporal::ITimeModelComponent
{

    Q_OBJECT

    Q_INTERFACES(HydroCouple::Temporal::ITimeModelComponent)
    Q_PROPERTY(HydroCouple::Temporal::IDateTime* CurrentDateTime READ currentDateTime)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeHorizon READ timeHorizon)

  public:

    AbstractTimeModelComponent(const QString &id, AbstractModelComponentInfo *modelComponentInfo = nullptr);

    AbstractTimeModelComponent(const QString &id, const QString &caption, AbstractModelComponentInfo *modelComponentInfo = nullptr);

    virtual ~AbstractTimeModelComponent();

    HydroCouple::Temporal::IDateTime *currentDateTime() const override;

    HydroCouple::Temporal::ITimeSpan *timeHorizon() const override;

  protected:

    SDKTemporal::DateTime *currentDateTimeInternal() const;

    SDKTemporal::TimeSpan *timeHorizonInternal() const;

    double getMinimumConsumerTime() const;

    double getMinimumConsumerTime(HydroCouple::IAdaptedOutput *adpatedOutput) const;

  private:

    SDKTemporal::DateTime *m_dateTime;
    SDKTemporal::TimeSpan *m_timeHorizon;
};

Q_DECLARE_METATYPE(AbstractTimeModelComponent*)

#endif // ABSTRACTTIMEMODELCOMPONENT_H
