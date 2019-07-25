/*!
 * \file exchangeitems.h
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

#ifndef EXCHANGEITEMS_H
#define EXCHANGEITEMS_H


#include "hydrocouplesdk.h"
#include "hydrocouple.h"

#include <QObject>

namespace Composition
{
  class ModelComponent;
  class Input;
  class Output;
  class AdaptedOutput;
}

namespace Composition
{
  class HYDROCOUPLESDK_EXPORT ExchangeItem : public QObject
  {
      Q_OBJECT

    public:

      ExchangeItem(const QString &id, ModelComponent *modelComponent);

      virtual ~ExchangeItem();

      QString id() const;

      virtual HydroCouple::IExchangeItem *exchangeItem() const = 0;

      ModelComponent *modelComponent() const;

      void establishSignalSlotConnections();

    private slots:

      void onPropertyChanged(const QString &propertyName);

    protected:

      QString m_id;
      ModelComponent *m_modelComponent;

  };


  class HYDROCOUPLESDK_EXPORT Input : public ExchangeItem
  {
      Q_OBJECT

    public:

      Input(HydroCouple::IInput *input, ModelComponent *modelComponent);

      virtual ~Input() override;

      HydroCouple::IExchangeItem *exchangeItem() const override;

      HydroCouple::IInput *input() const;

      Output *provider() const;

      void setProvider(Output *provider);

    private:

      Output *m_provider;

  };

  class HYDROCOUPLESDK_EXPORT MultiInput : public Input
  {
      Q_OBJECT

    public:

      MultiInput(HydroCouple::IMultiInput *input, ModelComponent *modelComponent);

      virtual ~MultiInput();

      HydroCouple::IMultiInput *multiInput() const;

      QList<Output*> providers() const;

      void addProvider(Output *provider);

      void removeProvider(Output *provider);

    private:

      QList<Output*> m_providers;

  };

  class HYDROCOUPLESDK_EXPORT Output : public ExchangeItem
  {
      Q_OBJECT

      friend class ModelComponent;

    public:

      Output(HydroCouple::IOutput *output, ModelComponent *modelComponent);

      virtual ~Output();

      HydroCouple::IExchangeItem *exchangeItem() const override;

      virtual HydroCouple::IOutput *output() const;

      QList<Input*> consumers() const;

      void addConsumer(Input *input);

      void removeConsumer(Input *input);

      QList<AdaptedOutput*> adaptedOutputs() const;

      void addAdaptedOutput(AdaptedOutput *adaptedOutput);

      void removeAdaptedOutput(AdaptedOutput *adaptedOutput);

      virtual bool createConnection(Input *input);

      virtual bool insertAdaptedOutput(HydroCouple::IIdentity *identity,
                                       HydroCouple::IAdaptedOutputFactory *adaptedOutputFactory,
                                       Input *consumer);

      virtual void establishConnections();

      virtual void disestablishConnections();

      virtual void reestablishConnections();

    protected:

      void copyConnections(Output *output);

    private:

      QList<Input*> m_consumers;
      QList<AdaptedOutput*> m_adaptedOutputs;

  };

  class HYDROCOUPLESDK_EXPORT AdaptedOutput : Output
  {
      Q_OBJECT

      friend class Output;

    public:

      AdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput,
                    Input *input, Output *adaptee,
                    ModelComponent *modelComponent);

      virtual ~AdaptedOutput();

      HydroCouple::IAdaptedOutputFactory *adaptedOutputFactory() const;

      HydroCouple::IOutput *output() const override;

      HydroCouple::IAdaptedOutput *adaptedOutput() const;

      Output *adaptee() const;

    private:

      AdaptedOutput *clone(Output *output, ModelComponent *modelComponent);

      bool reCreateAdaptedOutput() const;

    private:

      HydroCouple::IAdaptedOutput *m_adaptedOutput;
      HydroCouple::IIdentity *m_adaptedOutputIdentity;
      Input *m_input;
      Output *m_adaptee;
      HydroCouple::IAdaptedOutputFactory *m_adaptedOutputFactory;

  };

}



#endif // EXCHANGEITEMS_H
