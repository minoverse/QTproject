#include "fivenodemodel.h"

#include <cmath>

FiveNodeModel::FiveNodeModel(FiveNodeParameters& parameters)
    : params(parameters)
{
}

double FiveNodeModel::baseValueFromType(const QString& baseType,
                                        double baseConst) const
{
    if (baseType == "alpha1") return params.alpha1;
    if (baseType == "alpha2") return params.alpha2;
    if (baseType == "alpha3") return params.alpha3;
    if (baseType == "const")  return baseConst;

    return baseConst;
}

double FiveNodeModel::sinEFunction(double x)
{
    return std::sin(x);
}

double FiveNodeModel::tanhFunction(double x)
{
    return std::tanh(x);
}

double FiveNodeModel::reluFunction(double x)
{
    return (x > 0.0) ? x : 0.0;
}

double FiveNodeModel::applyFn(const QString& fn, double x) const
{
    if (fn == "sin")  return std::sin(x);
    if (fn == "tanh") return std::tanh(x);
    if (fn == "relu") return (x > 0.0) ? x : 0.0;

    return std::sin(x);
}

double FiveNodeModel::evalGateForNode(int nodeIndex, double yValue) const
{
    const GateConfig* gate = nullptr;

    if (nodeIndex == 3) gate = &params.gateNode4;
    if (nodeIndex == 4) gate = &params.gateNode5;
    if (!gate) return 0.0;

    const double base =
        baseValueFromType(gate->baseType, gate->baseConst);

    const double fnv = applyFn(gate->fn, yValue);

    return base - gate->coeff * fnv;
}
