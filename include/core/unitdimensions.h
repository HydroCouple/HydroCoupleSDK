#ifndef UNITDIMENSIONS_H
#define UNITDIMENSIONS_H

#include "description.h"

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

      double getPower(HydroCouple::FundamentalUnitDimension dimension) const override;

      void setPower(HydroCouple::FundamentalUnitDimension dimension, double power);

   signals:

      void propertyChanged(const QString &propertyName) override;

   private:
      QMap<HydroCouple::FundamentalUnitDimension,double> m_powers;

};

Q_DECLARE_METATYPE(UnitDimensions*)


#endif // UNITDIMENSIONS_H

