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


/*!
    * \brief  AbstractModelComponent class.
    */
class HYDROCOUPLESDK_EXPORT AbstractModelComponent : public Identity, public virtual HydroCouple::IModelComponent
{

      friend class ModelComponentInfo;

      Q_OBJECT
      Q_INTERFACES(HydroCouple::IModelComponent)
      Q_PROPERTY(HydroCouple::IModelComponentInfo* ComponentInfo READ componentInfo)
      Q_PROPERTY(HydroCouple::IModelComponent* Parent READ parent)
      Q_PROPERTY(QList<HydroCouple::IModelComponent*> Children READ children)
      Q_PROPERTY(QList<HydroCouple::IArgument*> Arguments READ arguments)
      Q_PROPERTY(QList<HydroCouple::IInput*> Inputs READ inputs)
      Q_PROPERTY(QList<HydroCouple::IOutput*> Outputs READ outputs)
      Q_PROPERTY(QList<HydroCouple::IAdaptedOutputFactory*> AdaptedOutputFactories READ adaptedOutputFactories)

   public:
      /*!
          * \brief AbstractModelComponent
          * \param parent
          */
      AbstractModelComponent(AbstractModelComponent* parent = nullptr);

      /*!
          * \brief AbstractModelComponent
          * \param id
          * \param caption
          * \param description
          * \param parent
          */
      AbstractModelComponent(const QString& id, const QString& caption, const QString& description, AbstractModelComponent* parent = nullptr);

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
          * \brief clone
          * \return
          */
      virtual HydroCouple::IModelComponent* clone() const override ;

      /*!
          * \brief children
          * \return
          */
      QList<HydroCouple::IModelComponent*> children() const override;

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

      /*!
          * \brief initialize
          */
      virtual void initialize() override;

      /*!
          * \brief validate
          * \return
          */
      virtual QList<QString> validate() override;

      /*!
          * \brief prepare
          */
      virtual void prepare() override;

      /*!
          * \brief update
          * \param requiredOutputs
          */
      virtual void update(const QList<HydroCouple::IOutput*> & requiredOutputs = QList<HydroCouple::IOutput*>()) override;

      /*!
          * \brief finish
          */
      virtual void finish() override;

   signals:
      /*!
          * \brief componentStatusChanged
          * \param statusChangedEvent
          */
      void componentStatusChanged(const HydroCouple::IComponentStatusChangeEventArgs& statusChangedEvent) override ;

      /*!
          * \brief propertyChanged
          * \param propertyName
          * \param value
          */
      void propertyChanged(const QString& propertyName, const QVariant& value) override;

   private:
      /*!
          * \brief setComponentInfo
          * \param modelComponentInfo
          */
      void setComponentInfo(HydroCouple::IModelComponentInfo* const modelComponentInfo);




   protected:
      QList<HydroCouple::IModelComponent*> m_children;
      HydroCouple::ComponentStatus m_status;
      QList<HydroCouple::IInput*> m_inputs;
      QList<HydroCouple::IOutput*> m_outputs;
      QList<HydroCouple::IAdaptedOutputFactory*> m_adaptedOutputFactories;
      QList<HydroCouple::IArgument*> m_arguments;

   private:
      HydroCouple::IModelComponentInfo* m_modelComponentInfo;
      HydroCouple::IModelComponent* m_parentModelComponent;

};


Q_DECLARE_METATYPE(AbstractModelComponent*)

#endif // ABSTRACTMODELCOMPONENT_H
