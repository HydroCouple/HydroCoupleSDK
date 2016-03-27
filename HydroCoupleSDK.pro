#Author Caleb Amoa Buahin
#Email caleb.buahin@gmail.com
#Date 2016
#License GNU General Public License (see <http://www.gnu.org/licenses/> for details).

TEMPLATE = lib
VERSION = 1.0.0.0
TARGET = HydroCoupleSDK

QT += core

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


SOURCES += ./src/stdafx.cpp \
           ./src/description.cpp \
           ./src/identity.cpp \
           ./src/componentinfo.cpp \
           ./src/modelcomponentinfo.cpp \
           ./src/abstractmodelcomponent.cpp \
           ./src/componentstatuschangeeventargs.cpp \


