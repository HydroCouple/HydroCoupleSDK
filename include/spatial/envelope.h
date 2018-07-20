/*!
 * \file envelope.h
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

#ifndef ENVELOPE_H
#define ENVELOPE_H

#include "hydrocouplespatial.h"
#include "hydrocouplesdk.h"

class HYDROCOUPLESDK_EXPORT Envelope :
    public virtual HydroCouple::Spatial::IEnvelope
{
   Q_INTERFACES(HydroCouple::Spatial::IEnvelope)

  public:

    Envelope();

    virtual ~Envelope();

    double minX() const override;

    void setMinX(double minX);

    double maxX() const override;

    void setMaxX(double maxX);

    double minY() const override;

    void setMinY(double minY);

    double maxY() const override;

    void setMaxY(double maxY);

    double minZ() const override;

    void setMinZ(double minZ);

    double maxZ() const override;

    void setMaxZ(double maxZ);

    void resetExtentsToInfinity();

    void merge(HydroCouple::Spatial::IEnvelope *envelope);

    bool contains(HydroCouple::Spatial::IEnvelope *envelope, bool threeDimensional = false);

    static bool contains(HydroCouple::Spatial::IEnvelope *envelope1, HydroCouple::Spatial::IEnvelope *envelope2, bool threeDimensional = false);

    bool contains(HydroCouple::Spatial::IPoint *point, bool threeDimensional = false);

    bool intersects(HydroCouple::Spatial::IEnvelope *envelope, bool threeDimensional = false);

    static bool intersects(HydroCouple::Spatial::IEnvelope *envelope1, HydroCouple::Spatial::IEnvelope *envelope2, bool threeDimensional = false);

  protected:
    double m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ;
};


#endif // ENVELOPE_H
