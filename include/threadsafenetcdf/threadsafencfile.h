#ifndef THREADSAFENCFILE_H
#define THREADSAFENCFILE_H

#ifdef USE_NETCDF

   #include "threadsafenetcdf/threadsafencgroup.h"


   class HYDROCOUPLESDK_EXPORT ThreadSafeNcFile : public ThreadSafeNcGroup
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

     private:

       netCDF::NcFile *m_ncFile;

   };

#endif

#endif // THREADSAFENCFILE_H
