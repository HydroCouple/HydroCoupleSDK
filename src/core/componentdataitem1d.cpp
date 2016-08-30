#include "stdafx.h"
#include "core/exchangeitems1d.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"

#include <QDebug>
#include <assert.h>

using namespace HydroCouple;

template<class T>
ComponentDataItem1D<T>::ComponentDataItem1D(int length, const T& defaultValue)
  : m_length(length),
    m_defaultValue(defaultValue),
    m_data(nullptr)
{
  createData();
}

template<class T>
ComponentDataItem1D<T>::~ComponentDataItem1D()
{
  deleteData();
}

template<class T>
void ComponentDataItem1D<T>::getValueT(int dimensionIndexes[], QVariant & data) const
{
  data = QVariant::fromValue(m_data[dimensionIndexes[0]]);
}

template<class T>
void ComponentDataItem1D<T>::getValueT(int dimensionIndex, QVariant & data) const
{
  data = QVariant::fromValue(m_data[dimensionIndex]);
}

template<class T>
void ComponentDataItem1D<T>::getValuesT(int dimensionIndexes[], int stride[], QVariant* data) const
{
  for(int i = 0 ; i < stride[0] ; i++)
  {
    data[i] = QVariant::fromValue(m_data[dimensionIndexes[0] + i]);
  }
}

template<class T>
void ComponentDataItem1D<T>::getValuesT(int dimensionIndex, int stride, QVariant* data) const
{
  for(int i = 0 ; i < stride ; i++)
  {
    data[i] = QVariant::fromValue(m_data[dimensionIndex + i]);
  }
}

template<class T>
void ComponentDataItem1D<T>::getValuesT(int dimensionIndexes[], int stride[], void *data) const
{
  T* dataCast = (T*) data;

  for(int i = 0 ; i < stride[0] ; i++)
  {
    dataCast[i] = m_data[dimensionIndexes[0] + i];
  }
}

template<class T>
void ComponentDataItem1D<T>::getValuesT(int dimensionIndex, int stride, void *data) const
{
  T* dataCast = (T*) data;

  for(int i = 0 ; i < stride ; i++)
  {
    dataCast[i] = m_data[dimensionIndex + i];
  }
}

template<class T>
void ComponentDataItem1D<T>::setValueT(int dimensionIndexes[], const QVariant &data)
{
  m_data[dimensionIndexes[0]] = qvariant_cast<T>(data);
}

template<class T>
void ComponentDataItem1D<T>::setValueT(int dimensionIndex, const QVariant &data)
{
  m_data[dimensionIndex] = qvariant_cast<T>(data);
}


template<class T>
void ComponentDataItem1D<T>::setValuesT(int dimensionIndexes[], int stride[], const QVariant data[])
{
  for(int i = 0 ; i < stride[0] ; i++)
  {
    m_data[dimensionIndexes[0] + i] = qvariant_cast<T>(data[i]);
  }
}

template<class T>
void ComponentDataItem1D<T>::setValuesT(int dimensionIndex, int stride, const QVariant data[])
{
  for(int i = 0 ; i < stride ; i++)
  {
    m_data[dimensionIndex + i] = qvariant_cast<T>(data[i]);
  }
}

template<class T>
void ComponentDataItem1D<T>::setValuesT(int dimensionIndexes[], int stride[], const void *data)
{
  T* dataCast = (T*) data;

  for(int i = 0 ; i < stride[0] ; i++)
  {
    m_data[dimensionIndexes[0] + i] = dataCast[i];
  }
}

template<class T>
void ComponentDataItem1D<T>::setValuesT(int dimensionIndex, int stride, const void *data)
{
  T* dataCast = (T*) data;

  for(int i = 0 ; i < stride ; i++)
  {
    m_data[dimensionIndex + i] = dataCast[i];
  }
}

template<class T>
void ComponentDataItem1D<T>::resetDataArray()
{
  createData();
}

template<class T>
void ComponentDataItem1D<T>::resetDataArray(int length)
{
  m_length = length;
  createData();
}

template<class T>
T ComponentDataItem1D<T>::defaultValue() const
{
  return m_defaultValue;
}

template<class T>
void ComponentDataItem1D<T>::setDefaultValue(const T &defaultValue)
{
  m_defaultValue = defaultValue;
}

template<class T>
int ComponentDataItem1D<T>::length() const
{
  return m_length;
}

template<class T>
void ComponentDataItem1D<T>::setLength(int length)
{
  m_length = length;
}

template<class T>
void ComponentDataItem1D<T>::createData()
{
  deleteData();

  if(m_length > 0)
  {
    m_data = new T[m_length];

    for(int i = 0 ; i < m_length ; i++)
    {
      m_data[i] = m_defaultValue;
    }
  }
}

template<class T>
void ComponentDataItem1D<T>::deleteData()
{
  if(m_data)
  {
    delete[] m_data;
    m_data = nullptr;
  }
}

template<class T>
T &ComponentDataItem1D<T>::operator [](int index)
{
  return m_data[index];
}

template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<QString>;
