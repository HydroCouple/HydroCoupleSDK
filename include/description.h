/*! \file   description.h
 *  \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version   1.0.0.0
 *  \section   Description
 *  This header files contains is part of the HydroCoupleSDK library.
 *  It represents the implementation of the IDescription interface in the
 *  HydroCouple interface definitions.
 *  \section License
 *  description.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  The HydroCoupleSDK library and its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2016
 *  \pre
 *  \bug
 *  \todo
 *  - test
 *  \warning
 */

#ifndef DESCRIPTION_H
#define DESCRIPTION_H

#include <QObject>
#include <QVariant>
#include "hydrocouplesdk.h"
#include "hydrocouple.h"


   /*!
    * \brief The Description class.
    *
    * \details The Description class is an implementation of the HydroCouple::IDescription class
    * in the HydroCouple interface definitions.
    *
    */
   class HYDROCOUPLESDK_EXPORT Description : public QObject, public virtual HydroCouple::IDescription
   {
         Q_OBJECT
         Q_INTERFACES(HydroCouple::IDescription)
         Q_PROPERTY(QString Caption READ getCaption WRITE setCaption NOTIFY propertyChanged)
         Q_PROPERTY(QString Description READ getDescription WRITE setDescription NOTIFY propertyChanged)

      public:
         /*!
          * \brief Description
          * \param parent
          */
         Description(QObject* parent = nullptr);

         /*!
       * \brief Description
       * \param caption
       * \param description
       * \param parent
       */
         Description(const QString & caption, const QString & description, QObject *parent = nullptr);

         /*!
       * \brief ~Description
       */
         virtual ~Description(){}

         /*!
       * \brief Gets caption for the entity.
       * \returns string representing caption for entity.
       */
         QString getCaption() const override;

         /*!
       * \brief Sets caption for the entity.
       * \param caption is a string representing the caption for the entity.
       */
         virtual void setCaption(const QString & caption) override;

         /*!
       * \brief Gets additional descriptive information for the entity.
       * \returns description of entity
       */
         QString getDescription() const override;

         /*!
       * \brief Gets additional descriptive information for the entity.
       * \param description is a string for describing an entity
       */
         virtual void setDescription(const QString& decription) override;

      signals:
         /*!
       * \brief propertyChanged
       * \param propertyName
       * \param value
       */
         void propertyChanged(const QString& propertyName, const QVariant& value) override;

      protected:
         QString m_caption, m_description;
   };

Q_DECLARE_METATYPE(Description*)

#endif // DESCRIPTION_H
