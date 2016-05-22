#include "stdafx.h"
#include <QtTest>
#include "valuesettest.h"

int main(int argc, char** argv)
{

   int status = 0;
   {
       ValueSetTest valueSetTest;
       status |= QTest::qExec(&valueSetTest, argc, argv);
   }

   {
     // TestClassB tc;
   //   status |= QTest::qExec(&tc, argc, argv);
   }

   return status;
}
