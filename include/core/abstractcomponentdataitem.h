#ifndef ABSTRACTCOMPONENTITEM_H
#define ABSTRACTCOMPONENTITEM_H

#include "identity.h"
#include <QXmlStreamReader>

class AbstractModelComponent;

/*!
 * \brief The AbstractComponentDataItem class
 */
class HYDROCOUPLESDK_EXPORT AbstractComponentDataItem : public Identity , public virtual HydroCouple::IComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IComponentDataItem)
    Q_PROPERTY(HydroCouple::IModelComponent* ModelComponent READ modelComponent NOTIFY propertyChanged)

  public:

    AbstractComponentDataItem(const QString &id, AbstractModelComponent *parentModelComponent);

    AbstractComponentDataItem(const QString &id, const QString &caption, AbstractModelComponent *parentModelComponent);

    virtual ~AbstractComponentDataItem();

    HydroCouple::IModelComponent* modelComponent() const override;

    virtual void readData(QXmlStreamReader &xmlReader);

    virtual void writeData(QXmlStreamWriter &xmlWriter);

  signals:
    void propertyChanged(const QString &propertyName) override;

  private:
    AbstractModelComponent *m_parentModelComponent;

};

Q_DECLARE_METATYPE(AbstractComponentDataItem*)

#endif // ABSTRACTCOMPONENTITEM_H

