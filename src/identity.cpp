#include "stdafx.h"
#include "identity.h"


Identity::Identity(QObject *parent)
   : Description(parent)
{

}

Identity::Identity(const QString & id, const QString & caption, const QString & description, QObject *parent)
   : Description( caption,description, parent) , m_id(id) , m_mpi_comm(nullptr) , m_mpi_rank(0)
{

}

void* Identity::MPI_Comm() const
{
   return m_mpi_comm;
}


void Identity::setMPI_Comm(void* mpi_comm)
{
   this->m_mpi_comm = mpi_comm;
}


int Identity::MPI_Rank() const
{
   return m_mpi_rank;
}


void Identity::setMPI_Rank(int rank)
{
   this->m_mpi_rank = rank;
}


QString Identity::id() const
{
   return m_id;
}

void Identity::setId(const QString& id)
{
   m_id = id;
   emit propertyChanged("Id", id);
}
