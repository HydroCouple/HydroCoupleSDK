/*! \file   abstractmodelcomponent.h
 *  \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version   1.0.0.0
 *  \section   Description
 *  This header files contains the the SDK interface definitions for the
 *  HydroCouple Component-Based modeling framework.
 *  \section License
 *  hydrocouple.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  hydrocouple.h its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2016
 *  \pre
 *  \bug
 *  \warning
 *  \todo
 */

#ifndef ABSTRACTMODELCOMPONENT_H
#define ABSTRACTMODELCOMPONENT_H

#include "identity.h"
#include <QMutex>


class ModelComponentInfo;
class AbstractOutput;
class AbstractInput;
class AbstractArgument;
class ComponentStatusChangeEventArgs;

/*!
 * \brief  AbstractModelComponent class.
 */
class HYDROCOUPLESDK_EXPORT AbstractModelComponent : public Identity, public virtual HydroCouple::IModelComponent
{

      friend class ModelComponentInfo;

      Q_OBJECT
      Q_INTERFACES(HydroCouple::IModelComponent)
      Q_PROPERTY(HydroCouple::IModelComponentInfo* ComponentInfo READ componentInfo NOTIFY propertyChanged)
      Q_PROPERTY(HydroCouple::IModelComponent* Parent READ parent NOTIFY propertyChanged)
      Q_PROPERTY(QList<HydroCouple::IModelComponent*> Clones READ clones NOTIFY propertyChanged)
      Q_PROPERTY(QList<HydroCouple::IArgument*> Arguments READ arguments NOTIFY propertyChanged)
      Q_PROPERTY(QList<HydroCouple::IInput*> Inputs READ inputs NOTIFY propertyChanged)
      Q_PROPERTY(QList<HydroCouple::IOutput*> Outputs READ outputs NOTIFY propertyChanged)
      Q_PROPERTY(QList<HydroCouple::IAdaptedOutputFactory*> AdaptedOutputFactories READ adaptedOutputFactories NOTIFY propertyChanged)

   public:

      /*!
       * \brief AbstractModelComponent
       * \param id
       * \param description
       * \param parent
       */
      AbstractModelComponent(const QString &id, AbstractModelComponent *parent = nullptr);

      /*!
       * \brief AbstractModelComponent
       * \param id
       * \param caption
       * \param description
       * \param parent
       */
      AbstractModelComponent(const QString &id, const QString &caption, AbstractModelComponent *parent = nullptr);

      /*!
       * \brief ~AbstractModelComponent
       */
      virtual ~AbstractModelComponent();

      /*!
       * \brief componentInfo
       * \return
       */
      HydroCouple::IModelComponentInfo* componentInfo() const override;

      /*!
       * \brief parent
       * \return
       */
      HydroCouple::IModelComponent* parent() const override;

      /*!
       * \brief children
       * \return
       */
      QList<HydroCouple::IModelComponent*> clones() const override;

      /*!
       * \brief arguments
       * \return
       */
      QList<HydroCouple::IArgument*> arguments() const override;

      /*!
       * \brief status
       * \return
       */
      HydroCouple::ComponentStatus status() const override;

      /*!
       * \brief inputs
       * \return
       */
      QList<HydroCouple::IInput*> inputs() const override;

      /*!
       * \brief outputs
       * \return
       */
      QList<HydroCouple::IOutput*> outputs() const override;

      /*!
       * \brief adaptedOutputFactories
       * \return
       */
      QList<HydroCouple::IAdaptedOutputFactory*> adaptedOutputFactories() const override;

   signals:

      /*!
       * \brief componentStatusChaSnged
       * \param statusChangedEvent
       */
      void componentStatusChanged(const std::shared_ptr<HydroCouple::IComponentStatusChangeEventArgs> &statusChangedEvent) override ;

      /*!
       * \brief propertyChanged
       * \param propertyName
       * \param value
       */
      void propertyChanged(const QString &propertyName) override;

   protected:

      void setComponentInfo(ModelComponentInfo *modelComponentInfo);

      void addChildComponent(AbstractModelComponent *modelComponent);

      bool removeChildComponent(AbstractModelComponent *modelComponent);

      void clearChildComponents();

      QHash<QString,AbstractModelComponent*> clonesMap() const;

      void addInputExchangeItem(AbstractInput *input);

      bool removeInputExchangeItem(AbstractInput *input);

      void clearInputExchangeItems();

      QHash<QString,AbstractInput*> inputsMap() const;

      void addOutputExchangeItem(AbstractOutput *output);

      bool removeOutputExchangeItem(AbstractOutput *output);

      void clearOutputExchangeItems();

      QHash<QString,AbstractOutput*> outputsMap() const;

      void addAdaptedOutputFactory(HydroCouple::IAdaptedOutputFactory* adaptedOutputFactory);

      bool removeAdaptedOutputFactory(HydroCouple::IAdaptedOutputFactory* adaptedOutputFactory);

      void clearAdaptedOutputFactory();

      void addArgument(AbstractArgument *argument);

      bool removeArgument(AbstractArgument *argument);

      void clearArguments();

      QHash<QString,AbstractArgument*> argumentsMap() const;

      void setStatus(HydroCouple::ComponentStatus status);

      virtual void setStatus(HydroCouple::ComponentStatus status, const QString &message);

      virtual void setStatus(HydroCouple::ComponentStatus status, const QString &message, int progress);

   private:
      QHash<QString,AbstractModelComponent*> m_children;
      HydroCouple::ComponentStatus m_status;
      QHash<QString,AbstractInput*> m_inputs;
      QHash<QString,AbstractOutput*> m_outputs;
      QHash<QString,HydroCouple::IAdaptedOutputFactory*> m_adaptedOutputFactories;
      QHash<QString,AbstractArgument*> m_arguments;
      ModelComponentInfo *m_modelComponentInfo;
      AbstractModelComponent *m_parentModelComponent;
     // ComponentStatusChangeEventArgs *m_componentStatusChangedEventArg;

};

Q_DECLARE_METATYPE(AbstractModelComponent*)

#endif // ABSTRACTMODELCOMPONENT_H
