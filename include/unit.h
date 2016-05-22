#ifndef UNIT_H
#define UNIT_H

#include "description.h"

class UnitDimensions;

/*!
 * \brief The Unit class
 */
class HYDROCOUPLESDK_EXPORT Unit : public Description , public virtual HydroCouple::IUnit
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

      double conversionFactorToSI() const override;

      void setConversionFactorToSI(double conversionFactor);

      double offsetToSI() const override;

      void setOffsetToSI(double offsetToSI);

      static Unit* lengthInMeters(QObject *parent = nullptr);

      static Unit* lengthInFeet(QObject *parent = nullptr);

      static Unit* areaInSquareMeters(QObject *parent = nullptr);

      static Unit* areaInSquareFeet(QObject *parent = nullptr);

      static Unit* volumeInCubicMeters(QObject *parent = nullptr);

      static Unit* volumeInCubicFeet(QObject *parent = nullptr);

      static Unit* flowInCMS(QObject *parent = nullptr);

      static Unit* flowInCFS(QObject *parent = nullptr);

      static Unit* unitlessCoefficient(QObject *parent = nullptr);

   signals:

      void propertyChanged(const QString &propertyName) override;

   private:
      UnitDimensions* m_dimensions;
      double m_conversionFactorToSI, m_offsetToSI;

};

Q_DECLARE_METATYPE(Unit*)

#endif // UNIT_H

