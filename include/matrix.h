#ifndef MATRIX_H
#define MATRIX_H

#include <assert.h>
#include <cstdio>
/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

template <typename T>
class Matrix
{

  public:

    Matrix(int column, int row);

    explicit Matrix(int column, int row, const T *values);

    ~Matrix();

    const T& operator()(int row, int column) const;

    T& operator()(int row, int column);

    int rows() const {return m_r;}

    int columns() const {return m_c; }

    bool isIdentity() const;

    void setToIdentity();

    void fill(T value);

    Matrix<T> transposed() const;

    Matrix<T>& operator+=(const Matrix<T>& other);

    Matrix<T>& operator-=(const Matrix<T>& other);

    Matrix<T>& operator*=(T factor);

    Matrix<T>& operator/=(T divisor);

    bool operator==(const Matrix<T>& other) const;

    bool operator!=(const Matrix<T>& other) const;

    void copyDataTo(T *values) const;

    T *data() { return *m; }

    const T *data() const { return *m; }

    const T *constData() const { return *m; }

    template<typename TT>
    friend Matrix<TT> operator+(const Matrix<TT>& m1, const Matrix<TT>& m2);

    template<typename TT>
    friend Matrix<TT> operator-(const Matrix<TT>& m1, const Matrix<TT>& m2);

    template<typename TT>
    friend Matrix<TT> operator*(const Matrix<TT>& m1, const Matrix<TT>& m2);

    template<typename TT>
    friend Matrix<TT> operator-(const Matrix<TT>& matrix);

    template<typename TT>
    friend Matrix<TT> operator*(TT factor, const Matrix<TT>& matrix);

    template<typename TT>
    friend Matrix<TT> operator*(const Matrix<TT>& matrix, TT factor);

    template<typename TT>
    friend Matrix<TT> operator/(const Matrix<TT>& matrix, TT divisor);

    void printMatrix() const;

  private:
    int m_r, m_c;
    T **m;//[m_c][m_r];    // Column-major order to match OpenGL.
    template <typename TT>
    friend class Matrix;
};

template <typename T>
inline Matrix<T>::Matrix(int column, int row):
  m_r(row), m_c(column)
{
  m = new T*[m_c];

  for(int i = 0 ; i < m_c ; i++)
  {
    m[i] = new T[m_r]();
  }

//  setToIdentity();
}

template <typename T>
inline Matrix<T>::Matrix(int column, int row, const T *values):
  m_r(row), m_c(column)
{
  m = new T*[m_c];
  for(int i = 0 ; i < m_c ; i++)
  {
    m[i] = new T[m_r]();
  }

  for (int col = 0; col < m_c; ++col)
    for (int row = 0; row < m_r; ++row)
      m[col][row] = values[row * m_c + col];
}

template <typename T>
inline Matrix<T>::~Matrix()
{
  for(int i = 0 ; i < m_c ; i++)
  {
    delete[] m[i];
  }

  delete[] m;
}

template <typename T>
inline const T& Matrix<T>::operator()(int row, int column) const
{
  assert(row >= 0 && row < m_r && column >= 0 && column < m_c);

  return m[column][row];
}

template <typename T>
inline T& Matrix<T>::operator()(int row, int column)
{
  assert(row >= 0 && row < m_r && column >= 0 && column < m_c);

  return m[column][row];
}

template <typename T>
inline bool Matrix<T>::isIdentity() const
{
  for (int col = 0; col < m_c; ++col)
  {
    for (int row = 0; row < m_r; ++row)
    {
      if (row == col)
      {
        if (m[col][row] != 1.0f)
          return false;
      }
      else
      {
        if (m[col][row] != 0.0f)
          return false;
      }
    }
  }
  return true;
}

template <typename T>
inline void Matrix<T>::setToIdentity()
{
  for (int col = 0; col < m_c; ++col)
  {
    for (int row = 0; row < m_r; ++row)
    {
      if (row == col)
        m[col][row] = 1.0f;
      else
        m[col][row] = 0.0f;
    }
  }
}

template <typename T>
inline void Matrix<T>::fill(T value)
{
  for (int col = 0; col < m_c; ++col)
    for (int row = 0; row < m_r; ++row)
      m[col][row] = value;
}

template <typename T>
inline Matrix<T> Matrix<T>::transposed() const
{
  Matrix<T> result(m_r, m_c);

  for (int row = 0; row < m_r; ++row)
    for (int col = 0; col < m_c; ++col)
      result.m[row][col] = m[col][row];

  return result;
}

template <typename T>
inline Matrix<T>& Matrix<T>::operator+=(const Matrix<T>& other)
{
  assert(m_r == other.m_r && m_c == other.m_c);

  for (int row = 0; row < m_r; ++row)
    for (int col = 0; col < m_c; ++col)
      m[col][row] += other.m[col][row];

  return *this;
}

template <typename T>
inline Matrix<T>& Matrix<T>::operator-=(const Matrix<T>& other)
{
  assert(m_r == other.m_r && m_c == other.m_c);

  for (int row = 0; row < m_r; ++row)
    for (int col = 0; col < m_c; ++col)
      m[col][row] -= other.m[col][row];

  return *this;
}

template <typename T>
inline Matrix<T>& Matrix<T>::operator*=(T factor)
{
  for (int row = 0; row < m_r; ++row)
    for (int col = 0; col < m_c; ++col)
      m[col][row] *= factor;

  return *this;
}

template <typename T>
inline bool Matrix<T>::operator==(const Matrix<T>& other) const
{
  if(m_r != other.m_r || m_c != other.m_c)
  {
    return false;
  }

  for (int row = 0; row < m_r; ++row)
    for (int col = 0; col < m_c; ++col)
    {
      if (m[col][row] != other.m[col][row])
        return false;
    }

  return true;
}

template <typename T>
inline bool Matrix<T>::operator!=(const Matrix<T>& other) const
{
  if(m_r != other.m_r || m_c != other.m_c)
  {
    return true;
  }

  for (int row = 0; row < m_r; ++row)
  {
    for (int col = 0; col < m_c; ++col)
    {
      if (m[col][row] != other.m[col][row])
        return true;
    }
  }

  return false;
}

template <typename T>
inline Matrix<T>& Matrix<T>::operator/=(T divisor)
{
  for (int row = 0; row < m_r; ++row)
    for (int col = 0; col < m_c; ++col)
      m[col][row] /= divisor;

  return *this;
}

template <typename T>
inline Matrix<T> operator+(const Matrix<T>& m1, const Matrix<T>& m2)
{
  assert(m1.m_r == m2.m_r && m1.m_c == m2.m_c);

  Matrix<T> result(m1.m_c,m1.m_r);

  for (int row = 0; row < m1.m_r; ++row)
    for (int col = 0; col < m1.m_c; ++col)

      result.m[col][row] = m1.m[col][row] + m2.m[col][row];

  return result;
}

template <typename T>
inline Matrix<T> operator-(const Matrix<T>& m1, const Matrix<T>& m2)
{
  assert(m1.m_r == m2.m_r && m1.m_c == m2.m_c);

  Matrix<T> result(m1.m_c , m1.m_r);

  for (int row = 0; row < m1.m_r; ++row)
    for (int col = 0; col < m1.m_c; ++col)
      result.m[col][row] = m1.m[col][row] - m2.m[col][row];

  return result;
}

template <typename T>
inline Matrix<T> operator*(const Matrix<T>& m1, const Matrix<T>& m2)
{
  assert(m1.m_c == m2.m_r);

  Matrix<T> result (m2.m_c , m1.m_r);

  for (int row = 0; row < m1.m_r; ++row)
  {
    for (int col = 0; col < m2.m_c; ++col)
    {
      T sum(0.0f);

      for (int j = 0; j < m1.m_c; ++j)

        sum += m1.m[j][row] * m2.m[col][j];

      result.m[col][row] = sum;
    }
  }

  return result;
}

template <typename T>
inline Matrix<T> operator-(const Matrix<T>& matrix)
{
  Matrix<T> result(matrix.m_c,matrix.m_r);

  for (int row = 0; row < matrix.m_r; ++row)
    for (int col = 0; col < matrix.m_c; ++col)
      result.m[col][row] = -matrix.m[col][row];

  return result;
}

template <typename T>
inline Matrix<T> operator*(T factor, const Matrix<T>& matrix)
{
  Matrix<T> result(matrix.m_c,matrix.m_r);

  for (int row = 0; row < matrix.m_r; ++row)
    for (int col = 0; col < matrix.m_c; ++col)

      result.m[col][row] = matrix.m[col][row] * factor;

  return result;
}

template <typename T>
inline Matrix<T> operator*(const Matrix<T>& matrix, T factor)
{
  Matrix<T> result(matrix.m_c,matrix.m_r);

  for (int row = 0; row < matrix.m_r; ++row)
    for (int col = 0; col < matrix.m_c; ++col)

      result.m[col][row] = matrix.m[col][row] * factor;

  return result;
}

template <typename T>
inline Matrix<T> operator/(const Matrix<T>& matrix, T divisor)
{
  Matrix<T> result(matrix.m_c, matrix.m_r);

  for (int row = 0; row < matrix.m_r; ++row)
    for (int col = 0; col < matrix.m_c; ++col)
      result.m[col][row] = matrix.m[col][row] / divisor;

  return result;
}

template <typename T>
inline void Matrix<T>::copyDataTo(T *values) const
{
  for (int col = 0; col < m_c; ++col)
    for (int row = 0; row < m_r; ++row)
      values[row * m_c + col] = T(m[col][row]);
}

template <typename T>
inline void Matrix<T>::printMatrix() const
{
  for(int j = 0; j < m_r ; j++)
  {
    for(int i = 0; i < m_c ; i++)
    {
      printf("r%ic%i:%f\t", j,i,m[i][j]);
    }

    printf("\n");
  }

  printf("\n");
}

// Define aliases for the useful variants of Matrix.
//typedef Matrix<2, 2, float> Matrix2x2;
//typedef Matrix<2, 3, float> Matrix2x3;
//typedef Matrix<2, 4, float> Matrix2x4;
//typedef Matrix<3, 2, float> Matrix3x2;
//typedef Matrix<3, 3, float> Matrix3x3;
//typedef Matrix<3, 4, float> Matrix3x4;
//typedef Matrix<4, 2, float> Matrix4x2;
//typedef Matrix<4, 3, float> Matrix4x3;


#endif // MATRIX_H
