#ifndef INTERACTIVEPLOTWINDOW_H
#define INTERACTIVEPLOTWINDOW_H

#include <QWidget>
#include <QVector>
#include <QPointF>

class QComboBox;
class QLabel;

class InteractivePlotWindow : public QWidget
{
    Q_OBJECT

public:
    explicit InteractivePlotWindow(
        const QString& runDir,
        const QString& graphStyle,
        QWidget* parent = nullptr);

private:
    bool loadData(const QString& runDir);
    void rebuildChart();

    QString m_graphStyle;
    QVector<QVector<QPointF>> m_data;

    QComboBox* m_nodeCombo = nullptr;
    QLabel* m_infoLabel = nullptr;

    class QChartView* m_chartView = nullptr;
};

#endif
