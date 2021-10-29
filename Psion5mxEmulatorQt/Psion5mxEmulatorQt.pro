#-------------------------------------------------
#
# Project created by QtCreator 2019-12-18T16:17:36
#
#-------------------------------------------------

QT       += core gui widgets serialport

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

FORMS += \
        mainwindow.ui

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/pkg_src


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Psion5mxEmulatorCore/release/ -lPsion5mxEmulatorCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Psion5mxEmulatorCore/debug/ -lPsion5mxEmulatorCore
else:unix: LIBS += -L$$OUT_PWD/../WindCore/ -lPsion5mxEmulatorCore

INCLUDEPATH += $$PWD/../WindCore
DEPENDPATH += $$PWD/../WindCore

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Psion5mxEmulatorCore/release/libPsion5mxEmulatorCore.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Psion5mxEmulatorCore/debug/libPsion5mxEmulatorCore.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Psion5mxEmulatorCore/release/Psion5mxEmulatorCore.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Psion5mxEmulatorCore/debug/Psion5mxEmulatorCore.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../WPsion5mxEmulatorCore/libPsion5mxEmulatorCore.a

DISTFILES += \
    pkg_src/AndroidManifest.xml \
    pkg_src/build.gradle \
    pkg_src/gradle.properties \
    pkg_src/gradle/wrapper/gradle-wrapper.jar \
    pkg_src/gradle/wrapper/gradle-wrapper.properties \
    pkg_src/gradlew \
    pkg_src/gradlew.bat \
    pkg_src/res/values/libs.xml
