@page sdk_guide_tools 18. HydroCoupleTools

[TOC]

HydroCoupleTools is the HydroCouple SDK's mesh-generation library. A component author uses it to
build the grids a model computes on, and the meshes its exchange items are defined over: constrained
Delaunay triangulations of a domain with holes and breaklines, quad-dominant meshes made from them,
boundary-fitted curvilinear grids, terrain elevations sampled from survey points, and
terrain-following (sigma) layers extruded into three-dimensional prism and hexahedron meshes.

Every tool is a static function that takes plain arrays and returns plain arrays (no component, no
state), returns false with a message on bad input, and never throws. Horizontal meshes are
@ref HydroCouple::SDK::IO::MeshDefinition values, the UGRID-style node and face arrays the SDK's
writers and mesh data items use (@ref sdk_guide_space, @ref sdk_guide_record). The library is in the
namespace `HydroCouple::SDK::Tools`.

## Building and linking

| | |
| --- | --- |
| CMake option | `BUILD_TOOLS=ON` (off by default; @ref sdk_ref_cmake) |
| Target | `HydroCoupleTools` in the build tree, `HydroCoupleSDK::HydroCoupleTools` once installed; it links `HydroCoupleSDK` publicly |
| Headers | `hydrocouplesdk/tools/`: `sigmagrid.h`, `triangulator.h`, `quadmesher.h`, `curvilineargrid.h`, `terrainsampler.h`, `meshprogress.h` |
| Third-party code | MeshKit (Apache-2.0), the constrained Delaunay engine the OpenSWMM GUI meshes with, fetched at a pinned release tag (`MESHKIT_GIT_TAG`, v0.2.0) unless `MESHKIT_SOURCE_DIR` or a sibling `../MeshKit` checkout is used; `MESHKIT_REQUIRE_PINNED=ON` (used by CI) always fetches the tag. It is linked privately into the shared library, so consumers never need MeshKit; MeshKit's install rules put its LICENSE and NOTICE under `share/licenses/MeshKit`. |

A compiled example that uses the library names it in its `LIBRARIES` file and requires
`BUILD_TOOLS`, as both examples of this chapter do: `docs/examples/ch18_sigma_grid/` and
`docs/examples/ch18_meshing/`.

## How the tools fit together

<pre class="mermaid">
flowchart LR
    B[Boundary, holes, breaklines] --> T[Triangulator::triangulate]
    T --> M[MeshDefinition: triangles]
    M --> Q[QuadMesher::triToQuadDominant]
    Q --> MQ[MeshDefinition: quads and triangles]
    C[Four boundary curves] --> TF[CurvilinearGridGenerator::transfinite]
    TF --> W[winslowSmooth]
    W --> SG[StructuredGrid2D]
    SG --> MD[toMeshDefinition: quads]
    SG --> E3[extrude: StructuredGrid3D, flat levels]
    S[Survey points] --> I[TerrainSampler::idw]
    M --> I
    I --> BED[bed elevation per node]
    BED --> BUILD[SigmaGridGenerator::build]
    L[SigmaLayering] --> BUILD
    BUILD --> GRID[SigmaGrid]
    GRID --> EX[SigmaGridGenerator::extrude]
    M --> EX
    EX --> LM[LayeredMesh: prisms or hexahedra]
</pre>

## Terrain-following sigma grids

A sigma grid divides every water column into the same number of layers, whose interfaces follow the
water surface and the bed. With \f$\sigma\f$ running from 0 at the surface to 1 at the bed, a column
with surface elevation \f$\eta\f$ and bed elevation \f$z_b\f$, of depth \f$D = \eta - z_b\f$, has its
nominal interfaces at

\f[ z_k = \eta - \sigma_k D, \qquad k = 0, \dots, L, \f]

where \f$\sigma_0 = 0 < \sigma_1 < \dots < \sigma_L = 1\f$ is a distribution shared by all columns.
Elevations are in whatever length unit the caller uses (meters in the examples).

### Distributions: SigmaLayering

@ref HydroCouple::SDK::Tools::SigmaLayering makes and checks distributions:

- `uniform(L)`: \f$\sigma_k = k / L\f$, equal layers.
- `stretched(L, theta, b)`: the Song–Haidvogel-style blend, with \f$s = k/L\f$,
  \f[ C(s) = (1 - b)\,\frac{\sinh(\theta s)}{\sinh\theta}
      + b\,\frac{\tanh\bigl(\theta(s - \tfrac12)\bigr) + \tanh(\theta/2)}{2\tanh(\theta/2)}, \f]
  with the first and last values set to exactly 0 and 1. \f$\theta > 0\f$ sets the strength of the
  clustering; \f$\theta \le 0\f$ returns `uniform(L)`. \f$b\f$ is clamped to [0, 1]. With
  \f$b = 0\f$ the layers are thinnest at the surface and grow toward the bed. With \f$b = 1\f$ the
  profile is the symmetric tanh, \f$C(s) + C(1 - s) = 1\f$, which thins the layers at the surface
  and at the bed equally and thickens them mid-depth. Values between blend the two.
- `validate(interfaces, message)`: at least two values, the first exactly 0, the last exactly 1,
  strictly increasing.

Both makers return an empty vector for \f$L < 1\f$. The example prints the layer thicknesses (as
fractions of the depth, surface layer first) of four distributions:

@snippet ch18_sigma_grid/main.cpp layering

```text
uniform(6)             0.1667 0.1667 0.1667 0.1667 0.1667 0.1667
stretched(6, 3, 0)     0.0520 0.0653 0.0952 0.1495 0.2419 0.3961
stretched(6, 3, 1)     0.0793 0.1654 0.2553 0.2553 0.1654 0.0793
stretched(6, 3, 0.5)   0.0657 0.1154 0.1753 0.2024 0.2037 0.2377
validate {0, 0.6, 0.4, 1}: sigma distribution must be strictly increasing (violated at interface 2)
```

### Building the grid

@ref HydroCouple::SDK::Tools::SigmaGridGenerator::build maps a distribution onto per-column bed and
surface elevations. Each column's layer thicknesses start as \f$(\sigma_{k+1} - \sigma_k) D\f$.
With a minimum thickness \f$h_{min} > 0\f$:

- a column with \f$D \ge L\,h_{min}\f$ lifts every layer thinner than \f$h_{min}\f$ to \f$h_{min}\f$
  and takes the added depth from the other layers in proportion to how much each exceeds
  \f$h_{min}\f$, so the column's depth is kept exactly and no layer falls below the minimum;
- a column with \f$D < L\,h_{min}\f$ cannot hold \f$L\f$ such layers and collapses to equal layers of
  \f$D / L\f$.

Interfaces are accumulated from the surface down, and the last one is set to the bed to absorb
rounding, so `z(column, 0)` is the surface and `z(column, L)` the bed, exactly. The result is a
@ref HydroCouple::SDK::Tools::SigmaGrid, which holds `columnCount`, `layerCount` and `interfaceZ`, stored
column by column, `interfaceZ[column * (L + 1) + k]`, with the accessors `z(column, k)` and
`thickness(column, k)`. A column whose surface is not above its bed fails the whole build.

The example triangulates a 100 m × 60 m basin, samples its bed from five survey points (see
@ref HydroCouple::SDK::Tools::TerrainSampler below), and builds five stretched layers with a
0.5 m minimum:

@snippet ch18_sigma_grid/main.cpp mesh

@snippet ch18_sigma_grid/main.cpp bed

@snippet ch18_sigma_grid/main.cpp build

The deepest column keeps the stretched profile with its thin surface layer lifted to the minimum;
the shallowest, 1 m deep, cannot hold five 0.5 m layers and collapses to five of 0.2 m:

```text
triangulation: 19 nodes, 20 triangles
column   8, bed -9.000 m: thicknesses 0.572 0.784 1.287 2.268 4.089
column   0, bed -1.000 m: thicknesses 0.200 0.200 0.200 0.200 0.200
```

### Extruding a horizontal mesh

@ref HydroCouple::SDK::Tools::SigmaGridGenerator::extrude turns a horizontal mesh into a layered one,
given a sigma grid with one column per horizontal node. Every face becomes one cell per layer: a
triangle a six-node prism, a quadrilateral an eight-node hexahedron, any polygon the matching prism.
The result is a @ref HydroCouple::SDK::Tools::LayeredMesh, with these members:

| Member | Content |
| --- | --- |
| `layerCount`, `nodesPerLayer` | L and the horizontal node count. |
| `nodeX`, `nodeY`, `nodeZ` | (L + 1) × nodesPerLayer nodes, interface by interface: node n of interface k has index `k * nodesPerLayer + n`, the horizontal node's x and y and the grid's `z(n, k)`. |
| `cellNodeOffsets`, `cellNodes` | Cells layer by layer, over the horizontal faces in order: cell (k, f) lists face f's nodes on interface k, then the same nodes, in the same order, on interface k + 1. |
| `nodeCount()`, `cellCount()` | The totals. |

The horizontal mesh's own `nodeZ` is ignored. The mesh must pass `MeshDefinition::validate()` and
have faces. A `LayeredMesh` is plain arrays: no SDK writer takes one directly, so a caller writing
it copies the arrays into the structure its writer expects.

@snippet ch18_sigma_grid/main.cpp extrude

```text
layered mesh: 114 nodes, 100 prisms (5 layers x 20 triangles)
dry column: column 1 has non-positive depth (surface 0.000000, bed 0.500000)
```

@snippet ch18_sigma_grid/main.cpp errors

## Constrained Delaunay triangulation

@ref HydroCouple::SDK::Tools::Triangulator::triangulate triangulates a polygonal domain described by a
@ref HydroCouple::SDK::Tools::TriangulationInput, whose fields are:

| Field | Meaning |
| --- | --- |
| `boundary` | The outer ring, counter-clockwise, first point not repeated; at least 3 points. Required. |
| `holes` | Rings (any orientation, at least 3 points each) whose inside is removed. |
| `constraintSegments` | Segments the triangulation must contain as edges: breaklines, channel banks, levee crests. Segments that cross each other or a ring are split at the crossing, which becomes a node. |
| `interiorPoints` | Extra nodes inside the domain, the way to control element size away from the boundary. |
| `maxEdgeLength` | When positive, every ring edge and constraint segment longer than this is split into `ceil(length / maxEdgeLength)` equal parts before triangulating. |

The output is a `MeshDefinition` named "triangulation" of counter-clockwise triangles. Its nodes are
the distinct input points in first-occurrence order (the boundary with its densification points,
the holes, the constraint segments, the interior points), followed by any crossing points; it has no
`nodeZ`. A triangle is kept when its centroid is inside the boundary and outside every hole; because
the rings are constraints, no kept triangle straddles one. Densification never adds interior points:
the triangulation is not refined by quality, so size the interior with `interiorPoints`.

The meshing example triangulates a 40 m × 20 m floodplain with a building as a hole and a levee
crest as a breakline, and checks the Euler characteristic of a domain with one hole,
\f$V - E + F = 1 - 1 = 0\f$:

@snippet ch18_meshing/main.cpp triangulate

## Quad-dominant meshes

@ref HydroCouple::SDK::Tools::QuadMesher::triToQuadDominant turns a triangle mesh into a mixed mesh by
merging pairs of adjacent triangles into quadrilaterals, best pairs first. Each interior edge shared
by exactly two triangles with consistent winding is a candidate; the quad it would make is scored by
@ref HydroCouple::SDK::Tools::QuadMesher::quadQuality,

\f[ q = \max\Bigl(0,\; 1 - \frac{\max_{\text{corners}} |\phi - 90^\circ|}{90^\circ}\Bigr), \f]

which is 1 for a rectangle and 0 for a non-convex, degenerate or clockwise quad. Candidates of
quality at least `minQuality` (clamped to [0, 1]) are taken in decreasing quality, each triangle at
most once. The output lists the quads first, then the triangles left over in their input order, and
copies the nodes unchanged, so every merge replaces exactly two triangles. Pairs of equal quality are
taken in the order of a hash map, so on a regular grid the pairs chosen can differ between standard
libraries. Node indexes must be below \f$2^{32}\f$.

@snippet ch18_meshing/main.cpp quads

## Boundary-fitted curvilinear grids

@ref HydroCouple::SDK::Tools::CurvilinearGridGenerator builds structured grids of \f$n_i \times n_j\f$
nodes (a @ref HydroCouple::SDK::Tools::StructuredGrid2D, node `j * ni + i`) fitted to four boundary
curves.

- **`transfinite(south, north, west, east, grid, message)`** blends the curves: with
  \f$\xi = i/(n_i - 1)\f$ and \f$\eta = j/(n_j - 1)\f$,
  \f[ \mathbf{x}(\xi,\eta) = (1-\eta)\,\mathbf{S}(\xi) + \eta\,\mathbf{N}(\xi)
      + (1-\xi)\,\mathbf{W}(\eta) + \xi\,\mathbf{E}(\eta) - \mathbf{B}(\xi,\eta), \f]
  where \f$\mathbf{B}\f$ is the bilinear blend of the four corners. South and north have \f$n_i\f$
  points running west to east, west and east \f$n_j\f$ points running south to north, and the ends
  must meet within \f$10^{-9}\f$. Point i of a curve sits at \f$\xi_i\f$, so the boundary spacing
  carries into the interior.
- **`winslowSmooth(grid, iterations)`** relaxes the interior nodes toward the solution of the
  Winslow elliptic equations, keeping the boundary fixed. Each Gauss–Seidel sweep sets
  \f[ x_{i,j} \leftarrow \frac{\alpha\,(x_{i+1,j} + x_{i-1,j}) + \gamma\,(x_{i,j+1} + x_{i,j-1})
      - \tfrac12 \beta\,(x_{i+1,j+1} - x_{i-1,j+1} - x_{i+1,j-1} + x_{i-1,j-1})}{2(\alpha + \gamma)} \f]
  (and likewise for y), with \f$\alpha = x_\eta^2 + y_\eta^2\f$, \f$\beta = x_\xi x_\eta + y_\xi y_\eta\f$,
  \f$\gamma = x_\xi^2 + y_\xi^2\f$ from central differences. It returns the largest node movement
  of the last sweep, the convergence monitor.
- **`minCornerJacobian(grid)`** is the smallest cross product of the two edges at any corner of any
  cell; it is positive exactly when every cell is convex and counter-clockwise.
  **`maxOrthogonalityDeviation(grid)`** is the largest deviation, in degrees, of the angle between
  the ξ and η directions from 90° at the interior nodes.
- **`toMeshDefinition(grid, name)`** emits the grid as a quad mesh with the grid's node order;
  each quad is counter-clockwise when ξ runs east and η north.
- **`extrude(grid, zLevels, volume, message)`** replicates every node at each of the given levels
  (strictly increasing or strictly decreasing) into a @ref HydroCouple::SDK::Tools::StructuredGrid3D,
  node `(k * nj + j) * ni + i`. The levels are flat; grade them with a sigma distribution mapped onto
  an elevation range, and use `SigmaGridGenerator` when the layers must follow terrain.

The example fits a quarter-annulus channel bend, disturbs its interior, and smooths it:

@snippet ch18_meshing/main.cpp curvilinear

```text
transfinite: min corner Jacobian 2.6105 m2, max skew 0.000 deg
disturbed: max skew 25.734 deg; after 300 Winslow sweeps 8.736 deg (last move 3.55e-15 m)
bend mesh: 78 nodes, 60 quads
```

The transfinite grid of an annulus is already orthogonal. Winslow smoothing converges (the last
sweep moves nodes by about \f$10^{-15}\f$ m) to the solution of its own equations, which for this
boundary spacing is not the orthogonal polar grid: smoothing repairs the disturbance but does not
restore the original grid, so check the quality measures after smoothing rather than assume them.

## Terrain sampling

@ref HydroCouple::SDK::Tools::TerrainSampler::idw interpolates elevations from scattered
\f$(x, y, z)\f$ samples by inverse-distance weighting:

\f[ z(q) = \frac{\sum_i w_i z_i}{\sum_i w_i}, \qquad w_i = d(q, p_i)^{-\text{power}}, \f]

over the samples within `searchRadius` of the query (every sample when the radius is 0). A query
exactly on a sample takes that sample's value. `power` must be positive (2 is usual), the query and
output arrays must have one length, and a query with no sample in range fails the call, after the
earlier queries have been written. Every query visits every sample, so the cost is queries ×
samples.

## Watching and stopping a run

The triangulator and the quad mesher each have an overload that takes a
@ref HydroCouple::SDK::Tools::MeshProgress callback, `bool(double fraction, const char *stage)`. The
tools call it at each phase, and inside their own loops every 4,096 items; MeshKit's vertex insertion
calls back between batches of vertices. `fraction` is how far through the phases the run is, not a
measure of work, so show the stage beside any bar drawn from it. The callback runs on the calling
thread.

| Tool | Stages, with their fractions |
| --- | --- |
| Triangulator | densifying (0.00, 0.05 after each hole, 0.10 after each segment), inserting vertices (0.20 to 0.50), inserting edges (0.50 to 0.80), removing holes (0.80), extracting (0.90 to 1.00), done (1.00) |
| QuadMesher | indexing edges (0.00), scoring pairs (0.30 to 0.60), merging (0.60 to 0.90), copying triangles (0.90), done (1.00) |

Returning false abandons the run: the tool returns false with the message "cancelled" and leaves its
output empty, even if the output held a mesh before the call. The last call, (1.00, "done"), is a
report: its return value is ignored and the finished mesh is kept. An empty callback is the
unwatched call, and a callback that always returns true does not change the mesh.

@snippet ch18_meshing/main.cpp progress

```text
triangulation: 40 nodes, 46 triangles, V - E + F = 0
quad-dominant: 14 quads + 18 triangles from 46 triangles
quality: unit square 1.000, 4 x 2 rhombus 0.590, same rhombus clockwise 0.000
  0.00 densifying
  0.20 inserting vertices
  0.50 inserting edges
  0.80 removing holes
  0.90 extracting
  1.00 done
stopped at "inserting edges": returned false, message "cancelled", 0 faces left
```

## Diagnostics

| Message | Raised by | Cause |
| --- | --- | --- |
| `sigma distribution needs at least 2 interfaces` | `SigmaLayering::validate`, `build` | Fewer than two values. |
| `sigma distribution must span [0, 1] exactly` | `validate`, `build` | The first value is not 0 or the last not 1. |
| `sigma distribution must be strictly increasing (violated at interface K)` | `validate`, `build` | A value not above its predecessor. |
| `bed and surface elevation counts differ` | `SigmaGridGenerator::build` | The two arrays have different lengths. |
| `minimum layer thickness must be non-negative` | `build` | A negative minimum. |
| `column C has non-positive depth (surface S, bed B)` | `build` | A dry or inverted column. |
| `horizontal mesh has no faces to extrude` | `SigmaGridGenerator::extrude` | A node-only mesh. |
| `sigma grid has C columns; horizontal mesh has N nodes` | `extrude` | The grid was built for another mesh. |
| `boundary ring needs at least 3 points`, `hole ring needs at least 3 points` | `Triangulator::triangulate` | A degenerate ring. |
| `triangulation failed: ...` | `triangulate` | MeshKit's reason, or a constraint (with its end points) that could not be recovered. |
| `triangulation produced no triangles` | `triangulate` | Every triangle fell outside the boundary or inside a hole. |
| `input mesh has no faces`, `face F is not a triangle` | `QuadMesher::triToQuadDominant` | The input is not a triangle mesh. |
| `cancelled` | `triangulate`, `triToQuadDominant` | The progress callback returned false. |
| `boundary curves need at least 2 points each` | `CurvilinearGridGenerator::transfinite` | A curve too short. |
| `opposite boundary curves must have equal point counts` | `transfinite` | South and north, or west and east, differ in length. |
| `boundary curves do not meet at consistent corners` | `transfinite` | An end mismatch above \f$10^{-9}\f$. |
| `2-D grid is empty`, `extrusion needs at least 2 z-levels`, `z-levels must be strictly monotone` | `CurvilinearGridGenerator::extrude` | Bad grid or levels. |
| `no samples`, `IDW power must be positive`, `query and output extents differ`, `query Q has no sample within the search radius` | `TerrainSampler::idw` | Bad samples, exponent, array lengths, or radius. |
| A `MeshDefinition::validate()` message | `extrude`, `triToQuadDominant` | Inconsistent node or face arrays. |

## What to read next

@ref sdk_guide_space describes the mesh data items these meshes define, @ref sdk_guide_mesh_packages
how meshes are imported and packaged, and @ref sdk_guide_record the UGRID writers.
