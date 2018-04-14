#-------------------------------------------------
#
# Project created by QtCreator 2018-04-12T18:20:20
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport

TARGET = app-plcfpx
TEMPLATE = app

RC_FILE += qrc/appsource.rc

HEADERS += \
    app/appplcfpx.h

SOURCES += \
    app/appplcfpx.cpp \
    app/main.cpp

RESOURCES += \
    qrc/appsource.qrc

