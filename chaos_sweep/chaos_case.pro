QT += core
QT -= gui

CONFIG += console c++17 release
CONFIG -= debug app_bundle

TEMPLATE = app
TARGET = chaos_case

INCLUDEPATH += ..

SOURCES += \
    chaos_case.cpp \
    ../model/fivenodeparameters.cpp \
    ../model/fivenodemodel.cpp \
    ../solver/solver.cpp \
    ../validation/validationrunner.cpp

HEADERS += \
    ../model/fivenodeparameters.h \
    ../model/fivenodemodel.h \
    ../solver/solver.h \
    ../validation/validationrunner.h

# Faster release build without -ffast-math, so floating-point semantics are not relaxed.
QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -flto -DNDEBUG
QMAKE_LFLAGS_RELEASE += -flto

LIBS += -lgsl -lgslcblas -lm
