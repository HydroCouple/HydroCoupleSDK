#ifndef TIMESERIESARGUMENT_H
#define TIMESERIESARGUMENT_H

#include "timeseriescomponentdataitem.h"
#include "core/abstractargument.h"

#include <QFileInfo>

class HYDROCOUPLESDK_EXPORT TimeSeriesArgumentDouble : public AbstractArgument,
    public TimeSeriesComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeSeriesComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                 HydroCouple::Temporal::ITimeSeriesComponentDataItem)

  public:
    TimeSeriesArgumentDouble(const QString &id,
                             Dimension *timeDimension,
                             ValueDefinition *valueDefinition,
                             AbstractModelComponent *modelComponent);

    virtual ~TimeSeriesArgumentDouble();

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    int timeCount() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, void *data) const override;

    void getValues(int timeIndex, int stride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int timeIndex, const void *data) override;

    void setValues(int timeIndex, int stride, const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) const override;

    void saveData() override;

    QString toString() const override;

    bool readValues(const QString &value, QString &message, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem, QString &message) override;

    bool readTimeSeriesFile(QFileInfo &fileInfo, QString &message);

  private:

    Dimension *m_timeDimension;
    QFileInfo m_inputFile;
};

#endif // TIMESERIESARGUMENT_H
