#include "stdafx.h"
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
    ComponentDataItem2D<int>(iLength,jLength,valueDefinition->defaultValue().toInt())
{

}

Input2DInt::~Input2DInt()
{
}

int Input2DInt::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength < dimensions().length());

  if(dimensionIndexesLength == 0)
  {
    return iLength();
  }
  else
  {
    return jLength();
  }
}

void Input2DInt::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem2D<int>::getValueT(dimensionIndexes,data);
}

void Input2DInt::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem2D<int>::getValuesT(dimensionIndexes,stride,data);
}

void Input2DInt::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem2D<int>::getValuesT(dimensionIndexes,stride,data);
}

void Input2DInt::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem2D<int>::setValueT(dimensionIndexes,data);
}

void Input2DInt::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem2D<int>::setValuesT(dimensionIndexes,stride,data);
}

void Input2DInt::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem2D<int>::setValuesT(dimensionIndexes,stride,data);
}


//==============================================================================================================================


Input2DDouble::Input2DDouble(const QString& id,
                             Dimension* iDimension, Dimension* jDimension,
                             int iDimensionLength, int jDimensionLength,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({iDimension,jDimension}),valueDefinition,modelComponent),
    ComponentDataItem2D<double>(iDimensionLength,jDimensionLength,valueDefinition->defaultValue().toInt())
{

}

Input2DDouble::~Input2DDouble()
{
}

int Input2DDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength < dimensions().length());

  if(dimensionIndexesLength == 0)
  {
    return iLength();
  }
  else
  {
    //int iloc = dimensionIndexes[0];
    return jLength();
  }
}

void Input2DDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem2D<double>::getValueT(dimensionIndexes,data);
}

void Input2DDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void Input2DDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void Input2DDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem2D<double>::setValueT(dimensionIndexes,data);
}

void Input2DDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

void Input2DDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

//==============================================================================================================================


Input2DString::Input2DString(const QString& id,
                             Dimension* iDimension, Dimension* jDimension,
                             int iDimensionLength, int jDimensionLength,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({iDimension,jDimension}),valueDefinition,modelComponent),
    ComponentDataItem2D<QString>(iDimensionLength,jDimensionLength,valueDefinition->defaultValue().toString())
{

}

Input2DString::~Input2DString()
{
}

int Input2DString::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength < dimensions().length());

  if(dimensionIndexesLength == 0)
  {
    return iLength();
  }
  else
  {
    // int iloc = dimensionIndexes[0];
    return jLength();
  }
}

void Input2DString::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem2D<QString>::getValueT(dimensionIndexes,data);
}

void Input2DString::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem2D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void Input2DString::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem2D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void Input2DString::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem2D<QString>::setValueT(dimensionIndexes,data);
}

void Input2DString::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem2D<QString>::setValuesT(dimensionIndexes,stride,data);
}

void Input2DString::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem2D<QString>::setValuesT(dimensionIndexes,stride,data);
}



