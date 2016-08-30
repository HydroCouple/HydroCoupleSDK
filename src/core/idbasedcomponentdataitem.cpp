#include "stdafx.h"
#include "core/idbasedexchangeitems.h"
#include <assert.h>


template<class T>
IdBasedComponentDataItem<T>::IdBasedComponentDataItem(const QStringList& identifiers,
                                                      const T& defaultValue)
  : ComponentDataItem1D<T>(identifiers.length(), defaultValue),
    m_identifiers(identifiers)
{
}

template<class T>
IdBasedComponentDataItem<T>::~IdBasedComponentDataItem()
{

}

template<class T>
bool IdBasedComponentDataItem<T>::addIdentifier(const QString &identifier, bool resetDataArray)
{
  if(!m_identifiers.contains(identifier))
  {
    m_identifiers.append(identifier);
    ComponentDataItem1D<T>::setLength(m_identifiers.length());

    if(resetDataArray)
    {
      ComponentDataItem1D<T>::resetDataArray();
    }

    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
void IdBasedComponentDataItem<T>::addIdentifiers(const QList<QString> &identifiers, bool resetDataArray)
{
  bool added = false;

  for(const QString &id : identifiers)
  {
    if(!m_identifiers.contains(id))
    {
      m_identifiers.append(id);
      added = true;
    }
  }

  if(added)
  {
    ComponentDataItem1D<T>::setLength(m_identifiers.length());

    if(resetDataArray)
    {
      ComponentDataItem1D<T>::resetDataArray();
    }
  }
}

template<class T>
bool IdBasedComponentDataItem<T>::removeIdentifier(const QString &identifier, bool resetDataArray)
{
  if(m_identifiers.removeOne(identifier))
  {
    ComponentDataItem1D<T>::setLength(m_identifiers.length());

    if(resetDataArray)
    {
      ComponentDataItem1D<T>::resetDataArray();
    }
    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
QStringList IdBasedComponentDataItem<T>::identifiersInternal() const
{
  return m_identifiers;
}

template<class T>
void IdBasedComponentDataItem<T>::clearIdentifiers()
{
  m_identifiers.clear();
}

template<class T>
T &IdBasedComponentDataItem<T>::operator [](const QString &id)
{
  int index = m_identifiers.indexOf(id);

  if(index > -1)
  {
   return ComponentDataItem1D<T>::operator [](index);
  }
  else
  {
    throw std::invalid_argument("Identifier provided was not found : " + id.toStdString());
  }
}

template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<QString>;
