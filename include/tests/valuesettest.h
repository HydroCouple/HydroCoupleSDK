#include "stdafx.h"
#include <QtTest/QtTest>
#include "core/unit.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "spatial/point.h"

using namespace HydroCouple;

class ValueSetTest : public QObject
{
      Q_OBJECT
   private slots:

      void init()
      {

      }

      void testValueSet1DDouble()
      {
         QBENCHMARK
         {
            Quantity* quantity = new Quantity("test", QVariant::Double , nullptr);
            Unit* unit = new Unit("Test" , 10.0,10.0, quantity);
            Dimension* dim = new Dimension("EET","", 600);

            double inputValues[600];


            int inp_index[1] = {0};
            int inp_stride[1] = {600};


            int out_index[1] = {231};
            int out_stride[1] = {321};
            double outputValues[321];


            delete dim;
            delete quantity;
         }
      }

      void testValueSet2DDouble()
      {
         QBENCHMARK
         {
            Quantity* quantity = new Quantity("test", QVariant::Double , nullptr);
            Unit* unit = new Unit("Test" , 10.0,10.0, quantity);

            QList<Dimension*> dimensions;
            dimensions.append(new Dimension("dim1","", 500));
            dimensions.append(new Dimension("dim2","", 600));


            double inputValues[300000];

            int inp_index[2] = {0,0};
            int inp_stride[2] = {500,600};


            int out_index[2] = {21,32};
            int out_stride[] = {321,532};
            double outputValues[170772];


            delete quantity;
            qDeleteAll(dimensions);
            dimensions.clear();
         }
      }

      void testValueSet3DDouble()
      {
         QBENCHMARK
         {
            Quantity* quantity = new Quantity("test", QVariant::Double , nullptr);
            Unit* unit = new Unit("Test" , 10.0,10.0, quantity);

            QList<Dimension*> dimensions;
            dimensions.append(new Dimension("dim1","", 521));
            dimensions.append(new Dimension("dim2","", 502));
            dimensions.append(new Dimension("dim3","", 511));


            double* inputValues = new double [133647962];
            int inp_index[3] = {0,0,0};
            int inp_stride[3] = {521,502,511};


            int out_index[3] = {21,32,11};
            int out_stride[3] = {321,317,331};
            double* outputValues = new double[33681567];


            delete[] inputValues;
            delete[] outputValues;
            delete quantity;
            qDeleteAll(dimensions);
            dimensions.clear();
         }
      }

      void testPoint()
      {
        HCPoint* point = new HCPoint();
      }

      void cleanup()
      {
         //delete valueSet;
      }

};

