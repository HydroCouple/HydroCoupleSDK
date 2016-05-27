#include "stdafx.h"
#include "core/dimension.h"
#include <stdexcept>


using namespace std;

Dimension::Dimension(const QString &id,int length, HydroCouple::DimensionLengthType lengthType, QObject *parent)
   :Identity(id,parent), m_length(length), m_previousLength(length), m_lengthType(lengthType)
{
   if(m_length < 1)
   {
     throw std::invalid_argument("length of dimension must be at least 1");
   }
}


Dimension::Dimension(const QString &id, const QString &caption, int length, HydroCouple::DimensionLengthType lengthType, QObject *parent)
   :Identity(id,caption,parent) , m_length(length), m_previousLength(length), m_lengthType(lengthType)
{
   if(m_length < 1)
   {
     throw std::invalid_argument("length of dimension must be at least 1");
   }
}

Dimension::~Dimension()
{

}

int Dimension::length() const
{
   return m_length;
}

void Dimension::setLength(int length)
{
   m_previousLength = m_length;
   m_length = length;
   emit propertyChanged("Length");
}

int Dimension::previousLength() const
{
   return m_previousLength;
}

HydroCouple::DimensionLengthType Dimension::lengthType() const
{
   return m_lengthType;
}
