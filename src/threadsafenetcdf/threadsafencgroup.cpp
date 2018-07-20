#include "stdafx.h"

#ifdef USE_NETCDF

#include "threadsafenetcdf/threadsafencgroup.h"
#include "threadsafenetcdf/threadsafencvar.h"
#include "threadsafenetcdf/threadsafencdim.h"
#include "threadsafenetcdf/threadsafencatt.h"

using namespace netCDF;

ThreadSafeNcGroup& ThreadSafeNcGroup::operator=(const ThreadSafeNcGroup& rhs)
{
  this->m_ncGroup = rhs.m_ncGroup;
  return *this;
}

ThreadSafeNcGroup::ThreadSafeNcGroup()
{
  m_ncGroup = NcGroup();
}

ThreadSafeNcGroup::ThreadSafeNcGroup(int groupId)
{
  m_ncGroup = NcGroup(groupId);
}

ThreadSafeNcGroup::ThreadSafeNcGroup(const ThreadSafeNcGroup &rhs)
{
  m_ncGroup = NcGroup(rhs.m_ncGroup);
}

ThreadSafeNcGroup::ThreadSafeNcGroup(const NcGroup &rhs)
{
  m_ncGroup = rhs;
}

ThreadSafeNcGroup::~ThreadSafeNcGroup()
{

}

bool ThreadSafeNcGroup::operator==(const ThreadSafeNcGroup& rhs) const
{
  return m_ncGroup == rhs.m_ncGroup;
}

bool ThreadSafeNcGroup::operator!=(const ThreadSafeNcGroup& rhs) const
{
  return m_ncGroup != rhs.m_ncGroup;
}

bool operator < (const ThreadSafeNcGroup& lhs,const ThreadSafeNcGroup& rhs)
{
  return lhs.m_ncGroup < rhs.m_ncGroup;
}

bool operator > (const ThreadSafeNcGroup& lhs,const ThreadSafeNcGroup& rhs)
{
  return lhs.m_ncGroup > rhs.m_ncGroup;
}

std::string ThreadSafeNcGroup::getName(bool fullName) const
{
  std::string name;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    name = m_ncGroup.getName(fullName);
  }

  return name;
}

ThreadSafeNcGroup ThreadSafeNcGroup::getParentGroup() const
{
  ThreadSafeNcGroup returnNcGroup;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    returnNcGroup = ThreadSafeNcGroup(m_ncGroup.getParentGroup());
  }

  return returnNcGroup;
}

int ThreadSafeNcGroup::getId()  const
{
  return m_ncGroup.getId();
}

int ThreadSafeNcGroup::getGroupCount(netCDF::NcGroup::GroupLocation location) const
{
  int count = 0;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    count = m_ncGroup.getGroupCount(location);
  }

  return count;
}

std::multimap<std::string,ThreadSafeNcGroup> ThreadSafeNcGroup::getGroups(netCDF::NcGroup::GroupLocation location) const
{
  std::multimap<std::string, NcGroup> groups;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    groups = m_ncGroup.getGroups(location);
  }

  std::multimap<std::string, ThreadSafeNcGroup> threadsafeGroups;

  for(const auto &it : groups)
    threadsafeGroups.insert({it.first, ThreadSafeNcGroup(it.second)});

  return threadsafeGroups;
}

std::set<ThreadSafeNcGroup> ThreadSafeNcGroup::getGroups(const std::string& name, netCDF::NcGroup::GroupLocation location) const
{
  std::set<NcGroup> groups;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    groups = m_ncGroup.getGroups(name, location);
  }

  std::set<ThreadSafeNcGroup> threadsafeGroups;

  for(const NcGroup &group : groups)
    threadsafeGroups.insert(ThreadSafeNcGroup(group));

  return threadsafeGroups;
}

ThreadSafeNcGroup ThreadSafeNcGroup::getGroup(const std::string& name, netCDF::NcGroup::GroupLocation location) const
{
  NcGroup group;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    group = m_ncGroup.getGroup(name, location);
  }

  return ThreadSafeNcGroup(group);
}

ThreadSafeNcGroup ThreadSafeNcGroup::addGroup(const std::string& name) const
{
  NcGroup group;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    group = m_ncGroup.addGroup(name);
  }

  return ThreadSafeNcGroup(group);

}

bool ThreadSafeNcGroup::isNull() const
{
  return m_ncGroup.isNull();
}

bool ThreadSafeNcGroup::isRootGroup() const
{
  bool isRoot;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    isRoot = m_ncGroup.isRootGroup();
  }

  return isRoot;
}

int ThreadSafeNcGroup::getVarCount(netCDF::NcGroup::Location location) const
{
  int varCount  = 0;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varCount = m_ncGroup.getVarCount(location);
  }

  return varCount;
}

std::multimap<std::string,ThreadSafeNcVar> ThreadSafeNcGroup::getVars(netCDF::NcGroup::Location location) const
{
  std::multimap<std::string, NcVar> ncVars;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncVars = m_ncGroup.getVars(location);
  }

  std::multimap<std::string, ThreadSafeNcVar> ncVarsTS;

  for(const auto& it : ncVars)
    ncVarsTS.insert({it.first, ThreadSafeNcVar(it.second)});

  return ncVarsTS;
}

std::set<ThreadSafeNcVar> ThreadSafeNcGroup::getVars(const std::string& name, netCDF::NcGroup::Location location) const
{
  std::set<NcVar> ncVars;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncVars = m_ncGroup.getVars(name, location);
  }

  std::set<ThreadSafeNcVar> ncVarsTS;

  for(const auto& ncVar : ncVars)
    ncVarsTS.insert(ThreadSafeNcVar(ncVar));

  return ncVarsTS;
}

ThreadSafeNcVar ThreadSafeNcGroup::getVar(const std::string& name, netCDF::NcGroup::Location location) const
{
  NcVar ncVar;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncVar = m_ncGroup.getVar(name, location);
  }

  return ThreadSafeNcVar(ncVar);

}

ThreadSafeNcVar ThreadSafeNcGroup::addVar(const std::string& name, const netCDF::NcType& ncType) const
{
  NcVar ncVar;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncVar = m_ncGroup.addVar(name, ncType);
  }

  return ThreadSafeNcVar(ncVar);
}

ThreadSafeNcVar ThreadSafeNcGroup::addVar(const std::string& name, const std::string& typeName, const std::string& dimName) const
{
  NcVar ncVar;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncVar = m_ncGroup.addVar(name, typeName, dimName);
  }

  return ThreadSafeNcVar(ncVar);
}

ThreadSafeNcVar ThreadSafeNcGroup::addVar(const std::string& name, const netCDF::NcType& ncType, const ThreadSafeNcDim& ncDim) const
{
  NcVar ncVar;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncVar = m_ncGroup.addVar(name, ncType, ncDim.ncDim());
  }

  return ThreadSafeNcVar(ncVar);
}

ThreadSafeNcVar ThreadSafeNcGroup::addVar(const std::string& name, const std::string& typeName, const std::vector<std::string>& dimNames) const
{
  NcVar ncVar;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncVar = m_ncGroup.addVar(name, typeName, dimNames);
  }

  return ThreadSafeNcVar(ncVar);

}

ThreadSafeNcVar ThreadSafeNcGroup::addVar(const std::string& name, const netCDF::NcType& ncType, const std::vector<ThreadSafeNcDim>& ncDimVector) const
{
  NcVar ncVar;
  std::vector<NcDim> ncDimVectorOr;

  for(const ThreadSafeNcDim &dim : ncDimVector)
    ncDimVectorOr.push_back(dim.ncDim());

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    ncVar = m_ncGroup.addVar(name, ncType, ncDimVectorOr);
  }

  return ThreadSafeNcVar(ncVar);
}

int ThreadSafeNcGroup::getAttCount(netCDF::NcGroup::Location location) const
{
  int nAttCount = 0;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    nAttCount = m_ncGroup.getAttCount(location);
  }


  return nAttCount;

}

std::multimap<std::string,ThreadSafeNcGroupAtt> ThreadSafeNcGroup::getAtts(netCDF::NcGroup::Location location) const
{
  std::multimap<std::string, NcGroupAtt> atts;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    atts = m_ncGroup.getAtts(location);
  }

  std::multimap<std::string, ThreadSafeNcGroupAtt> attsTS;

  for(const auto & att : atts)
    attsTS.insert({att.first, ThreadSafeNcGroupAtt(att.second)});

  return attsTS;
}

std::set<ThreadSafeNcGroupAtt> ThreadSafeNcGroup::getAtts(const std::string& name, netCDF::NcGroup::Location location) const
{
  std::set<NcGroupAtt> atts;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    atts = m_ncGroup.getAtts(name, location);
  }

  std::set<ThreadSafeNcGroupAtt> attsTS;

  for(const auto & att : atts)
    attsTS.insert(ThreadSafeNcGroupAtt(att));

  return attsTS;
}

ThreadSafeNcGroupAtt ThreadSafeNcGroup::getAtt(const std::string& name, netCDF::NcGroup::Location location) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.getAtt(name, location);
  }

  return  ThreadSafeNcGroupAtt(att);
}

ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, size_t len, const char** dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}

ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const std::string& dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}

ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, short datumValue) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, datumValue);
  }

  return  ThreadSafeNcGroupAtt(att);
}

ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, int datumValue) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, datumValue);
  }

  return  ThreadSafeNcGroupAtt(att);
}

ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, long datumValue) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, datumValue);
  }

  return  ThreadSafeNcGroupAtt(att);
}

ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, float datumValue) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, datumValue);
  }

  return  ThreadSafeNcGroupAtt(att);
}

ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, double datumValue) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, datumValue);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, unsigned short datumValue) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, datumValue);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, unsigned int datumValue) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, datumValue);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, unsigned long long datumValue) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, datumValue);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, long long datumValue) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, datumValue);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const unsigned char* dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const signed char* dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const short* dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const int* dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const long* dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const float* dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const double* dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const unsigned short* dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const unsigned int* dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const unsigned long long* dataValues) const
{
  NcGroupAtt att;

 #ifdef USE_OPENMP
 #pragma omp critical (ThreadSafeNcFile)
 #endif
   {
     att = m_ncGroup.putAtt(name,type, len, dataValues);
   }

   return  ThreadSafeNcGroupAtt(att);

}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const long long* dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}


ThreadSafeNcGroupAtt ThreadSafeNcGroup::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const void* dataValues) const
{
  NcGroupAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncGroup.putAtt(name,type, len, dataValues);
  }

  return  ThreadSafeNcGroupAtt(att);
}

int ThreadSafeNcGroup::getDimCount(netCDF::NcGroup::Location location) const
{
  int dimCount = 0;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    dimCount  = m_ncGroup.getDimCount(location);
  }

  return dimCount;
}

std::multimap<std::string,ThreadSafeNcDim> ThreadSafeNcGroup::getDims(netCDF::NcGroup::Location location) const
{
  std::multimap<std::string, NcDim> dims;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    dims  = m_ncGroup.getDims(location);
  }

  std::multimap<std::string, ThreadSafeNcDim> dimsTS;

  for(const auto &dim : dims)
    dimsTS.insert({dim.first, ThreadSafeNcDim(dim.second)});

  return dimsTS;
}

std::set<ThreadSafeNcDim> ThreadSafeNcGroup::getDims(const std::string& name, netCDF::NcGroup::Location location) const
{
  std::set<NcDim> dims;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    dims  = m_ncGroup.getDims(name, location);
  }

  std::set<ThreadSafeNcDim> dimsTS;

  for(const auto &dim : dims)
    dimsTS.insert(dim);

  return dimsTS;
}

ThreadSafeNcDim ThreadSafeNcGroup::getDim(const std::string& name, netCDF::NcGroup::Location location) const
{
  NcDim dim;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    dim  = m_ncGroup.getDim(name, location);
  }

  return ThreadSafeNcDim(dim);
}

ThreadSafeNcDim ThreadSafeNcGroup::addDim(const std::string& name, size_t dimSize) const
{
  NcDim dim;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    dim  = m_ncGroup.addDim(name, dimSize);
  }

  return ThreadSafeNcDim(dim);
}

ThreadSafeNcDim ThreadSafeNcGroup::addDim(const std::string& name) const
{
  NcDim dim;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    dim  = m_ncGroup.addDim(name);
  }

  return ThreadSafeNcDim(dim);
}

int ThreadSafeNcGroup::getTypeCount(netCDF::NcGroup::Location location) const
{
  int typeCount = 0;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    typeCount = m_ncGroup.getTypeCount(location);
  }

  return typeCount;
}

int ThreadSafeNcGroup::getTypeCount(netCDF::NcType::ncType enumType, netCDF::NcGroup::Location location) const
{
  int typeCount = 0;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    typeCount = m_ncGroup.getTypeCount(enumType, location);
  }

  return typeCount;
}

std::multimap<std::string,netCDF::NcType> ThreadSafeNcGroup::getTypes(netCDF::NcGroup::Location location) const
{
  std::multimap<std::string, NcType> types;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    types = m_ncGroup.getTypes(location);
  }

  return types;
}

std::set<netCDF::NcType> ThreadSafeNcGroup::getTypes(const std::string& name, netCDF::NcGroup::Location location) const
{
  std::set<NcType> types;

  types = m_ncGroup.getTypes(name, location);

  return types;
}

std::set<netCDF::NcType> ThreadSafeNcGroup::getTypes(netCDF::NcType::ncType enumType, netCDF::NcGroup::Location location) const
{
  std::set<NcType> types;

  types = m_ncGroup.getTypes(enumType, location);

  return types;
}

std::set<netCDF::NcType> ThreadSafeNcGroup::getTypes(const std::string& name, netCDF::NcType::ncType enumType, netCDF::NcGroup::Location location) const
{
  std::set<NcType> types;

  types = m_ncGroup.getTypes(name, enumType, location);

  return types;
}

netCDF::NcType ThreadSafeNcGroup::getType(const std::string& name, netCDF::NcGroup::Location location) const
{

  return m_ncGroup.getType(name, location);

}

netCDF::NcEnumType ThreadSafeNcGroup::addEnumType(const std::string& name, netCDF::NcEnumType::ncEnumType basetype) const
{
  NcEnumType enumType;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    enumType = m_ncGroup.addEnumType(name, basetype);
  }

  return enumType;
}

netCDF::NcVlenType ThreadSafeNcGroup::addVlenType(const std::string& name,netCDF::NcType& basetype) const
{
  NcVlenType vlenType;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    vlenType = m_ncGroup.addVlenType(name, basetype);
  }

  return vlenType;
}

netCDF::NcOpaqueType ThreadSafeNcGroup::addOpaqueType(const std::string& name, size_t size) const
{
  NcOpaqueType opaqueType;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    opaqueType = m_ncGroup.addOpaqueType(name, size);
  }

  return opaqueType;
}

netCDF::NcCompoundType ThreadSafeNcGroup::addCompoundType(const std::string& name, size_t size) const
{
  NcCompoundType compoundType;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    compoundType = m_ncGroup.addCompoundType(name, size);
  }

  return compoundType;
}

std::map<std::string,ThreadSafeNcGroup> ThreadSafeNcGroup::getCoordVars(netCDF::NcGroup::Location location) const
{
  std::map<std::string, NcGroup> coordVars = m_ncGroup.getCoordVars(location);

  std::map<std::string, ThreadSafeNcGroup> coordVarsTS;

  for(const auto &coordVar : coordVars)
    coordVarsTS[coordVar.first] = ThreadSafeNcGroup(coordVar.second);


  return coordVarsTS;
}

void ThreadSafeNcGroup::getCoordVar(std::string& coordVarName, ThreadSafeNcDim& ncDim, ThreadSafeNcVar& ncVar, netCDF::NcGroup::Location location) const
{
  NcDim dim;
  NcVar var;

  m_ncGroup.getCoordVar(coordVarName, dim, var, location);


  ncDim.m_ncDim = dim;
  ncVar.m_ncVar = var;
}


NcGroup ThreadSafeNcGroup::ncGroup() const
{
  return m_ncGroup;
}

#endif
