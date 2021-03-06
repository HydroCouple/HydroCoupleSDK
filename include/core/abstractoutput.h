/*!
 * \file  abstractoutput.h
 * \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
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

#ifndef ABSTRACTOUTPUT_H
#define ABSTRACTOUTPUT_H

#include "abstractexchangeitem.h"

/*!
   * \brief The AbstractOutput class
   */
class HYDROCOUPLESDK_EXPORT AbstractOutput : public AbstractExchangeItem,
    public virtual HydroCouple::IOutput
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IOutput)
    Q_PROPERTY(QList<HydroCouple::IInput*> Consumers READ consumers NOTIFY propertyChanged)
    Q_PROPERTY(QList<HydroCouple::IAdaptedOutput*> AdaptedOutputs READ adaptedOutputs NOTIFY propertyChanged)

  public:
    AbstractOutput(const QString& id,
                   const QList<Dimension*>& dimensions,
                   ValueDefinition* valueDefinition,
                   AbstractModelComponent* modelComponent);

    AbstractOutput(const QString& id,
                   const QString& caption,
                   const QList<Dimension*>& dimensions,
                   ValueDefinition* valueDefinition,
                   AbstractModelComponent* modelComponent);

    virtual ~AbstractOutput() override;

    QList<HydroCouple::IInput*> consumers() const override;

    void addConsumer(HydroCouple::IInput *consumer) override;

    bool removeConsumer(HydroCouple::IInput *consumer) override;

    QList<HydroCouple::IAdaptedOutput*> adaptedOutputs()  const override;

    void addAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutputs) override;

    bool removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput) override;

    void updateValues(HydroCouple::IInput *querySpecifier) override;

    virtual void updateValues() = 0;

    virtual void refreshAdaptedOutputs();

    virtual void initializeAdaptedOutputs();

  signals:

    void propertyChanged(const QString &propertyName) override;

  private:

    QHash<QString,HydroCouple::IInput*> m_consumers;
    QHash<QString,HydroCouple::IAdaptedOutput*> m_adaptedOutputs;

};

Q_DECLARE_METATYPE(AbstractOutput*)

#endif // ABSTRACTOUTPUT_H

