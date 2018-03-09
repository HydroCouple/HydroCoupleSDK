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
#include "spatial/octree.h"
#include "spatial/geometry.h"
#include "spatial/point.h"

using namespace HydroCouple::Spatial;

Octree::Octree(OctreeDimensions dimensions, OctreePartitionMode partitionMode, int maxDepth, int maxMembersBeforeSplit)
  :Envelope(),
    m_parent(nullptr),
    m_dimensions(dimensions),
    m_partitionMode(partitionMode),
    m_depth(maxDepth),
    m_maxMembersBeforePartion(maxMembersBeforeSplit)
{

  m_minX = m_minY =  m_minZ = std::numeric_limits<double>::lowest() / 3.0;
  m_maxX = m_maxY =  m_maxZ = std::numeric_limits<double>::max() / 3.0;

  m_childNodes[0] = m_childNodes[1] = m_childNodes[2] = m_childNodes[3] =
      m_childNodes[4] = m_childNodes[5] = m_childNodes[6] = m_childNodes[7] = nullptr;
}

Octree::~Octree()
{
  m_members.clear();
  deleteChildren();
}

Octree::Octree(double minx, double maxx,
               double miny, double maxy,
               OctreePartitionMode partitionMode,
               int depth, int maxMembersBeforeSplit, Octree *parent)
  :Envelope(),
    m_parent(parent),
    m_dimensions(OctreeDimensions::Octree2D),
    m_partitionMode(partitionMode),
    m_depth(depth),
    m_maxMembersBeforePartion(maxMembersBeforeSplit)
{
  m_minX = minx;
  m_maxX = maxx;
  m_minY = miny;
  m_maxY = maxy;

  m_childNodes[0] = m_childNodes[1] = m_childNodes[2] = m_childNodes[3] =
      m_childNodes[4] = m_childNodes[5] = m_childNodes[6] = m_childNodes[7] = nullptr;
}

Octree::Octree(double minx, double maxx,
               double miny, double maxy,
               double minz, double maxz,
               OctreePartitionMode partitionMode,
               int depth, int maxMembersBeforeSplit, Octree *parent)
  :Envelope(),
    m_parent(parent),
    m_dimensions(OctreeDimensions::Octree3D),
    m_partitionMode(partitionMode),
    m_depth(depth),
    m_maxMembersBeforePartion(maxMembersBeforeSplit)
{
  m_minX = minx;
  m_maxX = maxx;
  m_minY = miny;
  m_maxY = maxy;
  m_minZ = minz;
  m_maxZ = maxz;
  m_childNodes[0] = m_childNodes[1] = m_childNodes[2] = m_childNodes[3] =
      m_childNodes[4] = m_childNodes[5] = m_childNodes[6] = m_childNodes[7] = nullptr;
}

bool Octree::addGeometry(IGeometry *geometry)
{
  if(contains(geometry->envelope() , m_dimensions == OctreeDimensions::Octree3D))
  {
    if(m_childNodes[0] != nullptr)
    {
      switch(m_dimensions)
      {
        case OctreeDimensions::Octree3D:
          {
            for(int i = 0 ; i < 8; i++)
            {
              if(m_childNodes[i]->addGeometry(geometry))
              {
                return true;
              }
            }
          }
          break;
        case OctreeDimensions::Octree2D:
          {
            for(int i = 0 ; i < 4; i++)
            {
              if(m_childNodes[i]->addGeometry(geometry))
              {
                return true;
              }
            }
          }
          break;
      }
    }

    m_members[geometry] = geometry->envelope();

    split();

    return true;
  }

  return false;
}

std::vector<IGeometry*> Octree::findCollidingGeometries(HydroCouple::Spatial::IGeometry *geometry)
{
  std::vector<IGeometry*> eligible;

  IEnvelope *envelope = geometry->envelope();

  if(contains(envelope, m_dimensions == OctreeDimensions::Octree3D))
  {
    int count = m_dimensions == OctreeDimensions::Octree2D ? 4 : 8;

    if(m_childNodes[0] != nullptr)
    {
      for(int i = 0 ; i < count ; i++)
      {
        std::vector<IGeometry*> colliding = m_childNodes[i]->findCollidingGeometries(envelope);

        for(IGeometry *geom : colliding)
          eligible.push_back(geom);
      }
    }

    for(const std::pair<IGeometry*, IEnvelope*> &geom : m_members)
    {

      if(Envelope::contains(geom.second, envelope, m_dimensions == OctreeDimensions::Octree3D) ||
         Envelope::intersects(geom.second, envelope, m_dimensions == OctreeDimensions::Octree3D))
      {
        eligible.push_back(geom.first);
      }
    }
  }

  return eligible;
}

std::vector<IGeometry*> Octree::findCollidingGeometries(double x, double y, double z)
{
  std::vector<IGeometry*> eligible;

  HCPoint point(x,y,z);
  IEnvelope *envelope = point.envelope();

  if(contains(envelope , m_dimensions == OctreeDimensions::Octree3D))
  {
    int count = m_dimensions == OctreeDimensions::Octree2D ? 4 : 8;

    if(m_childNodes[0] != nullptr)
    {
      for(int i = 0 ; i < count ; i++)
      {
        std::vector<IGeometry*> colliding = m_childNodes[i]->findCollidingGeometries(envelope);

        for(IGeometry *geom : colliding)
          eligible.push_back(geom);
      }
    }

    for(const std::pair<IGeometry*, IEnvelope*> &geom : m_members)
    {
      if(Envelope::contains(geom.second, envelope, m_dimensions == OctreeDimensions::Octree3D) ||
         Envelope::intersects(geom.second, envelope, m_dimensions == OctreeDimensions::Octree3D))
      {
        eligible.push_back(geom.first);
      }
    }
  }

  return eligible;
}

std::vector<HydroCouple::Spatial::IGeometry*> Octree::findCollidingGeometries(HydroCouple::Spatial::IEnvelope *envelope)
{
  std::vector<IGeometry*> eligible;

  if(contains(envelope , m_dimensions == OctreeDimensions::Octree3D))
  {
    int count = m_dimensions == OctreeDimensions::Octree2D ? 4 : 8;

    if(m_childNodes[0] != nullptr)
    {
      for(int i = 0 ; i < count ; i++)
      {
        std::vector<IGeometry*> colliding = m_childNodes[i]->findCollidingGeometries(envelope);

        for(IGeometry *geom : colliding)
          eligible.push_back(geom);
      }
    }

    for(const std::pair<IGeometry*, IEnvelope*> &geom : m_members)
    {
      if(Envelope::contains(geom.second, envelope, m_dimensions == OctreeDimensions::Octree3D) ||
         Envelope::intersects(geom.second, envelope, m_dimensions == OctreeDimensions::Octree3D))
      {
        eligible.push_back(geom.first);
      }
    }
  }

  return eligible;
}

void Octree::clearObjects()
{
  m_members.clear();
  deleteChildren();
}

int Octree::deepestDepth() const
{
  int deepest  = m_depth;

  if(m_childNodes[0] != nullptr)
  {
    int iter = m_dimensions == Octree3D ? 8 : 4;

    for(int i = 0 ; i < iter; i++)
    {
      int cDepth = m_childNodes[i]->deepestDepth();

      if(cDepth < deepest)
      {
        deepest = cDepth;
      }
    }
  }

  return deepest;
}

Octree *Octree::parent() const
{
  return m_parent;
}

bool Octree::addGeometry(IGeometry *geometry, HydroCouple::Spatial::IEnvelope *envelope)
{
  if(contains(geometry->envelope() , m_dimensions == OctreeDimensions::Octree3D))
  {
    if(m_childNodes[0] != nullptr)
    {
      switch(m_dimensions)
      {
        case OctreeDimensions::Octree3D:
          {
            for(int i = 0 ; i < 8; i++)
            {
              if(m_childNodes[i]->addGeometry(geometry, envelope))
              {
                return true;
              }
            }
          }
          break;
        case OctreeDimensions::Octree2D:
          {
            for(int i = 0 ; i < 4; i++)
            {
              if(m_childNodes[i]->addGeometry(geometry, envelope))
              {
                return true;
              }
            }
          }
          break;
      }
    }

    m_members[geometry] = envelope;

    split();

    return true;
  }

  return false;
}

bool Octree::checkIfSplitConditionMet() const
{
  return m_depth > 1 && m_members.size() == (size_t)m_maxMembersBeforePartion;
}

void Octree::split()
{
  if(m_childNodes[0] == nullptr && checkIfSplitConditionMet())
  {
    double midX = getXPartition();
    double midY = getYPartition();

    switch(m_dimensions)
    {
      case OctreeDimensions::Octree3D:
        {
          double midZ = getZPartition();

          for(int i = 0 ; i < 8 ; i++)
          {
            switch (i)
            {
              case 0:
                {
                  m_childNodes[i] = new Octree(m_minX,midX, m_minY, midY, m_minZ, midZ,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
              case 1:
                {
                  m_childNodes[i] = new Octree(midX,m_maxX, m_minY, midY, m_minZ, midZ,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
              case 2:
                {
                  m_childNodes[i] = new Octree(midX,m_maxX, midY, m_maxY, m_minZ, midZ,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
              case 3:
                {
                  m_childNodes[i] = new Octree(m_minX,midX, midY, m_maxY, m_minZ, midZ,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
              case 4:
                {
                  m_childNodes[i] = new Octree(m_minX,midX, m_minY, midY, midZ, m_maxZ,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
              case 5:
                {
                  m_childNodes[i] = new Octree(midX,m_maxX, m_minY, midY, midZ, m_maxZ,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
              case 6:
                {
                  m_childNodes[i] = new Octree(midX,m_maxX, midY, m_maxY, midZ, m_maxZ,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
              case 7:
                {
                  m_childNodes[i] = new Octree(m_minX,midX, midY, m_maxY, midZ, m_maxZ,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
            }
          }
        }
        break;
      case OctreeDimensions::Octree2D:
        {
          for(int i = 0 ; i < 4 ; i++)
          {
            switch (i)
            {
              case 0:
                {
                  m_childNodes[i] = new Octree(m_minX,midX, m_minY, midY,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
              case 1:
                {
                  m_childNodes[i] = new Octree(midX,m_maxX, m_minY, midY,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
              case 2:
                {
                  m_childNodes[i] = new Octree(midX,m_maxX, midY, m_maxY,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
              case 3:
                {
                  m_childNodes[i] = new Octree(m_minX,midX, midY, m_maxY,
                                               m_partitionMode,m_depth-1,m_maxMembersBeforePartion,this);
                }
                break;
            }
          }
        }
        break;
    }

    distributeMembersAmongChildNodes();
  }
}

void Octree::distributeMembersAmongChildNodes()
{
  if(m_childNodes[0] != nullptr)
  {
    int numNodes = m_dimensions == OctreeDimensions::Octree2D ? 4 : 8;

    std::list<IGeometry*> addedGeometries;

    for(int j = 0 ; j < numNodes ; j++)
    {
      for(const std::pair<IGeometry*, IEnvelope*> &geom : m_members)
      {
        if(m_childNodes[j]->addGeometry(geom.first))
        {
          addedGeometries.push_back(geom.first);
        }
      }
    }

    for(IGeometry* geom : addedGeometries)
      m_members.erase(geom);

    addedGeometries.clear();
  }
}

double Octree::getXPartition() const
{

  if(m_partitionMode == OctreePartitionMode::AlongEnvelopes && m_members.size())
  {
    std::vector<double> values;

    for(const std::pair<IGeometry*, IEnvelope*> &geom : m_members)
    {
      values.push_back(geom.second->minX());
      values.push_back(geom.second->maxX());
    }

    int median = floor(values.size() * 0.5);
    return values[median];
  }

  return (m_minX + m_maxX) /2.0;
}

double Octree::getYPartition() const
{

  if(m_partitionMode == OctreePartitionMode::AlongEnvelopes && m_members.size())
  {
    std::vector<double> values;

    for(const std::pair<IGeometry*, IEnvelope*> &geom : m_members)
    {
      values.push_back(geom.second->minY());
      values.push_back(geom.second->maxY());
    }

    int median = floor(values.size() * 0.5);
    return values[median];
  }

  return (m_minY + m_maxY) /2.0;
}

double Octree::getZPartition() const
{
  if(m_partitionMode == OctreePartitionMode::AlongEnvelopes && m_members.size())
  {
    std::vector<double> values;

    for(const std::pair<IGeometry*, IEnvelope*> &geom : m_members)
    {
      values.push_back(geom.second->minZ());
      values.push_back(geom.second->maxZ());
    }

    int median = floor(values.size() * 0.5);
    return values[median];
  }

  return (m_minZ + m_maxZ) /2.0;
}

void Octree::deleteChildren()
{
  if(m_childNodes[0] != nullptr)
  {
    switch(m_dimensions)
    {
      case OctreeDimensions::Octree3D:
        {
          for(int i = 0 ; i < 8; i++)
          {
            delete m_childNodes[i];
            m_childNodes[i] = nullptr;
          }
        }
        break;
      case OctreeDimensions::Octree2D:
        {
          for(int i = 0 ; i < 8; i++)
          {
            if(i < 4)
              delete m_childNodes[i];

            m_childNodes[i] = nullptr;
          }
        }
        break;
    }
  }
}

void Octree::resetChildExtents()
{
  if(m_childNodes[0] != nullptr)
  {
    switch(m_dimensions)
    {
      case OctreeDimensions::Octree3D:
        {
          for(int i = 0 ; i < 8 ; i++)
          {
            switch (i)
            {
              case 0:
                {
                  m_childNodes[i]->m_minX = m_minX;
                  m_childNodes[i]->m_minY = m_minY;
                  m_childNodes[i]->m_minZ = m_minZ;
                  m_childNodes[i]->resetChildExtents();
                }
                break;
              case 1:
                {

                  m_childNodes[i]->m_maxX = m_maxX;
                  m_childNodes[i]->m_minY = m_minY;
                  m_childNodes[i]->m_minZ = m_minZ;
                  m_childNodes[i]->resetChildExtents();

                }
                break;
              case 2:
                {
                  m_childNodes[i]->m_maxX = m_maxX;
                  m_childNodes[i]->m_maxY = m_maxY;
                  m_childNodes[i]->m_minZ = m_minZ;
                  m_childNodes[i]->resetChildExtents();

                }
                break;
              case 3:
                {
                  m_childNodes[i]->m_minX = m_minX;
                  m_childNodes[i]->m_maxY = m_maxY;
                  m_childNodes[i]->m_minZ = m_minZ;
                  m_childNodes[i]->resetChildExtents();
                }
                break;
              case 4:
                {
                  m_childNodes[i]->m_minX = m_minX;
                  m_childNodes[i]->m_minY = m_minY;
                  m_childNodes[i]->m_maxZ = m_maxZ;
                  m_childNodes[i]->resetChildExtents();

                }
                break;
              case 5:
                {

                  m_childNodes[i]->m_maxX = m_maxX;
                  m_childNodes[i]->m_minY = m_minY;
                  m_childNodes[i]->m_maxZ = m_maxZ;
                  m_childNodes[i]->resetChildExtents();

                }
                break;
              case 6:
                {

                  m_childNodes[i]->m_maxX = m_maxX;
                  m_childNodes[i]->m_maxY = m_maxY;
                  m_childNodes[i]->m_maxZ = m_maxZ;
                  m_childNodes[i]->resetChildExtents();
                }
                break;
              case 7:
                {
                  m_childNodes[i]->m_minX = m_minX;
                  m_childNodes[i]->m_maxY = m_maxY;
                  m_childNodes[i]->m_maxZ = m_maxZ;
                  m_childNodes[i]->resetChildExtents();
                }
                break;
            }
          }
        }
        break;
      case OctreeDimensions::Octree2D:
        {
          for(int i = 0 ; i < 4 ; i++)
          {
            switch (i)
            {
              case 0:
                {
                  m_childNodes[i]->m_minX = m_minX;
                  m_childNodes[i]->m_minY = m_minY;
                  m_childNodes[i]->resetChildExtents();
                }
                break;
              case 1:
                {
                  m_childNodes[i]->m_maxX = m_maxX;
                  m_childNodes[i]->m_minY = m_minY;
                  m_childNodes[i]->resetChildExtents();

                }
                break;
              case 2:
                {
                  m_childNodes[i]->m_maxX = m_maxX;
                  m_childNodes[i]->m_maxY = m_maxY;
                  m_childNodes[i]->resetChildExtents();

                }
                break;
              case 3:
                {
                  m_childNodes[i]->m_minX = m_minX;
                  m_childNodes[i]->m_maxY = m_maxY;
                  m_childNodes[i]->resetChildExtents();
                }
                break;
            }
          }
        }
        break;
    }
  }
}
