#ifndef TIMESERIES_H
#define TIMESERIES_H

#include "hydrocouplesdk.h"
#include "temporal/timedata.h"

#include <QObject>
#include <QFileInfo>

class DataCursor;

namespace SDKTemporal
{
  class DateTime;
  class TimeSpan
;}

class HYDROCOUPLESDK_EXPORT TimeSeries : public QObject
{
    Q_OBJECT

  public:

    TimeSeries(const QString &id, int numColumns = 1,  QObject *parent = nullptr);

    ~TimeSeries();

    QString id() const;

    void setId(const QString &id);

    void clear();

    int numColumns() const;

    void setNumColumns(int columnCount);

    int numRows() const;

    QString getColumnName(int columnIndex) const;

    void setColumnName(int columnIndex, const QString &columnName);

    DataCursor *dataCursor() const;

    bool addRow(double dateTime, double defaultValue = 0);

    bool addRow(double dateTime, const std::vector<double>& values);

    void setValue(int rowIndex, int columnIndex, double value);

    double dateTime(int rowIndex) const;

    double value(double rowIndex, int columnIndex = 0) const;

    bool removeRow(int rowIndex);

    int findDateTimeIndex(double dateTime);

    int findDateTimeIndex(double dateTime, DataCursor *dataCursor);

    int findDateTimeIndex(double dateTime, int startIndex);

    bool interpolate(double dateTime, int columnIndex, double &value);

    bool interpolate(double dateTime, int columnIndex, DataCursor *dataCursor, double &value);

    static TimeSeries *createTimeSeries(const QString &id, const QFileInfo &filePath, QObject *parent = nullptr);

    static bool readTimeSeries(const QFileInfo &fileInfo, std::map<double, std::vector<double>> &timeSeriesValues, std::vector<std::string> &headers);

    /*!
     * \brief splitLine Keep here for now. Needs to move to more appropriate class.
     * \param line
     * \param delimiters
     * \return
     */
    static QStringList splitLine(const QString &line, const QString &delimiters);

  private:

    void resetTimeSpan();

  private:

    QString m_id;
    int m_numColumns;
    std::vector<QString> m_columnNames;
    std::vector<double> m_dateTimes;
    std::vector<std::vector<double>> m_values;
    static const QRegExp m_delimiters; //Regex delimiter
    DataCursor *m_cursor;
};

#endif // TIMESERIES_H
