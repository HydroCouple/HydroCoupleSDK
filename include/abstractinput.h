#ifndef ABSTRACTINPUT_H
#define ABSTRACTINPUT_H

#include "abstractexchangeitem.h"

class HYDROCOUPLE_EXPORT AbstractInput : public AbstractExchangeItem , public virtual HydroCouple::IInput
{

      Q_OBJECT
      Q_INTERFACES(HydroCouple::IInput)
      Q_PROPERTY(HydroCouple::IOutput* Provider READ provider NOTIFY propertyChanged)

   public:

      AbstractInput(const QString &id, AbstractModelComponent *modelComponent);

      AbstractInput(const QString &id, const QString &caption , AbstractModelComponent *modelComponent);

      virtual ~AbstractInput();

      HydroCouple::IOutput* provider() const override;

      void setProvider(HydroCouple::IOutput *provider) override;

      bool canConsume(HydroCouple::IOutput *provider, QString &message) const override;

   signals:

      void propertyChanged(const QString &propertyName) override;

      void itemChanged(const std::shared_ptr<HydroCouple::IExchangeItemChangeEventArgs> &statusChangedEvent) override;

   private:

      HydroCouple::IOutput* m_provider;

};

class HYDROCOUPLE_EXPORT AbstractMultiInput : public AbstractInput , public virtual HydroCouple::IMultiInput
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IMultiInput)
      Q_PROPERTY(QList<HydroCouple::IOutput*> Providers READ providers NOTIFY propertyChanged)

   public:

      AbstractMultiInput(const QString &id, AbstractModelComponent *modelComponent);

      AbstractMultiInput(const QString &id, const QString &caption, AbstractModelComponent *modelComponent);

      virtual ~AbstractMultiInput();

      QList<HydroCouple::IOutput*>  providers() const override;

      void addProvider(HydroCouple::IOutput *provider) override;

      bool removeProvider(HydroCouple::IOutput *provider) override;

   signals:

      void propertyChanged(const QString &propertyName) override;

   private:

      QList<HydroCouple::IOutput*> m_providers;
};

Q_DECLARE_METATYPE(AbstractInput*)
Q_DECLARE_METATYPE(AbstractMultiInput*)

#endif // ABSTRACTINPUT_H

