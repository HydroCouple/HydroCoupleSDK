#-------------------------------------------------
#
# Project created by QtCreator 2016-08-01T13:11:34
#
#-------------------------------------------------

QT       += testlib gui

TARGET = hydrocouplesdktest

CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

PRECOMPILED_HEADER += ./include/stdafx.h

INCLUDEPATH += ./include \
               ./include/tests \
               ../HydroCouple/include

HEADERS += ./include/stdafx.h \
           ./include/tests/componentdataitemtest.h \
    include/tests/geometrytest.h \
    include/tests/polyhedralsurfacetest.h

SOURCES += \
           ./src/tests/main.cpp

macx {
INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib/gdal -lgdal.20
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

   LIBS += -L./build/debug -lHydroCoupleSDK
}

CONFIG(release, debug|release) {
    DESTDIR = lib
    RELEASE_EXTRAS = ./build/release
    OBJECTS_DIR = $$RELEASE_EXTRAS/.obj
    MOC_DIR = $$RELEASE_EXTRAS/.moc
    RCC_DIR = $$RELEASE_EXTRAS/.qrc
    UI_DIR = $$RELEASE_EXTRAS/.ui

   LIBS += -L./lib -lHydroCoupleSDK
}
