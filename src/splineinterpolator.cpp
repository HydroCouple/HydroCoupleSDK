#include "stdafx.h"
#include "spline.h"
#include "splineinterpolator.h"

#include <math.h>
#include <assert.h>

double SplineInterpolator::interpolate(InterpolationMethod interpolationMode, const std::vector<double> &x,
                                       const std::vector<double> &y, double predx)
{
  double value = 0;

  switch (interpolationMode)
  {
    case InterpolationMethod::Linear:
      {
        assert(x.size() >= 2);
        int last = x.size() - 1;
        value = y[last - 1] + ((predx - x[last - 1]) * (y[last] - y[last -1]) / (x[last] - x[last - 1]));
      }
      break;
    case InterpolationMethod::Quadratic:
      {
        assert(x.size() >= 3);
        int n1 = x.size() - 1;
        int n = n1 - 1;
        int n_1 = n - 1;

        value = ((predx - x[n]) * (predx - x[n_1]) * y[n1] / ((x[n1] - x[n])*(x[n1] - x[n_1]))) +
                ((predx - x[n1]) * (predx - x[n_1]) * y[n] / ((x[n] - x[n1])*(x[n] - x[n_1]))) +
                ((predx - x[n1]) * (predx - x[n]) * y[n_1] / ((x[n_1] - x[n])*(x[n_1] - x[n1])));
      }
      break;
    case InterpolationMethod::Cubic:
      {
        assert(x.size() >= 4);

        tk::spline sp;
        sp.set_points(x,y);

        value = sp(predx);

//        if(value)
//        {
//          printf("%f\n",value);
//        }
      }
      break;
    case InterpolationMethod::Lagrange:
      {
        for(size_t i = 0; i < x.size(); i++)
        {
          value += y[i] * basis(i,predx,x);
        }
      }
      break;
  }

  return value;
}

double SplineInterpolator::basis(int index, double x, const std::vector<double> &xs)
{
  double xidv = xs[index];
  double value = 1.0;

  for(size_t i = 0; i < xs.size(); i++)
  {
    if((size_t)index != i)
    {
      value *= (x - xs[i])/(xidv-xs[i]);
    }
  }

  return value;
}
