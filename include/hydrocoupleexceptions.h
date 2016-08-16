#ifndef HYDROCOUPLEEXCEPTIONS_H
#define HYDROCOUPLEEXCEPTIONS_H

#include <stdexcept>
#include <string>

class NotImplementedException : public std::logic_error
{
  public:
    explicit NotImplementedException(const std::string& className,  const std::string& functionName);

    virtual ~NotImplementedException() throw() {}

    virtual const char* what() const throw() override ;

  private:
    std::string m_className, m_functionName;
};

#endif // HYDROCOUPLEEXCEPTIONS_H
