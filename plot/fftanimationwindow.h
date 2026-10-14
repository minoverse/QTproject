#ifndef FFTANIMATIONWINDOW_H
#define FFTANIMATIONWINDOW_H

#include <QElapsedTimer>
#include <QVector>
#include <QWidget>

class QChartView;
class QChart;
class QComboBox;
class QLabel;
class QPushButton;
class QTimer;
class QLineSeries;
class QScatterSeries;
class QValueAxis;

class FFTAnimationWindow : public QWidget
{
    Q_OBJECT

public:
    explicit FFTAnimationWindow(
        const QString& runDir,
        const QString& graphStyle,
        QWidget* parent = nullptr);

private:
    bool loadData(const QString& runDir);
    void setupChart();
    void startAnimation();
    void updateFrame();
    void updateSpectrum(int sampleCount);

    QString m_graphStyle;

    QVector<QVector<double>> m_data;

    QComboBox* m_nodeCombo = nullptr;
    QLabel* m_infoLabel = nullptr;
    QLabel* m_progressLabel = nullptr;
    QPushButton* m_restartButton = nullptr;

    QChartView* m_chartView = nullptr;
    QChart* m_chart = nullptr;

    QLineSeries* m_lineSeries = nullptr;
    QScatterSeries* m_pointSeries = nullptr;

    QValueAxis* m_xAxis = nullptr;
    QValueAxis* m_yAxis = nullptr;

    QTimer* m_timer = nullptr;
    QElapsedTimer m_elapsed;

    int m_currentSamples = 0;

    static constexpr int AnimationDurationMs = 30000;
    static constexpr int FrameIntervalMs = 100;
};

#endif
