/*!
 * \file timeseriesexchangeitem.h
 * \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#ifndef TIMESERIESEXCHANGEITEM_H
#define TIMESERIESEXCHANGEITEM_H

#include "timeseriescomponentdataitem.h"
#include "core/abstractinput.h"
#include "core/abstractoutput.h"


class HYDROCOUPLESDK_EXPORT TimeSeriesOutputDouble: public AbstractOutput,
    public TimeSeriesComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeSeriesComponentDataItem
{

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                 HydroCouple::Temporal::ITimeSeriesComponentDataItem)

    Q_PROPERTY(QList<HydroCouple::Temporal::IDateTime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:

    TimeSeriesOutputDouble(const QString& id,
                           const std::list<SDKTemporal::DateTime *> &times,
                           Dimension* timeDimension,
                           ValueDefinition* valueDefinition,
                           AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesOutputDouble();

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, void *data) const override;

    void getValues(int timeIndex, int stride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int timeIndex, const void *data) override;

    void setValues(int timeIndex, int stride, const void *data) override;

  private:

    Dimension *m_timeDimension;
};

//==============================================================================================================================


class HYDROCOUPLESDK_EXPORT TimeSeriesInputDouble: public AbstractInput,
    public TimeSeriesComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeSeriesComponentDataItem
{

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                 HydroCouple::Temporal::ITimeSeriesComponentDataItem)

    Q_PROPERTY(QList<HydroCouple::Temporal::IDateTime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:
    TimeSeriesInputDouble(const QString& id,
                          const std::list<SDKTemporal::DateTime *> &times,
                          Dimension* timeDimension,
                          ValueDefinition* valueDefinition,
                          AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesInputDouble();

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, void *data) const override;

    void getValues(int timeIndex, int stride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int timeIndex, const void *data) override;

    void setValues(int timeIndex, int stride, const void *data) override;

  private:

    Dimension *m_timeDimension;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT TimeSeriesMultiInputDouble: public AbstractMultiInput,
    public TimeSeriesComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeSeriesComponentDataItem
{

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                 HydroCouple::Temporal::ITimeSeriesComponentDataItem)

    Q_PROPERTY(QList<HydroCouple::Temporal::IDateTime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:

    TimeSeriesMultiInputDouble(const QString& id,
                               const std::list<SDKTemporal::DateTime*>& times,
                               Dimension* timeDimension,
                               ValueDefinition* valueDefinition,
                               AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesMultiInputDouble();

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

  private:

    Dimension *m_timeDimension;
};


Q_DECLARE_METATYPE(TimeSeriesOutputDouble*)
Q_DECLARE_METATYPE(TimeSeriesInputDouble*)
Q_DECLARE_METATYPE(TimeSeriesMultiInputDouble*)

#endif // TIMESERIESEXCHANGEITEM_H

