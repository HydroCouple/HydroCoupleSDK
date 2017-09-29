#include "stdafx.h"
#include "core/datacursor.h"

DataCursor::DataCursor()
  :m_min(0), m_max(std::numeric_limits<int>::max())
{
  m_index = m_min;
}

DataCursor::DataCursor(int min, int max)
  :m_min(min), m_max(max)
{
  m_index = m_min;
}

DataCursor::~DataCursor()
{
}

int DataCursor::min() const
{
  return m_min;
}

void DataCursor::setMin(int min)
{
  m_min = min;
  m_index =  m_index < m_min ? m_min : m_index;
}

int DataCursor::max() const
{
  return m_max;
}

void DataCursor::setMax(int max)
{
  m_max = max;
  m_index =  m_index > m_max ? m_max : m_index;
}

int DataCursor::index() const
{
  return m_index;
}

int DataCursor::moveNext()
{
  m_index++;
  m_index =  m_index > m_max ? m_min : m_index;
  return m_index;
}

void DataCursor::resetCursor()
{
  m_index = m_min;
}
