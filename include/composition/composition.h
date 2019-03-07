/*!
 * \file composition.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

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
