/*! \file   identity.h
 *  \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version   1.0.0.0
 *  \section   Description
 *  This header file is part of the HydroCoupleSDK library. It is an implementation
 * of the IIdentity interface of the HydroCouple interface definitions.
 *  \section License
 *  identity.h, associated files and libraries are free software;
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

#ifndef IDENTITY_H
#define IDENTITY_H

#include "hydrocouplesdk.h"
#include "description.h"


/*!
    * \brief The Identity class is an implementation of the HydroCouple::IIdentity of the HydroCouple interface definitions.
    */
class HYDROCOUPLESDK_EXPORT Identity : public Description, public virtual HydroCouple::IIdentity
{

      Q_OBJECT
      Q_INTERFACES(HydroCouple::IIdentity)
      Q_PROPERTY(QString Id READ id NOTIFY propertyChanged)

   public:
      /*!
          * \brief Identity
          * \param parent
          */
      Identity(QObject* parent = nullptr);

      /*!
          * \brief Identity
          * \param id
          * \param caption
          * \param description
          * \param parent
          */
      Identity(const QString & id, const QString & caption, const QString & description, QObject *parent);


      /*!
          * \brief ~Identity
          */
      virtual ~Identity(){}


      /*!
          * \brief MPI_Comm is the communicator in a distributed memory MPI HPC Environment.
          * \return
          */
      void* MPI_Comm() const override;


      /*!
          * \brief setMPI_Comm
          * \param mpi_comm
          */
      void setMPI_Comm(void* mpi_comm) override;


      /*!
          * \brief MPIRank is the rank of the processor in the communicator.
          * \return
          */
      int MPI_Rank() const override;


      /*!
          * \brief setMPI_Rank
          * \param rank
          */
      void setMPI_Rank(int rank) override;


      /*!
          * \brief Gets a unique identifier for the entity.
          * \return the Id as a String. The Id must be unique within its context but
          * does not need to be globally unique. E.g. the id of an input exchange
          * item must be unique in the list of inputs of a IModelComponent, but a
          * similar Id might be used by an exchange item of another IModelComponent.
          */
      QString id() const override;

   signals:

      /*!
          * \brief propertyChanged
          * \param propertyName
          * \param value
          */
      void propertyChanged(const QString& propertyName, const QVariant& value) override;

   protected:
      /*!
          * \brief setId
          * \param id
          */
      void setId(const QString& id);

   private:
      QString m_id;
      void* m_mpi_comm;
      int m_mpi_rank;
};


Q_DECLARE_METATYPE(Identity*)

#endif // IDENTITY_H