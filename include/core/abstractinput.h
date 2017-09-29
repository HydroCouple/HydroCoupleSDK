/*!
 *  \file    AbstractAdaptedOutputFactoryComponentInfo.h
 *  \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version 1.0.0.0
 *  \section Description
 *  \section License
 *  AbstractAdaptedOutputFactoryComponentInfo.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  AbstractAdaptedOutputFactoryComponentInfo.h its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2016
 *  \pre
 *  \bug
 *  \todo
 *  \warning
 */

#ifndef ABSTRACTINPUT_H
#define ABSTRACTINPUT_H

#include "abstractexchangeitem.h"

class HYDROCOUPLE_EXPORT AbstractInput : public AbstractExchangeItem,
    public virtual HydroCouple::IInput
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IInput)
    Q_PROPERTY(HydroCouple::IOutput* Provider READ provider NOTIFY propertyChanged)

  public:

    AbstractInput(const QString &id,
                  const QList<Dimension*> &dimensions,
                  ValueDefinition *valueDefinition,
                  AbstractModelComponent *modelComponent);

    AbstractInput(const QString &id,
                  const QString &caption,
                  const QList<Dimension*> &dimensions,
                  ValueDefinition *valueDefinition,
                  AbstractModelComponent *modelComponent);

    virtual ~AbstractInput();

    HydroCouple::IOutput *provider() const override;

    bool setProvider(HydroCouple::IOutput *provider) override;

    virtual bool canConsume(HydroCouple::IOutput *provider, QString &message) const override;

    virtual void retrieveValuesFromProvider() = 0;

    virtual void applyData() = 0;

  signals:

    void propertyChanged(const QString &propertyName) override;

  private:

    HydroCouple::IOutput *m_provider;

};

class HYDROCOUPLE_EXPORT AbstractMultiInput: public AbstractInput,
    public virtual HydroCouple::IMultiInput
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IMultiInput)
    Q_PROPERTY(QList<HydroCouple::IOutput*> Providers READ providers NOTIFY propertyChanged)

  public:

    AbstractMultiInput(const QString &id,
                       const QList<Dimension*> &dimensions,
                       ValueDefinition *valueDefinition,
                       AbstractModelComponent *modelComponent);

    AbstractMultiInput(const QString &id,
                       const QString &caption,
                       const QList<Dimension*> &dimensions,
                       ValueDefinition *valueDefinition,
                       AbstractModelComponent *modelComponent);

    virtual ~AbstractMultiInput();

    QList<HydroCouple::IOutput*>  providers() const override;

    bool addProvider(HydroCouple::IOutput *provider) override;

    bool removeProvider(HydroCouple::IOutput *provider) override;

  signals:

    void propertyChanged(const QString &propertyName) override;

  protected:

    QList<HydroCouple::IOutput*> m_providers;
};

Q_DECLARE_METATYPE(AbstractInput*)
Q_DECLARE_METATYPE(AbstractMultiInput*)

#endif // ABSTRACTINPUT_H

