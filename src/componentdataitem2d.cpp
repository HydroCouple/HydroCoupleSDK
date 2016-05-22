#include "stdafx.h"
#include <QDebug>
#include "componentdataitem2d.h"
#include "valuedefinition.h"
#include "dimension.h"
#include "abstractcomponentdataitem.h"

using namespace HydroCouple;

template<class T>
ComponentDataItem2D<T>::ComponentDataItem2D(const QList<Dimension*> &dimensions, ValueDefinition* valueDefinition)
   : m_dimensions(dimensions), m_data(nullptr)
{
   m_valueDefinition = valueDefinition;
   createData();
}

template<class T>
ComponentDataItem2D<T>::~ComponentDataItem2D()
{
   deleteData();
}

template<class T>
QList<IDimension*> ComponentDataItem2D<T>::dimensions() const
{
   QList<IDimension*> dimensions;

   for(Dimension* dimension : m_dimensions)
      dimensions.append(dimension);

   return dimensions;
}

template<class T>
IValueDefinition* ComponentDataItem2D<T>::valueDefinition() const
{
   return m_valueDefinition;
}


template<class T>
void ComponentDataItem2D<T>::getValue(int dimensionIndexes[], QVariant & data) const
{
   data = QVariant::fromValue(m_data[dimensionIndexes[0]][dimensionIndexes[1]]);
}

template<class T>
void ComponentDataItem2D<T>::getValues(int dimensionIndexes[], int stride[], QVariant data[]) const
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
void ComponentDataItem2D<T>::getValues(int dimensionIndexes[], int stride[], void *data) const
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
void ComponentDataItem2D<T>::setValue(int dimensionIndexes[], const QVariant &data)
{
   m_data[dimensionIndexes[0]][dimensionIndexes[1]] = qvariant_cast<T>(data);
}

template<class T>
void ComponentDataItem2D<T>::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
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
void ComponentDataItem2D<T>::setValues(int dimensionIndexes[], int stride[], const void *data)
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
void ComponentDataItem2D<T>::createData()
{
   if(m_data)
      deleteData();

   int ilength = m_dimensions[0]->length();
   int jlength = m_dimensions[1]->length();

   T defaultValue = qvariant_cast<T>(m_valueDefinition->defaultValue());

   m_data = new T*[ilength];

   for(int i = 0 ; i < ilength ; i++)
   {
      m_data[i] = new T[jlength];

      for(int j = 0; j < jlength ; j++)
      {
         m_data[i][j] = defaultValue;
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


ComponentDataItem2DInt::ComponentDataItem2DInt(const QString &id, const QList<Dimension*> &dimensions,
                                               ValueDefinition *valueDefinition,  AbstractModelComponent *parentModelComponent)
   : ComponentDataItem2D<int>(dimensions,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{

}

ComponentDataItem2DInt::~ComponentDataItem2DInt()
{
}



ComponentDataItem2DDouble::ComponentDataItem2DDouble(const QString &id, const QList<Dimension*> &dimensions,
                                                     ValueDefinition *valueDefinition,  AbstractModelComponent *parentModelComponent)
   : ComponentDataItem2D<double>(dimensions,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{

}

ComponentDataItem2DDouble::~ComponentDataItem2DDouble()
{
}



ComponentDataItem2DString::ComponentDataItem2DString(const QString &id, const QList<Dimension*> &dimensions,
                                                     ValueDefinition* valueDefinition,  AbstractModelComponent* parentModelComponent)
   : ComponentDataItem2D<QString>(dimensions,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{

}

ComponentDataItem2DString::~ComponentDataItem2DString()
{
}



template class ComponentDataItem2D<int>;
template class ComponentDataItem2D<double>;
template class ComponentDataItem2D<QString>;
