QT += core
CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app
TARGET = validate_solver

SOURCES += \
    main_validation.cpp \
    validationrunner.cpp \
    ../solver/solver.cpp \
    ../model/fivenodemodel.cpp \
    ../model/fivenodeparameters.cpp

HEADERS += \
    validationrunner.h \
    ../solver/solver.h \
    ../model/fivenodemodel.h \
    ../model/fivenodeparameters.h

INCLUDEPATH += ..

LIBS += -lgsl -lgslcblas -lm
