#include "bifurcationwindow.h"

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
#include <limits>

BifurcationWindow::BifurcationWindow(
    const QString& dataFile,
    const QString& graphStyle,
    QWidget* parent)
    : QWidget(parent),
      m_graphStyle(graphStyle)
{
    setWindowTitle("Bifurcation Diagram");
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

    m_fullButton =
        new QPushButton("Show Full Diagram");

    controls->addWidget(m_restartButton);
    controls->addWidget(m_fullButton);
    controls->addStretch();

    root->addLayout(controls);

    m_progressLabel =
        new QLabel("Preparing bifurcation diagram...");

    root->addWidget(m_progressLabel);

    m_chartView = new QChartView();
    m_chartView->setRenderHint(QPainter::Antialiasing);

    root->addWidget(m_chartView, 1);

    m_infoLabel =
        new QLabel(
            "Click a point to inspect alpha2 / value.");

    root->addWidget(m_infoLabel);

    if (!loadData(dataFile)) {
        QMessageBox::critical(
            this,
            "Bifurcation Diagram",
            "Cannot load alpha2_scan_2d.dat");
        return;
    }

    setupChart();

    m_timer = new QTimer(this);
    m_timer->setInterval(FrameIntervalMs);

    connect(
        m_timer,
        &QTimer::timeout,
        this,
        &BifurcationWindow::updateFrame);

    connect(
        m_restartButton,
        &QPushButton::clicked,
        this,
        &BifurcationWindow::startAnimation);

    connect(
        m_fullButton,
        &QPushButton::clicked,
        this,
        &BifurcationWindow::showFullDiagram);

    connect(
        m_nodeCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            updatePlot(m_currentRows);
        });

    startAnimation();
}

bool BifurcationWindow::loadData(
    const QString& dataFile)
{
    QFile file(dataFile);

    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text))
        return false;

    m_y = QVector<QVector<double>>(5);

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

        const double alpha =
            parts[0].toDouble(&ok);

        if (!ok)
            return false;

        m_alpha.append(alpha);

        for (int node = 0; node < 5; ++node) {
            const double value =
                parts[node + 1].toDouble(&ok);

            if (!ok)
                return false;

            m_y[node].append(value);
        }
    }

    return !m_alpha.isEmpty();
}

void BifurcationWindow::setupChart()
{
    m_chart = new QChart();
    m_chart->setTitle("Bifurcation Diagram");

    const QString style =
        m_graphStyle.trimmed().toLower();

    if (style == "line" ||
        style == "both") {

        m_lineSeries = new QLineSeries();
        m_lineSeries->setName("Trajectory");
        m_lineSeries->setColor(Qt::blue);

        m_chart->addSeries(m_lineSeries);
    }

    if (style == "point" ||
        style == "both") {

        m_pointSeries = new QScatterSeries();
        m_pointSeries->setName("Samples");
        m_pointSeries->setColor(Qt::green);
        m_pointSeries->setMarkerSize(6.0);

        m_chart->addSeries(m_pointSeries);

        connect(
            m_pointSeries,
            &QScatterSeries::clicked,
            this,
            [this](const QPointF& p) {
                m_infoLabel->setText(
                    QString(
                        "alpha2: %1    Value: %2")
                        .arg(p.x(), 0, 'g', 12)
                        .arg(p.y(), 0, 'g', 17));
            });
    }

    m_xAxis = new QValueAxis();
    m_yAxis = new QValueAxis();

    m_xAxis->setTitleText("alpha2");
    m_yAxis->setTitleText("State value");

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

void BifurcationWindow::startAnimation()
{
    if (m_alpha.isEmpty())
        return;

    m_currentRows = 1;

    m_elapsed.restart();

    updatePlot(m_currentRows);

    m_timer->start();
}

void BifurcationWindow::showFullDiagram()
{
    if (m_timer)
        m_timer->stop();

    m_currentRows =
        static_cast<int>(m_alpha.size());

    updatePlot(m_currentRows);

    m_progressLabel->setText(
        QString("Full diagram: %1 points")
            .arg(m_currentRows));
}

void BifurcationWindow::updateFrame()
{
    const int total =
        static_cast<int>(m_alpha.size());

    if (total <= 0)
        return;

    const qint64 elapsedMs =
        m_elapsed.elapsed();

    const double fraction =
        std::min(
            1.0,
            static_cast<double>(elapsedMs)
            / AnimationDurationMs);

    m_currentRows =
        1 +
        static_cast<int>(
            fraction * (total - 1));

    m_currentRows =
        std::min(
            total,
            m_currentRows);

    updatePlot(m_currentRows);

    m_progressLabel->setText(
        QString(
            "Points: %1 / %2    "
            "Time: %3 / 30.0 s")
            .arg(m_currentRows)
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
                "Completed: %1 points    "
                "Time: 30.0 s")
                .arg(total));
    }
}

void BifurcationWindow::updatePlot(
    int rowCount)
{
    const int node =
        m_nodeCombo->currentIndex();

    if (node < 0 ||
        node >= m_y.size())
        return;

    const int total =
        static_cast<int>(m_alpha.size());

    rowCount =
        std::min(rowCount, total);

    QList<QPointF> points;

    double minX =
        std::numeric_limits<double>::max();

    double maxX =
        std::numeric_limits<double>::lowest();

    double minY =
        std::numeric_limits<double>::max();

    double maxY =
        std::numeric_limits<double>::lowest();

    for (int i = 0; i < rowCount; ++i) {

        const double x = m_alpha[i];
        const double y = m_y[node][i];

        points.append(QPointF(x, y));

        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    }

    if (m_lineSeries)
        m_lineSeries->replace(points);

    if (m_pointSeries)
        m_pointSeries->replace(points);

    if (rowCount > 0) {
        if (minX == maxX) {
            minX -= 0.5;
            maxX += 0.5;
        }

        if (minY == maxY) {
            minY -= 0.5;
            maxY += 0.5;
        }

        const double yPadding =
            (maxY - minY) * 0.05;

        m_xAxis->setRange(minX, maxX);

        m_yAxis->setRange(
            minY - yPadding,
            maxY + yPadding);
    }

    m_chart->setTitle(
        QString(
            "Bifurcation Diagram - y%1")
            .arg(node + 1));
}
