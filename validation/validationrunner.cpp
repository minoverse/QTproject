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
    // Same target/source and accumulation order as professor's C reference.
    return {
        // y0
        {1, 2, -0.3, "tanh"},
        {1, 3, -0.8, "sin_exp"},
        {1, 4, -0.6, "sin_exp"},

        // y1
        {2, 1, -3.0, "sin_exp"},
        {2, 3,  2.0, "sin_exp"},
        {2, 5,  0.4, "sin_exp"},

        // y2
        {3, 1,  1.7, "tanh"},
        {3, 2, -0.4, "tanh"},
        {3, 3,  3.0, "sin_exp"},

        // y3
        {4, 1,  0.7, "tanh"},

        // y4
        {5, 2,  1.7, "tanh"}
    };
}

#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QStringList>
#include <cmath>

ValidationResult ValidationRunner::compareWithReference(
    const QVector<QVector<double>>& actual,
    const QString& referenceFile,
    double tolerance)
{
    ValidationResult result;

    QFile file(referenceFile);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.message = "Cannot open reference file: " + referenceFile;
        return result;
    }

    QTextStream in(&file);

    int step = 0;

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();

        if (line.isEmpty())
            continue;

        const QStringList values =
            line.split(' ', Qt::SkipEmptyParts);

        if (values.size() != 5) {
            result.message =
                QString("Invalid reference data at step %1").arg(step);
            return result;
        }

        if (actual.size() < 5 ||
            step >= actual[0].size()) {
            result.message =
                QString("Actual result is shorter than reference at step %1")
                    .arg(step);
            return result;
        }

        for (int node = 0; node < 5; ++node) {

            bool ok = false;
            const double expected = values[node].toDouble(&ok);

            if (!ok) {
                result.message =
                    QString("Invalid number at step %1 node %2")
                        .arg(step)
                        .arg(node + 1);
                return result;
            }

            const double error =
                std::abs(actual[node][step] - expected);

            if (error > result.maxAbsoluteError) {
                result.maxAbsoluteError = error;
                result.maxErrorStep = step;
                result.maxErrorNode = node + 1;
            }
        }

        ++step;
    }

    result.rowsCompared = step;

    if (step != 1001) {
        result.message =
            QString("Expected 1001 rows, but compared %1").arg(step);
        return result;
    }

    result.passed = result.maxAbsoluteError <= tolerance;

    if (result.maxAbsoluteError == 0.0) {
        result.message =
            QString("Exact match: %1 rows x 5 nodes")
                .arg(result.rowsCompared);
    } else {
        result.message =
            QString("Compared %1 rows x 5 nodes. Max absolute error = %2 "
                    "at step %3, node %4")
                .arg(result.rowsCompared)
                .arg(result.maxAbsoluteError, 0, 'g', 17)
                .arg(result.maxErrorStep)
                .arg(result.maxErrorNode);
    }

    return result;
}

QVector<QVector<double>> ValidationRunner::runFiveNodeReferencePreset(
    Solver& solver,
    FiveNodeParameters& params,
    double alpha2)
{
    configureFiveNodePreset(params, alpha2);

    QVector<SolverConnection> solverConnections;
    QMap<QString, double> weightValues;

    for (const auto& c : fiveNodeConnections()) {
        SolverConnection sc;
        sc.firstNode = c.from;
        sc.secondNode = c.to;
        sc.function = c.function;

        solverConnections.append(sc);

        const QString key =
            "s" + QString::number(c.from) + QString::number(c.to);

        weightValues[key] = c.weight;
    }

    return solver.runGamma(
        5,
        solverConnections,
        weightValues);
}
