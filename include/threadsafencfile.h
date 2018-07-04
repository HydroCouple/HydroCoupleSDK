#ifndef THREADSAFENCFILE_H
#define THREADSAFENCFILE_H

#ifdef USE_NETCDF

   #include "hydrocouple.h"
   #include <netcdf>

   class HYDROCOUPLE_EXPORT ThreadSafeNcFile
   {

     public:

       ThreadSafeNcFile();

       ThreadSafeNcFile(const std::string& filePath, netCDF::NcFile::FileMode fMode);

       ThreadSafeNcFile(const std::string& filePath, netCDF::NcFile::FileMode fMode, netCDF::NcFile::FileFormat fFormat);

       ~ThreadSafeNcFile();

       void open(const std::string& filePath, netCDF::NcFile::FileMode fMode);

       void open(const std::string& filePath, netCDF::NcFile::FileMode fMode, netCDF::NcFile::FileFormat fFormat);

       void sync();

       void close();

       netCDF::NcFile *ncFile() const;

       int getDimSize(const std::string &varName);

       bool putVar(const std::string &varName,
                   const std::vector<size_t> &startp,
                   const double values);

       bool putVar(const std::string &varName,
                   const std::vector<size_t> &startp,
                   const std::vector<size_t> &countp,
                   const double* values);

     private:

       netCDF::NcFile *m_ncFile;

   };

#endif

#endif // THREADSAFENCFILE_H
