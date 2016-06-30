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

#ifndef COMPONENTSTATUSCHANGEEVENTARGS_H
#define COMPONENTSTATUSCHANGEEVENTARGS_H

#include <QObject>
#include "hydrocouple.h"
#include "hydrocouplesdk.h"

class AbstractModelComponent;

/*!
 * \brief The ComponentStatusChangeEventArgs class.
 */
class HYDROCOUPLESDK_EXPORT ComponentStatusChangeEventArgs :
    public QObject, public virtual HydroCouple::IComponentStatusChangeEventArgs
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IComponentStatusChangeEventArgs)
    Q_PROPERTY(HydroCouple::IModelComponent* Component READ component)
    Q_PROPERTY(HydroCouple::ComponentStatus Status READ status)
    Q_PROPERTY(HydroCouple::ComponentStatus PreviousStatus READ previousStatus )
    Q_PROPERTY(QString Message READ message)

  public:

   /*!
    * \brief ComponentStatusChangeEventArgs
    * \param parent
    * \param newStatus
    * \param oldStatus
    * \param message
    * \param percentProgress
    */
    ComponentStatusChangeEventArgs(HydroCouple::ComponentStatus newStatus,
                                   HydroCouple::ComponentStatus oldStatus, const QString& message,
                                   float percentProgress, AbstractModelComponent *parent);

   /*!
    * \brief ComponentStatusChangeEventArgs
    * \param parent
    * \param newStatus
    * \param oldStatus
    * \param message
    */
    ComponentStatusChangeEventArgs(HydroCouple::ComponentStatus newStatus,
                                   HydroCouple::ComponentStatus oldStatus, const QString& message,
                                   AbstractModelComponent *parent);

    ~ComponentStatusChangeEventArgs();

    /*!
       * \brief Gets the IModelComponent that fired the event.
       */
    HydroCouple::IModelComponent* component() const override;

    /*!
       * \brief Gets the IModelComponent's status before the status change.
       */
    HydroCouple::ComponentStatus previousStatus() const override;

    /*!
       * \brief setPreviousStatus
       * \param previousStatus
       */
    void setPreviousStatus(HydroCouple::ComponentStatus previousStatus);

    /*!
       * \brief Gets the IModelComponent's status after the status change.
       */
    HydroCouple::ComponentStatus status() const override;

    /*!
       * \brief setStatus
       * \param status
       */
    void setStatus(HydroCouple::ComponentStatus status);

    /*!
       * \brief Gets additional information about the status change.
       */
    QString message() const override;

    void setMessage(const QString &message);

    /*!
       * Return true if status has a percent progress included in its definition/ otherwise progress bar shows busy
       */
    bool hasProgressMonitor() const override;

    void setHasProgressMonitor(bool hasProgress);

    /*!
       * \brief Number between 0 and 100 indicating the progress made by a component in its simulation.
       */
    float percentProgress() const override;

    void setPercentProgress(float value);

  private:
    bool m_hasProgress;
    QString m_message;
    float m_percentProgress;
    HydroCouple::IModelComponent* m_component;
    HydroCouple::ComponentStatus m_oldStatus, m_newStatus;
};

Q_DECLARE_METATYPE(ComponentStatusChangeEventArgs*)

#endif // COMPONENTSTATUSCHANGEEVENTARGS_H