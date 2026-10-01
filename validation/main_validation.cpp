#include <QCoreApplication>
#include <QDebug>
#include <QString>

#include "model/fivenodeparameters.h"
#include "model/fivenodemodel.h"
#include "solver/solver.h"
#include "validation/validationrunner.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const double alpha2Values[] = {0.0, 1.0, 2.0, 3.0};
    bool allPassed = true;

    qDebug().noquote() << "=== Numerical Validation ===";

    for (double alpha2 : alpha2Values) {
        FiveNodeParameters params;
        FiveNodeModel model(params);
        Solver solver(params, model);

        const QVector<QVector<double>> actual =
            ValidationRunner::runFiveNodeReferencePreset(
                solver, params, alpha2);

        const QString referenceFile =
            QString("validation/reference/result_a2_%1.dat")
                .arg(QString::number(alpha2, 'f', 6));

        const ValidationResult result =
            ValidationRunner::compareWithReference(
                actual, referenceFile, 1e-12);

        qDebug().noquote()
            << QString("alpha2 = %1 : %2")
                   .arg(alpha2, 0, 'f', 1)
                   .arg(result.passed ? "PASS" : "FAIL");

        qDebug().noquote()
            << "  " << result.message;

        if (!result.passed)
            allPassed = false;
    }

    qDebug().noquote() << "";
    qDebug().noquote()
        << "Overall:" << (allPassed ? "PASS" : "FAIL");

    return allPassed ? 0 : 1;
}
