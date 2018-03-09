/*!
 * \file octree.h
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

#ifndef OCTREE_H
#define OCTREE_H

#include "spatial/envelope.h"
#include <unordered_map>

class HCGeometry;

class HYDROCOUPLESDK_EXPORT Octree : public Envelope
{
  public:

    enum OctreeDimensions
    {
      Octree2D,
      Octree3D
    };

    enum OctreePartitionMode
    {
      AlongEnvelopes,
      Middle
    };

    Octree(OctreeDimensions dimensions, OctreePartitionMode partitionMode, int maxDepth, int maxMembersBeforeSplit);

    virtual ~Octree();

  private:
    Octree(double minx, double maxx,
           double miny, double maxy,
           OctreePartitionMode partitionMode,
           int depth, int maxMembersBeforeSplit, Octree *parent);

    Octree(double minx, double maxx,
           double miny, double maxy,
           double minz, double maxz,
           OctreePartitionMode partitionMode,
           int depth, int maxMembersBeforeSplit, Octree *parent);


  public:

    bool addGeometry(HydroCouple::Spatial::IGeometry *geometry);

    std::vector<HydroCouple::Spatial::IGeometry*> findCollidingGeometries(HydroCouple::Spatial::IGeometry *geometry);

    std::vector<HydroCouple::Spatial::IGeometry*> findCollidingGeometries(double x, double y, double z = 0.0);

    std::vector<HydroCouple::Spatial::IGeometry*> findCollidingGeometries(HydroCouple::Spatial::IEnvelope *envelope);

    void clearObjects();

    int deepestDepth() const;

    Octree *parent() const;

  private:

    bool addGeometry(HydroCouple::Spatial::IGeometry *geometry, HydroCouple::Spatial::IEnvelope *envelope);

    bool checkIfSplitConditionMet() const;

    void split();

    void distributeMembersAmongChildNodes();

    double getXPartition() const;

    double getYPartition() const;

    double getZPartition() const;

    void deleteChildren();

    void resetChildExtents();

  private:
    Octree *m_parent;
    Octree *m_childNodes[8];
    OctreeDimensions m_dimensions;
    OctreePartitionMode m_partitionMode;
    std::unordered_map<HydroCouple::Spatial::IGeometry*, IEnvelope*> m_members;
    int m_depth, m_maxMembersBeforePartion;
};

#endif // OCTREE_H
