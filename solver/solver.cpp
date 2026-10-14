#include "solver.h"

#include <QCoreApplication>
#include <gsl/gsl_sf_gamma.h>

Solver::Solver(FiveNodeParameters& parameters, FiveNodeModel& modelRef)
    : params(parameters),
      model(modelRef)
{
}

double Solver::gammaWeight(int om, int r, double nu) const
{
    int k = om - r;

    if (k == 0) {
        return gsl_sf_gamma(om - r + nu)
             / gsl_sf_gamma(om - r + 1)
             / gsl_sf_gamma(nu);
    }

    return 1.0 / k / gsl_sf_beta(k, nu);
}

QVector<QVector<double>> Solver::runODE(
    const QVector<SolverConnection>& connections,
    const QMap<QString, double>& weightValues)
{
    const int steps = params.tMax;
    const double h = 0.01;

    QVector<QVector<double>> y(5, QVector<double>(steps + 1));

    y[0][0] = 0.8;
    y[1][0] = 0.3;
    y[2][0] = 0.4;
    y[3][0] = 0.6;
    y[4][0] = 0.7;

    for (int t = 1; t <= steps; ++t) {
        if (t % 400 == 0)
            QCoreApplication::processEvents();

        for (int i = 0; i < 5; ++i) {
            double sum = -y[i][t - 1];

            for (const auto& conn : connections) {
                int from = conn.firstNode - 1;
                int to   = conn.secondNode - 1;

                if (to != i)
                    continue;

                QString key =
                    "s" + QString::number(conn.firstNode)
                        + QString::number(conn.secondNode);

                double w = weightValues.value(key, 0.0);
                double in = y[from][t - 1];

                if (conn.function == "sin_exp")
                    sum += w * model.sinEFunction(in);
                else if (conn.function == "tanh")
                    sum += w * model.tanhFunction(in);
                else if (conn.function == "relu")
                    sum += w * model.reluFunction(in);
            }

            if (i == 3) {
                if (params.gateNode4.enabled) {
                    const double G2 =
                        model.evalGateForNode(3, y[3][t - 1]);

                    sum += G2 * model.tanhFunction(y[3][t - 1]);
                } else {
                    sum +=
                        (params.alpha2
                         - params.alpha3 *
                           model.sinEFunction(y[4][t - 1]))
                        * model.tanhFunction(y[3][t - 1]);
                }
            }

            if (i == 4) {
                if (params.gateNode5.enabled) {
                    const double G1 =
                        model.evalGateForNode(4, y[4][t - 1]);

                    sum += G1 * model.tanhFunction(y[4][t - 1]);
                } else {
                    sum +=
                        (1 - params.alpha1 *
                         model.tanhFunction(y[2][t - 1]))
                        * model.tanhFunction(y[4][t - 1]);
                }
            }

            y[i][t] = y[i][t - 1] + h * sum;
        }
    }

    return y;
}

QVector<QVector<double>> Solver::runGamma(
    int numNodes,
    const QVector<SolverConnection>& connections,
    const QMap<QString, double>& weightValues)
{
    const int steps = params.tMax;

    QVector<QVector<double>> y(
        numNodes, QVector<double>(steps + 1));

    y[0][0] = 0.8;
    y[1][0] = 0.3;
    y[2][0] = 0.4;
    y[3][0] = 0.6;
    y[4][0] = 0.7;

    for (int om = 1; om <= steps; ++om) {
        if (om % 100 == 0)
            QCoreApplication::processEvents();

        for (int i = 0; i < numNodes; ++i)
            y[i][om] = 0.0;

        for (int r = 1; r <= om; ++r) {
            double bg = gammaWeight(om, r, params.nu);

            for (int i = 0; i < numNodes; ++i) {
                double sum = -y[i][r - 1];

                for (const auto& conn : connections) {
                    int target = conn.firstNode - 1;
                    int source = conn.secondNode - 1;

                    if (target != i)
                        continue;

                    QString key =
                        "s" + QString::number(conn.firstNode)
                            + QString::number(conn.secondNode);

                    double w = weightValues.value(key, 0.0);
                    double in = y[source][r - 1];

                    if (conn.function == "sin_exp")
                        sum += w * model.sinEFunction(in);
                    else if (conn.function == "tanh")
                        sum += w * model.tanhFunction(in);
                    else if (conn.function == "relu")
                        sum += w * model.reluFunction(in);
                }

                if (i == 3 && numNodes > 4) {
                    double gateTerm =
                        (params.alpha2
                         - params.alpha3 *
                           model.sinEFunction(y[4][r - 1]))
                        * model.tanhFunction(y[3][r - 1]);

                    sum += gateTerm;
                }

                if (i == 4 && numNodes > 4) {
                    double gateTerm =
                        (1.0
                         - params.alpha1 *
                           model.tanhFunction(y[2][r - 1]))
                        * model.tanhFunction(y[4][r - 1]);

                    sum += gateTerm;
                }

                y[i][om] += sum * bg;
            }
        }

        for (int i = 0; i < numNodes; ++i)
            y[i][om] += y[i][0];
    }

    return y;
}
