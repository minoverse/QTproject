QT += core
QT -= gui

CONFIG += console c++17
CONFIG -= app_bundle

TARGET = vary_test
TEMPLATE = app

INCLUDEPATH += ..

SOURCES += \
    main.cpp \
    ../model/fivenodeparameters.cpp \
    ../model/fivenodemodel.cpp \
    ../solver/solver.cpp \
    ../validation/validationrunner.cpp

HEADERS += \
    ../model/fivenodeparameters.h \
    ../model/fivenodemodel.h \
    ../solver/solver.h \
    ../validation/validationrunner.h

LIBS += -lgsl -lgslcblas -lm
