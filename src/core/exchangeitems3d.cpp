#include "stdafx.h"
#include "core/exchangeitems3d.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "core/abstractcomponentdataitem.h"
#include <QDebug>
#include <assert.h>

using namespace HydroCouple;

Input3DInt::Input3DInt(const QString& id,
                       Dimension* iDimension, Dimension* jDimension, Dimension* kDimension,
                       int iLength, int jLength, int kLength,
                       ValueDefinition* valueDefinition,
                       AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({iDimension,jDimension,kDimension}),valueDefinition,modelComponent),
    ComponentDataItem3D<int>(iLength,jLength,kLength, valueDefinition->defaultValue().toInt())
{
}

Input3DInt::~Input3DInt()
{
}

int Input3DInt::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength < dimensions().length());

  if(dimensionIndexesLength == 0)
  {
    return iLength();
  }
  else if(dimensionIndexesLength == 1)
  {
    //dimensionIndexes[0];
    return jLength();
  }
  else
  {
    // dimensionIndexes[0];
    return kLength();
  }
}

void Input3DInt::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem3D<int>::getValueT(dimensionIndexes,data);
}

void Input3DInt::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem3D<int>::getValuesT(dimensionIndexes,stride,data);
}

void Input3DInt::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem3D<int>::getValuesT(dimensionIndexes,stride,data);
}

void Input3DInt::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem3D<int>::setValueT(dimensionIndexes,data);
}

void Input3DInt::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem3D<int>::setValuesT(dimensionIndexes,stride,data);
}

void Input3DInt::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem3D<int>::setValuesT(dimensionIndexes,stride,data);
}

//==============================================================================================================================

Input3DDouble::Input3DDouble(const QString& id,
                             Dimension* iDimension, Dimension* jDimension, Dimension* kDimension,
                             int iLength, int jLength, int kLength,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({iDimension,jDimension,kDimension}),valueDefinition,modelComponent),
    ComponentDataItem3D<double>(iLength,jLength,kLength, valueDefinition->defaultValue().toDouble())
{
}

Input3DDouble::~Input3DDouble()
{
}

int Input3DDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength < dimensions().length());

  if(dimensionIndexesLength == 0)
  {
    return iLength();
  }
  else if(dimensionIndexesLength == 1)
  {
    //int iloc = dimensionIndexes[0];
    return jLength();
  }
  else
  {
    //int iloc = dimensionIndexes[0];
    //int jloc = dimensionIndexes[1];
    return kLength();
  }
}

void Input3DDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem3D<double>::getValueT(dimensionIndexes,data);
}

void Input3DDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem3D<double>::getValuesT(dimensionIndexes,stride,data);
}

void Input3DDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem3D<double>::getValuesT(dimensionIndexes,stride,data);
}

void Input3DDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem3D<double>::setValueT(dimensionIndexes,data);
}

void Input3DDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem3D<double>::setValuesT(dimensionIndexes,stride,data);
}

void Input3DDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem3D<double>::setValuesT(dimensionIndexes,stride,data);
}

//==============================================================================================================================

Input3DString::Input3DString(const QString& id,
                             Dimension* iDimension, Dimension* jDimension, Dimension* kDimension,
                             int iLength, int jLength, int kLength,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({iDimension,jDimension,kDimension}),valueDefinition,modelComponent),
    ComponentDataItem3D<QString>(iLength,jLength,kLength, valueDefinition->defaultValue().toString())
{
}

Input3DString::~Input3DString()
{
}

int Input3DString::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength < dimensions().length());

  if(dimensionIndexesLength == 0)
  {
    return iLength();
  }
  else if(dimensionIndexesLength == 1)
  {
    //int iloc = dimensionIndexes[0];
    return jLength();
  }
  else
  {
    //int iloc = dimensionIndexes[0];
    //int jloc = dimensionIndexes[1];
    return kLength();
  }
}

void Input3DString::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem3D<QString>::getValueT(dimensionIndexes,data);
}

void Input3DString::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem3D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void Input3DString::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem3D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void Input3DString::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem3D<QString>::setValueT(dimensionIndexes,data);
}

void Input3DString::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem3D<QString>::setValuesT(dimensionIndexes,stride,data);
}

void Input3DString::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem3D<QString>::setValuesT(dimensionIndexes,stride,data);
}

template class HYDROCOUPLESDK_EXPORT ComponentDataItem3D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem3D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem3D<QString>;
