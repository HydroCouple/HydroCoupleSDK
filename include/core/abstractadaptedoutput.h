#ifndef ABSTRACTADAPTEDOUTPUT_H
#define ABSTRACTADAPTEDOUTPUT_H

#include "abstractoutput.h"

class AbstractAdaptedOutputFactory;
class AbstractArgument;

class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutput : public Identity,
    public virtual HydroCouple::IAdaptedOutput
{

    Q_OBJECT
    Q_INTERFACES(HydroCouple::IAdaptedOutput)

    Q_PROPERTY(HydroCouple::IModelComponent* ModelComponent READ modelComponent NOTIFY propertyChanged)
    Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
    Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
    Q_PROPERTY(QStringList Comments READ comments WRITE setComments NOTIFY propertyChanged)

    Q_PROPERTY(QList<HydroCouple::IInput*> Consumers READ consumers NOTIFY propertyChanged)
    Q_PROPERTY(QList<HydroCouple::IAdaptedOutput*> AdaptedOutputs READ adaptedOutputs NOTIFY propertyChanged)

    Q_PROPERTY(HydroCouple::IAdaptedOutputFactory* AdaptedOutputFactory READ adaptedOutputFactory)
    Q_PROPERTY(QList<HydroCouple::IArgument*> Arguments READ arguments NOTIFY propertyChanged)
    Q_PROPERTY(HydroCouple::IOutput* Adaptee READ adaptee)
    Q_PROPERTY(HydroCouple::IInput* Input READ input NOTIFY propertyChanged)


  public:
    AbstractAdaptedOutput(const QString& id,
                          const QList<Dimension*>& dimensions,
                          ValueDefinition* valueDefinition,
                          HydroCouple::IOutput* adaptee,
                          AbstractAdaptedOutputFactory* adaptedOutputFactory);

    HydroCouple::IModelComponent* modelComponent() const override;

    QList<HydroCouple::IDimension*> dimensions() const override;

    HydroCouple::IValueDefinition* valueDefinition() const override;

    QStringList comments() const;

    void setComments(const QStringList& comments);

    void addComment(const QString& comment);

    QList<HydroCouple::IInput*> consumers() const override;

    void addConsumer(HydroCouple::IInput *consumer) override;

    bool removeConsumer(HydroCouple::IInput *consumer) override;

    QList<HydroCouple::IAdaptedOutput*> adaptedOutputs()  const override;

    void addAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutputs) override;

    bool removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput) override;

    HydroCouple::IAdaptedOutputFactory* adaptedOutputFactory() const override;

    QList<HydroCouple::IArgument*> arguments() const override;

    HydroCouple::IOutput* adaptee() const override;

    HydroCouple::IInput* input() const ;

    void setInput(HydroCouple::IInput* input);

  signals:

    void propertyChanged(const QString& propertyName);

    void itemChanged(const QSharedPointer<HydroCouple::IExchangeItemChangeEventArgs>& statusChangedEvent) override;

  protected:

    QList<Dimension*> dimensionsInternal() const;

    ValueDefinition* valueDefinitionInternal() const;

    AbstractAdaptedOutputFactory* adaptedOutputFactoryInternal() const;

    void addArgument(AbstractArgument *argument);

    bool removeArgument(AbstractArgument *argument);

    void clearArguments();

    QHash<QString,AbstractArgument*> argumentsInternal() const;

  private:
    QList<Dimension*> m_dimensions;
    ValueDefinition* m_valueDefinition;
    QStringList m_comments;
    QHash<QString,HydroCouple::IInput*> m_consumers;
    QHash<QString,HydroCouple::IAdaptedOutput*> m_adaptedOutputs;
    AbstractAdaptedOutputFactory* m_adaptedOutputFactory;
    QHash<QString,AbstractArgument*> m_arguments;
    HydroCouple::IOutput* m_adaptee;
    HydroCouple::IInput* m_input;
};

#endif // ABSTRACTADAPTEDOUTPUT_H
