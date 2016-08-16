#include "stdafx.h"
#include "hydrocoupleexceptions.h"

NotImplementedException::NotImplementedException(const std::string &className, const std::string &functionName):
  std::logic_error(""),
  m_className(className),
  m_functionName(functionName)
{
}

const char* NotImplementedException::what() const throw()
{
  std::string value = "The function " + m_functionName + " from the class " + m_className + " has not been implemented yet";
  return value.c_str();
}
