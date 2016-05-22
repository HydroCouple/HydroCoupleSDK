#include "stdafx.h"
#include <QDebug>
#include "componentdataitem3d.h"
#include "valuedefinition.h"
#include "dimension.h"
#include "abstractcomponentdataitem.h"

using namespace HydroCouple;

template<class T>
ComponentDataItem3D<T>::ComponentDataItem3D(const QList<Dimension*> &dimensions, ValueDefinition* valueDefinition)
   : m_dimensions(dimensions), m_data(nullptr)
{
   m_valueDefinition = valueDefinition;
   createData();
}

template<class T>
ComponentDataItem3D<T>::~ComponentDataItem3D()
{
   deleteData();
}

template<class T>
QList<IDimension*> ComponentDataItem3D<T>::dimensions() const
{
   QList<IDimension*> dimensions;

   for(Dimension* dimension : m_dimensions)
      dimensions.append(dimension);

   return dimensions;
}

template<class T>
IValueDefinition* ComponentDataItem3D<T>::valueDefinition() const
{
   return m_valueDefinition;
}

template<class T>
void ComponentDataItem3D<T>::getValue(int dimensionIndexes[], QVariant &data) const
{
   data = QVariant::fromValue(m_data[dimensionIndexes[0]][dimensionIndexes[1]][dimensionIndexes[2]]);
}

template<class T>
void ComponentDataItem3D<T>::getValues(int dimensionIndexes[], int stride[], QVariant *data) const
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
void ComponentDataItem3D<T>::getValues(int dimensionIndexes[], int stride[], void *data) const
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
void ComponentDataItem3D<T>::setValue(int dimensionIndexes[], const QVariant &data)
{
   m_data[dimensionIndexes[0]][dimensionIndexes[1]][dimensionIndexes[2]] = qvariant_cast<T>(data);
}

template<class T>
void ComponentDataItem3D<T>::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
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
void ComponentDataItem3D<T>::setValues(int dimensionIndexes[], int stride[], const void *data)
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
void ComponentDataItem3D<T>::createData()
{
   if(m_data)
      deleteData();

   int ilength = m_dimensions[0]->length();
   int jlength = m_dimensions[1]->length();
   int klength = m_dimensions[2]->length();

   T defaultValue = qvariant_cast<T>(m_valueDefinition->defaultValue());

   m_data = new T**[ilength];

   for(int i = 0 ; i < ilength ; i++)
   {
      m_data[i] = new T*[jlength];

      for(int j = 0; j < jlength ; j++)
      {
         m_data[i][j] = new T[klength];

         for(int k = 0 ; k < klength ; k++)
         {
            m_data[i][j][k] = defaultValue;
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


ComponentDataItem3DInt::ComponentDataItem3DInt(const QString &id, const QList<Dimension*> &dimensions, ValueDefinition *valueDefinition,  AbstractModelComponent* parentModelComponent)
   : ComponentDataItem3D<int>(dimensions,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{
}

ComponentDataItem3DInt::~ComponentDataItem3DInt()
{
}

ComponentDataItem3DDouble::ComponentDataItem3DDouble(const QString &id, const QList<Dimension*> &dimensions, ValueDefinition* valueDefinition,  AbstractModelComponent* parentModelComponent)
   : ComponentDataItem3D<double>(dimensions,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{
}

ComponentDataItem3DDouble::~ComponentDataItem3DDouble()
{
}

ComponentDataItem3DString::ComponentDataItem3DString(const QString &id, const QList<Dimension*> &dimensions, ValueDefinition* valueDefinition,  AbstractModelComponent* parentModelComponent)
   : ComponentDataItem3D<QString>(dimensions,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{
}

ComponentDataItem3DString::~ComponentDataItem3DString()
{
}

template class ComponentDataItem3D<int>;
template class ComponentDataItem3D<double>;
template class ComponentDataItem3D<QString>;
