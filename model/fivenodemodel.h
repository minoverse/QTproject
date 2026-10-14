#ifndef FIVENODEMODEL_H
#define FIVENODEMODEL_H

#include "fivenodeparameters.h"

class FiveNodeModel
{
public:
    explicit FiveNodeModel(FiveNodeParameters& parameters);

    double baseValueFromType(const QString& baseType, double baseConst) const;
    double applyFn(const QString& fn, double x) const;
    double evalGateForNode(int nodeIndex, double yValue) const;

    static double sinEFunction(double x);
    static double tanhFunction(double x);
    static double reluFunction(double x);

private:
    FiveNodeParameters& params;
};

#endif // FIVENODEMODEL_H
