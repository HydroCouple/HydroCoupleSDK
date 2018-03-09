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
