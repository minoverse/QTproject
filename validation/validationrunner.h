#ifndef VALIDATIONRUNNER_H
#define VALIDATIONRUNNER_H

#include <QString>
#include <QVector>

#include "model/fivenodeparameters.h"
#include "solver/solver.h"

struct ValidationConnection
{
    int from;
    int to;
    double weight;
    QString function;
};

struct ValidationResult
{
    bool passed = false;
    int rowsCompared = 0;
    int maxErrorStep = -1;
    int maxErrorNode = -1;
    double maxAbsoluteError = 0.0;
    QString message;
};

class ValidationRunner
{
public:
    static void configureFiveNodePreset(FiveNodeParameters& params,
                                        double alpha2);

    static QVector<ValidationConnection> fiveNodeConnections();

    static QVector<QVector<double>> runFiveNodeReferencePreset(
        Solver& solver,
        FiveNodeParameters& params,
        double alpha2);

    static ValidationResult compareWithReference(
        const QVector<QVector<double>>& actual,
        const QString& referenceFile,
        double tolerance = 1e-12);
};

#endif // VALIDATIONRUNNER_H
