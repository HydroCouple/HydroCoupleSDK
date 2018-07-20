#include "stdafx.h"
#ifdef USE_NETCDF

#include "threadsafenetcdf/threadsafencvar.h"
#include "threadsafenetcdf/threadsafencgroup.h"
#include "threadsafenetcdf/threadsafencdim.h"
#include "threadsafenetcdf/threadsafencatt.h"

using namespace  netCDF;

ThreadSafeNcVar::ThreadSafeNcVar()
{
  m_ncVar = NcVar();
}

ThreadSafeNcVar::ThreadSafeNcVar(const ThreadSafeNcGroup& grp, const int& varId)
{
  m_ncVar = NcVar(grp.ncGroup(), varId);
}


ThreadSafeNcVar& ThreadSafeNcVar::operator =(const ThreadSafeNcVar& rhs)
{
  this->m_ncVar = NcVar(rhs.m_ncVar);
  return  *this;
}

bool ThreadSafeNcVar::operator ==(const ThreadSafeNcVar& rhs) const
{
  return this->m_ncVar == rhs.m_ncVar;
}

bool ThreadSafeNcVar::operator !=(const ThreadSafeNcVar& rhs) const
{
  return this->m_ncVar != rhs.m_ncVar;
}


ThreadSafeNcVar::ThreadSafeNcVar(const ThreadSafeNcVar &ncVar)
{
  this->m_ncVar = NcVar(ncVar.m_ncVar);
}

ThreadSafeNcVar::ThreadSafeNcVar(const NcVar &ncVar)
{
  this->m_ncVar = NcVar(ncVar);
}

ThreadSafeNcVar::~ThreadSafeNcVar()
{

}

std::string ThreadSafeNcVar::getName() const
{
  std::string name;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    name = m_ncVar.getName();
  }

  return name;
}

ThreadSafeNcGroup ThreadSafeNcVar::getParentGroup() const
{
  return m_ncVar.getParentGroup();
}

netCDF::NcType ThreadSafeNcVar::getType() const
{
  return m_ncVar.getType();
}

void ThreadSafeNcVar::rename(const std::string& newname ) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.rename(newname);
  }
}

int  ThreadSafeNcVar::getId() const
{
  return m_ncVar.getId();
}

bool ThreadSafeNcVar::isNull() const
{
  return m_ncVar.isNull();
}

bool operator<(const ThreadSafeNcVar& lhs,const ThreadSafeNcVar& rhs)
{
  return lhs.m_ncVar < rhs.m_ncVar;
}

/*! comparator operator  */
bool operator>(const ThreadSafeNcVar& lhs,const ThreadSafeNcVar& rhs)
{
  return lhs.m_ncVar > rhs.m_ncVar;
}

int ThreadSafeNcVar::getDimCount() const
{
  int dimCount;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    dimCount = m_ncVar.getDimCount();
  }

  return dimCount;
}

ThreadSafeNcDim ThreadSafeNcVar::getDim(int i) const
{
  NcDim dim;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    dim = m_ncVar.getDim(i);
  }

  return ThreadSafeNcDim(dim);
}


std::vector<ThreadSafeNcDim> ThreadSafeNcVar::getDims() const
{
  std::vector<NcDim> dims;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    dims = m_ncVar.getDims();
  }

  std::vector<ThreadSafeNcDim> dimsTS;

  for(const auto& dim: dims )
    dimsTS.push_back(ThreadSafeNcDim(dim));

  return dimsTS;
}

int ThreadSafeNcVar::getAttCount() const
{
  int attCount;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    attCount = m_ncVar.getAttCount();
  }


  return attCount;
}

ThreadSafeNcVarAtt ThreadSafeNcVar::getAtt(const std::string& name) const
{
  NcVarAtt att;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    att = m_ncVar.getAtt(name);
  }

  return ThreadSafeNcVarAtt(att);
}

std::map<std::string,ThreadSafeNcVarAtt> ThreadSafeNcVar::getAtts() const
{
  std::map<std::string, NcVarAtt> atts;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    atts = m_ncVar.getAtts();
  }

  std::map<std::string, ThreadSafeNcVarAtt> attsTS;

  for(const auto &att : atts)
    attsTS[att.first] = ThreadSafeNcVarAtt(att.second);

  return attsTS;
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, size_t len, const char** dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}

ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const std::string& dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const unsigned char* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const signed char* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, short datumValue) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, datumValue);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, int datumValue) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, datumValue);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, long datumValue) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, datumValue);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, float datumValue) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, datumValue);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, double datumValue) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, datumValue);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, unsigned short datumValue) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, datumValue);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, unsigned int datumValue) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, datumValue);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, unsigned long long datumValue) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, datumValue);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, long long datumValue) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, datumValue);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const short* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const int* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const long* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const float* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const double* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const unsigned short* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const unsigned int* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const unsigned long long* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const long long* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


ThreadSafeNcVarAtt ThreadSafeNcVar::putAtt(const std::string& name, const netCDF::NcType& type, size_t len, const void* dataValues) const
{
  NcVarAtt varAtt;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    varAtt = m_ncVar.putAtt(name, type, len, dataValues);
  }

  return ThreadSafeNcVarAtt(varAtt);
}


void ThreadSafeNcVar::setChunking(netCDF::NcVar::ChunkMode chunkMode, std::vector<size_t>& chunksizes) const
{

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.setChunking(chunkMode, chunksizes);
  }

}


void ThreadSafeNcVar::getChunkingParameters(netCDF::NcVar::ChunkMode& chunkMode, std::vector<size_t>& chunkSizes) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getChunkingParameters(chunkMode, chunkSizes);
  }
}


void ThreadSafeNcVar::setFill(bool fillMode,void* fillValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.setFill(fillMode, fillValue);
  }
}


void ThreadSafeNcVar::setFill(bool fillMode,const void* fillValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.setFill(fillMode, fillValue);
  }
}


template<class T>
void ThreadSafeNcVar::setFill(bool fillMode, T fillValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.setFill(fillMode, fillValue);
  }
}


void ThreadSafeNcVar::getFillModeParameters(bool& fillMode, void* fillValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getFillModeParameters(fillMode, fillValue);
  }
}


template <class T>
void ThreadSafeNcVar::getFillModeParameters(bool& fillMode,T& fillValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getFillModeParameters(fillMode, fillValue);
  }
}


void ThreadSafeNcVar::setCompression(bool enableShuffleFilter, bool enableDeflateFilter, int deflateLevel) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.setCompression(enableShuffleFilter, enableDeflateFilter, deflateLevel);
  }
}


void ThreadSafeNcVar::getCompressionParameters(bool& shuffleFilterEnabled, bool& deflateFilterEnabled, int& deflateLevel) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getCompressionParameters(shuffleFilterEnabled, deflateFilterEnabled, deflateLevel);
  }
}


void ThreadSafeNcVar::setEndianness(netCDF::NcVar::EndianMode endianMode) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.setEndianness(endianMode);
  }
}


netCDF::NcVar::EndianMode ThreadSafeNcVar::getEndianness() const
{
  netCDF::NcVar::EndianMode endDianMode;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    endDianMode = m_ncVar.getEndianness();
  }

  return endDianMode;
}


void ThreadSafeNcVar::setChecksum(netCDF::NcVar::ChecksumMode checksumMode) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.setChecksum(checksumMode);
  }
}


netCDF::NcVar::ChecksumMode ThreadSafeNcVar::getChecksum() const
{
  netCDF::NcVar::ChecksumMode checkSum;

#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    checkSum =  m_ncVar.getChecksum();
  }

  return checkSum;
}


void ThreadSafeNcVar::getVar(void* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

void ThreadSafeNcVar::getVar(char** dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

void ThreadSafeNcVar::getVar(char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

void ThreadSafeNcVar::getVar(unsigned char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

void ThreadSafeNcVar::getVar(signed char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(float* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(double* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(unsigned short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(unsigned int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(unsigned long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}


void ThreadSafeNcVar::getVar(long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(dataValues);
  }
}

void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, void* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, char** datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, char* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, unsigned char* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, signed char* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, short* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, int* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, long* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, float* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, double* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, unsigned short* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, unsigned int* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, unsigned long long* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& index, long long* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(index, datumValue);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, void* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, char** dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, unsigned char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, signed char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, float* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, double* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, unsigned short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, unsigned int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}

/*! \overload
 */
void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, unsigned long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count, long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, void* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, char** dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, unsigned char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, signed char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, float* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, double* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, unsigned short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, unsigned int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, unsigned long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, void* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, char** dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, unsigned char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, signed char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, float* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, double* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, unsigned short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, unsigned int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, unsigned long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::getVar(const std::vector<size_t>& start, const std::vector<size_t>& count,  const std::vector<ptrdiff_t>& stride, const std::vector<ptrdiff_t>& imap, long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.getVar(start, count, stride, imap, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const void* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const char** dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const unsigned char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const signed char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const float* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const double* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const unsigned short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const unsigned int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const unsigned long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const void* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const char** datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const std::string& datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const unsigned char* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const signed char* datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const short datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const int datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const long datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const float datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const double datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const unsigned short datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const unsigned int datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const unsigned long long datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& index, const long long datumValue) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(index, datumValue);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const void* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const char** dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const unsigned char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const signed char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const float* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const double* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const unsigned short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const unsigned int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const unsigned long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const void* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const char** dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const unsigned char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const signed char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const float* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const double* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const unsigned short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const unsigned int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const unsigned long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const void* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const char** dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const unsigned char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const signed char* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const float* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const double* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const unsigned short* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const unsigned int* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const unsigned long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}


void ThreadSafeNcVar::putVar(const std::vector<size_t>& startp, const std::vector<size_t>& countp, const std::vector<ptrdiff_t>& stridep, const std::vector<ptrdiff_t>& imapp, const long long* dataValues) const
{
#ifdef USE_OPENMP
#pragma omp critical (ThreadSafeNcFile)
#endif
  {
    m_ncVar.putVar(startp, countp, stridep, imapp, dataValues);
  }
}



#endif
