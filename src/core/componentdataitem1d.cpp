/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#include "stdafx.h"
#include "core/componentdataitem.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"

#include <QDebug>
#include <assert.h>
#include <QDateTime>

using namespace HydroCouple;

template<class T>
ComponentDataItem1D<T>::ComponentDataItem1D(const QString &id, int length, const T& defaultValue)
  : m_length(length),
    m_defaultValue(defaultValue),
    m_id(id)

{
  resetDataArray();
}

template<class T>
ComponentDataItem1D<T>::~ComponentDataItem1D()
{
}

template<class T>
QString ComponentDataItem1D<T>::getId() const
{
  return m_id;
}

template<class T>
void ComponentDataItem1D<T>::getValueT(const std::vector<int> &dimensionIndexes, void *data) const
{
  T* dataCast = (T*) data;
  (*dataCast) = m_data[dimensionIndexes[0]];
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
void ComponentDataItem1D<T>::setValueT(const std::vector<int> &dimensionIndexes, const void *data)
{
  T* dataCast = (T*) data;
  m_data[dimensionIndexes[0]] = *dataCast;
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
T ComponentDataItem1D<T>::defaultValue() const
{
  return m_defaultValue;
}

template<class T>
T &ComponentDataItem1D<T>::operator [](int index)
{
  return m_data[index];
}

template<class T>
const T &ComponentDataItem1D<T>::operator [](int index) const
{
  return m_data[index];
}

template<class T>
int ComponentDataItem1D<T>::length() const
{
  return m_length;
}


template<class T>
void ComponentDataItem1D<T>::resetDataArray()
{
  if(m_length >= 0 )
  {
    m_data.resize(m_length);

    for(int i = 0 ; i < m_length ; i++)
    {
      m_data[i] = m_defaultValue;
    }
  }
}

template<class T>
void ComponentDataItem1D<T>::resizeDataArray(int length , bool initializeNewWithDefault)
{
  if(length >= 0)
  {
    m_data.resize(length);

    if(length > m_length && m_length > 0 && initializeNewWithDefault)
    {
      for(int i = m_length; i <  length ; i++)
      {
        m_data[i] = m_defaultValue;
      }
    }

    m_length = length;
  }
  else
  {
    m_data.clear();
  }
}

template<class T>
void ComponentDataItem1D<T>::removeItemAt(int i)
{
  m_data.erase(m_data.begin() + i);
  m_length = m_data.size();
}

template<class T>
void ComponentDataItem1D<T>::setDefaultValue(const T &defaultValue)
{
  m_defaultValue = defaultValue;
}


template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<QString>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<QDateTime>;
