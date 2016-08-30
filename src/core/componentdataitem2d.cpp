#include "stdafx.h"
#include "core/exchangeitems2d.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"

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

template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<QString>;
