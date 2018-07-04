#ifndef COMPOSITION_H
#define COMPOSITION_H

#include "hydrocouplesdk.h"
#include "hydrocouple.h"

#include <QObject>
#include <QVector>
#include <QSet>


namespace Composition
{
  class ModelComponent;
}

namespace Composition
{
  class HYDROCOUPLESDK_EXPORT ModelComposition : public QObject
  {
      Q_OBJECT

     friend class ModelComponent;

    public:

      ModelComposition(bool disposeComponentsOnDelete, const QList<HydroCouple::IModelComponent*> &searchComponents, QObject *parent);

      virtual ~ModelComposition();

      QVector<ModelComponent*> components() const;

      const HydroCouple::IWorkflowComponent *workflowComponent() const;

      ModelComposition *clone(QObject *parent = nullptr);

      bool isCloneable() const;

      void establishConnections();

      void disestablishConnections();

      void reestablishConnections();

      bool disposeComponentsOnDelete() const;

      void setDisposeComponentsOnDelete(bool disposeComponentsOnDelete);

      ModelComponent *findModelComponent(HydroCouple::IModelComponent *modelComponent);

      ModelComponent *findModelComponent(const QString &id);

    private:

      void findComponentsInComposition(const QList<HydroCouple::IModelComponent*> &searchComponents);

      static void findComponentsInComposition(HydroCouple::IModelComponent *modelComponent, QSet<HydroCouple::IModelComponent*> &foundComponents);

    private:

      bool m_disposeComponentOnDelete;
      const HydroCouple::IWorkflowComponent *m_workflowComponent;
      QVector<ModelComponent*> m_modelComponents;

  };
}

#endif // COMPOSITION_H
