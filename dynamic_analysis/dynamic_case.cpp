#include <QCoreApplication>
#include <QFile>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVector>

#include <cmath>

#include "model/fivenodeparameters.h"
#include "model/fivenodemodel.h"
#include "solver/solver.h"
#include "validation/validationrunner.h"

/*
 * SEPARATE ANALYSIS PROGRAM ONLY.
 *
 * Existing numerical core is NEVER modified.
 * Connection cuts are applied only to a local copy of the weights.
 */

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

static QMap<QString, double> makeWeights(const QString& cutsCsv)
{
    QMap<QString, double> out;

    for (const auto& c : ValidationRunner::fiveNodeConnections()) {
        const QString key =
            "s" + QString::number(c.from) + QString::number(c.to);

        out[key] = c.weight;
    }

    if (cutsCsv == "-" || cutsCsv.trimmed().isEmpty())
        return out;

    const QStringList cuts =
        cutsCsv.split(',', Qt::SkipEmptyParts);

    for (QString cut : cuts) {
        cut = cut.trimmed().toLower();

        if (!out.contains(cut)) {
            QTextStream(stderr)
                << "Unknown connection: " << cut << "\n";

            return QMap<QString, double>();
        }

        // LOCAL COPY ONLY
        out[cut] = 0.0;
    }

    return out;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    if (argc != 6) {
        QTextStream(stderr)
            << "Usage: dynamic_case "
            << "<alpha2> <nu> <tMax> <output.dat> <cuts>\n";

        return 1;
    }

    bool okA = false;
    bool okN = false;
    bool okT = false;

    const double alpha2 =
        QString::fromLocal8Bit(argv[1]).toDouble(&okA);

    const double nu =
        QString::fromLocal8Bit(argv[2]).toDouble(&okN);

    const int tMax =
        QString::fromLocal8Bit(argv[3]).toInt(&okT);

    const QString outputPath =
        QString::fromLocal8Bit(argv[4]);

    const QString cutsCsv =
        QString::fromLocal8Bit(argv[5]);

    if (!okA || !okN || !okT || tMax < 1) {
        QTextStream(stderr)
            << "Invalid alpha2, nu or tMax.\n";

        return 2;
    }

    /*
     * Do NOT modify the GAMMA numerical core.
     * Current direct evaluation is singular at nu = 0.
     */
    if (std::abs(nu) < 1e-14) {
        QTextStream(stderr)
            << "nu=0 skipped.\n";

        return 3;
    }

    FiveNodeParameters params;

    ValidationRunner::configureFiveNodePreset(
        params,
        alpha2
    );

    params.alpha2 = alpha2;
    params.nu = nu;
    params.tMax = tMax;
    params.solverMode = "GAMMA";

    const QMap<QString, double> weights =
        makeWeights(cutsCsv);

    if (weights.isEmpty()) {
        QTextStream(stderr)
            << "Invalid cut list.\n";

        return 7;
    }

    FiveNodeModel model(params);
    Solver solver(params, model);

    const QVector<QVector<double>> y =
        solver.runGamma(
            5,
            makeConnections(),
            weights
        );

    if (y.size() < 5) {
        QTextStream(stderr)
            << "Unexpected solver output.\n";

        return 4;
    }

    for (int node = 0; node < 5; ++node) {
        if (y[node].size() < tMax + 1) {
            QTextStream(stderr)
                << "Unexpected solver output size.\n";

            return 4;
        }
    }

    QFile f(outputPath);

    if (!f.open(
            QIODevice::WriteOnly |
            QIODevice::Text)) {

        QTextStream(stderr)
            << "Cannot open output file.\n";

        return 5;
    }

    QTextStream out(&f);

    out.setRealNumberNotation(
        QTextStream::ScientificNotation
    );

    out.setRealNumberPrecision(12);

    /*
     * y1 y2 y3 y4 y5
     * NO time column.
     */
    for (int t = 1; t <= tMax; ++t) {

        for (int node = 0; node < 5; ++node) {

            if (node != 0)
                out << ' ';

            const double v = y[node][t];

            if (!std::isfinite(v)) {
                QTextStream(stderr)
                    << "Non-finite value at t="
                    << t
                    << ", node="
                    << node + 1
                    << "\n";

                return 6;
            }

            out << v;
        }

        out << '\n';
    }

    return 0;
}
