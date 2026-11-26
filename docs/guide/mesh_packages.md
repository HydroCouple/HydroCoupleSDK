@page sdk_guide_mesh_packages 14. Mesh packages

[TOC]

A two-dimensional surface model needs more than a mesh. It needs to know what the coordinates are
measured in and against what, whether the node elevations are a channel bed or the terrain, what
roughness and initial depth each cell has, which edges are boundaries and of what kind, and which
source file all of this came from. A *mesh package* holds all of it:
@ref HydroCouple::SDK::IO::MeshPackage in memory, and a pair of files on disk, a JSON manifest
(`site.mesh.json`) and a UGRID NetCDF file (`site.mesh.nc`) beside it.

This chapter covers importing a SWMM 2-D or SMS 2DM text mesh (`importSurfaceMesh()`), the rules a
package must satisfy, its spatial reference (@ref HydroCouple::SDK::IO::MeshPackageReference),
writing and reading packages (`writeMeshPackage()`, `readMeshPackage()`), reading UGRID files
directly (`readUGRIDMesh()`), and the CF vertical coordinate that makes a mesh file layered
(@ref HydroCouple::SDK::IO::VerticalCoordinate). The declarations are in `meshpackage.h`,
`meshdefinition.h` and `ugridreader.h` under `include/hydrocouplesdk/io/`, in the namespace
`HydroCouple::SDK::IO`.

**NetCDF is required to store and read packages.** Importing and validating work in every build.
Writing a package needs an SDK built with `USE_NETCDF=ON`; without it `writeMeshPackage()` returns
false with exactly the message

```text
mesh packages require an SDK built with NetCDF
```

and reading a package fails in its UGRID step with "This build has no NetCDF support, so UGRID
files cannot be read." `ugridReadSupported()` tells a program at run time which build it has. A
projected coordinate reference system additionally needs `USE_GDAL=ON`; without GDAL only the
horizontal reference "LOCAL" is accepted.

<pre class="mermaid">
flowchart LR
    src["site.2dm<br/>(SWMM 2-D sections or SMS cards)"]
    pkg["MeshPackage<br/>(meters, positive up)"]
    files["site.mesh.json + site.mesh.nc"]
    arg["mesh argument<br/>PolyhedralSurfaceArgument"]
    model["2-D model component"]
    src -- "importSurfaceMesh()" --> pkg
    pkg -- "writeMeshPackage()<br/>needs NetCDF" --> files
    files -- "readMeshPackage()<br/>fingerprint checked" --> pkg
    files -- "uri reference in a composition" --> arg --> model
</pre>

## What a package holds

| Member | Type | Holds |
| --- | --- | --- |
| `mesh` | @ref HydroCouple::SDK::IO::MeshDefinition | Node coordinates and elevations, faces, optional edges, the mesh name ("mesh2d" after import). |
| `horizontalCrs` | string | "LOCAL" for a local Cartesian domain, or a projected CRS in meters (GDAL). |
| `verticalDatum` | string | Name or code of the datum the elevations refer to. Required; "unknown" is refused. |
| `elevationKind` | string | "bed-elevation" or "terrain-elevation": what the node elevations are. |
| `nodeIds`, `faceIds` | strings, one per node and face | The source's identities, unique and non-empty. |
| `regionTags` | string per face | The source's region or material tag; may be empty. |
| `roughness` | double per face, or empty | The source's per-cell roughness, not converted. |
| `initialDepth` | double per face, or empty | Initial water depth in meters. |
| `boundaries` | @ref HydroCouple::SDK::IO::MeshBoundary list | Boundary edges, identified by their two end nodes. |
| `sourceSections` | JSON object | Source sections kept as provenance: header to rows of string tokens. |
| `provenance` | JSON object | Source file, its fingerprint, the format, the declared units and CRS, the source's headers. |

In memory, coordinates, elevations and depths are always meters, and elevations are positive up.
Units are converted once, at import. Source sections the SDK does not understand are kept as
provenance and never applied as physics: a solver that wants them reads them deliberately.

A @ref HydroCouple::SDK::IO::MeshBoundary has `node0` and `node1` (zero-based node indexes), a
`type` token such as "WALL" or "FLOW", an optional `group` name, and up to two `parameters` as
written in the source. Identifying an edge by its end nodes, not by a cell and a local edge number,
keeps the boundary correct when cells are reordered or a tool numbers local edges differently.

### Package rules

`MeshPackage::validate(message)` checks every rule below and reports the first one broken. Import,
write and read all call it, so a package that exists in memory, on disk or in a model has passed
it.

1. A horizontal CRS, a vertical datum other than "unknown", and an elevation kind of
   "bed-elevation" or "terrain-elevation" are declared. A CRS other than "LOCAL" must, with GDAL,
   be projected and in meters; without GDAL it is refused.
2. The mesh passes `MeshDefinition::validate()`: non-empty `nodeX` and `nodeY` of equal length,
   `nodeZ` empty or of the same length, face offsets starting at 0, ending at the number of face
   nodes, never decreasing, and every node index in range.
3. The mesh has faces and one elevation per node.
4. There is one node id per node, one face id and one region tag per face, and the roughness and
   initial depths are empty or have one value per face.
5. Node ids and face ids are non-empty and unique.
6. Coordinates and elevations are finite; roughness and depths are finite and not negative.
7. Every face has at least three nodes and no node twice; no edge is shared by more than two faces.
8. Every boundary names an edge of exactly one face (an exterior edge), no edge twice, and a
   non-empty type.
9. `sourceSections` and `provenance` are JSON objects.

Geometry quality beyond these rules (orientation, convexity, cell size) is the consuming solver's
to check.

## Importing a surface mesh

`importSurfaceMesh(path, options, package, message)` reads a text mesh, recognizes its grammar by
content (not by file extension), converts units, keeps the source's identities and unknown
sections, builds the boundaries, and validates the result. On failure `package` is untouched.

| @ref HydroCouple::SDK::IO::MeshImportOptions | Default | Meaning |
| --- | --- | --- |
| `horizontalCrs` | (required) | Becomes `MeshPackage::horizontalCrs`. With GDAL, a projected CRS is normalized to meters (its false easting and northing converted too) and stored as WKT; the CRS as given is kept in the provenance. |
| `verticalDatum` | (required) | Becomes `MeshPackage::verticalDatum`. |
| `elevationKind` | (required) | "bed-elevation" or "terrain-elevation". |
| `horizontalUnits` | "m" | Units of x and y in the file. |
| `verticalUnits` | "m" | Units of the node elevations. |
| `depthUnits` | "m" | Units of the cells' initial depths. |

Each unit is "m", "ft" (0.3048 m) or "us-survey-ft" (1200/3937 m); any other spelling fails the
import ("unsupported length unit 'yards'; use m, ft or us-survey-ft"). The reference declarations
are checked before the file is opened.

### The SWMM 2-D grammar

A SWMM 2-D file is a sequence of sections, each starting with a header in brackets:

| Section | Row | Notes |
| --- | --- | --- |
| `[2D_VERTICES]` | `x y z` | Nodes are numbered by row position from 0; node ids are those numbers. |
| `[2D_TRIANGLES]` | `n0 n1 n2 roughness [depth] [tag]` | |
| `[2D_QUADS]` | `n0 n1 n2 n3 roughness [depth] [tag]` | Quads are kept as quads, never split. |
| `[2D_BOUNDARY_CONDITIONS]` | `cell edge type [p1 [p2 [group]]]` | Becomes boundaries; also kept in `sourceSections`. |
| any other section | anything | Kept in `sourceSections`, row by row. |

The token after the roughness is a depth when it starts with a digit, a sign or a period, and a tag
otherwise; a depth may be followed by a tag. Faces are numbered with all triangles first, then all
quads, whatever order the file interleaves them in, and face ids are those numbers. In a boundary
row, `cell` is that face number and `edge` a local edge: local edge \f$e\f$ of a cell with \f$n\f$
nodes \f$v_0, \ldots, v_{n-1}\f$ joins \f$v_{(e+1) \bmod n}\f$ and \f$v_{(e+2) \bmod n}\f$. A
parameter "*" is kept as written; a group "*" means no group.

Section names are case-sensitive. A semicolon starts a comment unless it is inside double quotes,
which group a token with spaces. A line that begins with two semicolons and holds a colon is a
*header*: `;; UNITS: m` and `;; SOURCE_CRS: LOCAL` declare metadata that must agree with the
options (`UNITS` is checked against the horizontal and vertical units and may be "SI (m)", "m",
"metre", "meter", "metres", "meters", "ft", "feet", "us-survey-ft" or "US survey foot";
`SOURCE_CRS` must equal the declared CRS, or with GDAL describe the same CRS). Every header is kept
in `provenance.source_headers`, and a header repeated with a different value is refused.

### The SMS 2DM grammar

An SMS file is a sequence of cards: an optional `MESH2D`, `ND id x y z` for each node,
`E3T id n1 n2 n3 [tag]` and `E4Q id n1 n2 n3 n4 [tag]` for cells, which refer to nodes by id. Node
and cell ids are kept as the package's ids, so sparse numbering survives. SMS cells carry no
roughness or depth, so those vectors stay empty. Any other card (node strings, materials, boundary
cards) is refused ("unsupported SMS card 'NS'; export geometry plus explicit metadata"); a file that
mixes cards and bracketed sections is refused too.

### Example: importing site.2dm

The example `ch14_mesh_import` reads this SWMM 2-D file: six nodes, two channel triangles and a
floodplain quad, a wall on the west edge and an inflow on the east edge.

@include ch14_mesh_import/site.2dm

@snippet ch14_mesh_import/main.cpp import

@snippet ch14_mesh_import/main.cpp inspect

The quad `0 1 4 3` is face 2. Its local edge 2 joins its nodes at positions 3 and 0, which are
nodes 3 and 0: the west edge at x = 0, an exterior edge. Face 0, `1 2 5`, has local edge 0 between
nodes 2 and 5, the east edge at x = 20.

@snippet ch14_mesh_import/main.cpp sms

@snippet ch14_mesh_import/main.cpp units

The same import with the file authored in feet converts x and y with the international foot and z
with the US survey foot, as declared, and the depth with the international foot.

Copied from the ctest run of `docs.example.ch14_mesh_import`:

```text
SWMM-2D mesh "mesh2d": 6 nodes, 3 faces
  face 0 (channel): nodes 1 2 5, roughness 0.03, initial depth 0.5 m
  face 1 (channel): nodes 1 5 4, roughness 0.03, initial depth 0.5 m
  face 2 (floodplain): nodes 0 1 4 3, roughness 0.045, initial depth 0.2 m
  boundary WALL on edge 3-0, group west_bank, parameters * *
  boundary FLOW on edge 2-5, group east, parameters 1.5 *
  kept section [2D_BOUNDARY_CONDITIONS] (2 rows)
  kept section [2D_OPTIONS] (1 rows)
  fingerprint "fnv1a64:29647334ef416d26", headers {"SOURCE_CRS":"LOCAL","UNITS":"m"}
SMS-2DM: face ids 9 7, node ids 101 102 103 104 105, roughness values 0
feet: x 9.99744 m, z -1.9812 m, depth 0.3048 m
no vertical datum: declare horizontal CRS, vertical datum and elevation kind (bed-elevation or terrain-elevation)
a projected CRS: projected CRS validation requires a GDAL-enabled SDK; use LOCAL only for an explicitly local Cartesian domain
a header that disagrees: surface mesh line 1: source UNITS header disagrees with explicit XY/Z units
a repeated node: mesh face repeats a node
an unknown node: surface mesh line 6: cell references unknown node id
a negative depth: surface mesh line 6: negative roughness or depth
no roughness: surface mesh line 6: cell needs vertex IDs and SWMM roughness
an interior boundary edge: boundary must identify one unique exterior edge by endpoints
horizontal: LOCAL_CS["Local Cartesian mesh",LOCAL_DATUM["Local",0],UNIT["metre",1],AXIS["Easting",EAST],AXIS["Northing",NORTH]]
vertical:   VERT_CS["site-benchmark-1",VERT_DATUM["site-benchmark-1",2005],UNIT["metre",1],AXIS["Up",UP]]
codes 0/0
all checks passed
```

The refusals come from this table in the example; each file is written to the example's working
directory:

@snippet ch14_mesh_import/main.cpp refusals

Parse errors are reported as "surface mesh line N: ...". Checks that run after the whole file has
been read (unknown node ids, negative values, boundary rows) report the number of the file's last
line, not the line of the offending row, as the "unknown node" and "negative depth" lines above
show. Rule violations found by `validate()` and the reference checks carry no line number.

### Provenance

The import fills `provenance` with:

| Key | Value |
| --- | --- |
| `source` | the file name, without its directory |
| `source_fingerprint` | "fnv1a64:" and 16 hexadecimal digits: a 64-bit FNV-1a hash of the file's bytes, a content identity for reproducibility, not a security signature |
| `format` | "SWMM-2D" or "SMS-2DM" |
| `source_horizontal_units`, `source_vertical_units`, `source_depth_units` | the units the import was told |
| `source_horizontal_crs` | the CRS as given in the options |
| `source_headers` | every `;; KEY: value` header the file declared (key in upper case) |
| `quad_conversion` | "none": quads were not split |

## The package's spatial reference

@ref HydroCouple::SDK::IO::MeshPackageReference implements `HydroCouple::Spatial::ISpatialReferenceSystem`
for a package, so geometry adapters can attach the package's reference to the surfaces they build.
It owns its strings and reads only the package's CRS, datum and elevation kind; its constructor
throws `std::invalid_argument` when those fail rule 1.

| Accessor | "LOCAL" or no GDAL | Projected CRS with GDAL |
| --- | --- | --- |
| `srText()` | `LOCAL_CS["Local Cartesian mesh",...]` for "LOCAL" | the CRS's WKT |
| `authName()`, `authSRID()` | empty, 0 | the CRS's authority and code |
| `verticalSrText()` | `VERT_CS["DATUM",VERT_DATUM["DATUM",2005],UNIT["metre",1],AXIS["Up",UP]]` | the same, unless the datum is an "EPSG:" vertical CRS, whose WKT is used |
| `verticalAuthName()`, `verticalAuthSRID()` | empty, 0 | the vertical CRS's authority and code, for an EPSG datum |
| `distanceUnits()`, `verticalDistanceUnits()` | meters | meters |

A named local datum carries WKT but no fabricated authority code. With GDAL, an EPSG vertical CRS
that is not in meters is refused.

@snippet ch14_mesh_import/main.cpp reference

## Storing and reading a package

### writeMeshPackage()

`writeMeshPackage(manifest, package, message)` validates the package and writes two files: the
manifest at the given path and the UGRID file at the same path with its last extension replaced by
`.nc`, so `site.mesh.json` pairs with `site.mesh.nc`. It never overwrites: if either file exists,
it fails with "mesh package outputs already exist; choose a new destination". Missing directories
are created. If writing fails part way, the files this call created are removed. The UGRID file is
written by @ref HydroCouple::SDK::IO::NetCDFUGRIDWriter with no fields: the topology, coordinates
and elevations of the mesh, in the layout @ref sdk_guide_record describes.

The manifest holds everything else:

| Key | Value |
| --- | --- |
| `schema`, `version` | "hydrocouple-mesh-package", "1.0" |
| `mesh`, `mesh_name` | the UGRID file's name (relative to the manifest) and the mesh name in it |
| `geometry_fingerprint` | FNV-1a hash of the geometry (coordinates, elevations, faces, edges, mesh name) |
| `horizontal_crs`, `vertical_datum`, `elevation_kind` | the package's references |
| `horizontal_units`, `vertical_units`, `positive` | "m", "m", "up" |
| `node_ids`, `face_ids`, `region_tags` | the identities and tags |
| `roughness`, `initial_depth_m` | the per-face attributes (empty arrays when absent) |
| `boundaries` | objects with `nodes` (two indexes), `type`, `group`, `parameters` |
| `source_sections`, `provenance` | as in memory |

### readMeshPackage()

`readMeshPackage(manifest, package, message)` reads the manifest, requires its schema, version,
units and direction, reads the UGRID mesh named `mesh_name` from the file named `mesh` beside the
manifest, and recomputes the geometry fingerprint: a UGRID file that is not the one the manifest was
written with is refused ("mesh geometry fingerprint disagrees with manifest"). It then validates the
package. The output is atomic: `package` is assigned only when everything succeeded. A missing
manifest key fails with the JSON library's message after the prefix "mesh package: ".

@snippet ch14_mesh_package/main.cpp write

@snippet ch14_mesh_package/main.cpp read

@snippet ch14_mesh_package/main.cpp tamper

### Packages in a composition

A model receives a mesh through a mesh argument, `HydroCouple::SDK::Spatial::PolyhedralSurfaceArgument`
(@ref sdk_guide_space). Given a file whose extension is `.json`, the argument reads it as a mesh
package with `readMeshPackage()`, keeps the package (`meshPackage()`), and uses its mesh; any other
file is read as plain UGRID. In a composition document the payload is an \@uri reference, which
@ref sdk_guide_compositions describes, and a relative path resolves against the document:

```json
"arguments": { "mesh": { "@uri": "package/site.mesh.json" } }
```

@snippet ch14_mesh_package/main.cpp argument

A mesh is never inlined in a document; the argument refuses an inline payload, and a remote mesh
URI is refused as not yet supported. Serializing the argument writes the \@uri back.

The package files are the unit to move: keep the `.json` and `.nc` side by side, because the
manifest names the UGRID file by file name only.

## Reading UGRID files

`readUGRIDMesh(path, meshName, mesh, message)` reads one mesh of any UGRID NetCDF file into a
@ref HydroCouple::SDK::IO::MeshDefinition, driven by the conventions rather than by the SDK
writer's variable names, so files from other tools read too. `ugridMeshNames(path, message)` lists
a file's meshes (variables with `cf_role` "mesh_topology"), in file order; an empty `meshName`
takes the first.

- Coordinates come from the first two variables the topology's `node_coordinates` names.
  Elevations come from a `node_z` or `node_elevation` attribute, or else from a third
  `node_coordinates` entry.
- Connectivity comes from `face_node_connectivity` and `edge_node_connectivity`. A `start_index`
  of 1 is converted to 0 here, once; any value other than 0 or 1 is refused. Fill values
  (`_FillValue`, -1 when absent) pad faces narrower than the widest; padding must be trailing.
  When the topology's `face_dimension` or `edge_dimension` names the second dimension, the array is
  read transposed.
- The result must pass `MeshDefinition::validate()`.

Files written by @ref HydroCouple::SDK::IO::HDF5UGRIDWriter open too (the NetCDF library reads
HDF5), but their elevations are not found, because that writer lists only x and y in
`node_coordinates`.

## Vertical coordinates

A layered surface model describes the water column over each face with a CF *sigma coordinate*: a
dimensionless \f$\sigma\f$ from 0 at the water surface to -1 at the bed. With surface elevation
\f$\eta\f$ (positive up) and bed depth \f$h\f$ (positive down), the elevation of level \f$\sigma\f$
is the CF `ocean_sigma_coordinate` formula

\f[ z = \eta + \sigma\,(h + \eta). \f]

The topology stays two-dimensional, as UGRID intends; the vertical coordinate is a set of
variables over it. @ref HydroCouple::SDK::IO::VerticalCoordinate holds what `readVerticalCoordinate()`
reads:

| Member | Holds |
| --- | --- |
| `interfaceSigma` | sigma at the layer interfaces, `layerCount() + 1` values from 0 down to -1 |
| `layerSigma` | sigma at the layer centers |
| `depth` | bed depth per face, positive down |
| `surface` | water surface elevation per face at the time read, positive up |
| `surfaceVariable` | the name `formula_terms` gives for the surface |
| `timeCount` | how many times the surface variable holds; 0 when it has no time axis |
| `interfacesAreDerived` | true when the file held only layer centers and the interfaces were computed |

`NetCDFUGRIDWriter::setVerticalCoordinate(interfaceSigma, depth, surface)`, called before
`initialize()`, writes one: `MESH_layer_sigma` (standard name `ocean_sigma_coordinate`,
`formula_terms` "sigma: ... eta: ... depth: ...", `positive` "up", `bounds` naming the interfaces),
`MESH_interface_sigma`, `MESH_depth` and `MESH_eta` over (time, face). The layer centers are the
midpoints of consecutive interfaces. The surface may hold several time levels, one run of
face-count values per level; a length that is not a whole number of levels writes none. The file
is layered only when at least two interfaces and a depth are given, and the writer does not check
the values: give interfaces from 0 to -1 and one depth per face.

`readVerticalCoordinate(path, meshName, timeIndex, vertical, message)` finds the coordinate by what
it is: the first variable whose standard name is `ocean_sigma_coordinate` or `ocean_s_coordinate`
and that has `formula_terms`, whose `sigma`, `eta` and `depth` terms name the variables to read. The
interfaces come from a CF `bounds` variable of shape (layers, 2) when there is one; otherwise from
any one-dimensional variable one longer than the layers whose values fall from 0 to -1; otherwise
they are derived from the layer centers (midpoints, with 0 and -1 at the ends), which is exact for
evenly spaced layers, and `interfacesAreDerived` says so. The surface is the slice `timeIndex` of a
(time, face) variable, or the whole of a per-face one; an index past the last time takes the last,
so a viewer asking for the newest time of a file still being written gets the newest there is.
`meshName` is only checked to exist: the first sigma coordinate in the file is used whatever mesh it
belongs to. `hasVerticalCoordinate(path, meshName)` performs the same read for time 0 and answers
whether it succeeded, so a viewer can offer a layered view only when it applies.

@snippet ch14_mesh_package/main.cpp vertical

Copied from the ctest run of `docs.example.ch14_mesh_package`:

```text
wrote site.mesh.json
wrote site.mesh.nc
second write: mesh package outputs already exist; choose a new destination
read back 6 nodes, 3 faces, 2 boundaries: identical to what was written
site.mesh.nc holds mesh "mesh2d"
replaced UGRID file: mesh package: mesh geometry fingerprint disagrees with manifest
argument: PolyhedralSurface(nodes=6, faces=3), package datum site-benchmark-1
2 layers, layer sigma -0.25 and -0.75, 2 surface times, surface mesh2d_eta
face 0 at time 1: interfaces at 0.4 -0.85 -2.1 m
site.mesh.nc has a vertical coordinate: no
all checks passed
```

At time 1 the surface over face 0 is 0.4 m and the bed depth 2.1 m, so the column is 2.5 m deep and
the interfaces are at \f$0.4\f$, \f$0.4 - 0.5 \times 2.5 = -0.85\f$ and \f$0.4 - 2.5 = -2.1\f$ m. The
example's `REQUIRES` file names `USE_NETCDF`, so builds without NetCDF skip it.

## Diagnostics

### importSurfaceMesh()

| Message | Cause |
| --- | --- |
| declare horizontal CRS, vertical datum and elevation kind (bed-elevation or terrain-elevation) | An option is empty, the datum is "unknown", or the kind is misspelled. |
| projected CRS validation requires a GDAL-enabled SDK; use LOCAL only for an explicitly local Cartesian domain | A CRS other than "LOCAL" without GDAL. |
| mesh CRS must be projected; reproject geographic coordinates before import / declare a valid projected source CRS | With GDAL: a geographic or unparsable CRS. |
| mesh package coordinates are metres; its projected CRS must also use metres | With GDAL: validating a package whose CRS is in other units. |
| cannot open surface mesh: PATH | |
| surface mesh line 0: unsupported length unit 'U'; use m, ft or us-survey-ft | A unit option misspelled. |
| surface mesh line N: unrecognized mesh grammar/card 'X' | A first card that is neither a section nor ND, E3T, E4Q or MESH2D. |
| surface mesh line N: mixed SMS and SWMM grammars | |
| surface mesh line N: unsupported SMS card 'X'; export geometry plus explicit metadata | An SMS card other than ND, E3T, E4Q. |
| surface mesh line N: node needs x y z / duplicate node id | |
| surface mesh line N: cell needs vertex IDs and SWMM roughness | A cell row too short (SWMM cells need a roughness). |
| surface mesh line N: ambiguous cell attribute tail | A tag after a token that was already read as a tag. |
| surface mesh line N: expected integer, got 'X' / expected finite number, got 'X' | |
| surface mesh line N: stod: no conversion | A number field that does not start with a number (the standard library's message). |
| surface mesh line N: unterminated quoted token | |
| surface mesh line N: conflicting source metadata headers / unrecognized source UNITS header: X / source UNITS header disagrees with explicit XY/Z units / source CRS header disagrees with explicit CRS | Headers that disagree with each other or with the options. |
| surface mesh line N: negative roughness or depth / cell references unknown node id | Checked after reading; N is the last line. |
| surface mesh line N: boundary needs cell, edge and type / boundary cell out of range / boundary local edge out of range | |
| (any rule of MeshPackage::validate()) | See below. |

### MeshPackage::validate()

| Message | Rule |
| --- | --- |
| MeshDefinition: nodeX/nodeY must be non-empty and equal length; nodeZ must be empty or match nodeX length; faceNodeOffsets must start at 0 and end at faceNodes.size(); faceNodeOffsets must be non-decreasing; faceNodes index out of range; edgeNodes index out of range | 2 |
| mesh package needs faces and one declared elevation per node | 3 |
| mesh identities/tags do not match node and face counts; mesh attributes do not match face count | 4 |
| empty or duplicate source mesh identity | 5 |
| non-finite mesh coordinate/elevation; mesh roughness/depth must be finite and nonnegative | 6 |
| mesh face needs at least three nodes; mesh face repeats a node; non-manifold mesh edge | 7 |
| boundary must identify one unique exterior edge by endpoints | 8 |
| mesh provenance and source sections must be objects | 9 |

### writeMeshPackage() and readMeshPackage()

| Message | Cause |
| --- | --- |
| mesh packages require an SDK built with NetCDF | `writeMeshPackage()` in a build without USE_NETCDF. |
| mesh package outputs already exist; choose a new destination | The manifest or its `.nc` exists. |
| writing mesh package UGRID failed / writing mesh package manifest failed | |
| mesh package: cannot open mesh package manifest | |
| mesh package: unsupported mesh package schema/version | |
| mesh package: package coordinates must be metres, positive up | |
| mesh package: mesh geometry fingerprint disagrees with manifest | The UGRID file was replaced or edited. |
| mesh package: [json.exception...] | A missing or mistyped manifest key. |
| (a readUGRIDMesh() or validate() message) | |

### UGRID reader and vertical coordinates

| Message | Cause |
| --- | --- |
| This build has no NetCDF support, so UGRID files cannot be read. | SDK built without USE_NETCDF. |
| The file could not be opened as NetCDF: PATH (vertical: The file could not be opened: PATH) | |
| The file holds no UGRID mesh topology variable. (vertical: The file holds no UGRID mesh.) | |
| The file holds no mesh named X. | |
| The mesh does not name both node coordinate variables. | |
| The file names a coordinate variable it does not hold: X / X is not a one-dimensional coordinate variable. / The coordinates in X could not be read. | |
| The mesh's x and y coordinates are of different lengths. / Declared node elevations do not match coordinate count. | |
| The file names a connectivity variable it does not hold: X / X is not a two-dimensional connectivity variable. / The connectivity in X could not be read. | |
| X start_index must be zero or one. / X does not contain declared entity dimension D | |
| Invalid or non-trailing connectivity padding in face N | A fill value before a node index, or an index out of range. |
| The mesh holds no nodes. | |
| The file has no CF vertical coordinate; its mesh is two-dimensional. | Not an error for a 2-D file. |
| The vertical coordinate's formula_terms is missing one of sigma, eta or depth: TERMS | |
| The vertical coordinate has no layers. | |
| The vertical coordinate names a surface elevation variable the file does not hold: X | |
| X is neither a per-face nor a per-face-per-time variable. / X holds no data. / The surface elevations in X could not be read. | |
| The vertical coordinate has N depths but M surface elevations. | |
