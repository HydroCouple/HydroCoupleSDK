#include "stdafx.h"

#ifdef USE_NETCDF

#include "threadsafencfile.h"

using namespace netCDF;

ThreadSafeNcFile::ThreadSafeNcFile()
{
#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    m_ncFile = new NcFile();
  }
}

ThreadSafeNcFile::ThreadSafeNcFile(const std::string& filePath, NcFile::FileMode fMode)
{
#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    m_ncFile = new NcFile(filePath, fMode);
  }
}

ThreadSafeNcFile::ThreadSafeNcFile(const std::string& filePath, NcFile::FileMode fMode, NcFile::FileFormat fFormat)
{
#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    m_ncFile = new NcFile(filePath, fMode, fFormat);
  }
}

ThreadSafeNcFile::~ThreadSafeNcFile()
{
#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    if(!m_ncFile->isNull())
    {
      m_ncFile->sync();
      m_ncFile->close();
    }

    delete m_ncFile;
  }
}

void ThreadSafeNcFile::open(const std::string &filePath, NcFile::FileMode fMode)
{
#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    m_ncFile->open(filePath, fMode);
  }
}

void ThreadSafeNcFile::open(const std::string& filePath, netCDF::NcFile::FileMode fMode, netCDF::NcFile::FileFormat fFormat)
{
#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    m_ncFile->open(filePath, fMode, fFormat);
  }
}

void ThreadSafeNcFile::sync()
{
#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    m_ncFile->sync();
  }
}

void ThreadSafeNcFile::close()
{
#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    m_ncFile->close();
  }
}

NcFile *ThreadSafeNcFile::ncFile() const
{
  return m_ncFile;
}

int ThreadSafeNcFile::getDimSize(const std::string &varName)
{
  int size = 0;

#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    NcDim dim =  m_ncFile->getDim(varName);
    size = dim.getSize();
  }

  return size;
}

bool ThreadSafeNcFile::putVar(const std::string &varName, const std::vector<size_t> &startp, const double values)
{
  bool returnVal = false;

#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    NcVar variable = m_ncFile->getVar(varName);

    if(!variable.isNull())
    {
      variable.putVar(startp, values);
      returnVal = true;
    }
  }

  return returnVal;
}

bool ThreadSafeNcFile::putVar(const std::string &varName, const std::vector<size_t> &startp, const std::vector<size_t> &countp, const double *values)
{
  bool returnVal = false;

#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    NcVar variable = m_ncFile->getVar(varName);

    if(!variable.isNull())
    {
      variable.putVar(startp, countp, values);
      returnVal = true;
    }
  }

  return returnVal;
}

#endif
