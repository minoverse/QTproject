#ifndef PHASEPLOTWINDOW_H
#define PHASEPLOTWINDOW_H

#include <QPointF>
#include <QVector>
#include <QWidget>

class QComboBox;
class QLabel;
class QChartView;

class PhasePlotWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PhasePlotWindow(
        const QString& runDir,
        const QString& graphStyle,
        QWidget* parent = nullptr);

private:
    bool loadData(const QString& runDir);
    void rebuildChart();

    QString m_graphStyle;

    QVector<QVector<double>> m_data;

    QComboBox* m_xCombo = nullptr;
    QComboBox* m_yCombo = nullptr;
    QLabel* m_infoLabel = nullptr;
    QChartView* m_chartView = nullptr;
};

#endif
