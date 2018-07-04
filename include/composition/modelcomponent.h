#ifndef COMPOSITIONMODELCOMPONENT_H
#define COMPOSITIONMODELCOMPONENT_H


#include "hydrocouplesdk.h"
#include "hydrocouple.h"

#include <QSharedPointer>

namespace Composition
{
  class ModelComposition;
  class Input;
  class Output;
}


namespace Composition
{
  class HYDROCOUPLESDK_EXPORT ModelComponent : public QObject
  {
      Q_OBJECT

      friend class ModelComposition;

    public:

      ModelComponent(HydroCouple::IModelComponent *modelComponent, ModelComposition *modelComposition);

      virtual ~ModelComponent();

      ModelComposition *modelComposition() const;

      HydroCouple::IModelComponent *modelComponent() const;

      QList<Input*> inputs() const;

      Input *findInput(HydroCouple::IInput *input);

      Input *findInput(const QString &id);

      QList<Output*> outputs() const;

      Output *findOutput(HydroCouple::IOutput *input);

      Output *findOutput(const QString &id);

      void establishConnections();

      void disestablishConnections();

      void reestablishConnections();

    private:

      ModelComponent *clone(ModelComposition *parentComposition);

      void copyConnections(ModelComponent *modelComponent);

      void createExchangeItems();

      void deleteExchangeItems();

    private slots:

      void onComponentStatusChanged(const QSharedPointer<HydroCouple::IComponentStatusChangeEventArgs> &statusChangedEvent);

    private:

      QList<Input*> m_inputs;
      QList<Output*> m_outputs;
      HydroCouple::IModelComponent *m_modelComponent;
      ModelComposition *m_modelComposition;

  };
}

#endif // COMPOSITIONMODELCOMPONENT_H
