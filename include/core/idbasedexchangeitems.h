#ifndef IDBASEDCOMPONENTDATAITEM_H
#define IDBASEDCOMPONENTDATAITEM_H

#include "abstractcomponentdataitem.h"
#include "exchangeitems1d.h"

template<class T>
class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem :
    public ComponentDataItem1D<T>
{
  public:

    IdBasedComponentDataItem(const QStringList& identifiers,
                             const T& defaultValue);

    virtual ~IdBasedComponentDataItem();

    bool addIdentifier(const QString &identifier, bool resetDataArray = true);

    void addIdentifiers(const QList<QString> &identifiers, bool resetDataArray = true);

    bool removeIdentifier(const QString &identifier, bool resetDataArray = true);

    T &operator[](const QString &id);

  protected:

    QStringList identifiersInternal() const;

    void clearIdentifiers();

  private:

    QStringList  m_identifiers;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedInputInt : public AbstractInput,
    public IdBasedComponentDataItem<int>,
    public virtual HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:

    IdBasedInputInt(const QString& id,
                    const QStringList& identifiers,
                    Dimension *identifierDimension,
                    ValueDefinition* valueDefinition,
                    AbstractModelComponent* modelComponent);

    virtual ~IdBasedInputInt();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

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

  private:
    Dimension* m_identifierDimension;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedInputDouble : public AbstractInput,
    public IdBasedComponentDataItem<double>,
    public virtual  HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:

    IdBasedInputDouble(const QString& id,
                       const QStringList& identifiers,
                       Dimension *identifierDimension,
                       ValueDefinition* valueDefinition,
                       AbstractModelComponent* modelComponent);

    virtual ~IdBasedInputDouble();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

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

  private:
    Dimension* m_identifierDimension;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedInputString : public AbstractInput,
    public IdBasedComponentDataItem<QString>,
    public virtual HydroCouple::IIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:

    IdBasedInputString(const QString& id,
                       const QStringList& identifiers,
                       Dimension *identifierDimension,
                       ValueDefinition* valueDefinition,
                       AbstractModelComponent* modelComponent);

    virtual ~IdBasedInputString();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

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

  private:
    Dimension* m_identifierDimension;
};

Q_DECLARE_METATYPE(IdBasedInputInt*)
Q_DECLARE_METATYPE(IdBasedInputDouble*)
Q_DECLARE_METATYPE(IdBasedInputString*)

#endif // IDBASEDCOMPONENTDATAITEM_H

