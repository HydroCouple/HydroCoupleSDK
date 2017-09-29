#include "stdafx.h"
#include "progresschecker.h"

#include <assert.h>

ProgressChecker::ProgressChecker(double begin, double end, double numSteps, QObject *parent)
  :QObject(parent),
    m_progPerformed(false),
    m_begin(begin),
    m_end(end),
    m_nextStep(begin)
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
  if(value >= m_nextStep && !m_progPerformed)
  {
    m_progPerformed = true;
    m_progress = std::min(100.0, std::max((value - m_begin) * 100.0 / (m_end - m_begin),0.0));
    m_nextStep += m_step;
    emit progressChanged(m_progress);
    return true;
  }
  else
  {

    m_progPerformed = false;
    return false;
  }
}

void ProgressChecker::reset(double begin, double end, double numSteps)
{
  assert(begin < end);
  assert(numSteps > 0);

  m_begin = begin;
  m_end = end;
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
  m_progress = 0;
}

double ProgressChecker::progress() const
{
  return m_progress;
}

void ProgressChecker::onPerformStep(double value)
{
  performStep(value);
}
