#include <QCoreApplication>
#include <QFile>
#include <QMap>
#include <QString>
#include <QTextStream>
#include <QVector>

#include <cmath>

#include "model/fivenodeparameters.h"
#include "model/fivenodemodel.h"
#include "solver/solver.h"
#include "validation/validationrunner.h"

static QVector<SolverConnection> makeConnections()
{
    QVector<SolverConnection> out;
    for (const auto& c : ValidationRunner::fiveNodeConnections()) {
        SolverConnection sc;
        sc.firstNode = c.from;
        sc.secondNode = c.to;
        sc.function = c.function;
        out.append(sc);
    }
    return out;
}

static QMap<QString, double> makeWeights()
{
    QMap<QString, double> out;
    for (const auto& c : ValidationRunner::fiveNodeConnections()) {
        const QString key = "s" + QString::number(c.from) + QString::number(c.to);
        out[key] = c.weight;
    }
    return out;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    if (argc != 4) {
        QTextStream(stderr) << "Usage: chaos_case <alpha2> <nu> <output.dat>\n";
        return 1;
    }

    bool okA = false;
    bool okN = false;
    const double alpha2 = QString::fromLocal8Bit(argv[1]).toDouble(&okA);
    const double nu = QString::fromLocal8Bit(argv[2]).toDouble(&okN);
    const QString outputPath = QString::fromLocal8Bit(argv[3]);

    if (!okA || !okN) {
        QTextStream(stderr) << "Invalid alpha2 or nu.\n";
        return 2;
    }

    // The current unchanged GAMMA formula has a singular direct evaluation at nu=0.
    // Do not alter the numerical core here.
    if (std::abs(nu) < 1e-14) {
        QTextStream(stderr) << "nu=0 skipped: unchanged GAMMA formula is singular at direct evaluation.\n";
        return 3;
    }

    FiveNodeParameters params;
    ValidationRunner::configureFiveNodePreset(params, alpha2);
    params.alpha2 = alpha2;
    params.nu = nu;
    params.tMax = 10000;
    params.solverMode = "GAMMA";

    FiveNodeModel model(params);
    Solver solver(params, model);

    const QVector<QVector<double>> y = solver.runGamma(
        5,
        makeConnections(),
        makeWeights());

    if (y.size() < 5 || y[0].size() < 10001) {
        QTextStream(stderr) << "Unexpected solver output size.\n";
        return 4;
    }

    QFile f(outputPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(stderr) << "Cannot open output file: " << outputPath << "\n";
        return 5;
    }

    QTextStream out(&f);
    out.setRealNumberNotation(QTextStream::ScientificNotation);
    out.setRealNumberPrecision(12);

    // Professor test01.c expects exactly 10000 rows and exactly 5 columns.
    // Keep tMax=10000, omit t=0, and export t=1..10000 as y1..y5 only.
    for (int t = 1; t <= 10000; ++t) {
        for (int node = 0; node < 5; ++node) {
            if (node != 0)
                out << ' ';
            const double v = y[node][t];
            if (!std::isfinite(v)) {
                QTextStream(stderr) << "Non-finite solver value at t=" << t
                                    << ", node=" << (node + 1) << "\n";
                return 6;
            }
            out << v;
        }
        out << '\n';
    }

    return 0;
}
