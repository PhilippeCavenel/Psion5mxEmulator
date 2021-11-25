#-------------------------------------------------
#
# Project created by QtCreator 2019-12-18T16:17:36
#
#-------------------------------------------------

QT       += core gui widgets serialport multimedia

TARGET = Psion5mxEmulatorQt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        pdascreenwindow.cpp

HEADERS += \
        mainwindow.h \
        pdascreenwindow.h

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/pkg_src


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -L$$OUT_PWD/../Psion5mxEmulatorCore/ -lPsion5mxEmulatorCore
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

win32-g++:PRE_TARGETDEPS += $$OUT_PWD/../Psion5mxEmulatorCore/libPsion5mxEmulatorCore.a
else:win32:!win32-g++: PRE_TARGETDEPS += $$OUT_PWD/../Psion5mxEmulatorCore/Psion5mxEmulatorCore.lib
else:unix:  PRE_TARGETDEPS += $$OUT_PWD/../Psion5mxEmulatorCore/libPsion5mxEmulatorCore.a

DISTFILES += \
    pkg_src/AndroidManifest.xml \
    pkg_src/build.gradle \
    pkg_src/gradle.properties \
    pkg_src/gradle/wrapper/gradle-wrapper.jar \
    pkg_src/gradle/wrapper/gradle-wrapper.properties \
    pkg_src/gradlew \
    pkg_src/gradlew.bat \
    pkg_src/res/values/libs.xml
