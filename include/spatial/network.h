/*!
 * \file network.h
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

#ifndef NETWORK_H
#define NETWORK_H

#include <QUuid>

#include "core/identity.h"
#include "hydrocouplespatial.h"

class HCVertex;
class Edge;

class HYDROCOUPLESDK_EXPORT Network : public Identity,
    public virtual HydroCouple::Spatial::INetwork
{

    friend class HCVertex;

    Q_INTERFACES(HydroCouple::Spatial::INetwork)

  public:

    Network(const QString &id = QUuid::createUuid().toString(), QObject* parent = nullptr);

    virtual ~Network();

    int edgeCount() const override;

    HydroCouple::Spatial::IEdge *edge(int index) const override;

    int vertexCount() const override;

    HydroCouple::Spatial::IVertex *vertex(int index) const override;

    Edge *createVertexEdge(HCVertex *origin, HCVertex *destination);

    void deleteVertexEdge(Edge *edge);

private:

    void addVertex(HCVertex *vertex);

    bool removeVertex(HCVertex *vertex);

    void addEdge(Edge *edge);

    bool removeEdge(Edge *edge);

    Edge *findEdge(HCVertex *origin, HCVertex *destination) const;

    Edge* getClosestOrbitLeft(HCVertex *origin, HCVertex *destination);

  private:

    std::vector<HCVertex*> m_vertices;
    std::vector<Edge*> m_edges;
};

#endif // NETWORK_H
