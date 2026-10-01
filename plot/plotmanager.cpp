#include "plotmanager.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTextStream>

bool PlotManager::generateTimeSeriesScript(const QString& runDir)
{
    QFile script(QDir(runDir).filePath("plot.gnu"));

    if (!script.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&script);

    // Combined plot
    out << "set terminal pngcairo size 1200,900\n";
    out << "set output 'y_all.png'\n";
    out << "set multiplot layout 5,1 title 'Hopfield Network Results'\n";
    out << "set grid\n";
    out << "set key left\n";
    out << "set xlabel 't (step)'\n";
    out << "set xrange [0:*]\n";

    for (int i = 1; i <= 5; ++i) {
        out << "set ylabel 'y" << i << "'\n";
        out << "plot 'result.dat' using 0:" << i
            << " with lines linewidth 2 title 'y" << i << "'\n\n";
    }

    out << "unset multiplot\n";
    out << "set output\n\n";

    // Individual plots
    out << "set terminal pngcairo size 1200,700\n";
    out << "set grid\n";
    out << "set key left\n";
    out << "set xlabel 't (step)'\n";
    out << "set xrange [0:*]\n";

    for (int i = 1; i <= 5; ++i) {
        out << "set output 'y" << i << ".png'\n";
        out << "set title 'y" << i << "'\n";
        out << "set ylabel 'y" << i << "'\n";
        out << "plot 'result.dat' using 0:" << i
            << " with lines linewidth 2 title 'y" << i << "'\n";
        out << "set output\n\n";
    }

    return true;
}

bool PlotManager::generateAlpha2ScanScript(const QString& runDir)
{
    QFile script(QDir(runDir).filePath("alpha2_scan.gnu"));

    if (!script.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream g(&script);

    g << "set term pngcairo size 900,700\n";
    g << "set grid\n";
    g << "set xlabel 'alpha2'\n";
    g << "unset key\n";
    g << "set pointsize 0.6\n";

    const char* yNames[5] = {"y1", "y2", "y3", "y4", "y5"};
    const int col2d[5] = {2, 3, 4, 5, 6};

    for (int i = 0; i < 5; ++i) {
        g << "set output 'alpha2_" << yNames[i] << ".png'\n";
        g << "set ylabel '" << yNames[i] << "'\n";
        g << "plot 'alpha2_scan_2d.dat' using 1:" << col2d[i]
          << " with points pt 7 ps 0.4\n\n";
    }

    g << "set output\n";

    return true;
}

PlotResult PlotManager::runGnuplot(const QString& runDir,
                                   const QString& scriptName)
{
    PlotResult result;

    QProcess proc;
    proc.setWorkingDirectory(runDir);
    proc.start("gnuplot", QStringList() << scriptName);

    if (!proc.waitForStarted()) {
        result.errorMessage = "Failed to start gnuplot. Is it installed?";
        return result;
    }

    const bool finished = proc.waitForFinished(-1);

    result.stdOut =
        QString::fromLocal8Bit(proc.readAllStandardOutput());

    result.stdErr =
        QString::fromLocal8Bit(proc.readAllStandardError());

    if (!finished ||
        proc.exitStatus() != QProcess::NormalExit ||
        proc.exitCode() != 0) {

        result.errorMessage = "gnuplot execution failed.";
        return result;
    }

    result.success = true;
    return result;
}

PlotResult PlotManager::runTimeSeriesPlot(const QString& runDir)
{
    PlotResult result;

    if (!generateTimeSeriesScript(runDir)) {
        result.errorMessage = "Cannot create plot.gnu";
        return result;
    }

    return runGnuplot(runDir, "plot.gnu");
}

PlotResult PlotManager::runAlpha2ScanPlot(const QString& runDir)
{
    PlotResult result;

    if (!generateAlpha2ScanScript(runDir)) {
        result.errorMessage = "Cannot create alpha2_scan.gnu";
        return result;
    }

    return runGnuplot(runDir, "alpha2_scan.gnu");
}
