#ifndef ALPHA2SCANNER_H
#define ALPHA2SCANNER_H

#include <QString>
#include <QVector>
#include <QMap>

#include "model/fivenodeparameters.h"
#include "solver/solver.h"

struct Alpha2ScanConfig
{
    double min = -10.0;
    double max = 10.0;
    double step = 0.5;

    int transientPercent = 70;
    int sampleStride = 20;
};

struct Alpha2ScanResult
{
    bool success = false;
    QString errorMessage;
};

class Alpha2Scanner
{
public:
    static Alpha2ScanResult run(
        const QString& runDir,
        Solver& solver,
        FiveNodeParameters& params,
        int numNodes,
        const QVector<SolverConnection>& connections,
        const QMap<QString, double>& weightValues,
        const Alpha2ScanConfig& config);
};

#endif // ALPHA2SCANNER_H
