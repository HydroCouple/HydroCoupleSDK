#include "stdafx.h"
#include "componentdataitemtest.h"
#include "geometrytest.h"
#include "polyhedralsurfacetest.h"
#include "hydrocouplevis.h"

#include <QtWidgets/QApplication>
#include <QtTest>

int main(int argc, char** argv)
{

  QApplication a(argc, argv);

  HydroCoupleVis * instance = HydroCoupleVis::getInstance();
  instance->show();

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
