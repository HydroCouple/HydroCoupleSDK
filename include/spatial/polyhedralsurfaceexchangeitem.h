#ifndef POLYHEDRALSURFACEEXCHANGEITEM_H
#define POLYHEDRALSURFACEEXCHANGEITEM_H

#include "core/abstractoutput.h"
#include "hydrocouplespatial.h"


class HCPolyhedralSurface;
class HCTIN;

class HYDROCOUPLESDK_EXPORT HCPolyhedralSurfaceOutputDouble : public AbstractOutput,
    public virtual HydroCouple::Spatial::IPolyhedralSurfaceExchangeItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem
                 HydroCouple::Spatial::IPolyhedralSurfaceExchangeItem)

  public:

    HCPolyhedralSurfaceOutputDouble(const QString& id,
                                    HydroCouple::Spatial::PolyhedralSurfaceDataType dataType,
                                    HCPolyhedralSurface *polyhedralSurface,
                                    Dimension *patchDimension,
                                    Dimension *patchEdgeDimension,
                                    Dimension *edgeNodeDimension,
                                    ValueDefinition *valueDefinition,
                                    AbstractModelComponent *modelComponent);

    virtual ~HCPolyhedralSurfaceOutputDouble();

    HydroCouple::Spatial::PolyhedralSurfaceDataType polyhedralSurfaceDataType() const override;

    HydroCouple::Spatial::IPolyhedralSurface *polyhedralSurface() const override;

    virtual void setPolyhedralSurface(HCPolyhedralSurface *polyhedralSurface);

    HydroCouple::IDimension *patchDimension() const override;

    HydroCouple::IDimension *patchEdgeDimension() const override;

    HydroCouple::IDimension *edgeNodeDimension() const override;

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int patchDimensionIndex,int edgeDimensionIndex,
                  int nodeDimensionIndex, QVariant& data) const override;

    void getValues(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                   int patchStride, int edgeStride, int nodeStride, QVariant data[]) const override;

    void getValues(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                   int patchStride, int edgeStride, int nodeStride, void *data) const override;

    void setValue(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                  const QVariant &data) override;

    virtual void setValues(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                           int patchStride, int edgeStride, int nodeStride, const QVariant data[]) override;

    virtual void setValues(int patchDimensionIndex,int edgeDimensionIndex,int nodeDimensionIndex,
                           int patchStride, int edgeStride, int nodeStride, const void *data) override;

  private:
    HydroCouple::Spatial::PolyhedralSurfaceDataType m_dataType;
    Dimension *m_patchDimension, *m_patchEdgeDimension, *m_edgeNodeDimension;
    HCPolyhedralSurface *m_polyhedralSurface;
};


class HYDROCOUPLESDK_EXPORT HCTINOutputDouble : public AbstractOutput,
    public virtual HydroCouple::Spatial::ITINExchangeItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem
                 HydroCouple::Spatial::ITINExchangeItem)

  public:
    HCTINOutputDouble(const QString& id,
                      HydroCouple::Spatial::PolyhedralSurfaceDataType dataType,
                      HCTIN  *TINSurface,
                      Dimension *patchDimension,
                      Dimension *patchEdgeDimension,
                      Dimension *edgeNodeDimension,
                      ValueDefinition *valueDefinition,
                      AbstractModelComponent *modelComponent);

    virtual ~HCTINOutputDouble();

    HydroCouple::Spatial::ITIN *TIN() const override;

    HydroCouple::Spatial::PolyhedralSurfaceDataType polyhedralSurfaceDataType() const override;

    HydroCouple::Spatial::IPolyhedralSurface *polyhedralSurface() const override;

    virtual void setPolyhedralSurface(HCTIN *TINSurface) ;

    HydroCouple::IDimension *patchDimension() const override;

    HydroCouple::IDimension *patchEdgeDimension() const override;

    HydroCouple::IDimension *edgeNodeDimension() const override;

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int patchDimensionIndex,int edgeDimensionIndex,
                  int nodeDimensionIndex, QVariant& data) const override;

    void getValues(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                   int patchStride, int edgeStride, int nodeStride, QVariant data[]) const override;

    void getValues(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                   int patchStride, int edgeStride, int nodeStride, void *data) const override;

    void setValue(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                  const QVariant &data) override;

    virtual void setValues(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                           int patchStride, int edgeStride, int nodeStride, const QVariant data[]) override;

    virtual void setValues(int patchDimensionIndex,int edgeDimensionIndex,int nodeDimensionIndex,
                           int patchStride, int edgeStride, int nodeStride, const void *data) override;



  private:
    HydroCouple::Spatial::PolyhedralSurfaceDataType m_dataType;
    Dimension *m_patchDimension, *m_patchEdgeDimension, *m_edgeNodeDimension;
    HCTIN *m_TINSurface;
};

Q_DECLARE_METATYPE(HCPolyhedralSurfaceOutputDouble*)
Q_DECLARE_METATYPE(HCTINOutputDouble*)

#endif // POLYHEDRALSURFACEEXCHANGEITEM_H
