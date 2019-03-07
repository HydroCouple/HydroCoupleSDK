/*!
 * \file modelcomponent.h
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
