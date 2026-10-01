QT += core gui qml quick quickcontrols2 dbus

CONFIG += c++17 release
TARGET = omaconvert
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/omarchytheme.h \
    src/systemtheme.h \
    src/unitcatalog.h \
    src/i18n.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/omarchytheme.cpp \
    src/systemtheme.cpp

RESOURCES += src/resources.qrc
