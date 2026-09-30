QT += core gui widgets

CONFIG += c++11

TEMPLATE = app
TARGET = ButtonNetwork

SOURCES += \
    main.cpp \
    buttonnetwork.cpp \
    model/fivenodeparameters.cpp \
    model/fivenodemodel.cpp \
    solver/solver.cpp

HEADERS += \
    buttonnetwork.h \
    model/fivenodeparameters.h \
    model/fivenodemodel.h \
    solver/solver.h

LIBS += -lgsl -lgslcblas -lm
