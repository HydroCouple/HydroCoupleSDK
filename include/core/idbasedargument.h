#ifndef IDBASEDARGUMENT_H
#define IDBASEDARGUMENT_H

#include "abstractargument.h"
#include "idbasedcomponentdataitem.h"
#include <QFileInfo>

class AbstractArgument;

class HYDROCOUPLESDK_EXPORT IdBasedArgumentInt : public AbstractArgument,
    public virtual HydroCouple::IIdBasedArgument,
    public virtual IdBasedComponentDataItem<int>
{
    Q_OBJECT

    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem HydroCouple::IIdBasedArgument)
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
    Q_PROPERTY(QStringList Identifiers READ identifiers)


  public:
    IdBasedArgumentInt(const QString &id,const QStringList &identifiers, Dimension *dimension,
                       ValueDefinition* valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedArgumentInt();

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


class HYDROCOUPLESDK_EXPORT IdBasedArgumentDouble : public AbstractArgument,
    public virtual HydroCouple::IIdBasedArgument,
    public virtual IdBasedComponentDataItem<double>
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem HydroCouple::IIdBasedArgument)
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
    Q_PROPERTY(QStringList Identifiers READ identifiers)

  public:
    IdBasedArgumentDouble(const QString &id,const QStringList &identifiers, Dimension *dimension,
                          ValueDefinition* valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedArgumentDouble();

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

class HYDROCOUPLESDK_EXPORT IdBasedArgumentQString : public AbstractArgument,
    public virtual HydroCouple::IIdBasedArgument,
    public virtual IdBasedComponentDataItem<QString>
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IIdBasedComponentDataItem HydroCouple::IIdBasedArgument)
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
    Q_PROPERTY(QStringList Identifiers READ identifiers)

  public:
    IdBasedArgumentQString(const QString &id,const QStringList &identifiers, Dimension *dimension,
                           ValueDefinition* valueDefinition, AbstractModelComponent* parentModelComponent);

    virtual ~IdBasedArgumentQString();

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

