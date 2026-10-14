#include "interactiveplotwindow.h"

#include <QComboBox>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QVBoxLayout>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

InteractivePlotWindow::InteractivePlotWindow(
    const QString& runDir,
    const QString& graphStyle,
    QWidget* parent)
    : QWidget(parent),
      m_graphStyle(graphStyle)
{
    setWindowTitle("Interactive Plot");
    resize(1100, 750);
    setAttribute(Qt::WA_DeleteOnClose);

    auto* layout = new QVBoxLayout(this);

    auto* controls = new QHBoxLayout();

    controls->addWidget(new QLabel("Node:"));

    m_nodeCombo = new QComboBox();
    for (int i = 1; i <= 5; ++i)
        m_nodeCombo->addItem(QString("y%1").arg(i));

    controls->addWidget(m_nodeCombo);
    controls->addStretch();

    layout->addLayout(controls);

    m_chartView = new QChartView();
    m_chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(m_chartView, 1);

    m_infoLabel = new QLabel(
        "Click a point to inspect its coordinates.");
    m_infoLabel->setMinimumHeight(35);

    layout->addWidget(m_infoLabel);

    if (!loadData(runDir)) {
        QMessageBox::critical(
            this,
            "Interactive Plot",
            "Cannot load result.dat");
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

bool InteractivePlotWindow::loadData(
    const QString& runDir)
{
    QFile file(runDir + "/result.dat");

    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text))
        return false;

    m_data =
        QVector<QVector<QPointF>>(5);

    QTextStream in(&file);

    int step = 0;

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

        for (int node = 0;
             node < 5;
             ++node) {

            bool ok = false;
            const double value =
                parts[node].toDouble(&ok);

            if (!ok)
                return false;

            m_data[node].append(
                QPointF(step, value));
        }

        ++step;
    }

    return step > 0;
}

void InteractivePlotWindow::rebuildChart()
{
    if (m_data.isEmpty())
        return;

    const int node =
        m_nodeCombo->currentIndex();

    if (node < 0 || node >= 5)
        return;

    auto* chart = new QChart();

    chart->setTitle(
        QString("Interactive Plot - y%1")
            .arg(node + 1));

    chart->legend()->setVisible(true);

    const QString style =
        m_graphStyle.trimmed().toLower();

    if (style == "line" ||
        style == "both") {

        auto* line =
            new QLineSeries();

        line->setName(
            QString("y%1 line")
                .arg(node + 1));

        line->setColor(Qt::blue);

        for (const QPointF& p :
             m_data[node]) {
            line->append(p);
        }

        chart->addSeries(line);
    }

    if (style == "point" ||
        style == "both") {

        auto* points =
            new QScatterSeries();

        points->setName(
            QString("y%1 points")
                .arg(node + 1));

        points->setColor(
            Qt::green);

        points->setMarkerSize(8.0);

        for (const QPointF& p :
             m_data[node]) {
            points->append(p);
        }

        connect(
            points,
            &QScatterSeries::clicked,
            this,
            [this, node](const QPointF& p) {

                m_infoLabel->setText(
                    QString(
                        "Node: y%1    "
                        "Step: %2    "
                        "Value: %3")
                        .arg(node + 1)
                        .arg(
                            static_cast<int>(
                                p.x()))
                        .arg(
                            p.y(),
                            0,
                            'g',
                            17));
            });

        chart->addSeries(points);
    }

    chart->createDefaultAxes();

    if (!chart->axes(
            Qt::Horizontal)
             .isEmpty()) {

        chart->axes(
            Qt::Horizontal)
            .first()
            ->setTitleText("Step");
    }

    if (!chart->axes(
            Qt::Vertical)
             .isEmpty()) {

        chart->axes(
            Qt::Vertical)
            .first()
            ->setTitleText(
                QString("y%1")
                    .arg(node + 1));
    }

    m_chartView->setChart(chart);

    m_infoLabel->setText(
        "Click a point to inspect its coordinates.");
}
