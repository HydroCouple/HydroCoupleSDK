#include "stdafx.h"
#include "spatial/network.h"
#include "spatial/edge.h"
#include "spatial/point.h"

using namespace  HydroCouple::Spatial;


Network::Network(const QString &id, QObject *parent)
  :Identity(id, parent)
{

}

Network::~Network()
{
  while (m_edges.size())
  {
    Edge *edge = m_edges[0];
    deleteVertexEdge(edge);
  }

  while (m_vertices.size())
  {
    HCVertex *vertex = m_vertices[0];
    delete vertex;
  }
}

int Network::edgeCount() const
{
  return m_edges.size();
}

IEdge *Network::edge(int index) const
{
  int i = 0;

  for(Edge *edge : m_edges)
  {
    if(i == index)
    {
      return edge;
    }

    i++;
  }

  return nullptr;
}

int Network::vertexCount() const
{
  return m_vertices.size();
}

IVertex *Network::vertex(int index) const
{
  int i = 0;

  for(HCVertex *vertex : m_vertices)
  {
    if(i == index)
      return vertex;

    i++;
  }

  return nullptr;

}

Edge *Network::createVertexEdge(HCVertex *origin, HCVertex *destination)
{
  Edge *newEdge = nullptr;

  if((newEdge = findEdge(origin,destination)) != nullptr)
  {
  }
  else
  {
    newEdge = Edge::createEdge();

    if(origin->edgeInternal())
    {
      Edge* originSpliceEge = getClosestOrbitLeft(origin, destination);
      Edge::splice(originSpliceEge,newEdge);
    }

    //    Edge *destinationSpliceEdge = nullptr;

    //    if(destination->edgeInternal())
    //    {
    //      //check for closure
    //      destinationSpliceEdge = getOrbitLeft(destination->edgeInternal(),face);

    //      if(destinationSpliceEdge == nullptr)
    //      {
    //        //check for unclosed polygon
    //        destinationSpliceEdge = getClosestOrbitLeftNull(destination, origin);
    //      }

    //      Edge::splice(destinationSpliceEdge,newEdge->symInternal());
    //    }

    newEdge->setOrig(origin);
    newEdge->setDest(destination);
    addEdge(newEdge);
  }

  return newEdge;
}

void Network::deleteVertexEdge(Edge *edge)
{
  //assert(edge != nullptr);

  // locate _edge1_ and _edge2_ as in _makeVertexEdge_
  Edge *edge1 = edge->origPrevInternal();
  Edge *edge2 = edge->leftNextInternal();

  // use _edge1_ for _edge2_ if the destination vertex is isolated
  if (edge2 == edge->sym())
    edge2 = edge1;

  // inverse of _makeVertexEdge_
  Edge::splice(edge1, edge->symInternal());
  Edge::splice(edge2, edge);

  //  // all edges leaving the destination orbit of the deleted edge now have its
  //  // origin vertex as their vertex of origin
  //  setOrbitOrg(edge2, edge1->origInternal());

  //  // don't use the deleted edge as a reference edge any more
  //  edge1->origInternal()->addEdge(edge1);
  //  edge1->leftInternal()->addEdge(edge1);
  //  edge2->leftInternal()->addEdge(edge2);

  //  // reclaim the vertex and the edge
  //  HCVertex *dest = edge->destInternal();
  //  dest->addEdge(nullptr);
  //  delete dest;

  edge->setOrig(nullptr);
  edge->setDest(nullptr);

  removeEdge(edge);
  Edge::deleteEdge(edge);
}

void Network::addVertex(HCVertex *vertex)
{
  m_vertices.push_back(vertex);
}

bool Network::removeVertex(HCVertex *vertex)
{
  std::vector<HCVertex*>::iterator it = std::find(m_vertices.begin(), m_vertices.end(), vertex);

  if(it != m_vertices.end())
  {
    m_vertices.erase(it);
    return true;
  }

  return false;
}

void Network::addEdge(Edge *edge)
{
  m_edges.push_back(edge);
}

bool Network::removeEdge(Edge *edge)
{
  std::vector<Edge*>::iterator it = std::find(m_edges.begin(), m_edges.end(), edge);

  if(it != m_edges.end())
  {
    m_edges.erase(it);
    return true;
  }

  return false;
}

Edge *Network::findEdge(HCVertex *origin, HCVertex *destination) const
{
  Edge *currentEdge = origin->edgeInternal();
  Edge *oEdge = currentEdge;

  if(currentEdge)
  {
    do
    {
      if(currentEdge->dest() == destination)
      {
        return currentEdge;
      }

      currentEdge = currentEdge->origNextInternal();

    }while(currentEdge != oEdge);
  }

  return nullptr;
}

Edge* Network::getClosestOrbitLeft(HCVertex *origin, HCVertex *destination)
{

  Edge *closestEdge = nullptr;

  double o_x = origin->x();
  double o_y = origin->y();

  double x1 = destination->x() - o_x;
  double y1 = destination->y() - o_y;

  Edge *scan = origin->edgeInternal();
  Edge *oEdge = scan;

  //start with large number
  double small = 100000.0;

  do
  {

    double x2 = scan->dest()->x() - o_x;
    double y2 = scan->dest()->y() - o_y;

    double angle2 =  atan2(x2*y1-y2*x1,x2*x1+y2*y1);

    if(angle2 < 0 )
      angle2 = angle2 + 2* M_PI;

    if(angle2 < small)
    {
      small = angle2;
      closestEdge = scan;
    }

    scan = scan->origNextInternal();
  }
  while (scan != oEdge);

  return closestEdge;
}

