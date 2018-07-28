#Author Caleb Amoa Buahin
#Email caleb.buahin@gmail.com
#Date 2014-2018
#License GNU Lesser General Public License (see <http: //www.gnu.org/licenses/> for details).
#Copyright 2014-2018, Caleb Buahin, All rights reserved.

TARGET = HydroCoupleSDK
VERSION = 1.0.0
QT += core testlib concurrent
QT -= gui

DEFINES += HYDROCOUPLESDK_LIBRARY
DEFINES += USE_CHPC
DEFINES += USE_OPENMP
DEFINES += USE_MPI
DEFINES += USE_NETCDF

CONFIG += c++11
CONFIG += debug_and_release
CONFIG += optimize_full

contains(DEFINES,HYDROCOUPLESDK_LIBRARY){

  TEMPLATE = lib
  message("Compiling HydroCoupleSDK as library")

} else {

  TEMPLATE = app
  CONFIG-=app_bundle
  message("Compiling HydroCoupleSDK as application")

}

INCLUDEPATH += ./include \
               ../HydroCouple/include

PRECOMPILED_HEADER += ./include/stdafx.h

HEADERS += ./include/stdafx.h \
           ./include/hydrocouplesdk.h \
           ./include/core/identity.h \
           ./include/core/description.h \
           ./include/core/componentinfo.h \
           ./include/core/abstractmodelcomponentinfo.h \
           ./include/core/abstractmodelcomponent.h \
           ./include/core/componentstatuschangeeventargs.h \
           ./include/core/dimension.h \
           ./include/core/abstractcomponentdataitem.h \
           ./include/core/valuedefinition.h \
           ./include/core/unit.h \
           ./include/core/unitdimensions.h \
           ./include/core/abstractoutput.h \
           ./include/core/abstractinput.h \
           ./include/core/abstractargument.h \
           ./include/core/exchangeitems1d.h \
           ./include/core/exchangeitems2d.h \
           ./include/core/idbasedinputs.h \
           ./include/core/exchangeitemchangeeventargs.h \
           ./include/core/abstractexchangeitem.h \
           ./include/core/idbasedargument.h \
           ./include/temporal/timedata.h \
           ./include/temporal/timeseriescomponentdataitem.h \
           ./include/temporal/timeseriesidbasedcomponentdataitem.h \
           ./include/temporal/timeseriesidbasedexchangeitem.h \
           ./include/temporal/timeseriesexchangeitem.h \
           ./include/spatial/geometry.h \
           ./include/core/argument1d.h \
           ./include/spatial/spatialreferencesystem.h \
           ./include/core/abstractadaptedoutput.h \
           ./include/core/abstractadaptedoutputfactory.h \
           ./include/temporal/temporalinterpolationfactory.h \
           ./include/temporal/timeseriesinterpolationadaptedoutput.h \
           ./include/core/abstractadaptedoutputfactorycomponentinfo.h \
           ./include/spatial/geometryfactory.h \
           ./include/spatial/point.h \
           ./include/spatial/geometrycollection.h \
           ./include/spatial/linestring.h \
           ./include/spatial/edge.h \
           ./include/spatial/polygon.h \
           ./include/spatial/polyhedralsurface.h \
           ./include/hydrocoupleexceptions.h \
           ./include/spatial/geometryexchangeitems.h \
           ./include/spatial/geometryargument.h \
           ./include/spatial/tinexchangeitem.h \
           ./include/spatial/envelope.h \
           ./include/spatial/octree.h \
           ./include/spatial/tinargument.h \
           ./include/spatial/triangle.h \
           ./include/matrix.h \
           ./include/spatiotemporal/timegeometrycomponentdataitem.h \
           ./include/core/datacursor.h \
           ./include/spatial/geometrycomponentdataitem.h \
           ./include/core/componentdataitem.h \
           ./include/spatiotemporal/timegeometryargument.h \
           ./include/spatiotemporal/timegeometryinput.h \
           ./include/spatiotemporal/timegeometryoutput.h \
           ./include/spatiotemporal/timegeometrymultiinput.h \
           ./include/temporal/timeseriesargument.h \
           ./include/spatiotemporal/timetinexchangeitem.h \
           ./include/progresschecker.h \
           ./include/splineinterpolator.h \
           ./include/spatiotemporal/timegeometryinterpolationadaptedoutput.h \
           ./include/spatiotemporal/timetininterpolationadaptedoutput.h \
           ./include/spline.h \
           ./include/specialmap.h \
           ./include/temporal/abstracttimemodelcomponent.h \
           ./include/spatial/network.h \
           ./include/composition/composition.h \
           ./include/composition/modelcomponent.h \
           ./include/composition/exchangeitems.h \
           ./include/threadsafenetcdf/threadsafencfile.h \
           ./include/core/idbasedcomponentdataitem.h \
           ./include/core/idbasedoutputs.h \
           ./include/timeseries.h \
           ./include/threadsafenetcdf/threadsafencgroup.h \
           ./include/threadsafenetcdf/threadsafencvar.h \
           ./include/threadsafenetcdf/threadsafencatt.h \
           ./include/threadsafenetcdf/threadsafencdim.h

HEADERS += ./include/tests/geometrytest.h \
           ./include/tests/polyhedralsurfacetest.h

SOURCES += ./src/stdafx.cpp \
           ./src/core/description.cpp \
           ./src/core/identity.cpp \
           ./src/core/componentinfo.cpp \
           ./src/core/abstractmodelcomponentinfo.cpp \
           ./src/core/abstractmodelcomponent.cpp \
           ./src/core/componentstatuschangeeventargs.cpp \
           ./src/core/dimension.cpp \
           ./src/core/abstractcomponentdataitem.cpp \
           ./src/core/valuedefinition.cpp \
           ./src/core/unit.cpp \
           ./src/core/unitdimensions.cpp \
           ./src/core/abstractinput.cpp \
           ./src/core/abstractoutput.cpp \
           ./src/core/exchangeitems1d.cpp \
           ./src/core/exchangeitems2d.cpp \
           ./src/core/idbasedinputs.cpp \
           ./src/core/abstractargument.cpp \
           ./src/core/abstractexchangeitem.cpp \
           ./src/core/exchangeitemchangeeventargs.cpp \
           ./src/core/idbasedargument.cpp \
           ./src/temporal/timedata.cpp \
           ./src/temporal/timeseriescomponentdataitem.cpp \
           ./src/temporal/timeseriesidbasedcomponentdataitem.cpp \
           ./src/temporal/timeseriesidbasedexchangeitem.cpp \
           ./src/temporal/timeseriesexchangeitem.cpp \
           ./src/core/argument1d.cpp \
           ./src/spatial/spatialreferencesystem.cpp \
           ./src/core/abstractadaptedoutputfactory.cpp \
           ./src/core/abstractadaptedoutput.cpp \
           ./src/temporal/timeseriesinterpolationadaptedoutput.cpp \
           ./src/temporal/temporalinterpolationfactory.cpp \
           ./src/core/abstractadaptedoutputfactorycomponentinfo.cpp \
           ./src/spatial/geometry.cpp \
           ./src/spatial/geometryfactory.cpp \
           ./src/spatial/point.cpp \
           ./src/spatial/vertex.cpp \
           ./src/spatial/geometrycollection.cpp \
           ./src/spatial/multipoint.cpp \
           ./src/spatial/linestring.cpp \
           ./src/spatial/multilinestring.cpp \
           ./src/spatial/line.cpp \
           ./src/spatial/linearring.cpp \
           ./src/spatial/edge.cpp \
           ./src/spatial/polygon.cpp \
           ./src/spatial/polyhedralsurface.cpp \
           ./src/spatial/tin.cpp \
           ./src/spatial/hctriangle.cpp \
           ./src/spatial/multipolygon.cpp \
           ./src/hydrocoupleexceptions.cpp \
           ./src/spatial/geometryexchangeitems.cpp \
           ./src/core/componentdataitem1d.cpp \
           ./src/core/componentdataitem2d.cpp \
           ./src/spatial/geometryargument.cpp \
           ./src/core/idbasedcomponentdataitem.cpp \
           ./src/spatial/tinexchangeitems.cpp \
           ./src/spatial/envelope.cpp \
           ./src/spatial/octree.cpp \
           ./src/main.cpp \
           ./src/spatial/tinargument.cpp \
           ./src/spatial/triangle.c \
           ./src/spatial/tricall.c \
           ./src/spatiotemporal/timegeometrycomponentdataitem.cpp \
           ./src/spatiotemporal/timegeometryargument.cpp \
           ./src/spatial/geometrycomponentdataitem.cpp \
           ./src/spatiotemporal/timegeometryinput.cpp \
           ./src/spatiotemporal/timegeometrymultiinput.cpp \
           ./src/spatiotemporal/timegeometryoutput.cpp \
           ./src/temporal/timeseriesargument.cpp \
           ./src/core/datacursor.cpp \
           ./src/spatiotemporal/timetinoutput.cpp \
           ./src/spatiotemporal/timtininput.cpp \
           ./src/spatiotemporal/timetinmultiinput.cpp \
           ./src/progresschecker.cpp \
           ./src/splineinterpolator.cpp \
           ./src/spatiotemporal/timegeometryinterpolationadaptedoutput.cpp \
           ./src/spatiotemporal/timetininterpolationadaptedoutput.cpp \
           ./src/temporal/abstracttimemodelcomponent.cpp \
           ./src/spatial/network.cpp \
           ./src/composition/composition.cpp \
           ./src/composition/modelcomponent.cpp \
           ./src/composition/exchangeitems.cpp \
           ./src/composition/input.cpp \
           ./src/composition/output.cpp \
           ./src/threadsafenetcdf/threadsafencfile.cpp \
           ./src/core/idbasedoutputs.cpp \
           ./src/timeseries.cpp \
           ./src/threadsafenetcdf/threadsafencgroup.cpp \
           ./src/threadsafenetcdf/threadsafencvar.cpp \
           ./src/threadsafenetcdf/threadsafencdim.cpp \
           ./src/threadsafenetcdf/threadsafencatt.cpp

macx{

    INCLUDEPATH += /usr/local \
                   /usr/local/include \
                   /usr/local/include/gdal \
                   /usr/X11/include

    LIBS += -L/usr/local/lib -lgdal

    contains(DEFINES, USE_NETCDF){
        LIBS += -L/usr/local/lib -lnetcdf-cxx4
     }

    contains(DEFINES,USE_OPENMP){

        QMAKE_CC = /usr/local/opt/llvm/bin/clang
        QMAKE_CXX = /usr/local/opt/llvm/bin/clang++
        QMAKE_LINK = /usr/local/opt/llvm/bin/clang++

        QMAKE_CFLAGS+= -fopenmp
        QMAKE_LFLAGS+= -fopenmp
        QMAKE_CXXFLAGS+= -fopenmp

        INCLUDEPATH += /usr/local/opt/llvm/lib/clang/5.0.0/include
        LIBS += -L /usr/local/opt/llvm/lib -lomp

        message("OpenMP enabled")

        } else {

        message("OpenMP disabled")
    }

    contains(DEFINES,USE_MPI){

        QMAKE_CC = /usr/local/bin/mpicc
        QMAKE_CXX = /usr/local/bin/mpicxx
        QMAKE_LINK = /usr/local/bin/mpicxx

        LIBS += -L/usr/local/lib -lmpi

        message("MPI enabled")

        } else {

        message("MPI disabled")
    }
}



win32{

    contains(DEFINES,USE_OPENMP){

        QMAKE_CFLAGS += /openmp
        QMAKE_CXXFLAGS += /openmp
        message("OpenMP enabled")

        } else {

        message("OpenMP disabled")
    }

    #Windows vspkg package manager installation path if not set as an environment variable
    #VCPKGDIR = C:/vcpkg/installed/x64-windows

    INCLUDEPATH += $${VCPKGDIR}/include \
                   $${VCPKGDIR}/include/gdal

    message ($$(VCPKGDIR))

    CONFIG(debug, debug|release) {

    LIBS += -L$${VCPKGDIR}/debug/lib -lgdald

            contains(DEFINES, USE_NETCDF){
                LIBS += -L$${VCPKGDIR}/debug/lib -lnetcdf \
                        -L$${VCPKGDIR}/debug/lib -lnetcdf-cxx4
             }

            contains(DEFINES,USE_MPI){
               message("MPI enabled")
               LIBS += -L$${VCPKGDIR}/debug/lib -lmsmpi

              } else {

               message("MPI disabled")
            }

        } else {

    LIBS += -L$${VCPKGDIR}/lib -lgdal \
            -L$${VCPKGDIR}/lib -lnetcdf \
            -L$${VCPKGDIR}/lib -lnetcdf-cxx4

            contains(DEFINES,USE_MPI){
               message("MPI enabled")
               LIBS += -L$${VCPKGDIR}/lib -lmsmpi

               } else {
               message("MPI disabled")
            }
    }

    QMAKE_CXXFLAGS += /MP
    QMAKE_LFLAGS += /MP /incremental /debug:fastlink
}

linux{

    INCLUDEPATH += /usr/include \
                   ../gdal/include

    LIBS += -L/usr/lib/ogdi -lgdal \
            -L../gdal/lib -lgdal

    contains(DEFINES,USE_CHPC){

         contains(DEFINES, USE_NETCDF){

         INCLUDEPATH += /uufs/chpc.utah.edu/sys/installdir/hdf5/1.8.17-c7/include \
                        /uufs/chpc.utah.edu/sys/installdir/netcdf-c/4.3.3.1/include \
                        /uufs/chpc.utah.edu/sys/installdir/netcdf-cxx/4.3.0-c7/include


         LIBS += -L/uufs/chpc.utah.edu/sys/installdir/hdf5/1.8.17-c7/lib -l:libhdf5.so.10.2.0 \
                 -L/uufs/chpc.utah.edu/sys/installdir/netcdf-c/4.4.1/lib -l:libnetcdf.so.11.0.3 \
                 -L/uufs/chpc.utah.edu/sys/installdir/netcdf-cxx/4.3.0-c7/lib -l:libnetcdf_c++4.so.1.0.3
         }


         message("Compiling on CHPC")
    }

    contains(DEFINES,USE_OPENMP){

        QMAKE_CFLAGS += -fopenmp
        QMAKE_LFLAGS += -fopenmp
        QMAKE_CXXFLAGS += -fopenmp

        message("OpenMP enabled")

        } else {

        message("OpenMP disabled")
    }

    contains(DEFINES,USE_MPI){

        QMAKE_CC = mpicc
        QMAKE_CXX = mpic++
        QMAKE_LINK = mpic++

        LIBS += -L/usr/local/lib/ -lmpi

        message("MPI enabled")

        } else {

        message("MPI disabled")

    }
}

CONFIG(debug, debug|release) {

    win32 {
<<<<<<< HEAD
       QMAKE_CXXFLAGS += /MDd /O2
    }

    macx {
       QMAKE_CXXFLAGS += -O3
    }

    linux {
       QMAKE_CXXFLAGS += -O3
=======
       QMAKE_CXXFLAGS += /MDd  /O2
    }

    macx {
        QMAKE_CFLAGS += -g -O3
        QMAKE_CXXFLAGS  += -g -O3
    }

    linux {
        QMAKE_CFLAGS += -g -O3
        QMAKE_CXXFLAGS  += -g -O3
>>>>>>> 0b90eeb6a1915b99bd65ab0309ac91bf1bfb2994
    }

   DESTDIR = ./build/debug
   OBJECTS_DIR = $$DESTDIR/.obj
   MOC_DIR = $$DESTDIR/.moc
   RCC_DIR = $$DESTDIR/.qrc
   UI_DIR = $$DESTDIR/.ui
}

CONFIG(release, debug|release) {

<<<<<<< HEAD
       win32 {
         QMAKE_CXXFLAGS += /MD
       }
=======
   win32 {
    QMAKE_CXXFLAGS += /MD
   }

>>>>>>> 0b90eeb6a1915b99bd65ab0309ac91bf1bfb2994

     contains(DEFINES,HYDROCOUPLESDK_LIBRARY){

         #MacOS
         macx{
             DESTDIR = lib/macx
        }

         #Linux
         linux{
             DESTDIR = lib/linux
        }

         #Windows
         win32{
             DESTDIR = lib/win32
        }

      } else {

         #MacOS
         macx{
             DESTDIR = bin/macx
        }

         #Linux
         linux{
             DESTDIR = bin/linux
        }

         #Windows
         win32{
             DESTDIR = bin/win32
        }
    }

    RELEASE_EXTRAS = ./build/release 
    OBJECTS_DIR = $$RELEASE_EXTRAS/.obj
    MOC_DIR = $$RELEASE_EXTRAS/.moc
    RCC_DIR = $$RELEASE_EXTRAS/.qrc
    UI_DIR = $$RELEASE_EXTRAS/.ui
}   

RESOURCES += hydrocouplesdk.qrc

