#ifndef ABSTRACTOUTPUT_H
#define ABSTRACTOUTPUT_H

#include "abstractexchangeitem.h"

class HYDROCOUPLE_EXPORT AbstractOutput : public AbstractExchangeItem , public virtual HydroCouple::IOutput
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IOutput)
      Q_PROPERTY(QList<HydroCouple::IInput*> Consumers READ consumers NOTIFY propertyChanged)
      Q_PROPERTY(QList<HydroCouple::IAdaptedOutput*> AdaptedOutputs READ adaptedOutputs NOTIFY propertyChanged)

   public:
      AbstractOutput(const QString& id, AbstractModelComponent* modelComponent);

      AbstractOutput(const QString& id, const QString& caption, AbstractModelComponent* modelComponent);

      virtual ~AbstractOutput();

      QList<HydroCouple::IInput*> consumers() const override;

      void addConsumer(HydroCouple::IInput *consumer) override;

      bool removeConsumer(HydroCouple::IInput *consumer) override;

      QList<HydroCouple::IAdaptedOutput*> adaptedOutputs()  const override;

      void addAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutputs) override;

      bool removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput) override;

   signals:
      void propertyChanged(const QString &propertyName) override;

      void itemChanged(const std::shared_ptr<HydroCouple::IExchangeItemChangeEventArgs> & statusChangedEvent) override;

   private:
      QList<HydroCouple::IInput*> m_consumers;
      QList<HydroCouple::IAdaptedOutput*> m_adaptedOutputs;

};

Q_DECLARE_METATYPE(AbstractOutput*)

#endif // ABSTRACTOUTPUT_H

