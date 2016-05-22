#ifndef EXCHANGEITEMCHANGEEVENTARGS_H
#define EXCHANGEITEMCHANGEEVENTARGS_H

#include "hydrocouple.h"
#include "hydrocouplesdk.h"

class AbstractExchangeItem;


class HYDROCOUPLESDK_EXPORT ExchangeItemEventArgs : public QObject, public virtual HydroCouple::IExchangeItemChangeEventArgs
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IExchangeItemChangeEventArgs)
      Q_PROPERTY(HydroCouple::IExchangeItem exchangeItem READ exchangeItem)
      Q_PROPERTY(QString Message READ message  WRITE setMessage)

   public:
      ExchangeItemEventArgs(AbstractExchangeItem* exchangeItem);

      ExchangeItemEventArgs(const QString &message, AbstractExchangeItem* exchangeItem);

      HydroCouple::IExchangeItem* exchangeItem() const override;

      QString message() const override;

      void setMessage(const QString &message);

   private:
      AbstractExchangeItem* m_exchangeItem;
      QString m_message;

};

Q_DECLARE_METATYPE(ExchangeItemEventArgs*)
#endif // EXCHANGEITEMCHANGEEVENTARGS_H

