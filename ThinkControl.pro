#-------------------------------------------------
#
# Project created by QtCreator 2012-06-20T17:11:52
#
#-------------------------------------------------

QT       += core gui

TARGET = ThinkControl
TEMPLATE = app


SOURCES += src/settings.cpp \
    src/mainwindow.cpp \
    src/main.cpp \
    src/governors.cpp \
    src/dialogs.cpp \
    src/devices.cpp

HEADERS  += h/settings.h \
    h/mainwindow.h \
    h/governors.h \
    h/dialogs.h \
    h/devices.h

FORMS    += ui/touchpad.ui \
    ui/mainwindow.ui \
    ui/fanpreset.ui \
    ui/profileline.ui \
    ui/settings.ui \
    ui/trackpoint.ui

RESOURCES += \
    icons.qrc































