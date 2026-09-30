QT += core gui widgets

CONFIG += c++11

TEMPLATE = app
TARGET = ButtonNetwork

SOURCES += \
    main.cpp \
    buttonnetwork.cpp \
    model/fivenodeparameters.cpp

HEADERS += \
    buttonnetwork.h \
    model/fivenodeparameters.h

LIBS += -lgsl -lgslcblas -lm
