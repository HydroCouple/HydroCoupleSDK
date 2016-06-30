#ifndef ABSTRACTEXCHANGEITEM_H
#define ABSTRACTEXCHANGEITEM_H

#include "hydrocouple.h"
#include "hydrocouplesdk.h"
#include "abstractcomponentdataitem.h"

class AbstractModelComponent;

class HYDROCOUPLESDK_EXPORT AbstractExchangeItem : public AbstractComponentDataItem,
    public virtual HydroCouple::IExchangeItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IExchangeItem)

  public:
    AbstractExchangeItem(const QString& id,
                         const QList<Dimension*>& dimensions,
                         ValueDefinition* valueDefinition,
                         AbstractModelComponent* modelComponent);

    AbstractExchangeItem(const QString& id,
                         const QString& caption,
                         const QList<Dimension*>& dimensions,
                         ValueDefinition* valueDefinition,
                         AbstractModelComponent *modelComponent);

    virtual ~AbstractExchangeItem();

  protected:

    void emitMessage(const QString &message);

  signals:

    void itemChanged(const QSharedPointer<HydroCouple::IExchangeItemChangeEventArgs> &statusChangedEvent) override;

};

Q_DECLARE_METATYPE(AbstractExchangeItem*)

#endif // ABSTRACTEXCHANGEITEM_H

