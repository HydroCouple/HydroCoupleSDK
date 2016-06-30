#include "stdafx.h"
#include "core/componentdataitem3d.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "core/abstractcomponentdataitem.h"
#include <QDebug>

using namespace HydroCouple;

template<class T>
ComponentDataItem3D<T>::ComponentDataItem3D(const QList<Dimension*> &dimensions, const T& defaultValue)
  : m_dimensions(dimensions),
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
void ComponentDataItem3D<T>::createData()
{
  if(m_data)
    deleteData();

  int ilength = m_dimensions[0]->length();
  int jlength = m_dimensions[1]->length();
  int klength = m_dimensions[2]->length();

  m_data = new T**[ilength];

  for(int i = 0 ; i < ilength ; i++)
  {
    m_data[i] = new T*[jlength];

    for(int j = 0; j < jlength ; j++)
    {
      m_data[i][j] = new T[klength];

      for(int k = 0 ; k < klength ; k++)
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
    int ilength = m_dimensions[0]->previousLength();
    int jlength = m_dimensions[1]->previousLength();

    for(int i = 0 ; i < ilength ; i++)
    {
      for(int j = 0 ; j < jlength ; j++)
      {
        delete[] m_data[i][j];
      }
      delete[] m_data[i];
    }
    delete[] m_data;
  }
}

//==============================================================================================================================

ComponentDataItem3DInt::ComponentDataItem3DInt(const QString& id,
                                               const QList<Dimension*>& dimensions,
                                               ValueDefinition* valueDefinition,
                                               AbstractModelComponent* modelComponent)
  : AbstractComponentDataItem(id,dimensions,valueDefinition,modelComponent),
    ComponentDataItem3D<int>(dimensions, valueDefinition->defaultValue().toInt())
{
}

ComponentDataItem3DInt::~ComponentDataItem3DInt()
{
}


void ComponentDataItem3DInt::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem3D<int>::getValueT(dimensionIndexes,data);
}

void ComponentDataItem3DInt::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem3D<int>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem3DInt::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem3D<int>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem3DInt::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem3D<int>::setValueT(dimensionIndexes,data);
}

void ComponentDataItem3DInt::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem3D<int>::setValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem3DInt::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem3D<int>::setValuesT(dimensionIndexes,stride,data);
}

//==============================================================================================================================

ComponentDataItem3DDouble::ComponentDataItem3DDouble(const QString& id,
                                                     const QList<Dimension*>& dimensions,
                                                     ValueDefinition* valueDefinition,
                                                     AbstractModelComponent* modelComponent)
  : AbstractComponentDataItem(id,dimensions,valueDefinition,modelComponent),
    ComponentDataItem3D<double>(dimensions, valueDefinition->defaultValue().toDouble())
{
}

ComponentDataItem3DDouble::~ComponentDataItem3DDouble()
{
}

void ComponentDataItem3DDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem3D<double>::getValueT(dimensionIndexes,data);
}

void ComponentDataItem3DDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem3D<double>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem3DDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem3D<double>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem3DDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem3D<double>::setValueT(dimensionIndexes,data);
}

void ComponentDataItem3DDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem3D<double>::setValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem3DDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem3D<double>::setValuesT(dimensionIndexes,stride,data);
}

//==============================================================================================================================

ComponentDataItem3DString::ComponentDataItem3DString(const QString& id,
                                                     const QList<Dimension*>& dimensions,
                                                     ValueDefinition* valueDefinition,
                                                     AbstractModelComponent* modelComponent)
  : AbstractComponentDataItem(id,dimensions,valueDefinition,modelComponent),
    ComponentDataItem3D<QString>(dimensions, valueDefinition->defaultValue().toString())
{
}

ComponentDataItem3DString::~ComponentDataItem3DString()
{
}

void ComponentDataItem3DString::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem3D<QString>::getValueT(dimensionIndexes,data);
}

void ComponentDataItem3DString::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem3D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem3DString::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem3D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem3DString::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem3D<QString>::setValueT(dimensionIndexes,data);
}

void ComponentDataItem3DString::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem3D<QString>::setValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem3DString::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem3D<QString>::setValuesT(dimensionIndexes,stride,data);
}

template class HYDROCOUPLESDK_EXPORT ComponentDataItem3D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem3D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem3D<QString>;
