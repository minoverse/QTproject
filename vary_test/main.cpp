#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVector>

#include "model/fivenodeparameters.h"
#include "model/fivenodemodel.h"
#include "solver/solver.h"
#include "validation/validationrunner.h"

static void configureProfessorPreset(FiveNodeParameters& params,
                                     double alpha2,
                                     double nu)
{
    params.alpha1 = -2.2;
    params.alpha2 = alpha2;
    params.alpha3 = 1.2;
    params.nu = nu;
    params.tMax = 1000;
    params.solverMode = "GAMMA";

    params.gateNode4.enabled = true;
    params.gateNode4.baseType = "alpha2";
    params.gateNode4.baseConst = 1.0;
    params.gateNode4.coeff = 1.2;
    params.gateNode4.fn = "sin";

    params.gateNode5.enabled = true;
    params.gateNode5.baseType = "const";
    params.gateNode5.baseConst = 1.0;
    params.gateNode5.coeff = 2.2;
    params.gateNode5.fn = "tanh";
}

static QVector<SolverConnection> buildConnections()
{
    QVector<SolverConnection> solverConnections;

    for (const auto& c : ValidationRunner::fiveNodeConnections()) {
        SolverConnection sc;
        sc.firstNode = c.from;
        sc.secondNode = c.to;
        sc.function = c.function;
        solverConnections.append(sc);
    }

    return solverConnections;
}

static QMap<QString, double> buildWeights(bool decoupleY1Y2)
{
    QMap<QString, double> weightValues;

    for (const auto& c : ValidationRunner::fiveNodeConnections()) {
        double w = c.weight;

        if (decoupleY1Y2) {
            if ((c.from == 1 && c.to == 2) ||
                (c.from == 2 && c.to == 1)) {
                w = 0.0;
            }
        }

        const QString key =
            "s" + QString::number(c.from) + QString::number(c.to);

        weightValues[key] = w;
    }

    return weightValues;
}

static bool writeResultDat(const QString& filePath,
                           const QVector<QVector<double>>& y)
{
    if (y.size() < 5) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(12);

    const int steps = y[0].size();

    for (int t = 0; t < steps; ++t) {
        out << y[0][t] << " "
            << y[1][t] << " "
            << y[2][t] << " "
            << y[3][t] << " "
            << y[4][t] << "\n";
    }

    return true;
}

static bool writePlotScript(const QString& scriptPath,
                            const QString& dataFileName,
                            const QString& pngFileName,
                            const QString& title)
{
    QFile file(scriptPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);

    out << "set terminal pngcairo size 1500,1000 enhanced\n";
    out << "set output '" << pngFileName << "'\n";
    out << "set multiplot layout 5,1 title '" << title << "'\n";
    out << "set grid\n";
    out << "unset key\n";
    out << "set tics out\n";
    out << "set lmargin 10\n";

    for (int i = 1; i <= 5; ++i) {
        out << "set xlabel 'n'\n";
        out << "set ylabel 'y" << i << "'\n";
        out << "plot '" << dataFileName << "' using 1:" << i
            << " with points pt 7 ps 0.5 lc rgb '#00aa00' notitle\n\n";
    }

    out << "unset multiplot\n";

    return true;
}

static bool runGnuplot(const QString& workDir,
                       const QString& scriptName,
                       QString* stdOut = nullptr,
                       QString* stdErr = nullptr)
{
    QProcess proc;
    proc.setWorkingDirectory(workDir);
    proc.start("gnuplot", QStringList() << scriptName);

    if (!proc.waitForFinished(-1)) {
        if (stdErr) *stdErr = "gnuplot failed or timeout";
        return false;
    }

    if (stdOut) *stdOut = proc.readAllStandardOutput();
    if (stdErr) *stdErr = proc.readAllStandardError();

    return proc.exitStatus() == QProcess::NormalExit &&
           proc.exitCode() == 0;
}

static QString makeCaseName(double alpha2,
                            double nu,
                            bool decoupleY1Y2)
{
    QString name =
        QString("alpha2_%1_nu_%2")
            .arg(alpha2, 0, 'f', 1)
            .arg(nu, 0, 'f', 2);

    if (decoupleY1Y2) {
        name += "_s12_0_s21_0";
    }

    return name;
}

static bool runCase(const QString& categoryDir,
                    double alpha2,
                    double nu,
                    bool decoupleY1Y2)
{
    FiveNodeParameters params;
    configureProfessorPreset(params, alpha2, nu);

    FiveNodeModel model(params);
    Solver solver(params, model);

    const QVector<SolverConnection> connections = buildConnections();
    const QMap<QString, double> weightValues = buildWeights(decoupleY1Y2);

    const QVector<QVector<double>> y =
        solver.runGamma(5, connections, weightValues);

    const QString caseName = makeCaseName(alpha2, nu, decoupleY1Y2);

    const QString datFile = categoryDir + "/" + caseName + ".dat";
    const QString gnuFile = categoryDir + "/" + caseName + ".gnu";
    const QString pngFile = categoryDir + "/" + caseName + ".png";

    if (!writeResultDat(datFile, y)) {
        QTextStream err(stderr);
        err << "Failed to write: " << datFile << "\n";
        return false;
    }

    if (!writePlotScript(
            gnuFile,
            QFileInfo(datFile).fileName(),
            QFileInfo(pngFile).fileName(),
            caseName)) {
        QTextStream err(stderr);
        err << "Failed to write: " << gnuFile << "\n";
        return false;
    }

    QString gpOut;
    QString gpErr;

    if (!runGnuplot(categoryDir,
                    QFileInfo(gnuFile).fileName(),
                    &gpOut,
                    &gpErr)) {
        QTextStream err(stderr);
        err << "gnuplot failed for: " << caseName << "\n";
        if (!gpErr.trimmed().isEmpty()) {
            err << gpErr << "\n";
        }
        return false;
    }

    QTextStream qout(stdout);
    qout << "Saved: " << pngFile << "\n";

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QString timeStamp =
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

    const QString rootDir =
        QDir::currentPath() + "/results/run_" + timeStamp;

    const QString fullDir =
        rootDir + "/full_alpha2_nu";

    const QString specialDir =
        rootDir + "/node1_Decouple";

    QDir().mkpath(fullDir);
    QDir().mkpath(specialDir);

    const QVector<double> nuList = {
        0.10, 0.20, 0.30, 0.40, 0.50,
        0.60, 0.70, 0.80, 0.90, 0.95
    };

    bool allPassed = true;
    int fullCount = 0;
    int specialCount = 0;

    {
        QTextStream qout(stdout);
        qout << "Running full alpha2 x nu sweep...\n";
    }

    for (int alpha2 = -10; alpha2 <= 10; ++alpha2) {
        for (double nu : nuList) {
            const QString caseName =
                makeCaseName(double(alpha2), nu, false);

            QTextStream qout(stdout);
            qout << "Running " << caseName << "\n";

            if (runCase(fullDir, double(alpha2), nu, false)) {
                ++fullCount;
            } else {
                allPassed = false;
            }
        }
    }

    {
        QTextStream qout(stdout);
        qout << "Running node1_Decouple special cases...\n";
    }

    for (int alpha2 = -10; alpha2 <= 10; ++alpha2) {
        const QString caseName =
            makeCaseName(double(alpha2), 0.70, true);

        QTextStream qout(stdout);
        qout << "Running " << caseName << "\n";

        if (runCase(specialDir, double(alpha2), 0.70, true)) {
            ++specialCount;
        } else {
            allPassed = false;
        }
    }

    QFile summary(rootDir + "/summary.txt");
    if (summary.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&summary);
        out << "vary_test summary\n";
        out << "root = " << rootDir << "\n";
        out << "full_alpha2_nu = " << fullCount << "\n";
        out << "node1_Decouple = " << specialCount << "\n";
        out << "total = " << (fullCount + specialCount) << "\n";
    }

    QTextStream qout(stdout);
    qout << "\n================================\n";
    qout << "vary_test finished\n";
    qout << "================================\n";
    qout << "full_alpha2_nu : " << fullCount << " cases\n";
    qout << "node1_Decouple : " << specialCount << " cases\n";
    qout << "total          : " << (fullCount + specialCount) << " cases\n";
    qout << "Results        : " << rootDir << "\n";

    return allPassed ? 0 : 1;
}
