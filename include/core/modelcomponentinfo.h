/*!
 *  \file    modelcomponentinfo.h
 *  \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version 1.0.0.0
 *  \section Description
 *  \section License
 *  modelcomponentinfo.h, associated files and libraries are free software;
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


#ifndef MODELCOMPONENTINFO_H
#define MODELCOMPONENTINFO_H

#include "hydrocouplesdk.h"
#include "componentinfo.h"

/*!
 * \brief The ModelComponentInfo class.
 */
class HYDROCOUPLESDK_EXPORT ModelComponentInfo : public ComponentInfo,
    public virtual HydroCouple::IModelComponentInfo
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IModelComponentInfo)

  public:

    ModelComponentInfo(QObject* parent = nullptr);

    virtual ~ModelComponentInfo(){}

};

Q_DECLARE_METATYPE(ModelComponentInfo*)

#endif // MODELCOMPONENTINFO_H
