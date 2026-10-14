#include "fftanimationwindow.h"

#include "analysis/fftanalyzer.h"

#include <QChart>
#include <QChartView>
#include <QComboBox>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineSeries>
#include <QList>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QScatterSeries>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QValueAxis>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

FFTAnimationWindow::FFTAnimationWindow(
    const QString& runDir,
    const QString& graphStyle,
    QWidget* parent)
    : QWidget(parent),
      m_graphStyle(graphStyle)
{
    setWindowTitle("FFT Animation - 30 seconds");
    resize(1050, 760);
    setAttribute(Qt::WA_DeleteOnClose);

    auto* root = new QVBoxLayout(this);
    auto* controls = new QHBoxLayout();

    controls->addWidget(new QLabel("Node:"));

    m_nodeCombo = new QComboBox();

    for (int i = 1; i <= 5; ++i)
        m_nodeCombo->addItem(QString("y%1").arg(i));

    controls->addWidget(m_nodeCombo);

    m_restartButton =
        new QPushButton("Restart 30s Animation");

    controls->addWidget(m_restartButton);
    controls->addStretch();

    root->addLayout(controls);

    m_progressLabel =
        new QLabel("Preparing animation...");

    root->addWidget(m_progressLabel);

    m_chartView = new QChartView();
    m_chartView->setRenderHint(QPainter::Antialiasing);

    root->addWidget(m_chartView, 1);

    m_infoLabel =
        new QLabel(
            "Click an FFT point to inspect "
            "Frequency / Magnitude.");

    root->addWidget(m_infoLabel);

    if (!loadData(runDir)) {
        QMessageBox::critical(
            this,
            "FFT Animation",
            "Cannot load result.dat");
        return;
    }

    setupChart();

    m_timer = new QTimer(this);
    m_timer->setInterval(FrameIntervalMs);

    connect(
        m_timer,
        &QTimer::timeout,
        this,
        &FFTAnimationWindow::updateFrame);

    connect(
        m_restartButton,
        &QPushButton::clicked,
        this,
        &FFTAnimationWindow::startAnimation);

    connect(
        m_nodeCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            if (m_currentSamples >= 2)
                updateSpectrum(m_currentSamples);
        });

    startAnimation();
}

bool FFTAnimationWindow::loadData(
    const QString& runDir)
{
    QFile file(runDir + "/result.dat");

    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text))
        return false;

    m_data = QVector<QVector<double>>(5);

    QTextStream in(&file);

    while (!in.atEnd()) {
        const QString line =
            in.readLine().trimmed();

        if (line.isEmpty() ||
            line.startsWith('#'))
            continue;

        const QStringList parts =
            line.split(
                QRegularExpression("\\s+"),
                Qt::SkipEmptyParts);

        if (parts.size() < 5)
            continue;

        for (int node = 0; node < 5; ++node) {
            bool ok = false;

            const double value =
                parts[node].toDouble(&ok);

            if (!ok)
                return false;

            m_data[node].append(value);
        }
    }

    return !m_data[0].isEmpty();
}

void FFTAnimationWindow::setupChart()
{
    m_chart = new QChart();
    m_chart->setTitle(
        "Cumulative FFT Spectrum");

    const QString style =
        m_graphStyle.trimmed().toLower();

    if (style == "line" ||
        style == "both") {

        m_lineSeries = new QLineSeries();
        m_lineSeries->setName("Magnitude");
        m_lineSeries->setColor(Qt::blue);

        m_chart->addSeries(m_lineSeries);
    }

    if (style == "point" ||
        style == "both") {

        m_pointSeries = new QScatterSeries();
        m_pointSeries->setName("FFT bins");
        m_pointSeries->setColor(Qt::green);
        m_pointSeries->setMarkerSize(7.0);

        m_chart->addSeries(m_pointSeries);

        connect(
            m_pointSeries,
            &QScatterSeries::clicked,
            this,
            [this](const QPointF& p) {
                m_infoLabel->setText(
                    QString(
                        "Frequency: %1    Magnitude: %2")
                        .arg(p.x(), 0, 'g', 12)
                        .arg(p.y(), 0, 'g', 17));
            });
    }

    m_xAxis = new QValueAxis();
    m_yAxis = new QValueAxis();

    m_xAxis->setTitleText("Frequency");
    m_yAxis->setTitleText("Magnitude");

    m_xAxis->setRange(0.0, 0.5);
    m_yAxis->setRange(0.0, 1.0);

    m_chart->addAxis(
        m_xAxis,
        Qt::AlignBottom);

    m_chart->addAxis(
        m_yAxis,
        Qt::AlignLeft);

    if (m_lineSeries) {
        m_lineSeries->attachAxis(m_xAxis);
        m_lineSeries->attachAxis(m_yAxis);
    }

    if (m_pointSeries) {
        m_pointSeries->attachAxis(m_xAxis);
        m_pointSeries->attachAxis(m_yAxis);
    }

    m_chartView->setChart(m_chart);
}

void FFTAnimationWindow::startAnimation()
{
    if (m_data.isEmpty())
        return;

    const int total =
        m_data[0].size();

    m_currentSamples =
        std::min(total, 16);

    m_elapsed.restart();

    updateSpectrum(m_currentSamples);

    m_timer->start();

    m_progressLabel->setText(
        QString("Samples: %1 / %2   Time: 0.0 / 30.0 s")
            .arg(m_currentSamples)
            .arg(total));
}

void FFTAnimationWindow::updateFrame()
{
    if (m_data.isEmpty())
        return;

    const int total =
        m_data[0].size();

    const qint64 elapsedMs =
        m_elapsed.elapsed();

    const double fraction =
        std::min(
            1.0,
            static_cast<double>(elapsedMs)
            / AnimationDurationMs);

    const int startSamples =
        std::min(total, 16);

    m_currentSamples =
        startSamples +
        static_cast<int>(
            std::round(
                fraction *
                (total - startSamples)));

    m_currentSamples =
        std::min(total, m_currentSamples);

    updateSpectrum(m_currentSamples);

    m_progressLabel->setText(
        QString(
            "Samples: %1 / %2   "
            "Time: %3 / 30.0 s")
            .arg(m_currentSamples)
            .arg(total)
            .arg(
                std::min(
                    elapsedMs / 1000.0,
                    30.0),
                0,
                'f',
                1));

    if (fraction >= 1.0) {
        m_timer->stop();

        m_progressLabel->setText(
            QString(
                "Completed: %1 / %2 samples   "
                "Time: 30.0 s")
                .arg(total)
                .arg(total));
    }
}

void FFTAnimationWindow::updateSpectrum(
    int sampleCount)
{
    const int node =
        m_nodeCombo->currentIndex();

    if (node < 0 ||
        node >= m_data.size())
        return;

    sampleCount =
        std::min(
            sampleCount,
            static_cast<int>(m_data[node].size()));

    if (sampleCount < 2)
        return;

    const QVector<double> samples =
        m_data[node].mid(
            0,
            sampleCount);

    QVector<double> frequency;
    QVector<double> magnitude;
    QString error;

    if (!FFTAnalyzer::computeSpectrum(
            samples,
            frequency,
            magnitude,
            &error)) {

        m_infoLabel->setText(error);
        return;
    }

    QList<QPointF> points;

    double maxMagnitude = 0.0;

    // Skip DC (k = 0) visually.
    for (int k = 1;
         k < frequency.size();
         ++k) {

        points.append(
            QPointF(
                frequency[k],
                magnitude[k]));

        maxMagnitude =
            std::max(
                maxMagnitude,
                magnitude[k]);
    }

    if (m_lineSeries)
        m_lineSeries->replace(points);

    if (m_pointSeries)
        m_pointSeries->replace(points);

    if (maxMagnitude <= 0.0)
        maxMagnitude = 1.0;

    m_yAxis->setRange(
        0.0,
        maxMagnitude * 1.10);

    m_chart->setTitle(
        QString(
            "Cumulative FFT - y%1 "
            "(samples 0..%2)")
            .arg(node + 1)
            .arg(sampleCount - 1));
}
