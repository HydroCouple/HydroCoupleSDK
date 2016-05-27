#ifndef IDBASEDCOMPONENTDATAITEM_H
#define IDBASEDCOMPONENTDATAITEM_H

#include "abstractcomponentdataitem.h"
#include "componentdataitem1d.h"

template<class T>
class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem : public ComponentDataItem1D<T>,  public virtual HydroCouple::IIdBasedComponentDataItem
{
  public:

    using HydroCouple::IComponentDataItem::getValue;
    using HydroCouple::IComponentDataItem::getValues;
    using HydroCouple::IComponentDataItem::setValue;
    using HydroCouple::IComponentDataItem::setValues;

    IdBasedComponentDataItem(const QStringList &identifiers, Dimension *dimension, ValueDefinition *valueDefinition);

    virtual ~IdBasedComponentDataItem();

    QStringList identifiers() const override;

    bool addIdentifier(const QString &identifier, bool resetDataArray = true);

    void addIdentifiers(const QList<QString> &identifiers, bool resetDataArray = true);

    bool removeIdentifier(const QString &identifier, bool resetDataArray = true);

    HydroCouple::IDimension* identifierDimension() const override;

    void getValue(int idIndex, QVariant &data) const override;

    void getValues(int idIndex, int stride, QVariant data[]) const override;

    void getValues(int idIndex, int stride, void *data) const override;

    void setValue(int idIndex, const QVariant &data) override;

    void setValues(int idIndex, int stride, const QVariant data[]) override;

    void setValues(int idIndex, int stride, const void *data) override;

  protected:
    void clearIdentifiers();

    Dimension* hIdentifierDimension() const;

  private:
    QStringList  m_identifiers;
    Dimension *m_identifierDimension;

};

class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItemInt : public AbstractComponentDataItem,
    public virtual IdBasedComponentDataItem<int>
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
    Q_PROPERTY(QStringList Identifiers READ identifiers)

  public:

    IdBasedComponentDataItemInt(const QString &id, const QStringList &identifiers, Dimension *dimension,
                                ValueDefinition* valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedComponentDataItemInt();

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

};

class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItemDouble : public  AbstractComponentDataItem,
    public virtual IdBasedComponentDataItem<double>
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
    Q_PROPERTY(QStringList Identifiers READ identifiers)

  public:

    IdBasedComponentDataItemDouble(const QString &id, const QStringList &identifiers, Dimension *dimension,
                                   ValueDefinition* valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedComponentDataItemDouble();

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

};

class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItemString : public AbstractComponentDataItem,
    public virtual IdBasedComponentDataItem<QString>
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
    Q_PROPERTY(QStringList Identifiers READ identifiers)

  public:

    IdBasedComponentDataItemString(const QString &id, const QStringList &identifiers, Dimension *dimension,
                                   ValueDefinition* valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedComponentDataItemString();

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;
};

Q_DECLARE_METATYPE(IdBasedComponentDataItemInt*)
Q_DECLARE_METATYPE(IdBasedComponentDataItemDouble*)
Q_DECLARE_METATYPE(IdBasedComponentDataItemString*)

#endif // IDBASEDCOMPONENTDATAITEM_H

