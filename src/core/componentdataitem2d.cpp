#include "stdafx.h"
#include "core/componentdataitem.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"

#include <QDebug>
#include <assert.h>
#include <QDateTime>

using namespace HydroCouple;

template<class T>
ComponentDataItem2D<T>::ComponentDataItem2D(const QString &id, int iLength, int jLength, const T& defaultValue)
  : m_iLength(iLength),
    m_jLength(jLength),
    m_defaultValue(defaultValue),
    m_id(id)
{
  resetDataArray();
}

template<class T>
ComponentDataItem2D<T>::~ComponentDataItem2D()
{
}

template<class T>
QString ComponentDataItem2D<T>::getId() const
{
  return m_id;
}

template<class T>
void ComponentDataItem2D<T>::getValueT(const std::vector<int> &dimensionIndexes, void *data) const
{
  T* dataC = (T*) data;

  int istart = dimensionIndexes[0];
  int jstart = dimensionIndexes[1];

  (*dataC) = m_data[istart][jstart];
}

template<class T>
void ComponentDataItem2D<T>::getValueT(int iIndex, int jIndex, void *data) const
{
  T* dataC = (T*) data;
  (*dataC) = m_data[iIndex][jIndex];
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
void ComponentDataItem2D<T>::getValuesT(int iIndex, int jIndex, int iStride, int jStride, void *data) const
{
  T* dataC = (T*) data;

  for(int i = 0 ; i < iStride ; i++)
  {
    for(int j =0 ; j < jStride ; j++)
    {
      dataC[i + j*iStride] = m_data[iIndex + i][jIndex + j];
    }
  }
}

template<class T>
void ComponentDataItem2D<T>::setValueT(const std::vector<int> &dimensionIndexes, const void *data)
{
  T* dataC = (T*) data;
  int istart = dimensionIndexes[0];
  int jstart = dimensionIndexes[1];
  m_data[istart][jstart] = *dataC;
}

template<class T>
void ComponentDataItem2D<T>::setValueT(int iIndex, int jIndex, const void *data)
{
  T* dataC = (T*) data;
  m_data[iIndex][jIndex] = *dataC;
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
void ComponentDataItem2D<T>::setValuesT(int iIndex, int jIndex, int iStride, int jStride, const void *data)
{
  T* dataC = (T*) data;

  for(int i = 0 ; i < iStride ; i++)
  {
    for(int j =0 ; j < jStride ; j++)
    {
      m_data[iIndex + i][jIndex + j] = dataC[i + j*iStride];
    }
  }
}

template<class T>
T ComponentDataItem2D<T>::defaultValue() const
{
  return m_defaultValue;
}

template<class T>
T &ComponentDataItem2D<T>::operator ()(int i , int j)
{
  return m_data[i][j];
}

template<class T>
const T &ComponentDataItem2D<T>::operator ()(int i , int j) const
{
  return m_data[i][j];
}

template<class T>
int ComponentDataItem2D<T>::iLength() const
{
  return m_iLength;
}

template<class T>
int ComponentDataItem2D<T>::jLength() const
{
  return m_jLength;
}

template<class T>
void ComponentDataItem2D<T>::resetDataArray()
{
  m_data.resize(m_iLength);

  for(int i = 0 ; i < m_iLength ; i++)
  {
    m_data[i].resize(m_jLength);

    for(int j = 0; j < m_jLength ; j++)
    {
      m_data[i][j] = m_defaultValue;
    }
  }
}

template<class T>
void ComponentDataItem2D<T>::resizeDataArrayILength(int iLength , bool initializeNewWithDefault)
{
  if(iLength >= 0)
  {
    m_data.resize(iLength);

    if(initializeNewWithDefault && iLength > m_iLength && m_iLength > 0)
    {
      for(int i = m_iLength; i < iLength ; i++)
      {
        std::vector<T> &data = m_data[i];
        std::fill(data.begin() , data.end(), m_defaultValue);
      }
    }

    m_iLength = iLength;
  }
}

template<class T>
void ComponentDataItem2D<T>::removeIItemAt(int i)
{
  m_data.erase(m_data.begin() + i);
  m_iLength = m_data.size();
}


template<class T>
void ComponentDataItem2D<T>::resizeDataArrayJLength(int jLength, bool initializeNewWithDefault)
{
  if(jLength >= 0)
  {
    for(int i = 0 ; i < m_iLength ; i++)
    {
      m_data[i].resize(jLength);
    }

    if(jLength > m_jLength && m_jLength > 0 && initializeNewWithDefault)
    {
      for(int i = 0 ; i < m_iLength ; i++)
      {
        std::vector<T> &data = m_data[i];
        std::fill(data.begin() , data.end(), m_defaultValue);
      }
    }

    m_jLength = jLength;
  }
}

template<class T>
void ComponentDataItem2D<T>::removeJItemAt(int j)
{
  for(int i = 0; i < m_iLength ; i++)
  {
    std::vector<T> & data = m_data[i];
    data.erase(data.begin() + j);
  }

  m_jLength = m_data[0].size();
}

template<class T>
void ComponentDataItem2D<T>::resizeDataArray(int iLength, int jLength, bool initializeNewWithDefault)
{
  resizeDataArrayILength(iLength,initializeNewWithDefault);
  resizeDataArrayJLength(jLength,initializeNewWithDefault);
}

template<class T>
void ComponentDataItem2D<T>::setDefaultValue(const T &defaultValue)
{
  m_defaultValue = defaultValue;
}

template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<QString>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<QDateTime>;
