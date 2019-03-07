/*!
 * \file unit.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
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

#ifndef UNIT_H
#define UNIT_H

#include "description.h"

class UnitDimensions;

/*!
 * \brief The Unit class
 */
class HYDROCOUPLESDK_EXPORT Unit : public Description,
    public virtual HydroCouple::IUnit
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IUnit)
      Q_PROPERTY(HydroCouple::IUnitDimensions* Dimensions READ dimensions NOTIFY propertyChanged)
      Q_PROPERTY(double ConversionFactorToSI READ conversionFactorToSI WRITE setConversionFactorToSI NOTIFY propertyChanged)
      Q_PROPERTY(double OffsetToSI READ offsetToSI WRITE setOffsetToSI NOTIFY propertyChanged)

   public:

      Unit(QObject *parent = nullptr);

      Unit(const QString &caption, double conversionFactorToSI , double offsetToSI , QObject *parent = nullptr);

      virtual ~Unit();

      HydroCouple::IUnitDimensions* dimensions() const override;

      UnitDimensions* dimensionsInternal() const;

      double conversionFactorToSI() const override;

      void setConversionFactorToSI(double conversionFactor);

      double offsetToSI() const override;

      void setOffsetToSI(double offsetToSI);

      static Unit *copy(const HydroCouple::IUnit* unit, QObject *parent = nullptr);

      static Unit *lengthInMeters(QObject *parent = nullptr);

      static Unit *lengthInFeet(QObject *parent = nullptr);

      static Unit *areaInSquareMeters(QObject *parent = nullptr);

      static Unit *areaInSquareFeet(QObject *parent = nullptr);

      static Unit *volumeInCubicMeters(QObject *parent = nullptr);

      static Unit *volumeInCubicFeet(QObject *parent = nullptr);

      static Unit *flowInCMS(QObject *parent = nullptr);

      static Unit *flowInCFS(QObject *parent = nullptr);

      static Unit *unitlessCoefficient(QObject *parent = nullptr);

      static Unit *timeInDays(QObject *parent = nullptr);

      static Unit *timeInSeconds(QObject *parent = nullptr);

      static Unit *parseUnit(const QString &line, QObject *parent = nullptr);

   signals:

      void propertyChanged(const QString &propertyName) override;

   private:
      UnitDimensions* m_dimensions;
      double m_conversionFactorToSI, m_offsetToSI;

};

Q_DECLARE_METATYPE(Unit*)

#endif // UNIT_H

