/*!
 * \file progresschecker.h
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

#ifndef PROGRESSCHECKER_H
#define PROGRESSCHECKER_H

#include "hydrocouplesdk.h"
#include <QObject>

class HYDROCOUPLESDK_EXPORT ProgressChecker : public QObject
{
    Q_OBJECT

  public:
    ProgressChecker(double begin, double end, double numSteps, QObject *parent);

    ~ProgressChecker();

    bool performStep(double value);

    void reset(double begin, double end, double numSteps = 100);

    void reset();

    double progress() const;

    bool isBusy() const;

    double begin() const;

    double end() const;

    double currentValue() const;

    bool isFinished() const;

  public slots:

    void onPerformStep(double value);

  signals:

    void progressChanged(double progress);

  private:

    bool m_progPerformed;
    double m_begin, m_end, m_step, m_nextStep, m_progress;
    double m_value;

};

#endif // PROGRESSCHECKER_H
