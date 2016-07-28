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

#ifndef ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H
#define ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H

#include "componentinfo.h"

class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactoryComponentInfo : public ComponentInfo,
    public virtual HydroCouple::IAdaptedOutputFactoryComponentInfo
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IAdaptedOutputFactoryComponentInfo)

  public:
    AbstractAdaptedOutputFactoryComponentInfo(QObject* parent);

    virtual ~AbstractAdaptedOutputFactoryComponentInfo();

    bool validateLicense(QString &validationMessage) const override;

    bool hasValidLicense() const override;


};

Q_DECLARE_METATYPE(AbstractAdaptedOutputFactoryComponentInfo*)

#endif // ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H

