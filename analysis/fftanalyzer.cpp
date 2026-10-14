#include "fftanalyzer.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QVector>

#include <fftw3.h>

#include <cmath>

FFTAnalysisResult FFTAnalyzer::analyze(const QString& runDir)
{
    FFTAnalysisResult result;

    QFile input(QDir(runDir).filePath("result.dat"));

    if (!input.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorMessage = "Cannot open result.dat";
        return result;
    }

    QVector<QVector<double>> y(5);
    QTextStream in(&input);

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();

        if (line.isEmpty() || line.startsWith('#'))
            continue;

        const QStringList parts =
            line.split(QRegularExpression("\\s+"),
                       Qt::SkipEmptyParts);

        if (parts.size() < 5)
            continue;

        for (int node = 0; node < 5; ++node) {
            bool ok = false;
            const double value = parts[node].toDouble(&ok);

            if (!ok) {
                result.errorMessage =
                    "Invalid value in result.dat";
                return result;
            }

            y[node].append(value);
        }
    }

    const int N = y[0].size();

    if (N < 2) {
        result.errorMessage = "Not enough samples for FFT";
        return result;
    }

    const int bins = N / 2 + 1;

    QVector<QVector<double>> magnitude(
        5, QVector<double>(bins));

    double* inputData = fftw_alloc_real(N);
    fftw_complex* outputData =
        fftw_alloc_complex(bins);

    if (!inputData || !outputData) {
        if (inputData)
            fftw_free(inputData);
        if (outputData)
            fftw_free(outputData);

        result.errorMessage =
            "FFTW memory allocation failed";
        return result;
    }

    fftw_plan plan =
        fftw_plan_dft_r2c_1d(
            N,
            inputData,
            outputData,
            FFTW_ESTIMATE);

    if (!plan) {
        fftw_free(inputData);
        fftw_free(outputData);

        result.errorMessage =
            "Cannot create FFTW plan";
        return result;
    }

    for (int node = 0; node < 5; ++node) {

        for (int n = 0; n < N; ++n)
            inputData[n] = y[node][n];

        fftw_execute(plan);

        for (int k = 0; k < bins; ++k) {
            const double re = outputData[k][0];
            const double im = outputData[k][1];

            double mag =
                std::hypot(re, im)
                / static_cast<double>(N);

            // One-sided spectrum.
            if (k > 0 &&
                !(N % 2 == 0 && k == N / 2)) {
                mag *= 2.0;
            }

            magnitude[node][k] = mag;
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(inputData);
    fftw_free(outputData);

    const QString outputPath =
        QDir(runDir).filePath("fft.dat");

    QFile output(outputPath);

    if (!output.open(
            QIODevice::WriteOnly |
            QIODevice::Text)) {

        result.errorMessage =
            "Cannot create fft.dat";
        return result;
    }

    QTextStream out(&output);

    out.setRealNumberNotation(
        QTextStream::ScientificNotation);

    out.setRealNumberPrecision(17);

    out << "# frequency mag_y1 mag_y2 mag_y3 mag_y4 mag_y5\n";

    for (int k = 0; k < bins; ++k) {

        // Normalized frequency.
        // Physical Hz can be added once dt is defined.
        const double frequency =
            static_cast<double>(k)
            / static_cast<double>(N);

        out << frequency;

        for (int node = 0; node < 5; ++node)
            out << " " << magnitude[node][k];

        out << "\n";
    }

    result.success = true;
    result.outputFile = outputPath;
    result.samples = N;
    result.bins = bins;

    return result;
}

bool FFTAnalyzer::computeSpectrum(
    const QVector<double>& samples,
    QVector<double>& frequency,
    QVector<double>& magnitude,
    QString* errorMessage)
{
    const int N = samples.size();

    if (N < 2) {
        if (errorMessage)
            *errorMessage = "Not enough samples for FFT";
        return false;
    }

    const int bins = N / 2 + 1;

    double* inputData = fftw_alloc_real(N);
    fftw_complex* outputData =
        fftw_alloc_complex(bins);

    if (!inputData || !outputData) {
        if (inputData)
            fftw_free(inputData);
        if (outputData)
            fftw_free(outputData);

        if (errorMessage)
            *errorMessage = "FFTW memory allocation failed";

        return false;
    }

    for (int n = 0; n < N; ++n)
        inputData[n] = samples[n];

    fftw_plan plan =
        fftw_plan_dft_r2c_1d(
            N,
            inputData,
            outputData,
            FFTW_ESTIMATE);

    if (!plan) {
        fftw_free(inputData);
        fftw_free(outputData);

        if (errorMessage)
            *errorMessage = "Cannot create FFTW plan";

        return false;
    }

    fftw_execute(plan);

    frequency.resize(bins);
    magnitude.resize(bins);

    for (int k = 0; k < bins; ++k) {
        const double re = outputData[k][0];
        const double im = outputData[k][1];

        double mag =
            std::hypot(re, im)
            / static_cast<double>(N);

        if (k > 0 &&
            !(N % 2 == 0 && k == N / 2)) {
            mag *= 2.0;
        }

        frequency[k] =
            static_cast<double>(k)
            / static_cast<double>(N);

        magnitude[k] = mag;
    }

    fftw_destroy_plan(plan);
    fftw_free(inputData);
    fftw_free(outputData);

    return true;
}
