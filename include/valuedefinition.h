#ifndef VALUEDEFINITION_H
#define VALUEDEFINITION_H

#include "valuedefinition.h"
#include "description.h"
#include <QXmlStreamReader>

class Quality;
class Unit;

/*!
 * \brief The ValueDefinition class
 */
class HYDROCOUPLESDK_EXPORT ValueDefinition : public Description , public virtual HydroCouple::IValueDefinition
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IValueDefinition)
      Q_PROPERTY(QVariant::Type Type READ type)
      Q_PROPERTY(QVariant MissingValue READ missingValue WRITE setMissingValue NOTIFY propertyChanged)
      Q_PROPERTY(QVariant DefaultValue READ defaultValue WRITE setDefaultValue NOTIFY propertyChanged)

   public:
      ValueDefinition(QVariant::Type type, QObject *parent);

      ValueDefinition(const QString &caption, QVariant::Type type, QObject *parent);

      virtual ~ValueDefinition();

      QVariant::Type type() const override;

      QVariant missingValue() const override;

      void setMissingValue(const QVariant &value);

      QVariant defaultValue() const override;

      void setDefaultValue(const QVariant &defaultValue);

      virtual void readData(QXmlStreamReader &xmlReader) = 0;

      virtual void writeData(QXmlStreamWriter &xmlWriter) = 0;

      static  QString serializeData(const QVariant &value);

      static QVariant deserializeData(const QString &value);

   protected:
      bool validateData(const QVariant& value);

   signals:

      void propertyChanged(const QString &propertyName) override;

   private:
      QVariant::Type m_type;
      QVariant m_missingValue, m_defaultValue;
};

/*!
 * \brief The Quality class
 */
class HYDROCOUPLESDK_EXPORT Quality : public ValueDefinition , public virtual HydroCouple::IQuality
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IQuality)
      Q_PROPERTY(QList<QVariant> Categories READ categories WRITE setCategories NOTIFY propertyChanged)
      Q_PROPERTY(bool Ordered READ isOrdered WRITE setIsOrdered NOTIFY propertyChanged)

   public:
      Quality(QVariant::Type type, const QList<QVariant>& categories, QObject* parent);

      Quality(const QString& caption, QVariant::Type type, const QList<QVariant>& categories, QObject* parent);

      virtual ~Quality();

      QList<QVariant> categories() const override;

      void setCategories(const QList<QVariant>& categories) ;

      void addCategory(const QVariant &category);

      bool isOrdered() const override;

      void setIsOrdered(bool isOrdered);

      void readData(QXmlStreamReader &xmlReader) override;

      void writeData(QXmlStreamWriter &xmlWriter) override;

   signals:

      void propertyChanged(const QString &propertyName) override;

   private:
      QList<QVariant> m_categories;
      bool m_isOrdered;
};

/*!
 * \brief The Quantity class
 */
class HYDROCOUPLESDK_EXPORT Quantity : public ValueDefinition , public virtual HydroCouple::IQuantity
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IQuantity)
      Q_PROPERTY(HydroCouple::IUnit Unit READ unit NOTIFY propertyChanged)

   public:
      Quantity(QVariant::Type type, Unit *unit, QObject* parent = nullptr);

      Quantity(const QString &caption, QVariant::Type type, Unit *unit, QObject *parent = nullptr);

      virtual ~Quantity();

      HydroCouple::IUnit* unit() const override;

      void readData(QXmlStreamReader &xmlReader) override;

      void writeData(QXmlStreamWriter &xmlWriter) override;

      static Quantity* lengthInMeters(const QString &caption = "Length In Meters",
                                      const QString &description ="Length in Meters",
                                      QObject *parent = nullptr);

      static Quantity* lengthInFeet(const QString &caption = "Length In Feet",
                                    const QString &description ="Length in Feet",
                                    QObject *parent = nullptr);

      static Quantity* areaInSquareMeters(const QString &caption = "Area In Square Meters",
                                          const QString &description ="Area in Square Meters",
                                          QObject *parent = nullptr);

      static Quantity* areaInSquareFeet(const QString &caption = "Area In Square Feet",
                                        const QString &description ="Area In Square Feet",
                                        QObject *parent = nullptr);

      static Quantity* volumeInCubicMeters(const QString &caption = "Volume In Cubic Meters",
                                           const QString &description ="Volume In Cubic Meters",
                                           QObject *parent = nullptr);

      static Quantity* volumeInCubicFeet(const QString &caption = "Volume In Cubic Feet",
                                         const QString &description ="Volume In Cubic Feet",
                                         QObject *parent = nullptr);

      static Quantity* flowInCMS(const QString &caption = "Flow In Cubic Meters Per Second",
                                 const QString &description ="Flow In Cubic Meters Per Second",
                                 QObject *parent = nullptr);

      static Quantity* flowInCFS(const QString &caption = "Flow In Cubic Feet Per Second",
                                 const QString &description ="Flow In Cubic Feet Per Second",
                                 QObject *parent = nullptr);

      static Quantity* unitLessValues(const QString &caption = "Unitless",
                                     const QString &description ="Unitless",
                                     QVariant::Type type = QVariant::String,
                                     QObject *parent = nullptr);

   signals:

      void propertyChanged(const QString &propertyName) override;

   private:
      Unit* m_unit;

};

Q_DECLARE_METATYPE(ValueDefinition*)
Q_DECLARE_METATYPE(Quality*)
Q_DECLARE_METATYPE(Quantity*)

#endif // VALUEDEFINITION_H

