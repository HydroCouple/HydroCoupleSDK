#ifndef IDBASEDARGUMENT_H
#define IDBASEDARGUMENT_H

#include "abstractargument.h"
#include "idbasedcomponentdataitem.h"
#include <QFileInfo>

class AbstractArgument;

class HYDROCOUPLESDK_EXPORT IdBasedArgumentInt : public AbstractArgument,
    public IdBasedComponentDataItem<int>,
    public virtual HydroCouple::IIdBasedArgument
{
    Q_OBJECT

    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem HydroCouple::IIdBasedArgument)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)


  public:
    IdBasedArgumentInt(const QString& id,
                       const QStringList& identifiers,
                       Dimension* dimension,
                       ValueDefinition* valueDefinition,
                       AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedArgumentInt();

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

    QString toString() const override;

    bool readValues(const QString &value, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem) override;

    bool matchIdentifiersWhenReading() const;

    void setMatchIdentifiersWhenReading(bool match);

  private:
    QFileInfo m_inputFile;
    bool m_matchIdentifiersDuringRead;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedArgumentDouble : public AbstractArgument,
    public IdBasedComponentDataItem<double>,
    public virtual HydroCouple::IIdBasedArgument
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem HydroCouple::IIdBasedArgument)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:
    IdBasedArgumentDouble(const QString& id,
                          const QStringList& identifiers,
                          Dimension* dimension,
                          ValueDefinition* valueDefinition,
                          AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedArgumentDouble();


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

    QString toString() const override;

    bool readValues(const QString &value, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem) override;

    bool matchIdentifiersWhenReading() const;

    void setMatchIdentifiersWhenReading(bool match);

  private:
    QFileInfo m_inputFile;
    bool m_matchIdentifiersDuringRead;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT IdBasedArgumentQString : public AbstractArgument,
    public IdBasedComponentDataItem<QString>,
    public virtual HydroCouple::IIdBasedArgument
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem HydroCouple::IIdBasedArgument)
    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)

  public:
    IdBasedArgumentQString(const QString& id,
                           const QStringList& identifiers,
                           Dimension* dimension,
                           ValueDefinition* valueDefinition,
                           AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedArgumentQString();

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

    QString toString() const override;

    bool readValues(const QString &value, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem) override;

    bool matchIdentifiersWhenReading() const;

    void setMatchIdentifiersWhenReading(bool match);

  private:
    QFileInfo m_inputFile;
    bool m_matchIdentifiersDuringRead;
};

Q_DECLARE_METATYPE(IdBasedArgumentInt*)
Q_DECLARE_METATYPE(IdBasedArgumentDouble*)
Q_DECLARE_METATYPE(IdBasedArgumentQString*)


#endif // IDBASEDARGUMENT_H

