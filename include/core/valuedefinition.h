/*!
 * \file valuedefinition.h
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
class HYDROCOUPLESDK_EXPORT ValueDefinition : public Description,
    public virtual HydroCouple::IValueDefinition
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

    virtual ValueDefinition *copy(QObject *parent = nullptr) = 0;

    static  ValueDefinition *copy(const HydroCouple::IValueDefinition *valueDefinition, QObject *parent = nullptr);

    static  QString serializeData(const QVariant &value);

    static QVariant deserializeData(const QString &value, QVariant::Type type);

  protected:

    bool validateData(const QVariant& value);

  signals:

    void propertyChanged(const QString &propertyName) override;

  protected:
    QVariant::Type m_type;

  private:
    QVariant m_missingValue, m_defaultValue;
};

/*!
 * \brief The Quality class
 */
class HYDROCOUPLESDK_EXPORT Quality : public ValueDefinition,
    public virtual HydroCouple::IQuality
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

    ValueDefinition *copy(QObject *parent = nullptr) override;

    static Quality *copy(const HydroCouple::IQuality *quality, QObject *parent = nullptr);

  signals:

    void propertyChanged(const QString &propertyName) override;

  private:

    QList<QVariant> m_categories;
    bool m_isOrdered;
};

/*!
 * \brief The Quantity class
 */
class HYDROCOUPLESDK_EXPORT Quantity : public ValueDefinition,
    public virtual HydroCouple::IQuantity
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IQuantity)
    Q_PROPERTY(HydroCouple::IUnit Unit READ unit NOTIFY propertyChanged)
    Q_PROPERTY(QVariant MinValue READ minValue WRITE setMinValue NOTIFY propertyChanged)
    Q_PROPERTY(QVariant MaxValue READ maxValue WRITE setMaxValue NOTIFY propertyChanged)

  public:
    Quantity(QVariant::Type type, Unit *unit, QObject* parent = nullptr);

    Quantity(const QString &caption, QVariant::Type type, Unit *unit, QObject *parent = nullptr);

    virtual ~Quantity();

    HydroCouple::IUnit* unit() const override;

    QVariant minValue() const override;

    void setMinValue(const QVariant& minValue);

    QVariant maxValue() const override;

    void setMaxValue(const QVariant& maxValue);

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

    ValueDefinition *copy(QObject *parent = nullptr) override;

    static Quantity *copy(const IQuantity* quantity, QObject *parent = nullptr);

    static Quantity* lengthInMeters(QObject *parent = nullptr);

    static Quantity* lengthInFeet(QObject *parent = nullptr);

    static Quantity* areaInSquareMeters(QObject *parent = nullptr);

    static Quantity* areaInSquareFeet(QObject *parent = nullptr);

    static Quantity* volumeInCubicMeters(QObject *parent = nullptr);

    static Quantity* volumeInCubicFeet(QObject *parent = nullptr);

    static Quantity* flowInCMS(QObject *parent = nullptr);

    static Quantity* flowInCFS(QObject *parent = nullptr);

    static Quantity* unitLessValues(const QString &caption = "Unitless",
                                    QVariant::Type type = QVariant::String,
                                    QObject *parent = nullptr);

  signals:

    void propertyChanged(const QString &propertyName) override;

  private:
    Unit* m_unit;
    QVariant m_minValue, m_maxValue;

};

Q_DECLARE_METATYPE(ValueDefinition*)
Q_DECLARE_METATYPE(Quality*)
Q_DECLARE_METATYPE(Quantity*)

#endif // VALUEDEFINITION_H

