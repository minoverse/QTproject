#ifndef PLOTMANAGER_H
#define PLOTMANAGER_H

#include <QString>

struct PlotResult
{
    bool success = false;
    QString stdOut;
    QString stdErr;
    QString errorMessage;
};

class PlotManager
{
public:
    static bool generateTimeSeriesScript(const QString& runDir, const QString& style);
    static bool generateAlpha2ScanScript(const QString& runDir);

    static PlotResult runTimeSeriesPlot(const QString& runDir, const QString& style);
    static PlotResult runAlpha2ScanPlot(const QString& runDir);

private:
    static PlotResult runGnuplot(const QString& runDir,
                                 const QString& scriptName);
};

#endif // PLOTMANAGER_H
