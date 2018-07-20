#ifndef THREADSAFENCDIM_H
#define THREADSAFENCDIM_H

#ifdef USE_NETCDF

#include "hydrocouplesdk.h"
#include <netcdf>

class ThreadSafeNcGroup;

class HYDROCOUPLESDK_EXPORT ThreadSafeNcDim
{
    friend class ThreadSafeNcGroup;

  public:
    /*! destructor*/

    /*! Constructor generates a \ref isNull "null object". */
    ThreadSafeNcDim ();

    /*!
      Constructor for a dimension .
      The dimension must already exist in the netCDF file. New netCDF variables can be added using NcGroup::addNcDim();
      \param grp    Parent NcGroup object.
      \param dimId  Id of the NcDim object.
    */
    ThreadSafeNcDim(const ThreadSafeNcGroup& grp, int dimId);

    /*! assignment operator  */
    ThreadSafeNcDim& operator =(const ThreadSafeNcDim &);

    ~ThreadSafeNcDim();

    /*! equivalence operator */
    bool operator==(const ThreadSafeNcDim& rhs) const;

    /*!  != operator */
    bool operator!=(const ThreadSafeNcDim& rhs) const;

    /*! The copy constructor. */
    ThreadSafeNcDim(const ThreadSafeNcDim& ncDim);

    ThreadSafeNcDim(const netCDF::NcDim& ncDim);

    /*! The name of this dimension.*/
    const std::string getName() const;

    /*! The netCDF Id of this dimension. */
    int getId() const;

    /*! Gets a  NcGroup object of the parent group. */
    ThreadSafeNcGroup getParentGroup() const;

    /*! Returns true if this is an unlimited dimension */
    bool isUnlimited() const;

    /*! The size of the dimension; for unlimited, this is the number of records written so far. */
    size_t  getSize() const;

    /*!renames the dimension */
    void rename( const std::string& newName);

    /*! Returns true if this object is null (i.e. it has no contents); otherwise returns false. */
    bool isNull() const;

    /*! comparator operator  */
    friend bool operator<(const ThreadSafeNcDim& lhs,const ThreadSafeNcDim& rhs);

    /*! comparator operator  */
    friend bool operator>(const ThreadSafeNcDim& lhs,const ThreadSafeNcDim& rhs);

    netCDF::NcDim ncDim() const;

  private:

    netCDF::NcDim m_ncDim;
};

#endif

#endif // THREADSAFENCDIM_H
