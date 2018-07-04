#ifndef TIMESERIES_H
#define TIMESERIES_H

#include "hydrocouplesdk.h"

#include <QObject>
#include <QFileInfo>

class HYDROCOUPLESDK_EXPORT TimeSeries : public QObject
{
    Q_OBJECT

  public:

    TimeSeries(const QString &id, int numColumns = 1,  QObject *parent = nullptr);

    ~TimeSeries();

    QString id() const;

    void clear();

    int numColumns() const;

    void setNumColumns(int columnCount);

    int numRows() const;

    QString getColumnName(int columnIndex) const;

    void setColumnName(int columnIndex, const QString &columnName);

    bool addRow(double dateTime, double defaultValue = 0);

    bool addRow(double dateTime, const std::vector<double>& values);

    void setValue(int row, int columnIndex, double value);

    double dateTime(double rowIndex) const;

    double value(double rowIndex, int columnIndex = 1) const;

    bool removeRow(int rowIndex);

    int findDateTimeIndex(double dateTime);

    int findDateTimeIndex(double dateTime, int startIndex = 0);

    void resetCursor();

    static TimeSeries *createTimeSeries(const QString &id, const QFileInfo &filePath, QObject *parent = nullptr);

    static bool readTimeSeries(const QFileInfo &fileInfo, std::map<double, std::vector<double>> &timeSeriesValues, std::vector<std::string> &headers);

  private:
    QString m_id;
    int m_numColumns;
    std::vector<QString> m_columnNames;
    std::vector<double> m_dateTimes;
    std::vector<std::vector<double>> m_values;
    int m_cursorLocation ;
    static const QRegExp m_delimiters; //Regex delimiter
};

#endif // TIMESERIES_H
