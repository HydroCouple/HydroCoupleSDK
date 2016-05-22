#ifndef COMPONENTDATAITEM1D_H
#define COMPONENTDATAITEM1D_H

#include "abstractcomponentdataitem.h"

class Dimension;
class ValueDefinition;

/*!
 * \brief The ComponentDataItem1D class
 */
template<class T>
class HYDROCOUPLESDK_EXPORT ComponentDataItem1D :  public virtual HydroCouple::IComponentDataItem
{

   public:

      ComponentDataItem1D(Dimension *dimension, ValueDefinition *valueDefinition);

      virtual ~ComponentDataItem1D();

      QList<HydroCouple::IDimension*> dimensions() const override;

      HydroCouple::IValueDefinition* valueDefinition() const override;

      void getValue(int dimensionIndexes[], QVariant &data) const override;

      void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

      void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

      void setValue(int dimensionIndexes[], const QVariant &data) override;

      void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

      void setValues(int dimensionIndexes[], int stride[], const void *data) override;

      virtual void resetDataArray();

   protected:

      Dimension* hDimension() const;

      ValueDefinition* hValueDefinition() const;

   private:
      void createData();

      void deleteData();

   private:
      Dimension* m_dimension;
      ValueDefinition* m_valueDefinition;

   private:
      T* m_data;
};

class HYDROCOUPLESDK_EXPORT ComponentDataItem1DInt: public AbstractComponentDataItem, public virtual ComponentDataItem1D<int>
{
      Q_OBJECT
      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)

   public:

      ComponentDataItem1DInt(const QString &id, Dimension *dimension, ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent);

      virtual ~ComponentDataItem1DInt();

      void readData(QXmlStreamReader &xmlReader) override;

      void writeData(QXmlStreamWriter &xmlWriter) override;

};

class HYDROCOUPLESDK_EXPORT ComponentDataItem1DDouble: public AbstractComponentDataItem,  public virtual ComponentDataItem1D<double>
{
      Q_OBJECT
      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)

   public:

      ComponentDataItem1DDouble(const QString &id, Dimension *dimension, ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent);

      virtual ~ComponentDataItem1DDouble();

      void readData(QXmlStreamReader &xmlReader) override;

      void writeData(QXmlStreamWriter &xmlWriter) override;

};

class HYDROCOUPLESDK_EXPORT ComponentDataItem1DString: public AbstractComponentDataItem,  public virtual ComponentDataItem1D<QString>
{
      Q_OBJECT
      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)

   public:

      ComponentDataItem1DString(const QString &id, Dimension *dimension, ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent);

      virtual ~ComponentDataItem1DString();

      void readData(QXmlStreamReader &xmlReader) override;

      void writeData(QXmlStreamWriter &xmlWriter) override;
};

Q_DECLARE_METATYPE(ComponentDataItem1DInt*)
Q_DECLARE_METATYPE(ComponentDataItem1DDouble*)
Q_DECLARE_METATYPE(ComponentDataItem1DString*)

#endif // COMPONENTDATAITEM1D_H

