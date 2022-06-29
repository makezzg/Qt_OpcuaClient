#-------------------------------------------------
#
# Project created by QtCreator 2019-06-11T11:49:35
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OPCUA_client
TEMPLATE = app
QMAKE_CFLAGS += -std=c99
LIBS += -lpthread libwsock32 libws2_32

SOURCES += main.cpp\
        mainwindow.cpp \
    open62541.c

HEADERS  += mainwindow.h \
    open62541.h

FORMS    += mainwindow.ui
