#include "stdafx.h"
#include <QtTest/QtTest>
#include "core/unit.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "spatial/point.h"
#include "core/componentdataitem1d.h"
#include "core/componentdataitem2d.h"
#include "core/componentdataitem3d.h"

using namespace HydroCouple;

class ComponentDataItemTest : public QObject
{
    Q_OBJECT

  private slots:

    void init()
    {

    }

    void testComponentDataItem1DDouble()
    {
      QBENCHMARK
      {
        Unit* unit = new Unit("Test" , 10.0,10.0);
        Quantity* quantity = new Quantity("test", QVariant::Double , unit);

        Dimension* dim = new Dimension("EET");
        ComponentDataItem1DDouble* data1D = new ComponentDataItem1DDouble("test1DDouble",dim,600,quantity,nullptr);

        double* inValues= new double[600];

        for(int i = 0 ; i < 600 ; i++)
        {
          inValues[i] = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/1000.0f));
        }

        int inp_index[1] = {0};
        int inp_stride[1] = {600};

        data1D->setValues(inp_index,inp_stride,inValues);

        int out_index[1] = {231};
        int out_stride[1] = {321};
        double* outValues = new double[321];

        data1D->getValues(out_index, out_stride,outValues);

        for(int i = 0 ; i < 321 ; i++)
        {
          QCOMPARE(inValues[231+i], outValues[i]);
        }

        delete unit;
        delete dim;
        delete quantity;
        delete data1D;
        delete[] inValues;
        delete[] outValues;
      }
    }

    void testComponentDataItem2DDouble()
    {
      QBENCHMARK
      {

        Unit* unit = new Unit("Test" , 10.0,10.0);
        Quantity* quantity = new Quantity("test", QVariant::Double , unit);

        QList<Dimension*> dims;
        dims.append(new Dimension("dim1"));
        dims.append(new Dimension("dim2"));

        ComponentDataItem2DDouble* data2D = new ComponentDataItem2DDouble("test2DDouble",dims[0],dims[1],500,600,quantity,nullptr);

        double* inValues = new double[300000];

        for(int i = 0 ; i < 500 ; i++)
        {
          for(int j = 0 ; j < 600 ; j++)
          {
            inValues[i + j*500] = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/1000.0f));
          }
        }

        int inp_index[2] = {0,0};
        int inp_stride[2] = {500,600};

        data2D->setValues(inp_index,inp_stride,inValues);

        int out_index[2] = {21,32};
        int out_stride[] = {321,532};
        double* outValues = new double[170772];

        data2D->getValues(out_index, out_stride,outValues);

        for(int i = 0 ; i < 321 ; i++)
        {
          for(int j = 0 ; j < 532 ; j++)
          {
            QCOMPARE(inValues[(i + 21) + (j+32)*500 ] , outValues[i + j*321]);// , "Input and Output values don't match");
          }
        }

        delete unit;
        delete quantity;
        qDeleteAll(dims);
        dims.clear();
        delete data2D;
        delete[] inValues;
        delete[] outValues;
      }
    }

    void testComponentDataItem3DDouble()
    {
      QBENCHMARK
      {
        Unit* unit = new Unit("Test",10.0,10.0);
        Quantity* quantity = new Quantity("test", QVariant::Double , unit);

        QList<Dimension*> dims;
        dims.append(new Dimension("dim1"));
        dims.append(new Dimension("dim2"));
        dims.append(new Dimension("dim2"));

        ComponentDataItem3DDouble* data3D = new ComponentDataItem3DDouble("test3DDouble",dims[0],dims[1],dims[2],
            121,132,105,quantity,nullptr);

        double* inValues = new double[1677060];

        for(int i = 0 ; i < 121 ; i++)
        {
          for(int j = 0 ; j < 132 ; j++)
          {
            for(int k = 0 ; k < 105 ; k++)
            {
              inValues[i + j*121 + k*121*132] = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/1000.0f));
            }
          }
        }

        int inp_index[3] = {0,0,0};
        int inp_stride[3] = {121,132,105};

        data3D->setValues(inp_index,inp_stride,inValues);

        int out_index[3] = {21,32,11};
        int out_stride[3] = {50,63,40};
        double* outValues = new double[126000];

        data3D->getValues(out_index, out_stride,outValues);

        for(int i = 0 ; i < 50 ; i++)
        {
          for(int j = 0 ; j < 63 ; j++)
          {
            for(int k = 0 ; k < 40 ; k++)
            {
              QCOMPARE(inValues[(i + 21) + (j+32)*121 + (k+11) * 121 * 132 ] , outValues[i + j*50 + k*50*63]);// , "Input and Output values don't match");
            }
          }
        }

        delete unit;
        delete quantity;
        qDeleteAll(dims);
        dims.clear();
        delete data3D;
        delete[] inValues;
        delete[] outValues;
      }
    }

    void cleanup()
    {
      //delete valueSet;
    }

};

