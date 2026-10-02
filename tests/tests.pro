QT += core gui testlib

CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omaunits

INCLUDEPATH += ../src

SOURCES += \
    tst_omaunits.cpp \
    ../src/backend.cpp \
    ../src/omarchytheme.cpp

HEADERS += \
    ../src/backend.h \
    ../src/omarchytheme.h \
    ../src/unitcatalog.h \
    ../src/i18n.h

RESOURCES += ../src/resources.qrc
