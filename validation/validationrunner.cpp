#include "validationrunner.h"

void ValidationRunner::configureFiveNodePreset(FiveNodeParameters& params,
                                               double alpha2)
{
    params.alpha1 = -2.2;
    params.alpha2 = alpha2;
    params.alpha3 = 1.2;
    params.nu = 0.70;
    params.tMax = 1000;
    params.solverMode = "GAMMA";
}

QVector<ValidationConnection> ValidationRunner::fiveNodeConnections()
{
    return {
        {1, 4, -0.6, "sin_exp"},
        {4, 1,  0.7, "tanh"},
        {1, 3, -0.8, "sin_exp"},
        {3, 1,  1.7, "tanh"},
        {2, 3,  2.0, "sin_exp"},
        {3, 2, -0.4, "tanh"},
        {1, 2, -0.3, "tanh"},
        {2, 1, -3.0, "sin_exp"},
        {2, 5,  0.4, "sin_exp"},
        {5, 2,  1.7, "tanh"},
        {3, 3,  3.0, "sin_exp"}
    };
}
