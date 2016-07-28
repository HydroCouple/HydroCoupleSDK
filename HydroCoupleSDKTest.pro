#Author Caleb Amoa Buahin
#Email caleb.buahin@gmail.com
#Date 2016
#License GNU General Public License (see <http://www.gnu.org/licenses/> for details).

TEMPLATE = app
VERSION = 1.0.0
TARGET = HydroCoupleSDKTest

CONFIG-=app_bundle

QT += core testlib

INCLUDEPATH += ./include \
               ./include/tests \
               ../HydroCouple/include \
               /usr/local/include

PRECOMPILED_HEADER += ./include/stdafx.h 


CONFIG(debug, debug|release) {
   DESTDIR = ./build/debug
   OBJECTS_DIR = $$DESTDIR/.obj
   MOC_DIR = $$DESTDIR/.moc
   RCC_DIR = $$DESTDIR/.qrc
   UI_DIR = $$DESTDIR/.ui

   LIBS += -L./build/debug -lHydroCoupleSDK.1.0.0

}

CONFIG(release, debug|release) {
    DESTDIR = bin
    RELEASE_EXTRAS = ./build/release 
    OBJECTS_DIR = $$RELEASE_EXTRAS/.obj
    MOC_DIR = $$RELEASE_EXTRAS/.moc
    RCC_DIR = $$RELEASE_EXTRAS/.qrc
    UI_DIR = $$RELEASE_EXTRAS/.ui
    LIBS += -L./build/debug -lHydroCoupleSDK.1.0.0
}

HEADERS +=\
         ./include/tests/valuesettest.h


SOURCES += \
        src/tests/main.cpp
