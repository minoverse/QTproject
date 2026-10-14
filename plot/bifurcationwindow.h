#ifndef BIFURCATIONWINDOW_H
#define BIFURCATIONWINDOW_H

#include <QElapsedTimer>
#include <QPointF>
#include <QVector>
#include <QWidget>

class QChart;
class QChartView;
class QComboBox;
class QLabel;
class QPushButton;
class QTimer;
class QLineSeries;
class QScatterSeries;
class QValueAxis;

class BifurcationWindow : public QWidget
{
    Q_OBJECT

public:
    explicit BifurcationWindow(
        const QString& dataFile,
        const QString& graphStyle,
        QWidget* parent = nullptr);

private:
    bool loadData(const QString& dataFile);
    void setupChart();
    void startAnimation();
    void updateFrame();
    void updatePlot(int rowCount);
    void showFullDiagram();

    QString m_graphStyle;

    QVector<double> m_alpha;
    QVector<QVector<double>> m_y;

    QComboBox* m_nodeCombo = nullptr;
    QLabel* m_infoLabel = nullptr;
    QLabel* m_progressLabel = nullptr;
    QPushButton* m_restartButton = nullptr;
    QPushButton* m_fullButton = nullptr;

    QChart* m_chart = nullptr;
    QChartView* m_chartView = nullptr;
    QLineSeries* m_lineSeries = nullptr;
    QScatterSeries* m_pointSeries = nullptr;
    QValueAxis* m_xAxis = nullptr;
    QValueAxis* m_yAxis = nullptr;

    QTimer* m_timer = nullptr;
    QElapsedTimer m_elapsed;

    int m_currentRows = 0;

    static constexpr int AnimationDurationMs = 30000;
    static constexpr int FrameIntervalMs = 100;
};

#endif
