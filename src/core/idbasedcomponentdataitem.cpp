#include "stdafx.h"
#include "core/idbasedexchangeitems.h"
#include <assert.h>


template<class T>
IdBasedComponentDataItem<T>::IdBasedComponentDataItem(const QString &id,
                                                      const QStringList& identifiers,
                                                      const T& defaultValue)
  : ComponentDataItem1D<T>(id, identifiers.length(), defaultValue),
    m_identifiers(identifiers)
{
}

template<class T>
IdBasedComponentDataItem<T>::~IdBasedComponentDataItem()
{

}

template<class T>
bool IdBasedComponentDataItem<T>::addIdentifier(const QString &identifier)
{
  if(!m_identifiers.contains(identifier))
  {
    m_identifiers.append(identifier);
    ComponentDataItem1D<T>::resizeDataArray(m_identifiers.length());
    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
void IdBasedComponentDataItem<T>::addIdentifiers(const QList<QString> &identifiers)
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
    ComponentDataItem1D<T>::resizeDataArray(m_identifiers.length());
  }
}

template<class T>
bool IdBasedComponentDataItem<T>::removeIdentifier(const QString &identifier)
{
  int index = m_identifiers.indexOf(identifier);

  if(index >= 0)
  {
    m_identifiers.removeAt(index);
    ComponentDataItem1D<T>::resizeDataArray(m_identifiers.length());
    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
bool IdBasedComponentDataItem<T>::containsIdentifier(const QString &identifier) const
{
   return m_identifiers.contains(identifier);
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
  ComponentDataItem1D<T>::resizeDataArray(m_identifiers.length());
}

template<class T>
T &IdBasedComponentDataItem<T>::operator [](const QString &id)
{
  int index = m_identifiers.indexOf(id);
  return ComponentDataItem1D<T>::operator [](index);
}

template<class T>
const T &IdBasedComponentDataItem<T>::operator [](const QString &id) const
{
  int index = m_identifiers.indexOf(id);
  return ComponentDataItem1D<T>::operator [](index);
}

template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<QString>;
template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<QDateTime>;
