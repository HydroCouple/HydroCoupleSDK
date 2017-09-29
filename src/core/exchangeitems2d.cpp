#include "stdafx.h"
#include "core/componentdataitem.h"
#include "core/exchangeitems2d.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "core/abstractcomponentdataitem.h"
#include <QDebug>
#include <assert.h>

using namespace HydroCouple;

Input2DInt::Input2DInt(const QString& id,
                       Dimension* iDimension, Dimension* jDimension,
                       int iLength, int jLength,
                       ValueDefinition* valueDefinition,
                       AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({iDimension,jDimension}),valueDefinition,modelComponent),
    ComponentDataItem2D<int>(id, iLength,jLength,valueDefinition->defaultValue().toInt())
{

}

Input2DInt::~Input2DInt()
{
}

int Input2DInt::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert((int)dimensionIndexes.size() < dimensions().length());

  if(dimensionIndexes.size() == 0)
  {
    return iLength();
  }
  else
  {
    return jLength();
  }
}

void Input2DInt::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem2D<int>::getValueT(dimensionIndexes,data);
}

void Input2DInt::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem2D<int>::setValueT(dimensionIndexes,data);
}


//==============================================================================================================================


Input2DDouble::Input2DDouble(const QString& id,
                             Dimension* iDimension, Dimension* jDimension,
                             int iDimensionLength, int jDimensionLength,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({iDimension,jDimension}),valueDefinition,modelComponent),
    ComponentDataItem2D<double>(id, iDimensionLength,jDimensionLength,valueDefinition->defaultValue().toInt())
{

}

Input2DDouble::~Input2DDouble()
{
}

int Input2DDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert((int)dimensionIndexes.size() < dimensions().length());

  if(dimensionIndexes.size() == 0)
  {
    return iLength();
  }
  else
  {
    //int iloc = dimensionIndexes[0];
    return jLength();
  }
}

void Input2DDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem2D<double>::getValueT(dimensionIndexes,data);
}

void Input2DDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem2D<double>::setValueT(dimensionIndexes,data);
}


//==============================================================================================================================


Input2DString::Input2DString(const QString& id,
                             Dimension* iDimension, Dimension* jDimension,
                             int iDimensionLength, int jDimensionLength,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({iDimension,jDimension}),valueDefinition,modelComponent),
    ComponentDataItem2D<QString>(id, iDimensionLength,jDimensionLength,valueDefinition->defaultValue().toString())
{

}

Input2DString::~Input2DString()
{
}

int Input2DString::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert((int)dimensionIndexes.size() < dimensions().length());

  if(dimensionIndexes.size() == 0)
  {
    return iLength();
  }
  else
  {
    //int iloc = dimensionIndexes[0];
    return jLength();
  }
}

void Input2DString::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem2D<QString>::getValueT(dimensionIndexes,data);
}

void Input2DString::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem2D<QString>::setValueT(dimensionIndexes,data);
}


