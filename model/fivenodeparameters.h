#ifndef FIVENODEPARAMETERS_H
#define FIVENODEPARAMETERS_H

#include <QString>

struct GateConfig
{
    bool enabled = false;
    QString baseType = "const";
    double baseConst = 1.0;
    double coeff = 1.0;
    QString fn = "tanh";
};

struct FiveNodeParameters
{
    // Simulation parameters
    double alpha1 = 1.0;
    double alpha2 = 1.0;
    double alpha3 = 1.0;
    double nu = 0.9;

    int tMax = 800;
    QString solverMode = "ODE";

    // Gate configuration
    GateConfig gateNode4;
    GateConfig gateNode5;
};

#endif // FIVENODEPARAMETERS_H
