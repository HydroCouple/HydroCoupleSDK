#ifndef ABSTRACTEXCHANGEITEM_H
#define ABSTRACTEXCHANGEITEM_H

#include "hydrocouple.h"
#include "hydrocouplesdk.h"
#include "abstractcomponentdataitem.h"

class AbstractModelComponent;

class HYDROCOUPLESDK_EXPORT AbstractExchangeItem : public AbstractComponentDataItem, public virtual HydroCouple::IExchangeItem
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IExchangeItem)

   public:
      AbstractExchangeItem(const QString &id, AbstractModelComponent *parentModelComponent);

      AbstractExchangeItem(const QString &id, const QString &caption, AbstractModelComponent *parentModelComponent);

      virtual ~AbstractExchangeItem();

   signals:
      void itemChanged(const std::shared_ptr<HydroCouple::IExchangeItemChangeEventArgs> &statusChangedEvent) override;

};

Q_DECLARE_METATYPE(AbstractExchangeItem*)

#endif // ABSTRACTEXCHANGEITEM_H

