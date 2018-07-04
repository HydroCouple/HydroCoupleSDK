#include "stdafx.h"
#include "timeseries.h"
#include "temporal/timedata.h"

#include <QTextStream>
#include <QDateTime>

using namespace std;

TimeSeries::TimeSeries(const QString &id, int numColumns, QObject *parent)
  : QObject(parent),
    m_id(id),
    m_numColumns(numColumns),
    m_cursorLocation(0)
{
  setNumColumns(m_numColumns);
}

TimeSeries::~TimeSeries()
{

}

QString TimeSeries::id() const
{
  return m_id;
}

void TimeSeries::clear()
{
  m_dateTimes.clear();
  m_values.clear();
}

int TimeSeries::numColumns() const
{
  return m_numColumns;
}

void TimeSeries::setNumColumns(int columnCount)
{
  size_t origSize = m_columnNames.size();
  m_columnNames.resize(columnCount);

  for(size_t i = origSize; i < m_columnNames.size(); i++)
  {
    m_columnNames[i] = "Untitled Column " + QString::number(i);
  }

  for(size_t i = 0; i < m_values.size(); i++)
  {
    std::vector<double>& row = m_values[i];
    row.resize(columnCount);
  }
}

int TimeSeries::numRows() const
{
  return (int)m_dateTimes.size();
}

QString TimeSeries::getColumnName(int columnIndex) const
{
  return m_columnNames[columnIndex];
}

void TimeSeries::setColumnName(int columnIndex, const QString &columnName)
{
  m_columnNames[columnIndex] = columnName;
}

bool TimeSeries::addRow(double dateTime, double defaultValue)
{
  if(m_dateTimes.size() && dateTime <= m_dateTimes[m_dateTimes.size() - 1])
  {
    return false;
  }

  m_dateTimes.push_back(dateTime);

  std::vector<double> rowValues(m_numColumns, defaultValue);
  m_values.push_back(rowValues);

  return true;
}

bool TimeSeries::addRow(double dateTime, const std::vector<double> &values)
{
  if(m_dateTimes.size() && dateTime <= m_dateTimes[m_dateTimes.size() - 1])
  {
    return false;
  }

  if(m_numColumns != (int)values.size())
  {
    return false;
  }

  m_dateTimes.push_back(dateTime);
  m_values.push_back(values);

  return true;
}

void TimeSeries::setValue(int row, int columnIndex, double value)
{
  m_values[row][columnIndex] = value;
}

double TimeSeries::dateTime(double rowIndex) const
{
  return m_dateTimes[rowIndex];
}

double TimeSeries::value(double rowIndex, int columnIndex) const
{
  return m_values[rowIndex][columnIndex];
}

bool TimeSeries::removeRow(int rowIndex)
{
  if(rowIndex < (int)m_values.size())
  {
    m_values.erase(m_values.begin() + rowIndex);
    m_dateTimes.erase(m_dateTimes.begin() + rowIndex);

    return true;
  }

  return false;
}

int TimeSeries::findDateTimeIndex(double dateTime)
{
  if(dateTime >= m_dateTimes[0] && dateTime <= m_dateTimes[m_dateTimes.size() - 1])
  {
    for(size_t i = m_cursorLocation; i < m_dateTimes.size() - 1; i++)
    {
      if(dateTime >= m_dateTimes[i] && dateTime <= m_dateTimes[i+1])
      {
        m_cursorLocation = i;
        return m_cursorLocation;
      }
    }

    //resetart
    for(size_t i = 0; i < m_dateTimes.size() - 1; i++)
    {
      if(dateTime >= m_dateTimes[i] && dateTime <= m_dateTimes[i+1])
      {
        m_cursorLocation = i;
        return m_cursorLocation;
      }
    }
  }

  return -1;
}

int TimeSeries::findDateTimeIndex(double dateTime, int startIndex)
{
  if(dateTime >= m_dateTimes[0] && dateTime <= m_dateTimes[m_dateTimes.size() - 1])
  {
    for(size_t i = startIndex; i < m_dateTimes.size() - 1; i++)
    {
      if(dateTime >= m_dateTimes[i] && dateTime <= m_dateTimes[i+1])
      {
        startIndex = i;
        return startIndex;
      }
    }

    //resetart
    for(size_t i = 0; i < m_dateTimes.size() - 1; i++)
    {
      if(dateTime >= m_dateTimes[i] && dateTime <= m_dateTimes[i+1])
      {
        startIndex = i;
        return startIndex;
      }
    }
  }

  return -1;
}

void TimeSeries::resetCursor()
{
  m_cursorLocation = 0;
}

TimeSeries *TimeSeries::createTimeSeries(const QString &id, const QFileInfo &filePath, QObject *parent)
{
  std::map<double, std::vector<double>> timeSeriesValues;
  std::vector<std::string> headers;

  if(readTimeSeries(filePath, timeSeriesValues, headers))
  {
    TimeSeries *timeSeries = new TimeSeries(id, headers.size(), parent);

    for(size_t i = 0;  i < headers.size(); i++)
    {
      timeSeries->setColumnName(i, QString::fromStdString(headers[i]));
    }

    for(auto const &it : timeSeriesValues)
    {
      timeSeries->addRow(it.first, it.second);
    }

    return timeSeries;
  }

  return nullptr;
}

bool TimeSeries::readTimeSeries(const QFileInfo &fileInfo, std::map<double, std::vector<double>> &timeSeriesValues, std::vector<string> &headers)
{
  timeSeriesValues.clear();
  headers.clear();

  if (!fileInfo.filePath().isEmpty() && QFile::exists(fileInfo.absoluteFilePath()))
  {
    QFile file(fileInfo.absoluteFilePath());

    if (file.open(QIODevice::ReadOnly))
    {
      QRegExp commaTabDel("(\\,|\\t|\\\n)");
      QTextStream streamReader(&file);
      QString line = file.readLine();
      QStringList columns = line.split(commaTabDel, QString::SkipEmptyParts);
      bool noError = true;

      if (columns.size() > 1)
      {
        headers.clear();

        for (int i = 1; i < columns.size(); i++)
        {
          headers.push_back(columns[i].toStdString());
        }

        while (!streamReader.atEnd())
        {
          line = file.readLine();
          columns = line.split(commaTabDel, QString::SkipEmptyParts);

          if (columns.size() > (int)(headers.size()))
          {
            QDateTime dt;

            if (SDKTemporal::DateTime::tryParse(columns[0], dt))
            {
              double dateTimeJD = SDKTemporal::DateTime::toJulianDays(dt);

              std::vector<double> values;
              values.reserve(headers.size());

              for (int i = 1; i < columns.size(); i++)
              {
                bool ok;
                double value = columns[i].toDouble(&ok);

                if (ok)
                {
                  values.push_back(value);
                }
                else
                {
                  noError = false;
                  break;
                }
              }

              if (noError)
              {
                timeSeriesValues[dateTimeJD] = values;
              }
            }
            else
            {
              SDKTemporal::DateTime::tryParse(columns[0], dt);
              noError = false;
            }
          }
          else
          {
            noError = false;
            break;
          }

          if (noError == false)
            break;
        }
      }
      else
      {
        noError = false;
      }

      file.close();
      return noError;
    }
  }

  return false;
}

const QRegExp TimeSeries::m_delimiters("(\\,|\\t|\\\n)");
