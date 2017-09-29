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

  public slots:

    void onPerformStep(double value);

  signals:

    void progressChanged(double progress);

  private:

    bool m_progPerformed;
    double m_begin, m_end, m_step, m_nextStep, m_progress;

};

#endif // PROGRESSCHECKER_H
