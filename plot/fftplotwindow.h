#ifndef FFTPLOTWINDOW_H
#define FFTPLOTWINDOW_H

#include <QPointF>
#include <QVector>
#include <QWidget>

class QChartView;
class QComboBox;
class QLabel;

class FFTPlotWindow : public QWidget
{
    Q_OBJECT

public:
    explicit FFTPlotWindow(
        const QString& fftFile,
        const QString& graphStyle,
        QWidget* parent = nullptr);

private:
    bool loadData(const QString& fftFile);
    void rebuildChart();

    QString m_graphStyle;

    QVector<double> m_frequency;
    QVector<QVector<double>> m_magnitude;

    QComboBox* m_nodeCombo = nullptr;
    QLabel* m_infoLabel = nullptr;
    QChartView* m_chartView = nullptr;
};

#endif
