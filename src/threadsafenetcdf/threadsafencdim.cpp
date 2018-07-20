#include "stdafx.h"

#ifdef USE_NETCDF

#include "threadsafenetcdf/threadsafencdim.h"
#include "threadsafenetcdf/threadsafencgroup.h"

using namespace netCDF;

ThreadSafeNcDim::ThreadSafeNcDim()
{

}

ThreadSafeNcDim::ThreadSafeNcDim(const ThreadSafeNcGroup &grp, int dimId)
{
  m_ncDim = NcDim(grp.ncGroup(), dimId);
}

ThreadSafeNcDim::ThreadSafeNcDim(const ThreadSafeNcDim &ncDim)
{
  m_ncDim = NcDim(ncDim.m_ncDim);
}

ThreadSafeNcDim::ThreadSafeNcDim(const NcDim &ncDim)
{
  m_ncDim = ncDim;
}


ThreadSafeNcDim::~ThreadSafeNcDim()
{

}

bool ThreadSafeNcDim::operator ==(const ThreadSafeNcDim& rhs) const
{
  return m_ncDim == rhs.m_ncDim;
}

bool ThreadSafeNcDim::operator !=(const ThreadSafeNcDim& rhs) const
{
  return m_ncDim != rhs.m_ncDim;
}

const std::string ThreadSafeNcDim::getName() const
{
  std::string name;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    name = m_ncDim.getName();
  }

  return name;
}

int ThreadSafeNcDim::getId() const
{
  return m_ncDim.getId();
}

ThreadSafeNcGroup ThreadSafeNcDim::getParentGroup() const
{
  return ThreadSafeNcGroup(m_ncDim.getParentGroup());
}

bool ThreadSafeNcDim::isUnlimited() const
{
  bool unlimited = false;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    unlimited = m_ncDim.isUnlimited();
  }

  return unlimited;
}

size_t  ThreadSafeNcDim::getSize() const
{
  size_t dimSize = 0;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    dimSize = m_ncDim.getSize();
  }

  return dimSize;
}

void ThreadSafeNcDim::rename( const std::string& newName)
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncDim.rename(newName);
  }
}

bool ThreadSafeNcDim::isNull() const
{
  return m_ncDim.isNull();
}

bool operator<(const ThreadSafeNcDim& lhs,const ThreadSafeNcDim& rhs)
{
  return lhs.m_ncDim < rhs.m_ncDim;
}

bool operator>(const ThreadSafeNcDim& lhs,const ThreadSafeNcDim& rhs)
{
  return lhs.m_ncDim > rhs.m_ncDim;
}

NcDim ThreadSafeNcDim::ncDim() const
{
  return m_ncDim;
}

#endif
