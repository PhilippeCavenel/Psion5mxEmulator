#-------------------------------------------------
#
# Project created by QtCreator 2019-12-18T16:18:09
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = Psion5mxEmulatorCore
TEMPLATE = lib
CONFIG += staticlib c++17
android : DEFINES += "SHOW_IN_FULL_SCREEN"

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    arm710.cpp \
    emubase.cpp \
    etna.cpp \
    decoder.c \
    decoder-arm.c \
    windermere.cpp

HEADERS += \
    arm710.h \
    common.h \
    decoder-inlines.h \
    decoder.h \
    emitter-inlines.h \
    emubase.h \
    etna.h \
    hardware.h \
    macros.h \
    wind_defs.h \
    isa-inlines.h \
    emitter-arm.h \
    windermere.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
