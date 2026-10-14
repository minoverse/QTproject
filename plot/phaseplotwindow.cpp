#include "phaseplotwindow.h"

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

#include <limits>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>

PhasePlotWindow::PhasePlotWindow(
    const QString& runDir,
    const QString& graphStyle,
    QWidget* parent)
    : QWidget(parent),
      m_graphStyle(graphStyle)
{
    setWindowTitle("Phase Plot");
    resize(900, 750);
    setAttribute(Qt::WA_DeleteOnClose);

    auto* root = new QVBoxLayout(this);
    auto* controls = new QHBoxLayout();

    controls->addWidget(new QLabel("X:"));

    m_xCombo = new QComboBox();

    controls->addWidget(m_xCombo);

    controls->addWidget(new QLabel("Y:"));

    m_yCombo = new QComboBox();

    controls->addWidget(m_yCombo);

    for (int i = 1; i <= 5; ++i) {
        m_xCombo->addItem(QString("y%1").arg(i));
        m_yCombo->addItem(QString("y%1").arg(i));
    }

    m_xCombo->setCurrentIndex(0); // y1
    m_yCombo->setCurrentIndex(1); // y2

    controls->addStretch();

    root->addLayout(controls);

    m_chartView = new QChartView();
    m_chartView->setRenderHint(QPainter::Antialiasing);

    root->addWidget(m_chartView, 1);

    m_infoLabel =
        new QLabel("Click a point to inspect its coordinates.");

    root->addWidget(m_infoLabel);

    if (!loadData(runDir)) {
        QMessageBox::critical(
            this,
            "Phase Plot",
            "Cannot load result.dat");
        return;
    }

    connect(
        m_xCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            rebuildChart();
        });

    connect(
        m_yCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            rebuildChart();
        });

    rebuildChart();
}

bool PhasePlotWindow::loadData(
    const QString& runDir)
{
    QFile file(runDir + "/result.dat");

    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text))
        return false;

    m_data =
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

        if (parts.size() < 5)
            continue;

        for (int i = 0; i < 5; ++i) {

            bool ok = false;

            const double value =
                parts[i].toDouble(&ok);

            if (!ok)
                return false;

            m_data[i].append(value);
        }
    }

    return !m_data[0].isEmpty();
}

void PhasePlotWindow::rebuildChart()
{
    const int xNode =
        m_xCombo->currentIndex();

    const int yNode =
        m_yCombo->currentIndex();

    if (xNode < 0 ||
        yNode < 0 ||
        m_data.isEmpty())
        return;

    auto* chart = new QChart();

    chart->setTitle(
        QString("Phase Plot: y%1 vs y%2")
            .arg(yNode + 1)
            .arg(xNode + 1));

    const QString style =
        m_graphStyle.trimmed().toLower();

    if (style == "line" ||
        style == "both") {

        auto* line =
            new QLineSeries();

        line->setName("Trajectory");
        line->setColor(Qt::blue);

        for (int i = 0;
             i < m_data[xNode].size();
             ++i) {

            line->append(
                m_data[xNode][i],
                m_data[yNode][i]);
        }

        chart->addSeries(line);
    }

    if (style == "point" ||
        style == "both") {

        auto* points =
            new QScatterSeries();

        points->setName("Samples");
        points->setColor(Qt::green);
        points->setMarkerSize(8.0);

        for (int i = 0;
             i < m_data[xNode].size();
             ++i) {

            points->append(
                m_data[xNode][i],
                m_data[yNode][i]);
        }

        connect(
            points,
            &QScatterSeries::clicked,
            this,
            [this, xNode, yNode](const QPointF& p) {

                int bestStep = 0;
                double bestDistance =
                    std::numeric_limits<double>::max();

                for (int i = 0;
                     i < m_data[xNode].size();
                     ++i) {

                    const double dx =
                        m_data[xNode][i] - p.x();

                    const double dy =
                        m_data[yNode][i] - p.y();

                    const double distance =
                        dx * dx + dy * dy;

                    if (distance < bestDistance) {
                        bestDistance = distance;
                        bestStep = i;
                    }
                }

                m_infoLabel->setText(
                    QString(
                        "Step: %1    "
                        "y%2 = %3    "
                        "y%4 = %5")
                        .arg(bestStep)
                        .arg(xNode + 1)
                        .arg(
                            m_data[xNode][bestStep],
                            0,
                            'g',
                            17)
                        .arg(yNode + 1)
                        .arg(
                            m_data[yNode][bestStep],
                            0,
                            'g',
                            17));
            });

        chart->addSeries(points);
    }

    chart->createDefaultAxes();

    if (!chart->axes(Qt::Horizontal).isEmpty()) {
        chart->axes(Qt::Horizontal)
            .first()
            ->setTitleText(
                QString("y%1").arg(xNode + 1));
    }

    if (!chart->axes(Qt::Vertical).isEmpty()) {
        chart->axes(Qt::Vertical)
            .first()
            ->setTitleText(
                QString("y%1").arg(yNode + 1));
    }

    m_chartView->setChart(chart);

    m_infoLabel->setText(
        "Click a point to inspect its coordinates.");
}
