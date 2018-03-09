/*!
 * \file  componentdataitem.h
 * \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#ifndef COMPONENTDATAITEM_H
#define COMPONENTDATAITEM_H

#include "hydrocouplesdk.h"

#include <vector>
#include <QString>
/*!
 * \brief The ComponentDataItem1D class
 */
template<class T>
class HYDROCOUPLESDK_EXPORT ComponentDataItem1D
{

  public:

    ComponentDataItem1D(const QString &id, int length, const T& defaultValue);

    virtual ~ComponentDataItem1D();

    virtual QString getId() const;

    void getValueT(const std::vector<int> &dimensionIndexes, void *data) const;

    void getValuesT(int dimensionIndexes[], int stride[],  void *data) const ;

    void getValuesT(int dimensionIndex, int stride,  void *data) const ;

    void setValueT(const std::vector<int> &dimensionIndexes, const void *data);

    void setValuesT(int dimensionIndexes[], int stride[], const void *data);

    void setValuesT(int dimensionIndex, int stride, const void *data) ;

    T defaultValue() const;

    T& operator[](int index);

    const T& operator [](int index) const;

    int length() const;

    void resetDataArray();

  protected:

    virtual void resizeDataArray(int length, bool initializeNewWithDefault = true);

    virtual void removeItemAt(int i);

    void setDefaultValue(const T& defaultValue);

  private:
    int m_length;
    T m_defaultValue;
    std::vector<T> m_data;
    QString m_id;
};



/*!
 * \brief The ComponentDataItem2D class
 */
template<class T>
class HYDROCOUPLESDK_EXPORT ComponentDataItem2D
{

  public:

    ComponentDataItem2D(const QString &id, int iLength, int jLength, const T& defaultValue);

    virtual ~ComponentDataItem2D();

    virtual QString getId() const;

    void getValueT(const std::vector<int> &dimensionIndexes, void *data) const;

    void getValueT(int iIndex, int jIndex, void *data) const;

    void getValuesT(int dimensionIndexes[], int stride[],  void *data) const ;

    void getValuesT(int iIndex, int jIndex, int iStride, int jStride, void *data) const ;

    void setValueT(const std::vector<int> &dimensionIndexes, const void *data);

    void setValueT(int iIndex, int jIndex, const void *data);

    void setValuesT(int dimensionIndexes[], int stride[], const void *data);

    void setValuesT(int iIndex, int jIndex, int iStride, int jStride, const void *data);

    T defaultValue() const;

    T& operator()(int i, int j);

    const T& operator()(int i , int j) const;

    int iLength() const;

    int jLength() const;

    void resetDataArray();

  protected:

    virtual void resizeDataArrayILength(int iLength, bool initializeNewWithDefault = true);

    virtual void removeIItemAt(int i);

    virtual void resizeDataArrayJLength(int jLength, bool initializeNewWithDefault = true);

    virtual void removeJItemAt(int j);

    virtual void resizeDataArray(int iLength, int jLength,  bool initializeNewWithDefault = true);

    void setDefaultValue(const T& defaultValue);

  private:

    int m_iLength, m_jLength;
    T m_defaultValue;
    std::vector<std::vector<T>> m_data;
    QString m_id;
};

#endif // COMPONENTDATAITEM_H
