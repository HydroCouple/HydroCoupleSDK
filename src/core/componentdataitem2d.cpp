#include "stdafx.h"
#include "core/componentdataitem2d.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "core/abstractcomponentdataitem.h"
#include <QDebug>

using namespace HydroCouple;

template<class T>
ComponentDataItem2D<T>::ComponentDataItem2D(const QList<Dimension*> &dimensions, const T& defaultValue)
  : m_dimensions(dimensions),
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
void ComponentDataItem2D<T>::createData()
{
  if(m_data)
    deleteData();

  int ilength = m_dimensions[0]->length();
  int jlength = m_dimensions[1]->length();

  m_data = new T*[ilength];

  for(int i = 0 ; i < ilength ; i++)
  {
    m_data[i] = new T[jlength];

    for(int j = 0; j < jlength ; j++)
    {
      m_data[i][j] = m_defaultValue;
    }
  }
}

template<class T>
void ComponentDataItem2D<T>::deleteData()
{
  if(m_data)
  {

    int ilength = m_dimensions[0]->previousLength();

    for(int i = 0 ; i < ilength ; i++)
      delete[] m_data[i];

    delete[] m_data;
  }
}

//==============================================================================================================================


ComponentDataItem2DInt::ComponentDataItem2DInt(const QString& id,
                                               const QList<Dimension*>& dimensions,
                                               ValueDefinition* valueDefinition,
                                               AbstractModelComponent* modelComponent)
  : ComponentDataItem2D<int>(dimensions,valueDefinition->defaultValue().toInt()),
    AbstractComponentDataItem(id,dimensions,valueDefinition,modelComponent)
{

}

ComponentDataItem2DInt::~ComponentDataItem2DInt()
{
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
                                                     const QList<Dimension*>& dimensions,
                                                     ValueDefinition* valueDefinition,
                                                     AbstractModelComponent* modelComponent)
  : ComponentDataItem2D<double>(dimensions,valueDefinition->defaultValue().toInt()),
    AbstractComponentDataItem(id,dimensions,valueDefinition,modelComponent)
{

}

ComponentDataItem2DDouble::~ComponentDataItem2DDouble()
{
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
                                                     const QList<Dimension*>& dimensions,
                                                     ValueDefinition* valueDefinition,
                                                     AbstractModelComponent* modelComponent)
  : ComponentDataItem2D<QString>(dimensions,valueDefinition->defaultValue().toString()),
    AbstractComponentDataItem(id,dimensions,valueDefinition,modelComponent)
{

}

ComponentDataItem2DString::~ComponentDataItem2DString()
{
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
