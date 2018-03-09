/*!
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


#include "stdafx.h"
#include "spatial/envelope.h"


Envelope::Envelope()
{
  resetExtentsToInfinity();
}

Envelope::~Envelope()
{

}

double Envelope::minX() const
{
  return m_minX;
}

void Envelope::setMinX(double minX)
{
  m_minX = minX;
}

double Envelope::maxX() const
{
  return m_maxX;
}

void Envelope::setMaxX(double maxX)
{
  m_maxX = maxX;
}

double Envelope::minY() const
{
  return m_minY;
}

void Envelope::setMinY(double minY)
{
  m_minY = minY;
}

double Envelope::maxY() const
{
  return m_maxY;
}

void Envelope::setMaxY(double maxY)
{
  m_maxY = maxY;
}

double Envelope::minZ() const
{
  return m_minZ;
}

void Envelope::setMinZ(double minZ)
{
  m_minZ = minZ;
}

double Envelope::maxZ() const
{
  return m_maxZ;
}

void Envelope::setMaxZ(double maxZ)
{
  m_maxZ = maxZ;
}

void Envelope::resetExtentsToInfinity()
{
  m_minX = m_minY = m_minZ = std::numeric_limits<double>::max();
  m_maxX = m_maxY = m_maxZ = std::numeric_limits<double>::lowest();
}

void Envelope::merge(HydroCouple::Spatial::IEnvelope *envelope)
{
  m_minX = std::min(envelope->minX(), m_minX);
  m_minY = std::min(envelope->minY(), m_minY);
  m_minZ = std::min(envelope->minZ(), m_minZ);

  m_maxX = std::max(envelope->maxX(), m_maxX);
  m_maxY = std::max(envelope->maxY(), m_maxY);
  m_maxZ = std::max(envelope->maxZ(), m_maxZ);
}

bool Envelope::contains(HydroCouple::Spatial::IEnvelope *envelope, bool threeDimensional)
{
  if(threeDimensional)
  {
    return m_minX <= envelope->minX() && m_minY <= envelope->minY() &&
        m_maxX >= envelope->maxX() && m_maxY >= envelope->maxY() &&
        m_minZ <= envelope->minZ() && m_maxZ >= envelope->maxZ();
  }
  else
  {
    return m_minX <= envelope->minX() && m_minY <= envelope->minY() &&
        m_maxX >= envelope->maxX() && m_maxY >= envelope->maxY();
  }
}

bool Envelope::contains(HydroCouple::Spatial::IEnvelope *envelope1, HydroCouple::Spatial::IEnvelope *envelope2, bool threeDimensional)
{
  if(threeDimensional)
  {
    return envelope1->minX() <= envelope2->minX() && envelope1->minY() <= envelope2->minY() &&
        envelope1->maxX() >= envelope2->maxX() && envelope1->maxY() >= envelope2->maxY() &&
        envelope1->minZ() <= envelope2->minZ() && envelope1->maxZ() >= envelope2->maxZ();
  }
  else
  {
    return envelope1->minX() <= envelope2->minX() && envelope1->minY() <= envelope2->minY() &&
        envelope1->maxX() >= envelope2->maxX() && envelope1->maxY() >= envelope2->maxY();
  }
}


bool Envelope::contains(HydroCouple::Spatial::IPoint *point, bool threeDimensional)
{
  if(threeDimensional)
  {
    return m_minX <= point->x() && m_minY <= point->y() &&
           m_maxX >= point->x() && m_maxY >= point->y() &&
        m_minZ <= point->z() && m_maxZ >= point->z();
  }
  else
  {
    return m_minX <= point->x() && m_minY <= point->y() &&
        m_maxX >= point->x() && m_maxY >= point->y();
  }
}


bool Envelope::intersects(HydroCouple::Spatial::IEnvelope *envelope, bool threeDimensional)
{
  if(threeDimensional)
  {
    return m_minX <= envelope->maxX() && m_maxX >= envelope->minX() &&
           m_minY <= envelope->maxY() && m_maxY >= envelope->minY() &&
           m_minZ <= envelope->maxZ() && m_maxZ >= envelope->minZ();
  }
  else
  {
    return m_minX <= envelope->maxX() && m_maxX >= envelope->minX() &&
        m_minY <= envelope->maxY() && m_maxY >= envelope->minY();
  }

}

bool Envelope::intersects(HydroCouple::Spatial::IEnvelope *envelope1, HydroCouple::Spatial::IEnvelope *envelope2, bool threeDimensional)
{
  if(threeDimensional)
  {
    return envelope1->minX() <= envelope2->maxX() && envelope1->maxX() >= envelope2->minX() &&
        envelope1->minY() <= envelope2->maxY() && envelope1->maxY() >= envelope2->minY() &&
        envelope1->minZ() <= envelope2->maxZ() && envelope1->maxZ() >= envelope2->minZ();
  }
  else
  {
    return envelope1->minX() <= envelope2->maxX() && envelope1->maxX() >= envelope2->minX() &&
        envelope1->minY() <= envelope2->maxY() && envelope1->maxY() >= envelope2->minY();
  }

}
