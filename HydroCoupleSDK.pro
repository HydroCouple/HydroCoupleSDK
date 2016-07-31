#Author Caleb Amoa Buahin
#Email caleb.buahin@gmail.com
#Date 2016
#License GNU General Public License (see <http://www.gnu.org/licenses/> for details).

TEMPLATE = lib
VERSION = 1.0.0.0
TARGET = HydroCoupleSDK
QT += core

DEFINES += HYDROCOUPLESDK_LIBRARY

INCLUDEPATH += ./include \
               ../HydroCouple/include

PRECOMPILED_HEADER += ./include/stdafx.h

HEADERS += ./include/stdafx.h \
           ./include/hydrocouplesdk.h \
           ./include/core/identity.h \
           ./include/core/description.h \
           ./include/core/componentinfo.h \
           ./include/core/modelcomponentinfo.h \
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
           ./include/core/componentdataitem1d.h \
           ./include/core/componentdataitem2d.h \
           ./include/core/componentdataitem3d.h \
           ./include/core/idbasedcomponentdataitem.h \
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
           ./include/spatial/polyhedralsurface.h

SOURCES += ./src/stdafx.cpp \
           ./src/core/description.cpp \
           ./src/core/identity.cpp \
           ./src/core/componentinfo.cpp \
           ./src/core/modelcomponentinfo.cpp \
           ./src/core/abstractmodelcomponent.cpp \
           ./src/core/componentstatuschangeeventargs.cpp \
           ./src/core/dimension.cpp \
           ./src/core/abstractcomponentdataitem.cpp \
           ./src/core/valuedefinition.cpp \
           ./src/core/unit.cpp \
           ./src/core/unitdimensions.cpp \
           ./src/core/abstractinput.cpp \
           ./src/core/abstractoutput.cpp \
           ./src/core/componentdataitem1d.cpp \
           ./src/core/componentdataitem2d.cpp \
           ./src/core/componentdataitem3d.cpp \
           ./src/core/idbasedcomponentdataitem.cpp \
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
           ./src/spatial/polyhedralsurface.cpp

macx {

INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib/ -lgdal

}

win32 {

}

linux{

INCLUDEPATH += /usr/include/gdal
LIBS += -L/usr/lib/ogdi -lgdal

}

CONFIG(debug, debug|release) {
   DESTDIR = ./build/debug
   OBJECTS_DIR = $$DESTDIR/.obj
   MOC_DIR = $$DESTDIR/.moc
   RCC_DIR = $$DESTDIR/.qrc
   UI_DIR = $$DESTDIR/.ui
}

CONFIG(release, debug|release) {
    DESTDIR = lib
    RELEASE_EXTRAS = ./build/release 
    OBJECTS_DIR = $$RELEASE_EXTRAS/.obj
    MOC_DIR = $$RELEASE_EXTRAS/.moc
    RCC_DIR = $$RELEASE_EXTRAS/.qrc
    UI_DIR = $$RELEASE_EXTRAS/.ui
}   
