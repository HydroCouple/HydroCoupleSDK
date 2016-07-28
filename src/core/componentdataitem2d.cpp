#include "stdafx.h"
#include "core/componentdataitem2d.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "core/abstractcomponentdataitem.h"
#include <QDebug>
#include <assert.h>

using namespace HydroCouple;

template<class T>
ComponentDataItem2D<T>::ComponentDataItem2D(int iLength, int jLength, const T& defaultValue)
  : m_iLength(iLength),
    m_jLength(jLength),
    m_defaultValue(defaultValue),
    m_data(nullptr)
{
  createData();
}

template<class T>
ComponentDataItem2D<T>::~ComponentDataItem2D()
{
  deleteData();
}

template<class T>
void ComponentDataItem2D<T>::getValueT(int dimensionIndexes[], QVariant & data) const
{
  data = QVariant::fromValue(m_data[dimensionIndexes[0]][dimensionIndexes[1]]);
}

template<class T>
void ComponentDataItem2D<T>::getValuesT(int dimensionIndexes[], int stride[], QVariant data[]) const
{
  int istart = dimensionIndexes[0];
  int jstart = dimensionIndexes[1];

  int ilength = stride[0];
  int jlength = stride[1];

  for(int i = 0 ; i < ilength ; i++)
  {
    for(int j =0 ; j < jlength ; j++)
    {
      data[i + j*ilength] = QVariant::fromValue(m_data[istart + i][jstart + j]);
    }
  }
}

template<class T>
void ComponentDataItem2D<T>::getValuesT(int dimensionIndexes[], int stride[], void *data) const
{
  T* dataC = (T*) data;

  int istart = dimensionIndexes[0];
  int jstart = dimensionIndexes[1];

  int ilength = stride[0];
  int jlength = stride[1];

  for(int i = 0 ; i < ilength ; i++)
  {
    for(int j =0 ; j < jlength ; j++)
    {
      dataC[i + j*ilength] = m_data[istart + i][jstart + j];
    }
  }
}

template<class T>
void ComponentDataItem2D<T>::setValueT(int dimensionIndexes[], const QVariant &data)
{
  m_data[dimensionIndexes[0]][dimensionIndexes[1]] = qvariant_cast<T>(data);
}

template<class T>
void ComponentDataItem2D<T>::setValuesT(int dimensionIndexes[], int stride[], const QVariant data[])
{
  int istart = dimensionIndexes[0];
  int jstart = dimensionIndexes[1];

  int ilength = stride[0];
  int jlength = stride[1];

  for(int i = 0 ; i < ilength ; i++)
  {
    for(int j =0 ; j < jlength ; j++)
    {
      m_data[istart + i][jstart + j] = qvariant_cast<T>(data[i + j*ilength]);
    }
  }
}

template<class T>
void ComponentDataItem2D<T>::setValuesT(int dimensionIndexes[], int stride[], const void *data)
{
  T* dataC = (T*) data;

  int istart = dimensionIndexes[0];
  int jstart = dimensionIndexes[1];

  int ilength = stride[0];
  int jlength = stride[1];

  for(int i = 0 ; i < ilength ; i++)
  {
    for(int j =0 ; j < jlength ; j++)
    {
      m_data[istart + i][jstart + j] = dataC[i + j*ilength];
    }
  }
}

template<class T>
void ComponentDataItem2D<T>::resetDataArray()
{
  createData();
}

template<class T>
void ComponentDataItem2D<T>::resetDataArray(int iLength, int jLength )
{
  m_iLength = iLength;
  m_jLength = jLength;

  createData();
}

template<class T>
T ComponentDataItem2D<T>::defaultValue() const
{
  return m_defaultValue;
}

template<class T>
void ComponentDataItem2D<T>::setDefaultValue(const T &defaultValue)
{
  m_defaultValue = defaultValue;
}

template<class T>
int ComponentDataItem2D<T>::iLength() const
{
  return m_iLength;
}

template<class T>
void ComponentDataItem2D<T>::setILength(int iLength)
{
  m_iLength = iLength;
}

template<class T>
int ComponentDataItem2D<T>::jLength() const
{
  return m_jLength;
}

template<class T>
void ComponentDataItem2D<T>::setJLength(int jLength)
{
  m_jLength = jLength;
}

template<class T>
void ComponentDataItem2D<T>::createData()
{

  deleteData();

  if(m_iLength > 0 && m_jLength > 0)
  {
    m_data = new T*[m_iLength];

    for(int i = 0 ; i < m_iLength ; i++)
    {
      m_data[i] = new T[m_jLength];

      for(int j = 0; j < m_jLength ; j++)
      {
        m_data[i][j] = m_defaultValue;
      }
    }
  }
}

template<class T>
void ComponentDataItem2D<T>::deleteData()
{
  if(m_data)
  {
    for(int i = 0 ; i < m_iLength ; i++)
    {
      delete[] m_data[i];
    }

    delete[] m_data;

    m_data = nullptr;
  }
}

//==============================================================================================================================

ComponentDataItem2DInt::ComponentDataItem2DInt(const QString& id,
                                               Dimension* iDimension, Dimension* jDimension,
                                               int iLength, int jLength,
                                               ValueDefinition* valueDefinition,
                                               AbstractModelComponent* modelComponent)
  : ComponentDataItem2D<int>(iLength,jLength,valueDefinition->defaultValue().toInt()),
    AbstractComponentDataItem(id,QList<Dimension*>({iDimension,jDimension}),valueDefinition,modelComponent)
{

}

ComponentDataItem2DInt::~ComponentDataItem2DInt()
{
}

int ComponentDataItem2DInt::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
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

void ComponentDataItem2DInt::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem2D<int>::getValueT(dimensionIndexes,data);
}

void ComponentDataItem2DInt::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem2D<int>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem2DInt::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem2D<int>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem2DInt::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem2D<int>::setValueT(dimensionIndexes,data);
}

void ComponentDataItem2DInt::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem2D<int>::setValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem2DInt::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem2D<int>::setValuesT(dimensionIndexes,stride,data);
}


//==============================================================================================================================


ComponentDataItem2DDouble::ComponentDataItem2DDouble(const QString& id,
                                                     Dimension* iDimension, Dimension* jDimension,
                                                     int iDimensionLength, int jDimensionLength,
                                                     ValueDefinition* valueDefinition,
                                                     AbstractModelComponent* modelComponent)
  : ComponentDataItem2D<double>(iDimensionLength,jDimensionLength,valueDefinition->defaultValue().toInt()),
    AbstractComponentDataItem(id,QList<Dimension*>({iDimension,jDimension}),valueDefinition,modelComponent)
{

}

ComponentDataItem2DDouble::~ComponentDataItem2DDouble()
{
}

int ComponentDataItem2DDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
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

void ComponentDataItem2DDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem2D<double>::getValueT(dimensionIndexes,data);
}

void ComponentDataItem2DDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem2DDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem2DDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem2D<double>::setValueT(dimensionIndexes,data);
}

void ComponentDataItem2DDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem2DDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

//==============================================================================================================================


ComponentDataItem2DString::ComponentDataItem2DString(const QString& id,
                                                     Dimension* iDimension, Dimension* jDimension,
                                                     int iDimensionLength, int jDimensionLength,
                                                     ValueDefinition* valueDefinition,
                                                     AbstractModelComponent* modelComponent)
  : ComponentDataItem2D<QString>(iDimensionLength,jDimensionLength,valueDefinition->defaultValue().toString()),
    AbstractComponentDataItem(id,QList<Dimension*>({iDimension,jDimension}),valueDefinition,modelComponent)
{

}

ComponentDataItem2DString::~ComponentDataItem2DString()
{
}

int ComponentDataItem2DString::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
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

void ComponentDataItem2DString::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem2D<QString>::getValueT(dimensionIndexes,data);
}

void ComponentDataItem2DString::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem2D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem2DString::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem2D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem2DString::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem2D<QString>::setValueT(dimensionIndexes,data);
}

void ComponentDataItem2DString::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem2D<QString>::setValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem2DString::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem2D<QString>::setValuesT(dimensionIndexes,stride,data);
}


template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<QString>;
