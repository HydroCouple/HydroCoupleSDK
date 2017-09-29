/*!
 *  \file    spatialreferencesystem.h
 *  \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version 1.0.0.0
 *  \section Description
 *  \section License
 *  spatialreferencesystem.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  abstractadaptedoutput.h its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2016
 *  \pre
 *  \bug
 *  \todo
 *  \warning
 */

#ifndef SPATIALREFERENCESYSTEM_H
#define SPATIALREFERENCESYSTEM_H

#include "core/description.h"
#include "hydrocouplespatial.h"

#include <unordered_map>
#include <string>

class OGRSpatialReference;

class HYDROCOUPLESDK_EXPORT SpatialReferenceSystem :
    public virtual HydroCouple::Spatial::ISpatialReferenceSystem
{

    Q_INTERFACES(HydroCouple::Spatial::ISpatialReferenceSystem)

  public:

    SpatialReferenceSystem();

    SpatialReferenceSystem(const QString& srText);

    SpatialReferenceSystem(const QString& authName, int authSRID);

    virtual ~SpatialReferenceSystem();

    int authSRID() const override;

    QString authName() const override;

    void setAuth(const QString& name , int authSRID);

    QString srText() const override;

    void setSrText(const QString& srtext);

    static std::string findEPSGWkt(int authSRID, bool& found);

  private:
    QString m_wkt , m_authName;
    int m_authSRID;
    static const std::unordered_map<int,std::string> m_EPGSAuthorityWkts;
};



#endif // SPATIALREFERENCESYSTEM_H

