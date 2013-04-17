#-------------------------------------------------
#
# Project created by QtCreator 2012-07-23T16:25:37
#
#-------------------------------------------------

QT       += core gui

TARGET = PedometerLog
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    PedometerLog.cpp \
    db.cpp \
    qcustomplot.cpp \
    MyDaemon.cpp \
    recalculatedialog.cpp \
    configdialog.cpp \
    notedialog.cpp

HEADERS  += mainwindow.h \
    qcustomplot.h \
    MyDaemon.h \
    recalculatedialog.h \
    configdialog.h \
    notedialog.h

FORMS    += mainwindow.ui \
    recalculatedialog.ui \
    configdialog.ui \
    notedialog.ui

LIBS += -L/usr/lib64/qt4/plugins
LIBS += -lusb-1.0

INCLUDEPATH += /usr/include/libusb-1.0

QT += sql

RESOURCES = PedometerLog.qrc









