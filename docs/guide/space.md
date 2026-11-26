@page sdk_guide_space 10. Space

[TOC]

Most coupled models exchange values that belong somewhere: a stage at a gauge, a flow on a river
reach, a depth on each cell of a mesh, a temperature in each layer of a lake. The HydroCouple
standard describes *where* with the interfaces of its `HydroCouple::Spatial` namespace (geometries,
networks, meshes, rasters, regular grids, vertical coordinates) and attaches values to them through
spatial data items, whose `shape()` puts the entity axis in a fixed position. This chapter
describes what the HydroCouple SDK provides for that: value types a component uses internally,
adapters that serve those values through the standard's interfaces, the spatial and spatiotemporal
data items, layered meshes, the mesh-valued argument, and how coordinate reference systems are
carried and checked.

The interfaces themselves are specified in the HydroCouple interface manual
(https://www.hydrocouple.org/HydroCouple/). Value storage, shapes and the hyperslab calls
`getValuesInto()`/`setValuesFrom()` are covered in @ref sdk_guide_ports, time series in
@ref sdk_guide_time, and reading and writing meshes in files in @ref sdk_guide_mesh_packages and
@ref sdk_guide_record.

## Values, adapters and data items

The SDK separates three things that the standard's interfaces combine.

- A *value type* holds geometry the way a model uses it: plain coordinates in contiguous arrays,
  copied and edited freely, with no virtual calls. @ref HydroCouple::SDK::Spatial::Point,
  @ref HydroCouple::SDK::Spatial::LineString, @ref HydroCouple::SDK::Spatial::Polygon,
  @ref HydroCouple::SDK::Spatial::PolyhedralSurface, @ref HydroCouple::SDK::Spatial::TIN,
  @ref HydroCouple::SDK::Spatial::Network, @ref HydroCouple::SDK::Spatial::RegularGrid2D,
  @ref HydroCouple::SDK::Spatial::RegularGrid3D and @ref HydroCouple::SDK::IO::MeshDefinition are
  value types. None of them implements a HydroCouple interface.
- An *adapter* serves a value through an interface. It copies the value when it is constructed,
  builds the interface objects the standard hands out by pointer (a line's points, a polygon's
  rings, a network's vertices and edges), and owns them, so the pointers it returns stay valid for
  the adapter's life.
- A *spatial data item* is an exchange item, argument or internal item whose `shape()` is indexed
  by spatial entities. It borrows the geometry (an adapter, or the component's own implementation
  of the interface) and owns the values and the dimensions.

<pre class="mermaid">
flowchart LR
    subgraph component["Inside the component"]
        V["Value types<br/>Point, LineString, Polygon,<br/>Network, MeshDefinition,<br/>RegularGrid2D"]
    end
    subgraph published["What crosses the boundary"]
        A["Adapters<br/>PointAdapter, PolygonAdapter,<br/>NetworkAdapter,<br/>PolyhedralSurfaceAdapter"]
        D["Spatial data items<br/>TimeGeometry..., TimeNetwork...,<br/>LayeredMeshOutput,<br/>PolyhedralSurfaceArgument"]
    end
    C["Consumer<br/>(another component, a writer,<br/>a viewer, the validator)"]
    V -- "copied at construction" --> A
    A -- "borrowed by" --> D
    D -- "IComponentDataItem<br/>+ spatial interface" --> C
</pre>

Use the value types for the model's own geometry and for anything that changes as it runs. Build an
adapter when an object must be handed to a consumer through an interface pointer: as the geometries
of an `IGeometryComponentDataItem`, the `network()` of a network item, or the `polyhedralSurface()`
of a mesh item. An adapter does not follow later changes to the value it was built from; to publish
changed geometry, build a new adapter. Keep each adapter alive for as long as any data item that
borrows it, which in practice means making it a member of the component next to the item.

### The interfaces and the SDK classes that serve them

| HydroCouple interface | SDK class | Notes |
| --- | --- | --- |
| `HydroCouple::Spatial::IEnvelope` | @ref HydroCouple::SDK::Spatial::EnvelopeAdapter | Over an @ref HydroCouple::SDK::Spatial::Envelope value. |
| `HydroCouple::Spatial::IGeometry` | @ref HydroCouple::SDK::Spatial::GeometryAdapter | Base of the geometry adapters: identity, reference system, envelope, WKT. |
| `HydroCouple::Spatial::IPoint` | @ref HydroCouple::SDK::Spatial::PointAdapter | Over a @ref HydroCouple::SDK::Spatial::Point. |
| `HydroCouple::Spatial::IVertex` | @ref HydroCouple::SDK::Spatial::VertexAdapter | A point that can name one edge leaving it. |
| `HydroCouple::Spatial::ILineString`, `ICurve` | @ref HydroCouple::SDK::Spatial::LineStringAdapter | Over a @ref HydroCouple::SDK::Spatial::LineString; also serves polygon rings. |
| `HydroCouple::Spatial::IPolygon`, `ISurface` | @ref HydroCouple::SDK::Spatial::PolygonAdapter | Over a @ref HydroCouple::SDK::Spatial::Polygon. |
| `HydroCouple::Spatial::IEdge` | @ref HydroCouple::SDK::Spatial::EdgeAdapter | Network edges; no quad-edge navigation. |
| `HydroCouple::Spatial::INetwork` | @ref HydroCouple::SDK::Spatial::NetworkAdapter | Over a @ref HydroCouple::SDK::Spatial::Network. |
| `HydroCouple::Spatial::IPolyhedralSurface` | @ref HydroCouple::SDK::Spatial::PolyhedralSurfaceAdapter | Over a @ref HydroCouple::SDK::Spatial::PolyhedralSurface and, preferably, the `MeshDefinition` it came from. |
| `HydroCouple::Spatial::IMeshView` | @ref HydroCouple::SDK::Spatial::MeshViewAdapter | Over a @ref HydroCouple::SDK::IO::MeshDefinition; derives the edge topology and metrics. |
| `HydroCouple::Spatial::ITIN`, `ITriangle` | none | @ref HydroCouple::SDK::Spatial::TIN is a value type only; serve a TIN as an `IPolyhedralSurface`, or implement `ITIN`. |
| `IMultiPoint`, `IMultiLineString`, `IMultiPolygon`, `IGeometryCollection`, `ILine`, `ILinearRing` | none | Implement them in the component if a consumer needs them. |
| `HydroCouple::Spatial::IRaster`, `IRasterBand` | none | @ref HydroCouple::SDK::Spatial::Raster and @ref HydroCouple::SDK::Spatial::RasterBand wrap GDAL directly. |
| `HydroCouple::Spatial::IRegularGrid2D`, `IRegularGrid3D` | none | The grid classes are value types; a component implements the interface over its grid (@ref sdk_space_grids "regular grids"). |
| `HydroCouple::Spatial::ISpatialReferenceSystem` | none in this module | @ref HydroCouple::SDK::IO::MeshPackageReference for mesh packages; otherwise the component implements it (@ref sdk_space_crs "reference systems"). |
| `HydroCouple::Spatial::IVerticalCoordinate` | none | The component implements it (@ref sdk_space_layered "layered meshes"). |
| `HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem` | @ref HydroCouple::SDK::Spatial::PolyhedralSurfaceArgument; the layered and time-series items below | |
| `HydroCouple::Spatial::ILayeredMeshComponentDataItem` | @ref HydroCouple::SDK::Spatial::LayeredMeshComponentDataItem, @ref HydroCouple::SDK::Spatial::LayeredMeshOutput, @ref HydroCouple::SDK::Spatial::LayeredMeshInput | Values indexed `{entity, layer}`. |
| `HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem` | @ref HydroCouple::SDK::SpatioTemporal::TimeGeometryComponentDataItem | `{time, geometry}`. |
| `HydroCouple::SpatioTemporal::ITimeNetworkComponentDataItem` | @ref HydroCouple::SDK::SpatioTemporal::TimeNetworkComponentDataItem | `{time, entity}`, nodes or edges. |
| `HydroCouple::SpatioTemporal::ITimeSeriesPolyhedralSurfaceComponentDataItem` | @ref HydroCouple::SDK::SpatioTemporal::TimeSeriesPolyhedralSurfaceComponentDataItem | `{time, entity}`. |
| `HydroCouple::SpatioTemporal::ITimeSeriesTINComponentDataItem` | @ref HydroCouple::SDK::SpatioTemporal::TimeSeriesTINComponentDataItem | As above, over an `ITIN`. |
| `HydroCouple::SpatioTemporal::ITimeRegularGrid2DComponentDataItem` | @ref HydroCouple::SDK::SpatioTemporal::TimeRegularGrid2DComponentDataItem | `{time, y cell, x cell}`. |
| `HydroCouple::SpatioTemporal::ITimeRegularGrid3DComponentDataItem` | @ref HydroCouple::SDK::SpatioTemporal::TimeRegularGrid3DComponentDataItem | `{time, z cell, y cell, x cell}`. |
| `IGeometryComponentDataItem`, `INetworkComponentDataItem`, `IRegularGrid2D/3DComponentDataItem`, `ITINComponentDataItem` without time | none | Use the time-varying item with one time step, or implement the interface. |
| `IRasterComponentDataItem`, `ITimeSeriesRasterComponentDataItem` | none | @ref HydroCouple::SDK::Spatial::RasterComponentDataItem is a plain rank-2 item over one band. |
| `ILayeredNetworkComponentDataItem`, `ICrossSection`, `ITimeLayeredMeshComponentDataItem`, `ITimeLayeredNetworkComponentDataItem` | none | Implement them in the component. |

The value types and adapters live in `hydrocouplesdk/spatial/` (namespace
`HydroCouple::SDK::Spatial`), the time-varying items in `hydrocouplesdk/spatiotemporal/` (namespace
`HydroCouple::SDK::SpatioTemporal`). The value-type headers (`point.h` and every header that
includes it, the adapters among them) also bring the standard's `HydroCouple::Spatial` names into
`HydroCouple::SDK::Spatial` with a using-directive.

### The examples of this chapter

Seven compiled programs in `docs/examples/` exercise everything below. Each checks its own results
and returns nonzero when a value is wrong; they run as the ctest tests `docs.example.<directory>`.

| Directory | Shows | Needs |
| --- | --- | --- |
| `ch10_geometry` | Point, LineString and Polygon values, their adapters, a reference system, a time-geometry item | |
| `ch10_network` | A river network, NetworkAdapter and its mesh view, edge flows in time | |
| `ch10_surface_series` | A mesh as a polyhedral surface, the derived mesh view, face depths in time, a TIN | |
| `ch10_regular_grid` | Cartesian 2-D and 3-D grids, an `IRegularGrid2D` written over a grid, grid items in time | |
| `ch10_layered_mesh` | A sigma vertical coordinate, a layered output connected to a layered input, what the checks report when the two disagree | |
| `ch10_mesh_argument` | PolyhedralSurfaceArgument from a UGRID file, an \@uri reference and a component output | `USE_NETCDF` |
| `ch10_raster` | Raster, RasterBand and RasterComponentDataItem over a GeoTIFF | `USE_GDAL` |

## Geometry values {#sdk_space_values}

The geometry value types have no units and no reference system of their own: their coordinates are
in whatever system the adapter that serves them declares. Every measurement they report (lengths,
areas, distances) is in the units of the coordinates. Except where noted, an index out of range is
not an error: the accessor returns a default (the origin, an empty result, 0 or -1), so check
indexes before relying on a result.

### Point

@ref HydroCouple::SDK::Spatial::Point is four public doubles, `x`, `y`, `z` and the measure `m`, 32
bytes with no allocation. A two-dimensional point has z = 0; m is 0 unless set. A *measure* is a
value attached to a position, such as the river station along a reach. Point also serves as a 3-D
vector: `operator-`, `operator+`, `operator*`, `dot()`, `cross()`, `magnitude()` and `normalized()`
use x, y and z, ignore m, and return points with m = 0. `normalized()` of the zero vector is the
zero vector. `distance()` and `distanceSq()` are three-dimensional. `equals()` compares x, y and z
(not m) with an absolute tolerance, 1e-10 by default, and a difference equal to the tolerance
counts as different.

### Envelope

@ref HydroCouple::SDK::Spatial::Envelope is an axis-aligned box: six public bounds `minX`, `maxX`,
`minY`, `maxY`, `minZ`, `maxZ`. A default-constructed envelope is *empty*: each minimum is the
largest double and each maximum the lowest, `isValid()` is false, and the first
`expandToInclude()` takes its argument's bounds. The two-dimensional constructor takes
`(minX, minY, maxX, maxY)` in that order and sets both z bounds to 0; nothing reorders the
arguments. Because `contains()` and `intersects()` test z as well (inclusively), a two-dimensional
envelope contains only points with z = 0. `width()`, `height()`, `depth()`, `area()` (plan) and
`volume()` are differences and products of the bounds.

### LineString

@ref HydroCouple::SDK::Spatial::LineString is an ordered list of points stored as four parallel
arrays (`getX()`, `getY()`, `getZ()`, `getM()`), joined by straight segments. Points are appended
with `addPoint()` or `addPoints()` and cannot be removed. `length()` sums the three-dimensional
segment lengths. `isClosed()` is true when there are at least two points and the first equals the
last under `Point::equals()`. A *ring* is a closed line that repeats its first point as its last;
Polygon's rings follow that rule. `centroid()` is the mean of the points, not a length-weighted
centroid, so the repeated point of a ring counts twice. `hasM()` reports whether the measure array
is non-empty, which is true for every non-empty line, measures set or not.

### Polygon

@ref HydroCouple::SDK::Spatial::Polygon is an exterior ring with optional holes
(`addInteriorRing()`), all LineString values copied into the polygon. Close every ring and orient
the exterior counterclockwise; nothing checks either when the rings are added. `isValid()` checks
only that the exterior ring is closed and has at least four points (three corners plus the
closing point); holes, orientation and self-intersection are not checked, because the SDK links no
geometry engine.

Area and containment work in plan, on x and y. The area is

\f[ A = \left| S_e - \sum_h S_h \right|, \qquad
    S = -\tfrac{1}{2} \sum_{i=0}^{n-2} (x_{i+1} - x_i)(y_{i+1} + y_i) \f]

where \f$S_e\f$ and \f$S_h\f$ are the signed areas of the exterior ring and of each hole, positive
for a counterclockwise ring. A hole reduces the area only when it runs in the *same* direction as
the exterior ring; a clockwise hole in a counterclockwise exterior adds its area (see
@ref sdk_space_limits "Known limitations"). `containsPoint()` casts a ray toward +x and counts ring
crossings: the point is inside when it crosses the exterior an odd number of times and no hole an
odd number of times. A point exactly on a ring follows the half-open rule of the crossing test, so
of a square from (0, 0) to (4, 4), the corner (0, 0) tests inside and (4, 4) outside.
`interiorRing(i)` with an index out of range returns the *exterior* ring rather than failing.

@snippet ch10_geometry/main.cpp values

### PolyhedralSurface

@ref HydroCouple::SDK::Spatial::PolyhedralSurface is a surface made of polygon *patches* (faces)
plus a separate vertex list. The two lists are independent: `addPatch()` stores a shared pointer to
a Polygon (not a copy; a null pointer is stored too) and grows the envelope, `addVertex()` appends a
point and returns its index, and nothing records which vertices a patch uses. `area()` is the sum
of the patches' plan areas, `findPatch()` returns the first patch that contains a point (a linear
search; there is no spatial index), and `centroid()` is the area-weighted mean of the patch
centroids. Because connectivity is not kept, a surface built patch by patch cannot report its faces
and edges as index arrays; @ref sdk_space_meshes "meshes" explains how the SDK keeps them.

### TIN

@ref HydroCouple::SDK::Spatial::TIN is a PolyhedralSurface whose patches are triangles.
`addTriangleByIndex(v0, v1, v2)` copies three existing vertices into a closed ring and returns the
new triangle's index, or -1 without adding anything when an index is out of range.
`addTriangle(p0, p1, p2)` appends three *new* vertices first, even where a vertex already exists at
that position, so build connected meshes from shared vertex indexes. The TIN does not store the
indexes; `triangleVertices()` recovers them by matching corner positions against the vertex list
(the highest index wins when vertices coincide), and `numEdges()` counts distinct vertex pairs
found that way. `isValid()` checks that every patch has four ring points. `averageTriangleArea()`
is `area() / numTriangles()`. The interpolation functions are not usable in this release (see
@ref sdk_space_limits "Known limitations").

@snippet ch10_surface_series/main.cpp tin

```text
tin: 2 triangles, 5 edges, valid 1, area 100.0, mean triangle 50.0, triangle 1 corners 0 2 3, refused index -1
```

## Serving geometry through the interfaces: the adapters {#sdk_space_adapters}

### GeometryAdapter

@ref HydroCouple::SDK::Spatial::GeometryAdapter implements the part of `IGeometry` that every
adapter shares. It is constructed with an id, an index in the owning collection, and a *borrowed*
`ISpatialReferenceSystem` pointer (nullable; it must outlive the adapter). What it answers:

| Methods | Behavior |
| --- | --- |
| `id()`, `index()` | As given at construction. Parts made by a composite adapter get derived ids: "reach:1" for point 1 of the line "reach", "catchment:exterior", "catchment:interior0". |
| `spatialReferenceSystem()` | The borrowed pointer. `setSpatialReferenceSystem()` changes this adapter only, not the point or ring adapters it already built, so pass the reference to the constructor. |
| `envelope()` | A pointer to the adapter's own EnvelopeAdapter, never null, valid for the adapter's life; the wrapped value's bounds, z included. |
| `getWKB()` | Well-known binary, two-dimensional, little-endian: x and y only. |
| `getWKT()` | "POINT (x y)", "LINESTRING (...)", "POLYGON ((...), (...))"; any other type writes "GEOMETRYCOLLECTION EMPTY". Coordinates use six significant digits. |
| `coordinateDimension()`, `is3D()`, `isMeasured()`, `isSimple()` | Fixed: 2, false, false, true. |
| `equals()`, `disjoint()`, `intersects()`, `touches()`, `crosses()`, `within()`, `contains()`, `overlaps()`, `relate()` | Always false: not computed. |
| `distance()` | Always 0: not computed. |
| `boundary()`, `buffer()`, `convexHull()`, `intersection()`, `unionG()`, `difference()`, `symmetricDifference()`, `locateAlong()`, `locateBetween()` | Always nullptr: not computed. |

The predicates and constructive operations need a geometry engine, which the SDK does not link. A
consumer that needs them (a GIS front end, for example) reads the WKB and uses GEOS or GDAL. The
adapters keep z in the point values and the envelope, but their WKB and WKT are two-dimensional and
`is3D()` is false; a consumer that needs elevations reads `IPoint::z()` or the mesh view.

### PointAdapter, LineStringAdapter and PolygonAdapter

@ref HydroCouple::SDK::Spatial::PointAdapter serves one point; its envelope is the point itself, a
box of zero size, and its WKB is 21 bytes (byte-order marker, type code 1, x, y).
@ref HydroCouple::SDK::Spatial::LineStringAdapter copies a line and builds one PointAdapter per
vertex at construction, so `point(i)`, `startPoint()` and `endPoint()` return stable pointers
(nullptr for an index out of range or an empty line); its WKB is 9 + 16·n bytes. `isRing()` is
"closed with more than three points", without a simplicity check.
@ref HydroCouple::SDK::Spatial::PolygonAdapter copies a polygon and builds a LineStringAdapter for
the exterior ring and each hole; `exteriorRing()` is never null, `interiorRing(i)` is null out of
range (unlike Polygon's own accessor). `centroid()` and `pointOnSurface()` return new caller-owned
PointAdapters, the second equal to the first and not guaranteed to lie inside a concave polygon.
`boundaryMultiCurve()` returns nullptr; read the rings instead. A free-standing polygon belongs to
no mesh: `edge()` and `polyhedralSurface()` return nullptr until a surface calls `setSurface()`,
as PolyhedralSurfaceAdapter does for its patches. The two composite adapters own their parts and
are not copyable.

@ref HydroCouple::SDK::Spatial::EnvelopeAdapter is the small `IEnvelope` the other adapters and the
geometry item embed; it holds a copy of an Envelope, and `setEnvelope()` changes it in place.

The example wraps the gauges, the reach and the catchment, declaring a reference system for each,
and reads them back through the interfaces only:

@snippet ch10_geometry/main.cpp adapters

```text
reach: 2 points, length 500.036 m (3-D), closed 0
catchment: valid 1, area 120000.0 m2, contains upstream gauge 1
gauge_a: type 1, z 210.0, WKB 21 bytes, WKT "POINT (512000 4.512e+06)"
reach adapter: 2 points, point 1 id "reach:1", end m 500.0, WKB 41 bytes
catchment adapter: area 120000.0, exterior ring 5 points, holes 0, envelope x 511900..512300
reference: EPSG:26917, vertical EPSG:5703; is3D 0; intersects 0; buffer nullptr
```

The WKT line shows the six-digit limit: the northing 4512000 is written as 4.512e+06. Use the WKB
where coordinates must survive.

## Networks {#sdk_space_networks}

A *network* is a graph of vertices joined by directed edges: river reaches, pipes, flow paths.
@ref HydroCouple::SDK::Spatial::Network stores vertices as Points and edges as
@ref HydroCouple::SDK::Spatial::NetworkEdge pairs of vertex indexes (`origin`, `dest`).
`addVertex()` returns the new index (a Point's measure is dropped); `addEdge()` returns the new
edge's index, or -1 when either vertex index does not exist. Self-loops, parallel edges and
disconnected parts are accepted. Indexes never change, because nothing is removed.
`NetworkEdge::length()` and `Network::totalLength()` are three-dimensional. Out-of-range `vertex()`
and `edge()` calls return references to static defaults; the non-const `vertex()` returns one
static Point shared by every network, so writing through an out-of-range reference changes what the
next out-of-range call returns. Network has an identity (`id()`, a generated UUID) and, like every
SDK class built on Identity, can be neither copied nor moved.

@snippet ch10_network/main.cpp network

@ref HydroCouple::SDK::Spatial::NetworkAdapter serves a Network as `INetwork`. It copies the network
at construction into one @ref HydroCouple::SDK::Spatial::VertexAdapter per vertex (id "vertexI"),
one @ref HydroCouple::SDK::Spatial::EdgeAdapter per well-formed edge, and a MeshViewAdapter, and
takes the network's id and caption. An edge that names a missing vertex is skipped and the edges
kept are renumbered consecutively. Each vertex's `edge()` is the first kept edge that leaves it,
nullptr for a vertex no edge leaves (the outlet). An EdgeAdapter's `orig()` and `dest()` are the
adapter's own vertices, so identity comparisons work; `left()`, `right()`, `face()` and the
quad-edge navigation (`rot()`, `sym()`, `origNext()` and the rest) return nullptr, although the
interface describes them as never null. The network's *mesh view* (`meshView()`) has the vertices
as nodes (x and y only, so `nodeZ()` is empty), the kept edges as `edgeNodes()` pairs in order, no
faces, plan edge lengths and unit normals.

@snippet ch10_network/main.cpp adapter

```text
network: 4 vertices, 3 edges, bad edge index -1, total length 3500.121 m
adapter: id is the network's (36 characters), 4 vertices, 3 edges; edge 1 runs vertex1 -> vertex2
vertex edges: headwater -> edge 0, confluence -> edge 2, outlet -> nullptr
mesh view: 4 nodes, 3 edges, 0 faces, nodeZ 0 values, edge 1 plan length 1500.0 m, normal (0.0, 1.0), sym nullptr
flow item: dimensions "time" x "edges", shape {2, 3}, outlet reach 5.5 then 6.3 m3/s
```

The total length (3500.121 m) includes the drops in elevation; the mesh view's 1500.0 m for the
tributary does not. The flow item on the last line is described under
@ref sdk_space_items "spatiotemporal data items".

## Meshes {#sdk_space_meshes}

### MeshDefinition and the mesh view

@ref HydroCouple::SDK::IO::MeshDefinition is the SDK's structure-of-arrays mesh: node coordinate
arrays `nodeX`, `nodeY` and optional `nodeZ`; face-to-node connectivity in compressed sparse row
(CSR) form, where face f uses `faceNodes[faceNodeOffsets[f]]` up to (not including)
`faceNodes[faceNodeOffsets[f + 1]]`; and optional `edgeNodes` pairs. The layout follows the UGRID
conventions, so the SDK's readers and writers use it directly (@ref sdk_guide_mesh_packages). Face
nodes are listed counterclockwise. `MeshDefinition::validate()` checks array lengths, CSR
monotonicity and index bounds; nothing else in this chapter does, so validate a mesh from outside
the program before adapting it.

@ref HydroCouple::SDK::Spatial::MeshViewAdapter serves a MeshDefinition as `IMeshView`, the bulk
accessor that partitioners, remappers, writers and device staging read. It owns a copy of the
definition (move it in to avoid the copy), hands out spans over the node and face arrays, flattens
the edge pairs, and derives at construction what the standard requires of a mesh with faces:

- Edges implied by face loops but missing from the definition are appended after the definition's
  own edges, which keep their indexes. Edge k of a face joins its node k to node k + 1, the last to
  the first; `faceEdgeOffsets()`/`faceEdges()` list them in that order.
- `edgeFaces()` gives each edge e the face on its left at 2e and on its right at 2e + 1, looking
  from `edgeNodes()[2e]` to `edgeNodes()[2e + 1]`, with -1 for the outside. A counterclockwise face
  whose loop runs along an edge's direction is on its left.
- `faceAreas()`, `faceX()` and `faceY()` are the plan shoelace area and centroid of each face,
  \f$A_f = \tfrac12 \left| \sum_k (x_k y_{k+1} - x_{k+1} y_k) \right|\f$; a face of zero area gets
  the mean of its nodes as its point.
- `edgeLengths()` are plan lengths and the unit normal is
  \f$ (n_x, n_y) = (\Delta y, -\Delta x) / L \f$, which points from the left face to the right
  face; 0 for an edge of zero length.

Publishing these metrics means both sides of a coupling integrate over the same areas and lengths.
A definition without faces gets edge lengths and normals and empty face spans.

### From a mesh to a surface and back

<pre class="mermaid">
flowchart LR
    M["MeshDefinition<br/>(arrays: nodes, CSR faces,<br/>optional edges)"]
    S["PolyhedralSurface<br/>(patches + vertices,<br/>no connectivity)"]
    P["PolyhedralSurfaceAdapter<br/>IPolyhedralSurface"]
    V["MeshViewAdapter<br/>IMeshView"]
    M -- "polyhedralSurfaceFromMesh()" --> S
    S -- "patches, vertices" --> P
    M -- "second constructor:<br/>the connectivity" --> P
    P -- "meshView()" --> V
    V -- "PolyhedralSurfaceArgument::initialize(item)<br/>copies nodes, faces, edges" --> M
</pre>

@ref HydroCouple::SDK::Spatial::polyhedralSurfaceFromMesh turns every node into a vertex (with z when
`nodeZ` has one value per node) and every face into a Polygon patch whose ring is its nodes in order
with the first repeated. The result records no connectivity, so pass the MeshDefinition along with
it to @ref HydroCouple::SDK::Spatial::PolyhedralSurfaceAdapter. That adapter copies the surface at
construction: one PolygonAdapter per non-null patch (id "patchI", told that it belongs to this
surface), one VertexAdapter per vertex (with no edge), and the area, envelope and centroid. Its own
id is always "surface". Its mesh view depends on the constructor:

| Constructor | `meshView()` |
| --- | --- |
| `PolyhedralSurfaceAdapter(surface, mesh, srs)` | The supplied MeshDefinition, with its faces and derived edges. Use this one. |
| `PolyhedralSurfaceAdapter(surface, srs)`, or a supplied mesh with no nodes | The vertices alone: nodes, no faces, no edges. A consumer reads this as a mesh without cells. |

`boundingPolygons()` returns nullptr and `isClosed()` false: the surface keeps no adjacency; use
`meshView()->edgeFaces()` for neighbors. `getWKB()` writes a complete type-15 record (each patch a
full polygon record); `getWKT()` writes "GEOMETRYCOLLECTION EMPTY".

@snippet ch10_surface_series/main.cpp mesh

```text
surface: 2 patches, 5 vertices, area 150.0 m2, patch 1 ring 4 points
view: 5 nodes, 2 faces, 6 edges (none were given)
  edge 0: nodes 0-1, left face 0, right face -1, length 10.000, normal (0.000, -1.000)
  edge 1: nodes 1-2, left face 0, right face -1, length 10.000, normal (1.000, 0.000)
  edge 2: nodes 2-3, left face 0, right face 1, length 10.000, normal (0.000, 1.000)
  edge 3: nodes 3-0, left face 0, right face -1, length 10.000, normal (-1.000, 0.000)
  edge 4: nodes 2-4, left face 1, right face -1, length 11.180, normal (0.894, 0.447)
  edge 5: nodes 4-3, left face 1, right face -1, length 11.180, normal (-0.894, 0.447)
  face 0: area 100.0, centroid (5.000, 5.000)
  face 1: area 50.0, centroid (5.000, 13.333)
```

Edge 2 is shared: the square (face 0) traverses it from node 2 to node 3, so the square is on its
left, and the triangle traverses it the other way and is on its right. Its normal (0, 1) points from
the square into the triangle, which is the direction a flux across it is positive in the
`NormalTangential` basis.

## Regular grids {#sdk_space_grids}

@ref HydroCouple::SDK::Spatial::RegularGrid2D is a logically rectangular array of nx × ny nodes with
(nx − 1) × (ny − 1) cells. Node (i, j), i along x and j along y, is stored at j·nx + i in
`nodeXData()` and `nodeYData()`; cell (i, j) has corners (i, j), (i + 1, j), (i + 1, j + 1) and
(i, j + 1). Grids are made by three factories, which return shared pointers because the class (an
Identity) cannot be copied:

| Factory | Node (i, j) is at | Returns nullptr when |
| --- | --- | --- |
| `cartesian(x0, y0, dx, dy, nx, ny)` | (x0 + i·dx, y0 + j·dy) | never (nothing is validated) |
| `rectilinear(nodeX, nodeY, nx, ny)` | (nodeX[i], nodeY[j]) | the arrays are not nx and ny long |
| `curvilinear(nodeX, nodeY, nx, ny)` | (nodeX[j·nx + i], nodeY[j·nx + i]) | either array is not nx·ny long |

The counts are numbers of *nodes*. All three store full nx·ny arrays; `gridType()` is the label the
factory set (the SDK's own enumeration, with the same values as the standard's `RegularGridType`)
and does not change when `setNodeX()`/`setNodeY()` move a node. `cellCenter()` is the mean of the
four corners; `cellArea()` sums the signed areas of the triangles (i, j)–(i+1, j)–(i, j+1) and
(i+1, j)–(i+1, j+1)–(i, j+1), exact for any simple quadrilateral cell. Out-of-range nodes read as 0
and out-of-range cells have area 0 and center (0, 0). `envelope()` returns misplaced bounds in this
release (see @ref sdk_space_limits "Known limitations").

@ref HydroCouple::SDK::Spatial::RegularGrid3D is the same for nx × ny × nz nodes, node (i, j, k) at
k·nx·ny + j·nx + i, with `cartesian()` (node at (x0 + i·dx, y0 + j·dy, z0 + k·dz), so layer k = 0
is at z0) and `curvilinear()`. Every node has its own x, y and z, so it can hold a grid that is
curvilinear in three dimensions, which the standard's `IRegularGrid3D` (plan coordinates shared by
every layer) cannot express. It has no grid type, envelope or cell geometry.

The SDK does not implement `IRegularGrid2D` or `IRegularGrid3D`. A component that publishes a grid
item writes the interface over its own grid, which takes a few lines because RegularGrid2D's arrays
already have the row-major `[yNode][xNode]` layout `nodeXs()` requires:

@snippet ch10_regular_grid/main.cpp grid_view

@snippet ch10_regular_grid/main.cpp grid

```text
grid: 4 x 3 nodes, 3 x 2 cells, cell (2,1) center (500250.0, 4500150.0), area 10000.0 m2, node (3,2) at (500300.0, 4500200.0)
depth item: shape {2, 2, 3}, axes "time" "y_cells" "x_cells", row y=1 at t1: 0.4 0.5 0.6, grid grid with 4 x-nodes
grid3d: 36 nodes, 12 cells, node (0,0,3) z 8.5; temperature shape {1, 3, 2, 2}, column 12 16 20
```

## Rasters {#sdk_space_rasters}

@ref HydroCouple::SDK::Spatial::Raster opens a raster file through GDAL and
@ref HydroCouple::SDK::Spatial::RasterBand reads one band. They exist only in a library built with
the CMake option `USE_GDAL` (off by default; @ref sdk_ref_cmake): without it `raster.cpp` is not
compiled and a program calling them does not link. Neither implements `IRaster` or `IRasterBand`.

- The program registers GDAL's drivers once, with `GDALAllRegister()`, before the first
  `Raster::open()`; the SDK does not. Without it every open fails ("Failed to open raster file: ...",
  and GDAL prints "No driver registered").
- `open()` always opens read only. Band writes (`setValue()`, `writeBlock()`, and
  `RasterComponentDataItem::setValuesFrom()`) therefore fail; `setValue()` fails silently and GDAL
  prints its own error.
- A RasterBand is a handle into the Raster's dataset, valid until `close()`, another `open()`, or
  the Raster's destruction. Bands are numbered from 1. Values move as doubles whatever the band's
  storage type; x is the column from the left, y the row from the top, both from 0.
- `getGeoTransform()` returns GDAL's six affine coefficients:
  \f$ X = g_0 + P g_1 + L g_2, \; Y = g_3 + P g_4 + L g_5 \f$ for pixel column P and line L.
  Coordinate conversion is left to the caller. `envelope()` returns misplaced bounds in this release
  (see @ref sdk_space_limits "Known limitations").

@ref HydroCouple::SDK::Spatial::RasterComponentDataItem puts one band on the data plane: a rank-2
Float64 item of shape `{rows, columns}` read from the band on every call (`{0, 0}` without a
raster), whose `getValuesInto()` reads the requested window from the file. It is a plain
AbstractComponentDataItem, not an `IRasterComponentDataItem`, and borrows the Raster and the two
dimensions it is given (name them "rows" and "columns" to get the Row and Column roles).

@snippet ch10_raster/main.cpp open

@snippet ch10_raster/main.cpp item

Output of a run in a `USE_GDAL` build (the default build skips this example):

```text
raster: driver GTiff, 4 x 3 pixels, 1 band(s); origin (512000, 4512015), pixel 10 x 5 m
band 1: type Float32, nodata 1 (-9999), pixel (2, 1) = 106, outside = 0
item: shape {3, 4}, window 105 106 / 109 110, write 0 (Raster band write failed)
all checks passed
```

## Coordinate reference systems {#sdk_space_crs}

A *coordinate reference system* says what coordinates mean: which projection or geographic system
x and y are in, and which vertical datum z is measured from. The standard declares one through
`HydroCouple::Spatial::ISpatialReferenceSystem`: an authority name and code for the horizontal
system (`authName()`, `authSRID()`, such as EPSG and 26917), its well-known text, its distance units,
and the same three for the vertical datum (`verticalAuthName()`, `verticalAuthSRID()`, such as EPSG
5703 for NAVD88 height) with its own units. Elevations are positive up in that datum; an empty
vertical authority with code 0 means "relative to an unknown surface".

### How the SDK carries a reference system

The SDK never stores a reference system in a value type. It is attached where geometry crosses the
boundary, as a borrowed pointer:

| Where | How |
| --- | --- |
| PointAdapter, LineStringAdapter, PolygonAdapter | Constructor argument; passed on to the point and ring adapters they build. |
| NetworkAdapter | Constructor argument, given to every vertex. |
| PolyhedralSurfaceAdapter | Constructor argument, given to the surface, its patches and its vertices. |
| PolyhedralSurfaceArgument | None: the surface it builds has a null reference. |
| Mesh packages | @ref HydroCouple::SDK::IO::MeshPackageReference builds one from a package's declared CRS and datum (@ref sdk_guide_mesh_packages). |
| Reopened runs | @ref HydroCouple::SDK::ResultsModelComponent rebuilds the recorded reference for each reopened surface (@ref sdk_guide_record). |

The pointer must outlive every adapter and item that hands it out, so it is normally a member of the
component. The SDK does not reproject: coordinates are passed as they are, and the reference only
declares what they are. A component that has no reference system library implements the interface
over the codes it knows:

@snippet ch10_geometry/main.cpp reference

### What the connection validator checks

@ref HydroCouple::SDK::validateConnection (@ref sdk_guide_workflow) finds each end's reference system
through its data item and compares them:

| Item | Reference read from |
| --- | --- |
| `IGeometryComponentDataItem` | `geometry(0)->spatialReferenceSystem()` |
| `IPolyhedralSurfaceComponentDataItem` (mesh, layered, time-series surface items) | `polyhedralSurface()->spatialReferenceSystem()` |
| `INetworkComponentDataItem` | `network()->vertex(0)->spatialReferenceSystem()` |
| `IRasterComponentDataItem` | `raster()->spatialReferenceSystem()` |
| `IRegularGrid2D/3DComponentDataItem` | `grid()->spatialReferenceSystem()` |

When either end has none, nothing is compared. Otherwise:

| Condition | Finding |
| --- | --- |
| Both declare a horizontal system (non-empty authority, nonzero code) and they differ | Error: horizontal reference systems differ (EPSG:a vs EPSG:b); wire a reprojecting adapter between them |
| Both declare a vertical datum and they differ | Error: vertical datums differ (...); elevations need a datum shift |
| Same vertical datum, different declared vertical units | Error: same vertical datum, different vertical units |
| Only one end declares a vertical datum | Warning: provider (or consumer) declares no vertical datum while the other does; elevations cannot be checked |

The layered-mesh example below shows the vertical-datum error.

## Spatiotemporal data items {#sdk_space_items}

The five spatiotemporal item templates combine the time-series storage of
@ref sdk_guide_time (`HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem`) with one spatial
association. They share these rules:

- The item starts with no time steps. The component appends them with `addTimeStep(time, values)`,
  where `values` holds one value per entity (per cell for a grid) and its length must equal the
  element count, or `std::invalid_argument` is thrown. Times should increase; nothing checks.
- Storage is time-major and contiguous: "one time step, every entity" is one slab. `setValuesFrom()`
  overwrites existing steps only and emits the value-changed signal; `setValueAt()` and `rawData()`
  write without a signal.
- `times()` returns Julian days; `timeSpan()` covers the first to the last step and is a new object
  after every `addTimeStep()`, so do not keep its pointer. The temporal semantics (`timeKind()`,
  `timeInterpolation()`, `timeExtrapolation()`) are Unknown until the component calls
  `setTimeKind()` and the other setters; the connection validator warns about undeclared ones.
- The spatial association (geometry list, network, surface, grid) is borrowed and may be null. The
  element count is fixed at construction and is not checked against it.
- Every item is rank 2 (rank 3 or 4 for grids). None adds the trailing Component dimension the
  standard calls for when the data type is Vector, Tensor or MultiScalar, so pack such values in the
  entity axis or write your own item.
- The items derive from `AbstractComponentDataItem`, not from `AbstractOutput` or `AbstractInput`,
  so a component cannot add one with `addOutput()` or `addInput()`. They describe values a component
  holds (internal stores, arguments, the items a reopened run publishes). An exchangeable spatial
  output composes `AbstractOutput` with the storage and the spatial interface, as
  @ref HydroCouple::SDK::Spatial::LayeredMeshOutput does and as the mesh generator of
  HydroCoupleComponents does for a mesh surface
  (https://www.hydrocouple.org/HydroCoupleComponents/).
- `int` and `double` instantiations are exported from the library, with `...Double` aliases.

| Item | `shape()` | Dimension ids (role) |
| --- | --- | --- |
| @ref HydroCouple::SDK::SpatioTemporal::TimeGeometryComponentDataItem | `{time, geometry}` | "time" (Time), "geometries" (Entity) |
| @ref HydroCouple::SDK::SpatioTemporal::TimeNetworkComponentDataItem | `{time, entity}` | "time", "edges" for Edge and "nodes" otherwise (Entity) |
| @ref HydroCouple::SDK::SpatioTemporal::TimeSeriesPolyhedralSurfaceComponentDataItem | `{time, entity}` | "time", "nodes"/"edges"/"faces"/"volumes" (Entity) |
| @ref HydroCouple::SDK::SpatioTemporal::TimeSeriesTINComponentDataItem | `{time, entity}` | as above |
| @ref HydroCouple::SDK::SpatioTemporal::TimeRegularGrid2DComponentDataItem | `{time, y cell, x cell}` | "time", "y_cells" (Row), "x_cells" (Column) |
| @ref HydroCouple::SDK::SpatioTemporal::TimeRegularGrid3DComponentDataItem | `{time, z cell, y cell, x cell}` | "time", "z_cells" (Depth), "y_cells", "x_cells" |

The roles come from the dimension ids (see @ref HydroCouple::SDK::Dimension), and the connection
validator compares them axis by axis.

### Where values sit and what they are

The mesh, network and grid items declare where their values are attached, the *location*, a
`HydroCouple::Spatial::MeshLocation`; mesh and network items also declare what each value is, a
`SpatialDataType`, and in which frame a vector is expressed, a `VectorBasis` (set with
`setVectorBasis()`, Unknown by default).

| `MeshLocation` | Values on |
| --- | --- |
| `Node` | Mesh nodes or network vertices. |
| `Edge` | Mesh edges or network edges (reaches, conduits). |
| `Face` | Mesh faces (a cell of a two-dimensional mesh is a face), TIN triangles, regular-grid cells. |
| `Volume` | Extruded cells of a layered mesh; cells of a 3-D grid. |

| `SpatialDataType` | Each value is |
| --- | --- |
| `Scalar` | One number. |
| `MultiScalar` | Several independent numbers, such as constituents. |
| `Vector` | 2 or 3 components in the order `VectorBasis` defines. |
| `Tensor` | 4 or 9 entries, row-major. |

| `VectorBasis` | Components along |
| --- | --- |
| `Unknown` | Not declared; legal only for Scalar and MultiScalar. |
| `Cartesian` | The reference system's x, y (and z) axes. |
| `EastNorthUp` | Local east, north (and up), for geographic systems. |
| `NormalTangential` | Normal to the edge or face (positive from left to right face, as `edgeFaces()` orders them), then tangential. |
| `AlongEntity` | A network edge, from its first node to its second. |

For mesh and network items the validator reports an error when the two ends' locations or data
types differ, or when both declare vector bases that differ, and a warning when a Vector or Tensor
end declares no basis; for grid items it compares the locations only. The
grid items report the location they were given but always store one value per cell, and their
`cellEdgeDimension()`, `cellFaceDimension()` and `cellVertexDimension()` return nullptr.

### TimeGeometryComponentDataItem

Values on a list of geometries, typically point gauges or polygons. The constructor takes the
declared geometry type and a vector of borrowed `IGeometry` pointers (entries may be null; the type
is not checked against them). `envelope()` reports what `setEnvelope()` stored, empty until then;
nothing computes it from the geometries.

@snippet ch10_geometry/main.cpp geometry_item

```text
gauge_stage: shape {3, 2}, dimensions "time", "geometries", geometry 1 "gauge_b", latest 211.60 205.55, envelope maxY 4512400
```

### TimeNetworkComponentDataItem

Values on the nodes or the edges of a borrowed `INetwork`. The example records hourly edge flows and
reads one edge through every time step, a strided read the hyperslab call performs without a copy
of the whole item:

@snippet ch10_network/main.cpp network_item

### TimeSeriesPolyhedralSurfaceComponentDataItem and TimeSeriesTINComponentDataItem

Values on the nodes, edges, faces or volumes of a borrowed `IPolyhedralSurface`; entity indexes are
those of the surface's mesh view. The TIN variant also answers `TIN()`, returning the same object as
`polyhedralSurface()`; since the SDK has no `ITIN` implementation, the TIN comes from the component.
The example integrates face depths over the areas the mesh view publishes:

@snippet ch10_surface_series/main.cpp series

```text
depth item: dimensions "time" x "faces", shape {3, 2}, stored water 0.0, 22.5, 40.0 m3
```

### TimeRegularGrid2DComponentDataItem and TimeRegularGrid3DComponentDataItem

Cell values on a borrowed `IRegularGrid2D` or `IRegularGrid3D`. Each time step holds the cells in
row order: cell (y, x) of a 2-D item at y·xCells + x, cell (z, y, x) of a 3-D item at
(z·yCells + y)·xCells + x. The 2-D item has `yCells()` and `xCells()`; the 3-D item has no count
accessors (read `shape()`). A window of rows, a column through the layers, or one time step is a
single hyperslab call:

@snippet ch10_regular_grid/main.cpp grid_item

@snippet ch10_regular_grid/main.cpp grid3d

## Layered meshes {#sdk_space_layered}

A *layered mesh* is a two-dimensional mesh extruded downward into layers: lake and estuary models
resolve temperature or salinity this way. Values are indexed `{entity, layer}`, entity first because
`IPolyhedralSurfaceComponentDataItem` fixes the entity as dimension 0, so a partner that understands
only surfaces can still read the item. Layer 0 is at the top.

Values by layer mean nothing without knowing where the layers are, and under a terrain-following
coordinate they move every step as the free surface rises and falls. A layered item therefore hands
out a *vertical coordinate*, `HydroCouple::Spatial::IVerticalCoordinate`, which answers with
elevations directly:

| Member | Meaning |
| --- | --- |
| `kind()` | The family: Sigma (terrain-following), ZLevel, ZStar, Hybrid, Isopycnal, DepthBelowSurface, or Unknown. |
| `layerCount()`, `columnCount()` | Layers per column; columns (plan entities). |
| `interfaceElevation(c, k)` | Elevation of interface k of column c, positive up in the reference system's vertical datum; interface 0 is the top, `layerCount()` the bed. |
| `interfaceElevations()` | All of them, `[column][interface]`, `columnCount() · (layerCount() + 1)` values. |
| `isTimeVarying()`, `geometryEpoch()` | Whether elevations move, and a counter that increases each time they do, so a consumer re-reads only when it changed. |

The SDK has no implementation of the interface; the model knows its own coordinate exactly and
writes one. A sigma coordinate divides each column's depth equally,
\f$ z_k = \eta + (b - \eta)\,k/K \f$ for interface k of K layers, free surface \f$\eta\f$ and bed
\f$b\f$:

@snippet ch10_layered_mesh/main.cpp sigma

### The three layered classes

| Class | Base | Use |
| --- | --- | --- |
| @ref HydroCouple::SDK::Spatial::LayeredMeshComponentDataItem | `AbstractComponentDataItem` | Layered values a component holds internally; cannot be added as an input or output. |
| @ref HydroCouple::SDK::Spatial::LayeredMeshOutput | `AbstractOutput` | Layered values a component publishes (`addOutput()`). `updateValues()` does nothing: the model writes values as it steps. |
| @ref HydroCouple::SDK::Spatial::LayeredMeshInput | `AbstractInput` | Layered values a component consumes (`addInput()`). |

All three store values in a `ComponentDataItem2D<T>` (`operator()(e, k)`, `rawData()`, entity-major:
value (e, k) at e·layerCount + k) and own two dimensions: the entity dimension, named after the
location ("faces", "volumes", ...) with role Entity, and "layers" with role Layer. They borrow the
surface (nullable) and the vertical coordinate (which must not be null and must outlive the item).
The coordinate is borrowed, not copied, so that the item always reports the elevations the model has
now. The input and output share their associations through
@ref HydroCouple::SDK::Spatial::detail::LayeredMeshFacets so the two cannot drift apart; only the
`double` instantiations are exported.

`addInput()` and `addOutput()` only register an item: AbstractModelComponent lists it by id (a
second item with the same id is ignored) and never deletes it. The component owns its layered items
as it owns every other item: hold each in a `std::unique_ptr` member declared after the surface,
vertical coordinate and value definition it borrows (members are destroyed in reverse order, so the
item goes first), and pass `.get()` to the add call. The example keeps everything on the stack in
`main()`, in the same order, because it has no component.

<pre class="mermaid">
sequenceDiagram
    participant P as Provider component
    participant O as LayeredMeshOutput
    participant I as LayeredMeshInput
    participant C as Consumer component
    P->>O: write values (e, k) as it steps
    C->>I: setProvider(output)
    I->>O: canConsume: kind, rank 2, same layer count
    Note over C: each update
    C->>I: applyInputValues() calls retrieveValuesFromProvider()
    I->>O: getValuesInto(first min(entities) rows, all layers)
    O-->>I: values copied into the input's store
    I-->>C: value-changed signal; applyData()
</pre>

`LayeredMeshInput::canConsume()` accepts a provider of the same element kind, rank 2, and the same
layer count; `AbstractInput::setProvider()` calls it and refuses the provider otherwise. It does not
compare entity counts (the shared leading entities are copied and the rest keep their values),
elevations (a sigma model and a z-level model legitimately place layers differently, and reconciling
them is a remapping adapter's job) or reference systems. `retrieveValuesFromProvider()`, called by
`AbstractModelComponent::applyInputValues()` before `applyData()`, copies the values and emits the
value-changed signal; it does nothing, silently, when there is no provider, the provider no longer
matches, or the read fails.

@snippet ch10_layered_mesh/main.cpp link

@snippet ch10_layered_mesh/main.cpp live

@snippet ch10_layered_mesh/main.cpp mismatch

```text
output: shape {2, 3}, axes "volumes" "layers"; layer 1 of face 0 spans -2.0 to -4.0 m
connected 1; input face 1 layers 23.5 21.0 18.0
matching link: 0 finding(s)
after the stage rises 1.5 m: interface 0 at 1.50 m, interface 1 at -1.00 m, epoch 0 -> 1
four-layer input: canConsume 0, "Provider has 3 layers, this input has 4"
z-level model on NGVD29: 2 finding(s), has errors
  error   link 'temperature' -> 'z_model': vertical datums differ (EPSG:5703 vs EPSG:7968); elevations need a datum shift
  error   link 'temperature' -> 'z_model': layer k means different things (Sigma vs ZLevel); wire a vertical remapping adapter between them
```

The z-level model passes `canConsume()` (same kind, rank and layer count) and is accepted by
`setProvider()`; the validator, which `AbstractWorkflowComponent::validate()` runs over every link,
is what refuses it. Its layering checks are: an error when an end is layered but has no vertical
coordinate, a warning when an end declares the kind Unknown, an error when the kinds differ, and an
error when the layer counts differ.

## A mesh as an argument: PolyhedralSurfaceArgument {#sdk_space_argument}

@ref HydroCouple::SDK::Spatial::PolyhedralSurfaceArgument lets a component take its mesh as an
argument, so a composition can supply it from a file or from another component's output (a mesh
generator, for example) without the component knowing which. Both arrive as a MeshDefinition, which
the component reads with `mesh()`.

| Route | Call | Source reference kept |
| --- | --- | --- |
| UGRID NetCDF file | `initialize(path, File, message)` | the path |
| Mesh package manifest (a path ending in ".json") | `initialize(path, File, message)` | the path, and `meshPackage()` |
| JSON object whose \@uri member names a file, with no scheme or the file scheme | `initialize(text, JSON, message)` | the URI as written |
| Another data item: any `IPolyhedralSurfaceComponentDataItem`, or another PolyhedralSurfaceArgument | `initialize(item, message)` | none (another argument's, when copied from one) |
| A MeshDefinition the program holds | `setMesh(mesh, message)` | none |

A UGRID file is read with `readUGRIDMesh()`, taking the file's first mesh; a manifest with
`readMeshPackage()`, which also checks the companion file and the geometry fingerprint
(@ref sdk_guide_mesh_packages). Both need a library built with NetCDF. From another
PolyhedralSurfaceArgument the mesh, the mesh package and the source reference are copied as they
are. From any other surface item the argument copies the surface's mesh view: nodes, CSR faces and
edge pairs, including the edges a MeshViewAdapter derived. `setMesh()` and the surface-item route
check that the coordinate arrays agree in length and that the face offsets end at the face-node
count; they do not check index bounds.

The argument never puts a mesh into a document. `serialize(JSON)` writes an object whose only member, \@uri, is the path, for a mesh
read from a file; a mesh adopted from an output has no file, and the composition document keeps the
\@from binding instead (@ref sdk_guide_compositions), so serializing it is refused. `writeData()`
writes the base fields plus an \@uri member when there is a source; `readData()` reads them back and loads
the mesh, ignoring a load failure.

Through the data plane a generic host sees the node coordinates: a read-only Float64 table of shape
`{nodeCount, 3}` (x, y, z; z is 0 without `nodeZ`), with the dimensions "node" (Entity) and
"coordinate" (Component). As an `IPolyhedralSurfaceComponentDataItem` the argument reports location
Node, data type Vector and basis Cartesian, and `polyhedralSurface()` returns a
PolyhedralSurfaceAdapter built from the mesh, with its connectivity, on the first call after each
change (nullptr while no mesh is loaded: no mesh is not an empty surface). Build that surface once
before reading it from several threads, since the first call fills a cache. The value definition is
a unitless quantity named "Coordinate" that the argument owns, and the argument registers the file
filters "UGRID NetCDF (*.nc)" and "Mesh packages (*.mesh.json)".

@snippet ch10_mesh_argument/main.cpp file

@snippet ch10_mesh_argument/main.cpp uri

@snippet ch10_mesh_argument/main.cpp from_output

@snippet ch10_mesh_argument/main.cpp refusals

```text
from file: loaded 1, PolyhedralSurface(nodes=5, faces=2); JSON {"@uri":"floodplain.nc"}; surface 2 patches, view 2 faces and 6 edges
data plane: shape {5, 3}, location Node 1, type Vector 1, node 4 = (5.0, 20.0, 2.0)
from @uri: 1, PolyhedralSurface(nodes=5, faces=2)
from an output: adopted 1, PolyhedralSurface(nodes=5, faces=2), 6 edges copied; serialize 0: "This mesh came from a component output and has no file to reference; the composition document keeps the @from binding instead"
inline payload 0: A mesh is never inlined into a document; pass {"@uri": "<mesh file>"} or bind the argument with @from
remote uri 0: Remote UGRID meshes ("https://example.org/mesh.nc") are not yet supported; pass a local file or bind the argument to a mesh-producing component
String input 0: Mesh arguments accept a UGRID file or an @uri JSON reference
setValuesFrom 0: A mesh argument is loaded through initialize(), not written one coordinate at a time
all checks passed
```

## Diagnostics {#sdk_space_diagnostics}

The messages the spatial classes return, in the `message` argument of the call named:

| Message | From | Cause | Fix |
| --- | --- | --- | --- |
| A mesh is never inlined into a document; pass {"@uri": "<mesh file>"} or bind the argument with \@from | `PolyhedralSurfaceArgument::initialize(JSON)` | The JSON has no non-empty \@uri member. | Reference a file, or bind the argument to an output in the composition. |
| Remote UGRID meshes ("uri") are not yet supported; pass a local file or bind the argument to a mesh-producing component | `PolyhedralSurfaceArgument::initialize(JSON)` | The URI has a scheme other than "file". | Download the mesh, or bind to a component. |
| Mesh arguments accept a UGRID file or an \@uri JSON reference | `PolyhedralSurfaceArgument::initialize(value, type)` | The input type is not File or JSON. | Pass File or JSON. |
| This build has no NetCDF support, so UGRID files cannot be read. | `PolyhedralSurfaceArgument::initialize(File)` | The SDK was built without NetCDF. | Rebuild with `USE_NETCDF` (@ref sdk_ref_cmake). |
| Source data item carries no polyhedral surface | `PolyhedralSurfaceArgument::initialize(item)` | The item is not an `IPolyhedralSurfaceComponentDataItem`. | Bind to a mesh output. |
| Source data item names no surface | `PolyhedralSurfaceArgument::initialize(item)` | The item's `polyhedralSurface()` is null. | Give the provider's item a surface before binding to it. |
| Source surface exposes no mesh view to copy from | `PolyhedralSurfaceArgument::initialize(item)` | The surface's mesh view is null or has no nodes. | Build the provider's surface with its MeshDefinition. |
| Mesh has N x coordinates and M y coordinates; Mesh has N nodes but M z coordinates; Mesh face offsets end at K but there are F face nodes | `setMesh()`, `initialize(item)` | The mesh's arrays disagree. | Fix the arrays; run `MeshDefinition::validate()`. |
| This mesh came from a component output and has no file to reference; the composition document keeps the \@from binding instead | `PolyhedralSurfaceArgument::serialize(JSON)` | The mesh has no source file. | Nothing; the composition keeps the binding. |
| Mesh arguments serialize as String or JSON | `PolyhedralSurfaceArgument::serialize()` | Another input type was asked for. | Ask for String or JSON. |
| A mesh argument is loaded through initialize(), not written one coordinate at a time | `PolyhedralSurfaceArgument::setValuesFrom()` | The data plane is read-only. | Use `initialize()` or `setMesh()`. |
| Provider is null; Provider data kind mismatch; Provider is not two-dimensional {entity, layer}; Provider has P layers, this input has L | `LayeredMeshInput::canConsume()` | The provider cannot feed the input. | Match the element type and the layer count, or insert an adapter. |
| No raster attached; Raster band read failed; Raster band write failed | `RasterComponentDataItem` | No raster, a GDAL read error, or a write to a read-only dataset. | Attach an open raster; writes are not supported through `Raster::open()`. |
| Failed to open raster file: path | `Raster::open()` | GDAL could not open the file, or no driver is registered. | Call `GDALAllRegister()`; check the path. |

The hyperslab calls also return the selection errors of the data plane when a `start`/`count` pair
or a buffer does not fit the item (@ref sdk_guide_ports). The connection validator's spatial findings
are listed in @ref sdk_space_crs "reference systems" and @ref sdk_space_layered "layered meshes".

Most accessors of the value types do not report errors: an index out of range returns a default
value. These are the defaults:

| Call | Out of range returns |
| --- | --- |
| `LineString::pointAt()`, `startPoint()`, `endPoint()` of an empty line, `PolyhedralSurface::vertex()` | The origin `Point()` |
| `Polygon::interiorRing()` | The exterior ring |
| `PolyhedralSurface::patch()`, `TIN::triangle()` | nullptr |
| `Network::vertex()`, `Network::edge()` | A static origin point; a static edge (-1, -1) |
| `Network::addEdge()`, `TIN::addTriangleByIndex()` | -1, nothing added |
| `RegularGrid2D/3D::nodeX()` and the like; `cellArea()`; `cellCenter()` | 0; 0; (0, 0) |
| `RegularGrid2D::rectilinear()`/`curvilinear()`, `RegularGrid3D::curvilinear()` with wrong array sizes | nullptr |
| Adapter accessors (`point()`, `interiorRing()`, `patch()`, `vertex()`, `edge()`) | nullptr |
| `RasterBand::getValue()` | 0 (the nodata value on a GDAL read error) |

## Known limitations {#sdk_space_limits}

Behavior of this release that differs from what the names promise, measured by running the code:

- **Polygon centroids are half their true value.** The ring centroid divides by twelve times the
  area instead of six, so `Polygon::centroid()` of a unit square at the origin is (0.25, 0.25). The
  error carries into `PolygonAdapter::centroid()` and `pointOnSurface()`,
  `PolyhedralSurface::centroid()` (and so TIN's) and `PolyhedralSurfaceAdapter::centroid()`. With
  holes the divisor is also the exterior area rather than the net area. The mesh view's `faceX()`
  and `faceY()` are computed separately and are correct; use them.
- **Hole orientation changes the area.** `Polygon::area()` subtracts signed hole areas, so a hole
  wound opposite to the exterior ring (the conventional clockwise hole) adds its area: a 4 × 4
  square with a clockwise 2 × 2 hole reports 20 instead of 12. Wind holes in the same direction as
  the exterior ring.
- **TIN interpolation returns the sum of the corner elevations.** The barycentric helper ignores the
  query point, so `findContainingTriangle()` reports all three weights as 1 and `interpolateZ()`
  returns \f$z_0 + z_1 + z_2\f$. Do not use either for interpolation.
- **Grid and raster envelopes are misplaced.** `RegularGrid2D::envelope()` and `Raster::envelope()`
  pass (minX, maxX, minY, maxY) to a constructor that expects (minX, minY, maxX, maxY): the result's
  minY holds the largest x and its maxX the smallest y. Compute the extent from `nodeXData()`/
  `nodeYData()` or `getGeoTransform()`.
- **WKT loses digits.** `getWKT()` writes six significant digits; projected coordinates are rounded
  (4512345.67 becomes 4.51235e+06). Use `getWKB()`.
- **Vector values have no Component dimension.** The layered and spatiotemporal items are rank 2
  (3 or 4 for grids) whatever `SpatialDataType` they declare.
- **No quad-edge navigation.** EdgeAdapter's navigation accessors return nullptr, which the
  interface does not allow.
- **`LineString::hasM()` is true for every non-empty line.**

## Related chapters

- @ref sdk_guide_ports — data items, shapes, the hyperslab calls, inputs and outputs.
- @ref sdk_guide_time — the time-series storage the spatiotemporal items use.
- @ref sdk_guide_workflow — where the connection validator runs.
- @ref sdk_guide_mesh_packages — importing meshes, mesh packages and their reference systems.
- @ref sdk_guide_record — writing UGRID and GeoPackage results and reopening them.
- @ref sdk_guide_tools — the meshing tools that produce MeshDefinitions.
