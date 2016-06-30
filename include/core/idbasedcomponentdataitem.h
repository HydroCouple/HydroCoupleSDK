#ifndef IDBASEDCOMPONENTDATAITEM_H
#define IDBASEDCOMPONENTDATAITEM_H

#include "abstractcomponentdataitem.h"
#include "componentdataitem1d.h"

template<class T>
class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem : public ComponentDataItem1D<T>
{
  public:

    IdBasedComponentDataItem(const QStringList& identifiers,
                             Dimension* dimension,
                             const T& defaultValue);

    virtual ~IdBasedComponentDataItem();

    bool addIdentifier(const QString &identifier, bool resetDataArray = true);

    void addIdentifiers(const QList<QString> &identifiers, bool resetDataArray = true);

    bool removeIdentifier(const QString &identifier, bool resetDataArray = true);

  protected:

    QStringList identifiersInternal() const;

    Dimension* identifierDimensionInternal() const;

    void clearIdentifiers();

  private:

    QStringList  m_identifiers;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItemInt : public AbstractComponentDataItem,
    public IdBasedComponentDataItem<int>,
    public virtual HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:

    IdBasedComponentDataItemInt(const QString& id,
                                const QStringList& identifiers,
                                Dimension *dimension,
                                ValueDefinition* valueDefinition,
                                AbstractModelComponent* modelComponent);

    virtual ~IdBasedComponentDataItemInt();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int idIndex, QVariant& data) const override;

    void getValues(int idIndex, int stride, QVariant data[]) const override;

    void getValues(int idIndex, int stride, void *data) const override;

    void setValue(int idIndex, const QVariant &data) override;

    void setValues(int idIndex, int stride, const QVariant data[]) override;

    void setValues(int idIndex, int stride, const void *data) override;

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItemDouble : public  AbstractComponentDataItem,
    public IdBasedComponentDataItem<double>,
    public virtual  HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:

    IdBasedComponentDataItemDouble(const QString& id,
                                   const QStringList& identifiers,
                                   Dimension *dimension,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent* modelComponent);

    virtual ~IdBasedComponentDataItemDouble();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int idIndex, QVariant& data) const override;

    void getValues(int idIndex, int stride, QVariant data[]) const override;

    void getValues(int idIndex, int stride, void *data) const override;

    void setValue(int idIndex, const QVariant &data) override;

    void setValues(int idIndex, int stride, const QVariant data[]) override;

    void setValues(int idIndex, int stride, const void *data) override;

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItemString : public AbstractComponentDataItem,
    public IdBasedComponentDataItem<QString>,
    public virtual  HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:

    IdBasedComponentDataItemString(const QString& id,
                                   const QStringList& identifiers,
                                   Dimension *dimension,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent* modelComponent);

    virtual ~IdBasedComponentDataItemString();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int idIndex, QVariant& data) const override;

    void getValues(int idIndex, int stride, QVariant data[]) const override;

    void getValues(int idIndex, int stride, void *data) const override;

    void setValue(int idIndex, const QVariant &data) override;

    void setValues(int idIndex, int stride, const QVariant data[]) override;

    void setValues(int idIndex, int stride, const void *data) override;

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;
};

Q_DECLARE_METATYPE(IdBasedComponentDataItemInt*)
Q_DECLARE_METATYPE(IdBasedComponentDataItemDouble*)
Q_DECLARE_METATYPE(IdBasedComponentDataItemString*)

#endif // IDBASEDCOMPONENTDATAITEM_H

