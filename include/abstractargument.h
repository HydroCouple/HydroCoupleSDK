#ifndef ABSTRACTARGUMENT_H
#define ABSTRACTARGUMENT_H

#include "identity.h"
#include <QXmlStreamReader>

class AbstractModelComponent;
class Dimension;
class AbstractComponentDataItem;

class HYDROCOUPLESDK_EXPORT AbstractArgument : public Identity , public virtual HydroCouple::IArgument
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IArgument)
      Q_PROPERTY(bool Optional READ isOptional NOTIFY propertyChanged)
      Q_PROPERTY(bool ReadOnly READ isReadOnly NOTIFY propertyChanged)
      Q_PROPERTY(QStringList InputFileTypeFilters READ inputFileTypeFilters NOTIFY propertyChanged)

   public:
      AbstractArgument(const QString &id, AbstractModelComponent* parentModelComponent);

      virtual ~AbstractArgument();

      bool isOptional() const override;

      bool isReadOnly() const override;

      QStringList inputFileTypeFilters() const override;

      bool canReadFromFile() const override;

      bool canReadFromString() const override;

      HydroCouple::ArgumentIOType argumentIOType() const override;

      virtual void addInputFileTypeFilter(const QString &extension);

      virtual void setIsOptional(bool isOptional);

      virtual void setIsReadOnly(bool isReadOnly);

      virtual void setArgumentIOType(HydroCouple::ArgumentIOType argumentIOType);

      virtual void setCanReadFromFile(bool canReadFromFile);

      virtual void setCanReadFromString(bool canReadFromString);

   signals:

      void propertyChanged(const QString &propertyName) override;

   private:
      QStringList m_inputFileTypeFilters;
      bool m_isOptional , m_isReadOnly, m_canReadFromFile, m_canReadFromString;
      HydroCouple::ArgumentIOType m_argumentIOType;
};


Q_DECLARE_METATYPE(AbstractArgument*)
Q_DECLARE_METATYPE(QList<AbstractArgument*>)

#endif // ABSTRACTARGUMENT_H

