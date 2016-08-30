#include "stdafx.h"
#include "core/exchangeitems3d.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "core/abstractcomponentdataitem.h"
#include <QDebug>
#include <assert.h>

using namespace HydroCouple;

template<class T>
ComponentDataItem3D<T>::ComponentDataItem3D(int iLength, int jLength, int kLength, const T& defaultValue)
  : m_iLength(iLength),
    m_jLength(jLength),
    m_kLength(kLength),
    m_defaultValue(defaultValue),
    m_data(nullptr)
{
  createData();
}

template<class T>
ComponentDataItem3D<T>::~ComponentDataItem3D()
{
  deleteData();
}

template<class T>
void ComponentDataItem3D<T>::getValueT(int dimensionIndexes[], QVariant &data) const
{
  data = QVariant::fromValue(m_data[dimensionIndexes[0]][dimensionIndexes[1]][dimensionIndexes[2]]);
}

template<class T>
void ComponentDataItem3D<T>::getValuesT(int dimensionIndexes[], int stride[], QVariant *data) const
{
  int istart = dimensionIndexes[0];
  int jstart = dimensionIndexes[1];
  int kstart = dimensionIndexes[2];

  int ilength = stride[0];
  int jlength = stride[1];
  int klength = stride[2];

  for(int i = 0 ; i < ilength ; i++)
  {
    for(int j =0 ; j < jlength ; j++)
    {
      for(int k = 0 ; k < klength ; k++)
      {
        data[i + j*ilength + k*ilength*jlength] = QVariant::fromValue(m_data[istart + i][jstart + j][kstart + k]);
      }
    }
  }
}

template<class T>
void ComponentDataItem3D<T>::getValuesT(int dimensionIndexes[], int stride[], void *data) const
{
  T* dataC = (T*) data;

  int istart = dimensionIndexes[0];
  int jstart = dimensionIndexes[1];
  int kstart = dimensionIndexes[2];

  int ilength = stride[0];
  int jlength = stride[1];
  int klength = stride[2];

  for(int i = 0 ; i < ilength ; i++)
  {
    for(int j =0 ; j < jlength ; j++)
    {
      for(int k = 0 ; k < klength ; k++)
      {
        dataC[i + j*ilength + k*ilength*jlength] = m_data[istart + i][jstart + j][kstart + k];
      }
    }
  }
}

template<class T>
void ComponentDataItem3D<T>::setValueT(int dimensionIndexes[], const QVariant &data)
{
  m_data[dimensionIndexes[0]][dimensionIndexes[1]][dimensionIndexes[2]] = qvariant_cast<T>(data);
}

template<class T>
void ComponentDataItem3D<T>::setValuesT(int dimensionIndexes[], int stride[], const QVariant data[])
{
  int istart = dimensionIndexes[0];
  int jstart = dimensionIndexes[1];
  int kstart = dimensionIndexes[2];

  int ilength = stride[0];
  int jlength = stride[1];
  int klength = stride[2];

  for(int i = 0 ; i < ilength ; i++)
  {
    for(int j =0 ; j < jlength ; j++)
    {
      for(int k = 0 ; k < klength ; k++)
      {
        m_data[istart + i][jstart + j][kstart + k] = qvariant_cast<T>(data[i + j*ilength + k*ilength*jlength]);
      }
    }
  }
}

template<class T>
void ComponentDataItem3D<T>::setValuesT(int dimensionIndexes[], int stride[], const void *data)
{
  T* dataC = (T*) data;

  int istart = dimensionIndexes[0];
  int jstart = dimensionIndexes[1];
  int kstart = dimensionIndexes[2];

  int ilength = stride[0];
  int jlength = stride[1];
  int klength = stride[2];

  for(int i = 0 ; i < ilength ; i++)
  {
    for(int j =0 ; j < jlength ; j++)
    {
      for(int k = 0 ; k < klength ; k++)
      {
        m_data[istart + i][jstart + j][kstart + k] = dataC[i + j*ilength + k*ilength*jlength];
      }
    }
  }
}

template<class T>
void ComponentDataItem3D<T>::resetDataArray()
{
  createData();
}

template<class T>
void ComponentDataItem3D<T>::resetDataArray(int iLength, int jLength, int kLength)
{
  m_iLength = iLength;
  m_jLength = jLength;
  m_kLength = kLength;

  createData();
}

template<class T>
T ComponentDataItem3D<T>::defaultValue() const
{
  return m_defaultValue;
}

template<class T>
void ComponentDataItem3D<T>::setDefaultValue(const T& defaultValue)
{
  m_defaultValue = defaultValue;
}

template<class T>
int ComponentDataItem3D<T>::iLength() const
{
  return m_iLength;
}

template<class T>
void ComponentDataItem3D<T>::setILength(int ilength)
{
  m_iLength = ilength;
}

template<class T>
int ComponentDataItem3D<T>::jLength() const
{
  return m_jLength;
}

template<class T>
void ComponentDataItem3D<T>::setJLength(int jlength)
{
  m_jLength = jlength;
}

template<class T>
int ComponentDataItem3D<T>::kLength() const
{
  return m_kLength;
}

template<class T>
void ComponentDataItem3D<T>::setKLength(int klength)
{
  m_kLength = klength;
}

template<class T>
void ComponentDataItem3D<T>::createData()
{
  deleteData();

  m_data = new T**[m_iLength];

  for(int i = 0 ; i < m_iLength ; i++)
  {
    m_data[i] = new T*[m_jLength];

    for(int j = 0; j < m_jLength ; j++)
    {
      m_data[i][j] = new T[m_kLength];

      for(int k = 0 ; k < m_kLength ; k++)
      {
        m_data[i][j][k] = m_defaultValue;
      }

    }
  }
}

template<class T>
void ComponentDataItem3D<T>::deleteData()
{
  if(m_data)
  {
    for(int i = 0 ; i < m_iLength ; i++)
    {
      for(int j = 0 ; j < m_jLength ; j++)
      {
        delete[] m_data[i][j];
      }
      delete[] m_data[i];
    }

    delete[] m_data;
    m_data = nullptr;
  }
}

template class HYDROCOUPLESDK_EXPORT ComponentDataItem3D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem3D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem3D<QString>;
