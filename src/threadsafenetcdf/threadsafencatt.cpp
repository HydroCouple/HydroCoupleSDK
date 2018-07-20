#include "stdafx.h"

#ifdef USE_NETCDF

#include "threadsafenetcdf/threadsafencatt.h"
#include "threadsafenetcdf/threadsafencgroup.h"
#include "threadsafenetcdf/threadsafencvar.h"

using namespace netCDF;

ThreadSafeNcAtt::ThreadSafeNcAtt()
{
}

ThreadSafeNcAtt::~ThreadSafeNcAtt()
{

}

std::string ThreadSafeNcAtt::getName() const
{
  return ncAtt()->getName();
}

NcType ThreadSafeNcAtt::getType() const
{
  return ncAtt()->getType();
}

ThreadSafeNcGroup ThreadSafeNcAtt::getParentGroup() const
{
  NcGroup parentGroup = ncAtt()->getParentGroup();
  return ThreadSafeNcGroup(parentGroup);
}


size_t  ThreadSafeNcAtt::getAttLength() const
{
  size_t attLength;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    attLength = ncAtt()->getAttLength();
  }

  return attLength;
}

bool ThreadSafeNcAtt::operator== (const ThreadSafeNcAtt& rhs) const
{
  return *ncAtt() == *rhs.ncAtt();
}

bool ThreadSafeNcAtt::operator!=(const ThreadSafeNcAtt& rhs) const
{
  return *ncAtt() != *rhs.ncAtt();
}

void ThreadSafeNcAtt::getValues(char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(unsigned char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(signed char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(float* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(double* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(unsigned short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(unsigned int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(unsigned long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(char** dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(std::string& dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

void ThreadSafeNcAtt::getValues(void* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncAtt()->getValues(dataValues);
  }
}

bool ThreadSafeNcAtt::isNull() const
{
  return ncAtt()->isNull();
}


ThreadSafeNcGroupAtt::ThreadSafeNcGroupAtt()
  : ThreadSafeNcAtt()
{
}

ThreadSafeNcGroupAtt::ThreadSafeNcGroupAtt(const ThreadSafeNcGroupAtt &rhs)
  : ThreadSafeNcAtt()
{
  m_ncGroupAtt = rhs.m_ncGroupAtt;
}

ThreadSafeNcGroupAtt::ThreadSafeNcGroupAtt(const NcGroupAtt &rhs)
  : ThreadSafeNcAtt()
{
  m_ncGroupAtt = rhs;
}


ThreadSafeNcGroupAtt::ThreadSafeNcGroupAtt(const ThreadSafeNcGroup &grp, const int index)
  : ThreadSafeNcAtt()
{
  m_ncGroupAtt = NcGroupAtt(grp.ncGroup(), index);
}

ThreadSafeNcGroupAtt& ThreadSafeNcGroupAtt::operator =(const ThreadSafeNcGroupAtt &rhs)
{
  this->m_ncGroupAtt = rhs.ncGroupAtt();
  return *this;
}

bool ThreadSafeNcGroupAtt::operator ==(const ThreadSafeNcGroupAtt& rhs)
{
  return m_ncGroupAtt == rhs.m_ncGroupAtt;
}

bool operator<(const ThreadSafeNcGroupAtt& lhs,const ThreadSafeNcGroupAtt& rhs)
{
  return lhs.m_ncGroupAtt < rhs.m_ncGroupAtt;
}

bool operator>(const ThreadSafeNcGroupAtt& lhs,const ThreadSafeNcGroupAtt& rhs)
{
  return lhs.m_ncGroupAtt > rhs.m_ncGroupAtt;
}

ThreadSafeNcVarAtt::ThreadSafeNcVarAtt()
  : ThreadSafeNcAtt()
{

}

ThreadSafeNcVarAtt::ThreadSafeNcVarAtt(const ThreadSafeNcVarAtt &rhs)
  : ThreadSafeNcAtt()
{
  m_ncVarAtt = rhs.m_ncVarAtt;
}

ThreadSafeNcVarAtt::ThreadSafeNcVarAtt(const NcVarAtt &rhs)
  : ThreadSafeNcAtt()
{
  m_ncVarAtt = rhs;
}

ThreadSafeNcVarAtt &ThreadSafeNcVarAtt::operator =(const ThreadSafeNcVarAtt &rhs)
{
  m_ncVarAtt = rhs.m_ncVarAtt;
  return *this;
}

ThreadSafeNcVarAtt::ThreadSafeNcVarAtt(const ThreadSafeNcGroup &grp, const ThreadSafeNcVar &ncVar, const int index)
  : ThreadSafeNcAtt()
{
  m_ncVarAtt = NcVarAtt(grp.ncGroup(), ncVar.ncVar(), index);
}

bool operator<(const ThreadSafeNcVarAtt& lhs,const ThreadSafeNcVarAtt& rhs)
{
  return lhs.m_ncVarAtt < rhs.m_ncVarAtt;
}

bool operator>(const ThreadSafeNcVarAtt& lhs,const ThreadSafeNcVarAtt& rhs)
{
  return lhs.m_ncVarAtt > rhs.m_ncVarAtt;
}

#endif
