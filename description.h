/****************************************************************************
**
**  Copyright (C) 2014 Caleb Amoa Buahin
**  Contact: calebgh@gmail.com
**
**  This file is part of HydroCoupleSDK.dll
**
**  HydroCoupleSDK.dll and its associated files is free software; you can redistribute it and/or modify
**  it under the terms of the Lesser GNU General Public License as published by
**  the Free Software Foundation; either version 3 of the License, or
**  (at your option) any later version.
**
**  HydroCoupleSDK.dll and its associated files is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  Lesser GNU General Public License for more details.
**
**  You should have received a copy of the Lesser GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>
**
****************************************************************************/
/****************************************************************************
**
**  Copyright (C) 2014 Caleb Amoa Buahin
**  Contact: calebgh@gmail.com
**
**  This file is part of HydroCoupleSDK.dll
**
**  HydroCoupleSDK.dll and its associated files is free software; you can redistribute it and/or modify
**  it under the terms of the Lesser GNU General Public License as published by
**  the Free Software Foundation; either version 3 of the License, or
**  (at your option) any later version.
**
**  HydroCoupleSDK.dll and its associated files is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  Lesser GNU General Public License for more details.
**
**  You should have received a copy of the Lesser GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>
**
****************************************************************************/

#ifndef DESCRIPTION_H
#define DESCRIPTION_H

#include <QObject>
#include <QVariant>
#include "idescription.h"

class Description : public QObject , public virtual HydroCouple::IDescription
{
	Q_OBJECT;
	Q_INTERFACES(HydroCouple::IDescription);

public:
	Description(QObject* parent);

	virtual ~Description();

	//!Gets caption for the entity.
	/*!
	\returns string representing caption for entity
	*/
	QString getCaption() const override;

	//!Sets caption for the entity
	/*!
	\param caption is a string representing the caption for the entity
	*/
	virtual void setCaption(const QString & caption);

	//!Gets additional descriptive information for the entity.
	/*!
	\returns description of entity
	*/
	QString getDescription() const override;

	//!Gets additional descriptive information for the entity.
	/*!
	\param description is a string for describing an entity
	*/
	virtual void setDescription(const QString& decription);

signals:
	//!called to emit signal/event when property of child class changes
	/*!
	\param propertyName is a string representing the name of the property
	\param value is a QVariant representing the value of the property
	*/
	 void propertyChanged(const QString& propertyName, const QVariant& value);

protected:
	QString m_caption, m_description;
};

#endif // DESCRIPTION_H
