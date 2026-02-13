#ifndef BUTTONNETWORK_H
#define BUTTONNETWORK_H

#include <QWidget>
#include <QPushButton>
#include <QVector>
#include <QMap>
#include <QString>
#include <QColor>
#include <QTextEdit>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPointF>

struct GateConfig {
    bool enabled = false;
    QString baseType = "const"; // "const", "alpha1", "alpha2", "alpha3"
    double baseConst = 1.0;
    double coeff = 1.0;
    QString fn = "tanh";        // "sin", "tanh", "relu"
};

struct Connection {
    QPushButton* start = nullptr;
    QPushButton* end   = nullptr;
    QColor color;
    QString function; // "sin_exp", "tanh", "relu"
};

class ButtonNetwork : public QWidget
{
    Q_OBJECT
public:
    explicit ButtonNetwork(QWidget *parent = nullptr);

    void updateEquationEditor(QTextEdit* editor);

    // UI helpers / actions
    void clearNetwork();
    void setSolverMode(const QString& mode);
    void setTimeLimit(int t);
    void setAlpha2ScanRange(double minVal, double maxVal, double stepVal);
    void setAlpha2ScanSampling(int transientPercent, int sampleStride);

public slots:
    void computeResults();
    void showGraph();
    void showTable();
    void scanAlpha2();

    // Auto test preset
    void runAutoTestNode5Preset();

signals:
    void fileSaved(const QString& path);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *) override;

private slots:
    void buttonClicked();

private:
    // Build / edit connections
    void showFunctionDialog(QPushButton* start, QPushButton* end);
    QString buildTerm(const QString& from, const QString& to,
                      const QString& function, double val);

    // Gate helpers
    double baseValueFromType(const QString& baseType, double baseConst) const;
    double applyFn(const QString& fn, double x) const;
    double evalGateForNode(int nodeIndex, double yValue) const;

    // Save / run folder
    bool ensureBaseResultDir();
    bool createNewRunDir();
    QString runPath(const QString& filename) const;
    void writeRunInfoFile() const;
    void saveParams(const QString& path);

    // Solver core
    static double sinEFunction(double x);
    static double tanhFunction(double x);
    static double reluFunction(double x);
    double gammaWeight(int om, int r, double nu);
    void runODE();
    void runGamma();
    void saveAndDisplayResult(const QVector<QVector<double>>& y, int steps);

    // Table display
    void showOutputTable();

    // gnuplot
    void generateGnuplotScript();
    void generateAlpha2ScanGnuplotScripts() const;

    // Alpha2 scan
    void scanAlpha2ReuseCurrentRun();

    // Connection click-edit
    double distancePointToSegment(const QPointF& p, const QPointF& a, const QPointF& b) const;
    int findClickedConnectionIndex(const QPoint& pos) const;
    void editConnectionAt(int index);

    // Auto preset helpers
    bool copyOverwrite(const QString& src, const QString& dst) const;
    void ensurePresetNodes5();
    void addOrUpdateConnection(int from, int to, double w, const QString& fn);

private:
    // UI state
    QVector<QPushButton*> buttons;
    QVector<Connection> connections;
    QMap<QString, double> weightValues;
    QPushButton* firstSelected = nullptr;
    QTextEdit* equationEditor = nullptr;

    int maxNodes = 5;
    int connectionHitRadiusPx = 10;

    // Parameters
    QString solverMode = "ODE";
    int tMax = 800;          // steps
    double alpha1 = 1.0;
    double alpha2 = 1.0;
    double alpha3 = 1.0;
    double nu = 0.9;

    GateConfig gateNode4;
    GateConfig gateNode5;

    // Alpha2 scan settings
    double scanAlpha2Min = -10.0;
    double scanAlpha2Max =  10.0;
    double scanAlpha2Step = 0.5;
    int scanTransientPercent = 70;
    int scanSampleStride = 20;

    // Saving folders
    QString baseResultDir;
    QString currentRunDir;
};

#endif // BUTTONNETWORK_H
