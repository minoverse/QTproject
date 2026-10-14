#include "fftplotwindow.h"

#include <QComboBox>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QVBoxLayout>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>

FFTPlotWindow::FFTPlotWindow(
    const QString& fftFile,
    const QString& graphStyle,
    QWidget* parent)
    : QWidget(parent),
      m_graphStyle(graphStyle)
{
    setWindowTitle("FFT Spectrum");
    resize(1000, 750);
    setAttribute(Qt::WA_DeleteOnClose);

    auto* root = new QVBoxLayout(this);
    auto* controls = new QHBoxLayout();

    controls->addWidget(new QLabel("Node:"));

    m_nodeCombo = new QComboBox();

    for (int i = 1; i <= 5; ++i)
        m_nodeCombo->addItem(
            QString("y%1").arg(i));

    controls->addWidget(m_nodeCombo);
    controls->addStretch();

    root->addLayout(controls);

    m_chartView = new QChartView();
    m_chartView->setRenderHint(
        QPainter::Antialiasing);

    root->addWidget(m_chartView, 1);

    m_infoLabel = new QLabel(
        "Click a point to inspect Frequency / Magnitude.");

    root->addWidget(m_infoLabel);

    if (!loadData(fftFile)) {
        QMessageBox::critical(
            this,
            "FFT",
            "Cannot load fft.dat");
        return;
    }

    connect(
        m_nodeCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            rebuildChart();
        });

    rebuildChart();
}

bool FFTPlotWindow::loadData(
    const QString& fftFile)
{
    QFile file(fftFile);

    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text))
        return false;

    m_magnitude =
        QVector<QVector<double>>(5);

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

        if (parts.size() < 6)
            continue;

        bool ok = false;

        const double frequency =
            parts[0].toDouble(&ok);

        if (!ok)
            return false;

        m_frequency.append(frequency);

        for (int node = 0; node < 5; ++node) {

            const double value =
                parts[node + 1].toDouble(&ok);

            if (!ok)
                return false;

            m_magnitude[node].append(value);
        }
    }

    return !m_frequency.isEmpty();
}

void FFTPlotWindow::rebuildChart()
{
    const int node =
        m_nodeCombo->currentIndex();

    if (node < 0 || m_frequency.isEmpty())
        return;

    auto* chart = new QChart();

    chart->setTitle(
        QString("FFT Spectrum - y%1")
            .arg(node + 1));

    const QString style =
        m_graphStyle.trimmed().toLower();

    // Skip k=0 only in the visual plot because
    // DC can hide the useful spectral peaks.
    // fft.dat still contains the DC value.
    const int firstBin =
        m_frequency.size() > 1 ? 1 : 0;

    if (style == "line" ||
        style == "both") {

        auto* line = new QLineSeries();

        line->setName("Magnitude");
        line->setColor(Qt::blue);

        for (int i = firstBin;
             i < m_frequency.size();
             ++i) {

            line->append(
                m_frequency[i],
                m_magnitude[node][i]);
        }

        chart->addSeries(line);
    }

    if (style == "point" ||
        style == "both") {

        auto* points =
            new QScatterSeries();

        points->setName("FFT bins");
        points->setColor(Qt::green);
        points->setMarkerSize(7.0);

        for (int i = firstBin;
             i < m_frequency.size();
             ++i) {

            points->append(
                m_frequency[i],
                m_magnitude[node][i]);
        }

        connect(
            points,
            &QScatterSeries::clicked,
            this,
            [this](const QPointF& p) {

                m_infoLabel->setText(
                    QString(
                        "Frequency: %1    "
                        "Magnitude: %2")
                        .arg(p.x(), 0, 'g', 12)
                        .arg(p.y(), 0, 'g', 17));
            });

        chart->addSeries(points);
    }

    chart->createDefaultAxes();

    if (!chart->axes(Qt::Horizontal).isEmpty()) {
        chart->axes(Qt::Horizontal)
            .first()
            ->setTitleText("Frequency");
    }

    if (!chart->axes(Qt::Vertical).isEmpty()) {
        chart->axes(Qt::Vertical)
            .first()
            ->setTitleText("Magnitude");
    }

    m_chartView->setChart(chart);

    m_infoLabel->setText(
        "Click a point to inspect Frequency / Magnitude.");
}
