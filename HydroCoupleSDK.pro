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

HEADERS += ./include/hydrocouplesdk.h \
           ./include/identity.h \
           ./include/description.h \
           ./include/componentinfo.h \
           ./include/modelcomponentinfo.h \
           ./include/abstractmodelcomponent.h \
           ./include/componentstatuschangeeventargs.h \
           ./include/dimension.h \
           ./include/abstractcomponentdataitem.h \
           ./include/valuedefinition.h \
           ./include/unit.h \
           ./include/unitdimensions.h \
           ./include/abstractoutput.h \
           ./include/abstractinput.h \
           ./include/abstractargument.h \
           ./include/componentdataitem1d.h \
           ./include/componentdataitem2d.h \
           ./include/componentdataitem3d.h \
           ./include/idbasedcomponentdataitem.h \
           ./include/timedata.h \
           ./include/timeseriescomponentdataitem.h \
           ./include/exchangeitemchangeeventargs.h \
           ./include/abstractexchangeitem.h \
           ./include/idbasedtimecomponentdataitem.h \
           ./include/idbasedtimeexchangeitem.h \
           ./include/timeseriesexchangeitem.h


SOURCES += ./src/stdafx.cpp \
           ./src/description.cpp \
           ./src/identity.cpp \
           ./src/componentinfo.cpp \
           ./src/modelcomponentinfo.cpp \
           ./src/abstractmodelcomponent.cpp \
           ./src/componentstatuschangeeventargs.cpp \
           ./src/dimension.cpp \
           ./src/abstractcomponentdataitem.cpp \
           ./src/valuedefinition.cpp \
           ./src/unit.cpp \
           ./src/unitdimensions.cpp \
           ./src/abstractinput.cpp \
           ./src/abstractoutput.cpp \
           ./src/componentdataitem1d.cpp \
           ./src/componentdataitem2d.cpp \
           ./src/componentdataitem3d.cpp \
           ./src/idbasedcomponentdataitem.cpp \
           ./src/timedata.cpp \
           ./src/abstractargument.cpp \
           ./src/timeseriescomponentdataitem.cpp \
           ./src/abstractexchangeitem.cpp \
           ./src/exchangeitemchangeeventargs.cpp \
           ./src/idbasedtimecomponentdataitem.cpp \
           ./src/idbasedtimeexchangeitem.cpp \
           ./src/timeseriesexchangeitem.cpp


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
