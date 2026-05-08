// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   hydrocoupleforward.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Forward declarations for all HydroCoupleSDK concrete classes,
 *         organised by namespace.  Include this header to break include
 *         cycles without pulling in full class definitions.
 */

#ifndef HYDROCOUPLESDK_FORWARD_H
#define HYDROCOUPLESDK_FORWARD_H

// ── namespace HydroCouple ─────────────────────────────────────────────────
namespace HydroCouple {

class Description;
class Identity;
class ComponentInfo;
class ModelComponentInfo;
class ModelComponent;
class CloneableModelComponent;
class WorkflowComponent;
class WorkflowComponentInfo;
class WorkflowComponentStatusChangeEventArgs;

class ValueDefinition;
class Quality;
class Quantity;
class Unit;
class UnitDimensions;
class Dimension;

template<typename T> class ComponentDataItem1D;
template<typename T> class ComponentDataItem2D;
template<typename T> class IdBasedComponentDataItem;

template<typename T> class Argument1D;
template<typename T> class IdBasedArgument;

class AbstractExchangeItem;
class AbstractInput;
class AbstractMultiInput;
class AbstractOutput;
class AbstractAdaptedOutput;
class AbstractAdaptedOutputFactory;
class AbstractAdaptedOutputFactoryComponent;
class AbstractAdaptedOutputFactoryComponentInfo;

template<typename T> class Input1D;
template<typename T> class Output1D;
template<typename T> class Input2D;
template<typename T> class Output2D;
template<typename T> class IdBasedInput;
template<typename T> class IdBasedOutput;
template<typename T> class IdBasedMultiInput;

class ComponentStatusChangeEventArgs;
class ComponentDataItemValueChanged;
class ExchangeItemChangeEventArgs;

class DataCursor;
class ProgressChecker;

} // namespace HydroCouple


// ── namespace HydroCouple::Temporal ──────────────────────────────────────
namespace HydroCouple::Temporal {

class DateTime;
class TimeSpan;
class TimeSeries;
class TimeModelComponent;

template<typename T> class TimeSeriesComponentDataItem;
template<typename T> class TimeIdBasedComponentDataItem;
template<typename T> class TimeSeriesArgument;
template<typename T> class TimeSeriesInput;
template<typename T> class TimeSeriesOutput;
template<typename T> class TimeSeriesMultiInput;
template<typename T> class TimeIdBasedInput;
template<typename T> class TimeIdBasedOutput;
template<typename T> class TimeIdBasedMultiInput;

class TimeSeriesInterpolationAdaptedOutput;
class TemporalInterpolationFactory;

} // namespace HydroCouple::Temporal


// ── namespace HydroCouple::Spatial ───────────────────────────────────────
namespace HydroCouple::Spatial {

// Geometry types
class Geometry;
class Point;
class Vertex;
class LineString;
class Line;
class LinearRing;
class Polygon;
class Triangle;
class GeometryCollection;
class MultiPoint;
class MultiLineString;
class MultiPolygon;
class MultiCurve;
class MultiSurface;
class Envelope;
class SpatialReferenceSystem;

// Mesh / topology
class QuadEdgePool;
class VertexPool;
class FacePool;
class Edge;
class PolyhedralSurface;
class TIN;

// Network
class Network;

// Grids and rasters
class RegularGrid2D;
class RegularGrid3D;
class Raster;
class RasterBand;

// Spatial index
class Octree;

// Geometry factory
class GeometryFactory;

// Component data items
template<typename T> class GeometryComponentDataItem;
template<typename T> class NetworkComponentDataItem;
template<typename T> class PolyhedralSurfaceComponentDataItem;
template<typename T> class TINComponentDataItem;
template<typename T> class RasterComponentDataItem;
template<typename T> class RegularGrid2DComponentDataItem;
template<typename T> class RegularGrid3DComponentDataItem;

// Arguments and exchange items
class GeometryArgumentDouble;
class TINArgumentDouble;
class GeometryOutputDouble;
class GeometryInputDouble;
class GeometryMultiInputDouble;
class TINOutputDouble;
class TINInputDouble;

} // namespace HydroCouple::Spatial


// ── namespace HydroCouple::SpatioTemporal ────────────────────────────────
namespace HydroCouple::SpatioTemporal {

template<typename T> class TimeGeometryComponentDataItem;
template<typename T> class TimeNetworkComponentDataItem;
template<typename T> class TimeSeriesPolyhedralSurfaceComponentDataItem;
template<typename T> class TimeSeriesTINComponentDataItem;
template<typename T> class TimeSeriesRasterComponentDataItem;
template<typename T> class TimeRegularGrid2DComponentDataItem;
template<typename T> class TimeRegularGrid3DComponentDataItem;

class TimeGeometryOutputDouble;
class TimeGeometryInputDouble;
class TimeGeometryMultiInputDouble;
class TimeTINOutputDouble;
class TimeTINInputDouble;
class TimeTINMultiInputDouble;
class TimeGeometryInterpolationAdaptedOutput;
class TimeTINInterpolationAdaptedOutput;

} // namespace HydroCouple::SpatioTemporal


// ── namespace HydroCouple::IO::NetCDF ────────────────────────────────────
namespace HydroCouple::IO::NetCDF {

class ThreadSafeNcFile;
class ThreadSafeNcGroup;
class ThreadSafeNcVar;
class ThreadSafeNcDim;
class ThreadSafeNcAtt;
class UGRIDMeshWriter;
class UGRIDMeshReader;

} // namespace HydroCouple::IO::NetCDF


// ── namespace HydroCouple::IO::HDF5 ──────────────────────────────────────
namespace HydroCouple::IO::HDF5 {

class ThreadSafeH5File;
class ThreadSafeH5Group;
class ThreadSafeH5Dataset;
class ThreadSafeH5Attribute;
class UGRIDMeshH5Writer;
class UGRIDMeshH5Reader;

} // namespace HydroCouple::IO::HDF5


// ── namespace HydroCouple::Tools ─────────────────────────────────────────
namespace HydroCouple::Tools {

class TerrainDomain;
struct ConstraintSegment;
struct RegionMarker;

} // namespace HydroCouple::Tools


// ── namespace HydroCouple::Tools::Mesh ───────────────────────────────────
namespace HydroCouple::Tools::Mesh {

struct MeshVertex;
struct MeshElement;
struct MeshEdge;
struct MeshResult;
class IMeshGenerator;
class DelaunayMeshGenerator;
class RectilinearGrid2DGenerator;
class RectilinearGrid3DGenerator;
class CurvilinearGrid2DGenerator;
class CurvilinearGrid3DGenerator;

} // namespace HydroCouple::Tools::Mesh


// ── namespace HydroCouple::Tools::Interpolation ──────────────────────────
namespace HydroCouple::Tools::Interpolation {

class MeshToMeshInterpolator;
class GridToMeshInterpolator;
class MeshToGridInterpolator;
class GridToGridInterpolator;

} // namespace HydroCouple::Tools::Interpolation


// ── namespace HydroCouple::Tools::Terrain ────────────────────────────────
namespace HydroCouple::Tools::Terrain {

class TerrainSampler;
class TerrainThinner;

} // namespace HydroCouple::Tools::Terrain


// ── namespace HydroCouple::SDK (MPI layer) ───────────────────────────────
namespace HydroCouple::SDK {

class ComponentWorker;
class ProxyModelComponent;
class ProxyOutput;
class ProxyInput;

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_FORWARD_H
