QT += core gui widgets charts

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
    plot/interactiveplotwindow.cpp \
    plot/phaseplotwindow.cpp \
    plot/fftplotwindow.cpp \
    plot/fftanimationwindow.cpp \
    analysis/alpha2scanner.cpp \
    analysis/fftanalyzer.cpp

HEADERS += \
    buttonnetwork.h \
    model/fivenodeparameters.h \
    model/fivenodemodel.h \
    solver/solver.h \
    output/resultwriter.h \
    validation/validationrunner.h \
    plot/plotmanager.h \
    plot/interactiveplotwindow.h \
    plot/phaseplotwindow.h \
    plot/fftplotwindow.h \
    plot/fftanimationwindow.h \
    analysis/alpha2scanner.h \
    analysis/fftanalyzer.h

LIBS += -lgsl -lgslcblas -lfftw3 -lm
