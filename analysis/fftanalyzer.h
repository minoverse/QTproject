#ifndef FFTANALYZER_H
#define FFTANALYZER_H

#include <QString>
#include <QVector>

struct FFTAnalysisResult
{
    bool success = false;
    QString outputFile;
    QString errorMessage;
    int samples = 0;
    int bins = 0;
};

class FFTAnalyzer
{
public:
    static FFTAnalysisResult analyze(const QString& runDir);

    static bool computeSpectrum(
        const QVector<double>& samples,
        QVector<double>& frequency,
        QVector<double>& magnitude,
        QString* errorMessage = nullptr);
};

#endif
