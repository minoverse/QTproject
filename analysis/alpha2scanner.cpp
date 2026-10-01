#include "alpha2scanner.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

#include <algorithm>
#include <cmath>

Alpha2ScanResult Alpha2Scanner::run(
    const QString& runDir,
    Solver& solver,
    FiveNodeParameters& params,
    int numNodes,
    const QVector<SolverConnection>& connections,
    const QMap<QString, double>& weightValues,
    const Alpha2ScanConfig& config)
{
    Alpha2ScanResult result;

    if (!(config.step > 0.0) || config.max < config.min) {
        result.errorMessage = "Invalid alpha2 scan range.";
        return result;
    }

    int transientPercent = config.transientPercent;
    int sampleStride = config.sampleStride;

    if (transientPercent < 0 || transientPercent > 99)
        transientPercent = 70;

    if (sampleStride < 1)
        sampleStride = 20;

    QFile f3d(QDir(runDir).filePath("alpha2_scan_3d.dat"));
    QFile f2d(QDir(runDir).filePath("alpha2_scan_2d.dat"));

    if (!f3d.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.errorMessage = "Cannot write alpha2_scan_3d.dat";
        return result;
    }

    if (!f2d.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.errorMessage = "Cannot write alpha2_scan_2d.dat";
        return result;
    }

    QTextStream out3d(&f3d);
    QTextStream out2d(&f2d);

    const int steps = params.tMax;

    const int transientStart =
        std::min(
            std::max(
                int(std::floor(
                    steps * (transientPercent / 100.0))),
                0),
            steps);

    const double originalAlpha2 = params.alpha2;

    for (double a2 = config.min;
         a2 <= config.max + 1e-12;
         a2 += config.step) {

        params.alpha2 = a2;

        QVector<QVector<double>> y;

        if (params.solverMode == "ODE") {
            y = solver.runODE(
                connections,
                weightValues);
        } else {
            y = solver.runGamma(
                numNodes,
                connections,
                weightValues);
        }

        for (int t = sampleStride;
             t <= steps;
             t += sampleStride) {

            out3d << a2 << " " << t << " "
                  << y[0][t] << " "
                  << y[1][t] << " "
                  << y[2][t] << " "
                  << y[3][t] << " "
                  << y[4][t] << "\n";
        }

        if (steps % sampleStride != 0) {
            out3d << a2 << " " << steps << " "
                  << y[0][steps] << " "
                  << y[1][steps] << " "
                  << y[2][steps] << " "
                  << y[3][steps] << " "
                  << y[4][steps] << "\n";
        }

        for (int t = transientStart;
             t <= steps;
             t += sampleStride) {

            out2d << a2 << " "
                  << y[0][t] << " "
                  << y[1][t] << " "
                  << y[2][t] << " "
                  << y[3][t] << " "
                  << y[4][t] << "\n";
        }

        out3d << "\n";
        out2d << "\n";
    }

    params.alpha2 = originalAlpha2;

    result.success = true;
    return result;
}
