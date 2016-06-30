#ifndef ABSTRACTARGUMENT_H
#define ABSTRACTARGUMENT_H

#include "abstractcomponentdataitem.h"
#include <QXmlStreamReader>

class Dimension;
class AbstractComponentDataItem;

class HYDROCOUPLESDK_EXPORT AbstractArgument : public AbstractComponentDataItem,
    public virtual HydroCouple::IArgument
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IArgument)
    Q_PROPERTY(bool Optional READ isOptional NOTIFY propertyChanged)
    Q_PROPERTY(bool ReadOnly READ isReadOnly NOTIFY propertyChanged)
    Q_PROPERTY(QStringList InputFileTypeFilters READ inputFileTypeFilters NOTIFY propertyChanged)
    Q_PROPERTY(bool CanReadFromFile READ canReadFromFile NOTIFY propertyChanged)
    Q_PROPERTY(bool CanReadFromString READ canReadFromString NOTIFY propertyChanged)

  public:
    AbstractArgument(const QString& id,
                     const QList<Dimension*>& dimensions,
                     ValueDefinition* valueDefinition,
                     AbstractModelComponent *modelComponent);

    AbstractArgument(const QString& id,
                     const QString& caption,
                     const QList<Dimension*>& dimensions,
                     ValueDefinition* valueDefinition,
                     AbstractModelComponent *modelComponent);

    virtual ~AbstractArgument();

    bool isOptional() const override;

    bool isReadOnly() const override;

    QStringList inputFileTypeFilters() const override;

    bool canReadFromFile() const override;

    bool canReadFromString() const override;

    HydroCouple::ArgumentIOType currentArgumentIOType() const override;

    void setIsOptional(bool isOptional);

    void setIsReadOnly(bool isReadOnly);

    void addInputFileTypeFilter(const QString &extension);

  protected:

    void setCanReadFromFile(bool canReadFromFile);

    void setCanReadFromString(bool canReadFromString);

    void setArgumentIOType(HydroCouple::ArgumentIOType argumentIOType);

  signals:

    void propertyChanged(const QString &propertyName) override;

  private:
    QStringList m_inputFileTypeFilters;
    bool m_isOptional , m_isReadOnly, m_canReadFromFile, m_canReadFromString;
    HydroCouple::ArgumentIOType m_argumentIOType;
};

Q_DECLARE_METATYPE(AbstractArgument*)
Q_DECLARE_METATYPE(QList<AbstractArgument*>)

#endif // ABSTRACTARGUMENT_H

