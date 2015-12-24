/****************************************************************************
**
**  Copyright (C) 2014 Caleb Amoa Buahin
**  Contact: calebgh@gmail.com
**
**  This file is part of HydroCoupleSDK.dll
**
**  HydroCoupleSDK.dll and its associated files is free software; you can redistribute it and/or modify
**  it under the terms of the Lesser GNU General Public License as published by
**  the Free Software Foundation; either version 3 of the License, or
**  (at your option) any later version.
**
**  HydroCoupleSDK.dll and its associated files is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  Lesser GNU General Public License for more details.
**
**  You should have received a copy of the Lesser GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>
**
****************************************************************************/

#ifndef HYDROCOUPLESDK_H
#define HYDROCOUPLESDK_H

#include <QtCore/qglobal.h>

#ifdef HYDROCOUPLESDK_LIB
# define HYDROCOUPLESDK_EXPORT Q_DECL_EXPORT
#else
# define HYDROCOUPLESDK_EXPORT Q_DECL_IMPORT
#endif

#endif // HYDROCOUPLESDK_H
