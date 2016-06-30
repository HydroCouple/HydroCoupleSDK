#ifndef ABSTRACTCOMPONENTITEM_H
#define ABSTRACTCOMPONENTITEM_H

#include "identity.h"
#include <QXmlStreamReader>

class AbstractModelComponent;
class Dimension;
class ValueDefinition;

/*!
 * \brief The AbstractComponentDataItem class
 */
class HYDROCOUPLESDK_EXPORT AbstractComponentDataItem : public Identity,
    public virtual HydroCouple::IComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IComponentDataItem)

    Q_PROPERTY(HydroCouple::IModelComponent* ModelComponent READ modelComponent NOTIFY propertyChanged)
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
    Q_PROPERTY(QStringList Comments READ comments WRITE setComments NOTIFY propertyChanged)

  public:

    AbstractComponentDataItem(const QString& id,
                              const QList<Dimension*>& dimensions,
                              ValueDefinition* valueDefinition,
                              AbstractModelComponent *modelComponent);

    AbstractComponentDataItem(const QString& id,
                              const QString& caption,
                              const QList<Dimension*>& dimensions,
                              ValueDefinition* valueDefinition,
                              AbstractModelComponent *modelComponent);

    virtual ~AbstractComponentDataItem();

    HydroCouple::IModelComponent* modelComponent() const override;

    AbstractModelComponent* modelComponentInternal() const;

    QList<HydroCouple::IDimension*> dimensions() const override;

    HydroCouple::IValueDefinition* valueDefinition() const override;

    QStringList comments() const;

    void setComments(const QStringList& comments);

    void addComment(const QString& comment);

    virtual void readData(QXmlStreamReader &xmlReader);

    virtual void writeData(QXmlStreamWriter &xmlWriter);

  signals:

    void propertyChanged(const QString &propertyName) override;

  protected:

    QList<Dimension*> dimensionsInternal() const;

    ValueDefinition* valueDefinitionInternal() const;

  private:
    QList<Dimension*> m_dimensions;
    ValueDefinition* m_valueDefinition;
    AbstractModelComponent *m_modelComponent;
    QStringList m_comments;
};

Q_DECLARE_METATYPE(AbstractComponentDataItem*)

#endif // ABSTRACTCOMPONENTITEM_H

