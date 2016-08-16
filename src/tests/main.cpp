#include "stdafx.h"
#include "componentdataitemtest.h"
#include "geometrytest.h"
#include "polyhedralsurfacetest.h"

#include <QtTest>

int main(int argc, char** argv)
{

   int status = 0;
   {
       ComponentDataItemTest componentDataItemTest;
       status |= QTest::qExec(&componentDataItemTest, argc, argv);
   }

   {
      GeometryTest geomTest;
      status |= QTest::qExec(&geomTest, argc, argv);
   }

   {
     PolyhedralSurfaceTest polyhedralSurfaceTest;
     status |= QTest::qExec(&polyhedralSurfaceTest,argc,argv);
   }

   return status;
}
