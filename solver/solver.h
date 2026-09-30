#ifndef SOLVER_H
#define SOLVER_H

#include <QVector>
#include <QMap>
#include <QString>

#include "../model/fivenodeparameters.h"
#include "../model/fivenodemodel.h"

struct SolverConnection
{
    int firstNode;
    int secondNode;
    QString function;
};

class Solver
{
public:
    Solver(FiveNodeParameters& parameters, FiveNodeModel& model);

    QVector<QVector<double>> runODE(
        const QVector<SolverConnection>& connections,
        const QMap<QString, double>& weightValues);

    QVector<QVector<double>> runGamma(
        int numNodes,
        const QVector<SolverConnection>& connections,
        const QMap<QString, double>& weightValues);

private:
    double gammaWeight(int om, int r, double nu) const;

    FiveNodeParameters& params;
    FiveNodeModel& model;
};

#endif // SOLVER_H
