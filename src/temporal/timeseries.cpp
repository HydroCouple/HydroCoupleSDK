#include "stdafx.h"
#include "temporal/timeseries.h"
#include "temporal/timedata.h"
#include "core/datacursor.h"

#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>

using namespace std;

TimeSeries::TimeSeries(const QString &id, int numColumns, QObject *parent)
  : QObject(parent),
    m_id(id),
    m_numColumns(numColumns)
{
  m_cursor = new DataCursor();
  setNumColumns(m_numColumns);
}

TimeSeries::~TimeSeries()
{
  delete m_cursor;
}

QString TimeSeries::id() const
{
  return m_id;
}

void TimeSeries::setId(const QString &id)
{
  m_id = id;
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

DataCursor *TimeSeries::dataCursor() const
{
  return m_cursor;
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

  m_cursor->setMin(0); m_cursor->setMax(m_dateTimes.size() - 1);

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

  m_cursor->setMin(0); m_cursor->setMax(m_dateTimes.size() - 1);

  return true;
}

void TimeSeries::setValue(int rowIndex, int columnIndex, double value)
{
  m_values[rowIndex][columnIndex] = value;
}

double TimeSeries::dateTime(int rowIndex) const
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

    m_cursor->setMin(0); m_cursor->setMax(m_dateTimes.size() - 1);

    return true;
  }

  return false;
}

int TimeSeries::findDateTimeIndex(double dateTime)
{
  if(dateTime >= m_dateTimes[0] && dateTime <= m_dateTimes[m_dateTimes.size() - 1])
  {
    for(size_t i = m_cursor->index(); i < m_dateTimes.size() - 1; i++)
    {
      if(dateTime >= m_dateTimes[i] && dateTime <= m_dateTimes[i+1])
      {
        return m_cursor->index();
      }

      m_cursor->moveNext();
    }

    m_cursor->resetCursor();

    //resetart
    for(size_t i = m_cursor->index(); i < m_dateTimes.size() - 1; i++)
    {
      if(dateTime >= m_dateTimes[i] && dateTime <= m_dateTimes[i+1])
      {
        return m_cursor->index();
      }

      m_cursor->moveNext();
    }
  }

  return -1;
}

int TimeSeries::findDateTimeIndex(double dateTime, DataCursor *dataCursor)
{
  if(dateTime >= m_dateTimes[0] && dateTime <= m_dateTimes[m_dateTimes.size() - 1])
  {
    for(size_t i = dataCursor->index(); i < m_dateTimes.size() - 1; i++)
    {
      if(dateTime >= m_dateTimes[i] && dateTime <= m_dateTimes[i+1])
      {
        return dataCursor->index();
      }
      dataCursor->moveNext();
    }

    dataCursor->resetCursor();

    //resetart
    for(size_t i = dataCursor->index(); i < m_dateTimes.size() - 1; i++)
    {
      if(dateTime >= m_dateTimes[i] && dateTime <= m_dateTimes[i+1])
      {
        return dataCursor->index();
      }
      dataCursor->moveNext();
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

bool TimeSeries::interpolate(double dateTime, int columnIndex, double &value)
{
  int index = findDateTimeIndex(dateTime);

  if(index > -1)
  {
    if(index == (int)m_dateTimes.size() - 1)
    {
      value = m_values[index][columnIndex];
    }
    else
    {
      double downDate = m_dateTimes[index];
      double upDate = m_dateTimes[index + 1];
      double downValue = m_values[index][columnIndex];
      double upValue = m_values[index + 1][columnIndex];
      value = downValue + (upValue - downValue) / (upDate - downDate) * (dateTime -  downDate);
    }

    return true;
  }

  value = -9999999;

  return false;
}

bool TimeSeries::interpolate(double dateTime, int columnIndex, DataCursor *dataCursor, double &value)
{
  int index = findDateTimeIndex(dateTime, dataCursor);

  if(index > -1)
  {
    if(index == static_cast<int>(m_dateTimes.size() - 1))
    {
      value = m_values[index][columnIndex];
    }
    else
    {
      double downDate = m_dateTimes[index];
      double upDate = m_dateTimes[index + 1];
      double downValue = m_values[index][columnIndex];
      double upValue = m_values[index + 1][columnIndex];
      value = downValue + (upValue - downValue) / (upDate - downDate) * (dateTime -  downDate);
    }

    return true;
  }

  value = -9999999;

  return false;
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
  int currentLine = 0;

  if (!fileInfo.filePath().isEmpty() && QFile::exists(fileInfo.absoluteFilePath()))
  {
    QFile file(fileInfo.absoluteFilePath());

    if (file.open(QIODevice::ReadOnly))
    {
      QRegExp commaTabDel("(\\,|\\t|\\\n)");
      QTextStream streamReader(&file);
      QString line = file.readLine();

      currentLine++;

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
          line = file.readLine().trimmed();
          currentLine++;

          if(!line.isEmpty())
          {
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
                    printf("Error cannot parse value on line: %i, %s\n", currentLine, line.toStdString().c_str());
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
                printf("Error cannot parse date time on line: %i, %s\n", currentLine, line.toStdString().c_str());
                SDKTemporal::DateTime::tryParse(columns[0], dt);
                noError = false;
              }
            }
            else
            {
              printf("Error on line: %i, %s\n", currentLine, line.toStdString().c_str());
              noError = false;
              break;
            }
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

QStringList TimeSeries::splitLine(const QString &line, const QString &delimiters)
{
  QString temp = line;
  temp = temp.replace("'","\"");

  QString field;
  QStringList field_list;

  // regex explaination
  //
  //    /(?:^|,)(\"(?:[^\"]+|\"\")*\"|[^,]*)/g
  //        (?:^|,) Non-capturing group
  //            1st Alternative: ^
  //                ^ assert position at start of the string
  //            2nd Alternative: ,
  //                , matches the character , literally
  //        1st Capturing group (\"(?:[^\"]+|\"\")*\"|[^,]*)
  //            1st Alternative: \"(?:[^\"]+|\"\")*\"
  //                \" matches the character " literally
  //                (?:[^\"]+|\"\")* Non-capturing group
  //                    Quantifier: * Between zero and unlimited times, as many times as possible, giving back as needed [greedy]
  //                    1st Alternative: [^\"]+
  //                        [^\"]+ match a single character not present in the list below
  //                            Quantifier: + Between one and unlimited times, as many times as possible, giving back as needed [greedy]
  //                            \" matches the character " literally
  //                    2nd Alternative: \"\"
  //                        \" matches the character " literally
  //                        \" matches the character " literally
  //                \" matches the character " literally
  //            2nd Alternative: [^,]*
  //                [^,]* match a single character not present in the list below
  //                    Quantifier: * Between zero and unlimited times, as many times as possible, giving back as needed [greedy]
  //                    , the literal character ,
  //        g modifier: global. All matches (don't return on first match)
  //

  QString regexTemplate = "(?:^|[regex]+)(\"(?:[^\"]+|\"\")*\"|[^[regex]+]*)";
  QString regex = regexTemplate.replace("regex", delimiters);


  QRegularExpression re(regex);

  if(temp.right(1) == "\n")
    temp.chop(1);

  QRegularExpressionMatchIterator it = re.globalMatch(temp);

  while (it.hasNext())
  {
      QRegularExpressionMatch match = it.next();

      if (match.hasMatch())
      {
          field = match.captured(1);

          if (field.left(1) == "\"" && field.right(1) == "\"")
          {
              field = field.mid(1, field.length()-2);
          }

          field_list.push_back(field);
      }
  }

  return field_list;
}

const QRegExp TimeSeries::m_delimiters("(\\,|\\t|\\\n)");
