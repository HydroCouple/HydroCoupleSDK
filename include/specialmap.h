#ifndef SPECIALMAP_H
#define SPECIALMAP_H

#include <map>
#include <vector>
#include <unordered_map>
#include "hydrocouplesdk.h"

class HCPolygon;
class HCVertex;

template<class T>
class HYDROCOUPLESDK_EXPORT SpecialMap
{
    typedef typename std::unordered_map<T,long>::iterator unordered_map_iterator;
    typedef typename std::map<long,T>::iterator map_iterator;
    typedef typename std::map<long,T>::const_iterator const_map_iterator;

  public:
    SpecialMap();

    ~SpecialMap();

    size_t size() const;

    void push_back(const T &value);

    bool contains(const T &value);

    bool erase(const T &value);

    const T& operator[](const int index) const;

    T& operator[](const int index);

    const std::vector<T> &data() const;

  private:
    long m_counter;
    std::vector<T>  m_data;
    std::map<long,T> m_orderedData;
    std::unordered_map<T,long> m_unorderedData;
};

template<class T>
SpecialMap<T>::SpecialMap():
  m_counter(0)
{
}

template<class T>
SpecialMap<T>::~SpecialMap()
{
  m_data.clear();
  m_unorderedData.clear();
  m_orderedData.clear();
}

template<class T>
inline size_t SpecialMap<T>::size() const
{
  return m_orderedData.size();
}

template<class T>
inline void SpecialMap<T>::push_back(const T &value)
{
  m_orderedData[m_counter] = value;
  m_data.push_back(value);
  m_unorderedData[value] = m_counter;
  m_counter++;
}

template<class T>
inline bool SpecialMap<T>::contains(const T &value)
{
  unordered_map_iterator it = m_unorderedData.find(value);
  return it != m_unorderedData.end();
}

template<class T>
inline bool SpecialMap<T>::erase(const T &value)
{
  if(m_unorderedData.size())
  {
    unordered_map_iterator it = m_unorderedData.find(value);

    if(it != m_unorderedData.end())
    {
      long id = m_unorderedData[value];
      m_unorderedData.erase(value);

      map_iterator it = m_orderedData.find(id);
      int distance = std::distance(m_orderedData.begin(), it);
      m_orderedData.erase(it);
      m_data.erase(m_data.begin() + distance);
      return true;
    }
  }

  return false;
}

template<class T>
inline const T& SpecialMap<T>::operator[](const int index) const
{
  return m_data[index];
}

template<class T>
inline T& SpecialMap<T>::operator[](const int index)
{
  return m_data[index];
}

template<class T>
inline const std::vector<T> &SpecialMap<T>::data() const
{
  return m_data;
}

template class HYDROCOUPLESDK_EXPORT SpecialMap<HCPolygon*>;
template class HYDROCOUPLESDK_EXPORT SpecialMap<HCVertex*>;

#endif // SPECIALMAP_H
