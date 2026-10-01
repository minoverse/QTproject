QT += core gui widgets

CONFIG += c++11

TEMPLATE = app
TARGET = ButtonNetwork

SOURCES += \
    main.cpp \
    buttonnetwork.cpp \
    model/fivenodeparameters.cpp \
    model/fivenodemodel.cpp \
    solver/solver.cpp \
    output/resultwriter.cpp \
    validation/validationrunner.cpp \
    plot/plotmanager.cpp \
    analysis/alpha2scanner.cpp

HEADERS += \
    buttonnetwork.h \
    model/fivenodeparameters.h \
    model/fivenodemodel.h \
    solver/solver.h \
    output/resultwriter.h \
    validation/validationrunner.h \
    plot/plotmanager.h \
    analysis/alpha2scanner.h

LIBS += -lgsl -lgslcblas -lm
