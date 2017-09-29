/*!
 *  \file    datacursor.h
 *  \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version 1.0.0.0
 *  \section Description
 *  \section License
 *  datacursor.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  abstractadaptedoutput.h its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2016
 *  \pre
 *  \bug
 *  \todo
 *  \warning
 */

#ifndef DATACURSOR_H
#define DATACURSOR_H

#include "hydrocouplesdk.h"

class HYDROCOUPLESDK_EXPORT DataCursor
{
  public:

    DataCursor();

    DataCursor(int min, int max);

    ~DataCursor();

    int min() const;

    void setMin(int min);

    int max() const;

    void setMax(int max);

    int index() const;

    int moveNext();

    void resetCursor();

  private:

    int m_min, m_max, m_index;
};


#endif // DATACURSOR_H
