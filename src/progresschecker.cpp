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
#include "progresschecker.h"


#include <assert.h>
#include <cmath>

ProgressChecker::ProgressChecker(double begin, double end, double numSteps, QObject *parent)
  :QObject(parent),
    m_progPerformed(false),
    m_begin(begin),
    m_end(end),
    m_nextStep(begin),
    m_value(begin)
{
  assert(begin < end);
  assert(numSteps > 0);

  m_step = (m_end - m_begin) / numSteps;

}

ProgressChecker::~ProgressChecker()
{

}

bool ProgressChecker::performStep(double value)
{
  m_value = value;

  if(value >= m_nextStep /*&& !m_progPerformed*/)
  {
//    m_progPerformed = true;
    m_progress = std::min(100.0, std::max((value - m_begin) * 100.0 / (m_end - m_begin),0.0));
    m_nextStep = m_begin + (int)((m_value - m_begin) / m_step) * m_step + m_step;
    emit progressChanged(m_progress);
    return true;
  }
  else
  {
//    m_progPerformed = false;
    return false;
  }
}

void ProgressChecker::reset(double begin, double end, double numSteps)
{
  assert(begin < end);
  assert(numSteps > 0);

  m_begin = begin;
  m_end = end;
  m_value = begin;
  m_step = (m_end - m_begin) / numSteps;
  m_progPerformed = false;
  m_nextStep = m_begin;
  m_progress = 0;
  emit progressChanged(m_progress);
}

void ProgressChecker::reset()
{
  m_progPerformed = false;
  m_nextStep = m_begin;
  m_value = m_begin;
  m_progress = 0;
}

double ProgressChecker::progress() const
{
  return m_progress;
}

bool ProgressChecker::isBusy() const
{
  if(m_progress > m_begin && m_progress < m_end)
  {
    return true;
  }
  else if(m_begin == m_end && m_progress > m_end)
  {
    return true;
  }

  return false;
}

double ProgressChecker::begin() const
{
  return m_begin;
}

double ProgressChecker::end() const
{
  return m_end;
}

double ProgressChecker::currentValue() const
{
  return m_value;
}

bool ProgressChecker::isFinished() const
{
  return currentValue() >= m_end;
}

void ProgressChecker::onPerformStep(double value)
{
  performStep(value);
}
