QT += core
QT += gui
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QTCPServer
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui
