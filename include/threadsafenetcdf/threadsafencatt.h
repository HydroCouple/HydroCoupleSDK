#ifndef THREADSAFENCATT_H
#define THREADSAFENCATT_H

#ifdef USE_NETCDF

#include "hydrocouplesdk.h"
#include <netcdf>

class ThreadSafeNcGroup;
class ThreadSafeNcVar;

class HYDROCOUPLESDK_EXPORT ThreadSafeNcAtt
{
  public:

    /*! Constructor generates a \ref isNull "null object". */
    ThreadSafeNcAtt ();

    //    /*! Constructor for non-null instances. */
    //    ThreadSafeNcAtt(bool nullObject);

    //    /*! The copy constructor. */
    //    ThreadSafeNcAtt(const ThreadSafeNcAtt& rhs);

    //    ThreadSafeNcAtt(const netCDF::NcAtt& rhs);

    /*! destructor */
    virtual ~ThreadSafeNcAtt() = 0;

    /*! Get the attribute name. */
    std::string getName() const;

    /*! Gets attribute length. */
    size_t  getAttLength() const;

    netCDF::NcType getType() const;

    /*! Gets parent group. */
    ThreadSafeNcGroup getParentGroup() const;

    /*! equivalence operator */
    bool operator== (const ThreadSafeNcAtt& rhs) const;

    /*!  != operator */
    bool operator!=(const ThreadSafeNcAtt& rhs) const;

    /*! \overload
     */
    void getValues(char* dataValues) const;
    /*! \overload
     */
    void getValues(unsigned char* dataValues) const;
    /*! \overload
     */
    void getValues(signed char* dataValues) const;
    /*! \overload
     */
    void getValues(short* dataValues) const;
    /*! \overload
     */
    void getValues(int* dataValues) const;
    /*! \overload
     */
    void getValues(long* dataValues) const;
    /*! \overload
     */
    void getValues(float* dataValues) const;
    /*! \overload
     */
    void getValues(double* dataValues) const;
    /*! \overload
     */
    void getValues(unsigned short* dataValues) const;
    /*! \overload
     */
    void getValues(unsigned int* dataValues) const;
    /*! \overload
     */
    void getValues(long long* dataValues) const;
    /*! \overload
     */
    void getValues(unsigned long long* dataValues) const;
    /*! \overload
     */
    void getValues(char** dataValues) const;

    /*! \overload
      (The string variable does not need preallocating.)
     */
    void getValues(std::string& dataValues) const;

    /*!
      Gets a netCDF attribute.
      The user must ensure that the variable "dataValues" has sufficient space to hold the attribute.
      \param  dataValues On return contains the value of the attribute.
      If the type of data values differs from the netCDF variable type, type conversion will occur.
      (However, no type conversion is carried out for variables using the user-defined data types:
      nc_Vlen, nc_Opaque, nc_Compound and nc_Enum.)
    */
    void getValues(void* dataValues) const;

    /*! Returns true if this object is null (i.e. it has no contents); otherwise returns false. */
    bool isNull() const;

    virtual netCDF::NcAtt *ncAtt() const = 0;

};

class HYDROCOUPLESDK_EXPORT ThreadSafeNcGroupAtt : public ThreadSafeNcAtt
{
  public:

    /*! Constructor generates a \ref isNull "null object". */
    ThreadSafeNcGroupAtt ();

    /*! The copy constructor. */
    ThreadSafeNcGroupAtt(const ThreadSafeNcGroupAtt& rhs) ;

    ThreadSafeNcGroupAtt(const netCDF::NcGroupAtt &rhs);

    /*!
    Constructor for an existing global attribute.
    \param  grp        Parent Group object.
    \param  index      The index (id) of the attribute.
  */
    ThreadSafeNcGroupAtt(const ThreadSafeNcGroup& grp, const int index);

    /*! assignment operator */
    ThreadSafeNcGroupAtt& operator= (const ThreadSafeNcGroupAtt& rhs);

    ~ThreadSafeNcGroupAtt(){}

    /*! equivalence operator */
    bool operator== (const ThreadSafeNcGroupAtt& rhs);

    /*! comparator operator */
    friend bool operator<(const ThreadSafeNcGroupAtt& lhs,const ThreadSafeNcGroupAtt& rhs);

    /*! comparator operator */
    friend bool operator>(const ThreadSafeNcGroupAtt& lhs,const ThreadSafeNcGroupAtt& rhs);

    netCDF::NcAtt *ncAtt() const override
    {
      return &const_cast<netCDF::NcGroupAtt&>(m_ncGroupAtt);
    }

    netCDF::NcGroupAtt ncGroupAtt() const
    {
      return m_ncGroupAtt;
    }

  private:

    netCDF::NcGroupAtt m_ncGroupAtt;
};



class HYDROCOUPLESDK_EXPORT ThreadSafeNcVarAtt : public ThreadSafeNcAtt
{
  public:

    /*! Constructor generates a \ref isNull "null object". */
    ThreadSafeNcVarAtt ();

    /*! The copy constructor. */
    ThreadSafeNcVarAtt(const ThreadSafeNcVarAtt& rhs) ;

    ThreadSafeNcVarAtt(const netCDF::NcVarAtt& rhs) ;

    /*! assignment operator */
    ThreadSafeNcVarAtt& operator= (const ThreadSafeNcVarAtt& rhs);

    /*!
    Constructor for an existing local attribute.
    \param  grp        Parent Group object.
    \param  NcVar      Parent NcVar object.
    \param  index      The index (id) of the attribute.
  */
    ThreadSafeNcVarAtt(const ThreadSafeNcGroup& grp, const ThreadSafeNcVar& ncVar, const int index);

    ~ThreadSafeNcVarAtt(){}

    /*! Returns the NcVar parent object. */
    ThreadSafeNcVar getParentVar() const;

    /*! comparator operator */
    friend bool operator<(const ThreadSafeNcVarAtt& lhs,const ThreadSafeNcVarAtt& rhs);

    /*! comparator operator  */
    friend bool operator>(const ThreadSafeNcVarAtt& lhs,const ThreadSafeNcVarAtt& rhs);


    netCDF::NcAtt *ncAtt() const override
    {
      return &const_cast<netCDF::NcVarAtt&>(m_ncVarAtt);
    }

    netCDF::NcVarAtt ncVarAtt() const
    {
      return m_ncVarAtt;
    }

  private:

    netCDF::NcVarAtt m_ncVarAtt;

};

#endif
#endif // THREADSAFENCATT_H
