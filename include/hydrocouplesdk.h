/*! \file   hydrocouplesdk.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version   1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
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

#ifndef HYDROCOUPLESDK_H
#define HYDROCOUPLESDK_H

#include <QtCore/qglobal.h>

#ifdef HYDROCOUPLESDK_LIBRARY
# define HYDROCOUPLESDK_EXPORT Q_DECL_EXPORT
#else
# define HYDROCOUPLESDK_EXPORT Q_DECL_IMPORT
#endif

#endif // HYDROCOUPLESDK_H
