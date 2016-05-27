#ifndef ARGUMENT1D_H
#define ARGUMENT1D_H

#include "abstractargument.h"
#include "componentdataitem1d.h"
#include <QFileInfo>

class HYDROCOUPLESDK_EXPORT Argument1DInt : public AbstractArgument,
    public virtual ComponentDataItem1D<int>
{
  public:
    Argument1DInt(const QString &id, Dimension *dimension,
                  ValueDefinition *valueDefinition, AbstractModelComponent *modelComponent);

    virtual ~Argument1DInt(){}

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

    QString toString() const override;

    bool readValues(const QString &value, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem) override;

  private:
    QFileInfo m_inputFile;
};

class HYDROCOUPLESDK_EXPORT Argument1DDouble : public AbstractArgument,
    public virtual ComponentDataItem1D<double>
{
  public:
    Argument1DDouble(const QString &id, Dimension *dimension,
                  ValueDefinition *valueDefinition, AbstractModelComponent *modelComponent);

    virtual ~Argument1DDouble(){}

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

    QString toString() const override;

    bool readValues(const QString &value, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem) override;

  private:
    QFileInfo m_inputFile;

};

class HYDROCOUPLESDK_EXPORT Argument1DString : public AbstractArgument,
    public virtual ComponentDataItem1D<QString>
{
  public:
    Argument1DString(const QString &id, Dimension *dimension,
                  ValueDefinition *valueDefinition, AbstractModelComponent *modelComponent);

    virtual ~Argument1DString(){}

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

    QString toString() const override;

    bool readValues(const QString &value, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem) override;

  private:
    QFileInfo m_inputFile;
};


Q_DECLARE_METATYPE(Argument1DInt*)
Q_DECLARE_METATYPE(Argument1DDouble*)
Q_DECLARE_METATYPE(Argument1DString*)

#endif // ARGUMENT1D_H

