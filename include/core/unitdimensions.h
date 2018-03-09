/*!
 * \file unitdimensions.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
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

#ifndef UNITDIMENSIONS_H
#define UNITDIMENSIONS_H

#include "description.h"
#include <map>

/*!
 * \brief The UnitDimensions class
 */
class HYDROCOUPLESDK_EXPORT UnitDimensions : public Description,
    public virtual HydroCouple::IUnitDimensions
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IUnitDimensions)

   public:
      UnitDimensions(QObject* parent = nullptr);

      UnitDimensions(const QString& caption, QObject* parent = nullptr);

      virtual  ~UnitDimensions();

      double getPower(HydroCouple::FundamentalUnitDimension dimension) override;

      void setPower(HydroCouple::FundamentalUnitDimension dimension, double power);

   signals:

      void propertyChanged(const QString &propertyName) override;

   private:
      std::map<HydroCouple::FundamentalUnitDimension,double> m_powers;

};

Q_DECLARE_METATYPE(UnitDimensions*)


#endif // UNITDIMENSIONS_H

