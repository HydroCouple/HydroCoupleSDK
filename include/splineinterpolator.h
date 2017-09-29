#ifndef SPLINEINTERPOLATOR_H
#define SPLINEINTERPOLATOR_H

#include "hydrocouplesdk.h"

#include <QObject>
#include <vector>

class HYDROCOUPLESDK_EXPORT SplineInterpolator
{
  private:
    SplineInterpolator(){}

  public:
   enum InterpolationMethod
   {
     Linear = 0,
     Quadratic = 1,
     Cubic = 2,
     Lagrange = 3
   };

   static double interpolate(InterpolationMethod interpolationMode, const std::vector<double> &x,
                                          const std::vector<double> &y, double predx);

   static double basis(int index, double x, const std::vector<double> &xs);

};

#endif // SPLINEINTERPOLATOR_H
