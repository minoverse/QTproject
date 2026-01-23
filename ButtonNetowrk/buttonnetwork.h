#ifndef BUTTONNETWORK_H
#define BUTTONNETWORK_H

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QPainter>
#include <QMouseEvent>
#include <QVector>
#include <QMap>
#include <QTextEdit>
#include <QString>
#include <cmath>
#include <complex>

struct Connection {
    QPushButton* start;
    QPushButton* end;
    QColor color;
    QString function;
};

class ButtonNetwork : public QWidget {
    Q_OBJECT

public:
    explicit ButtonNetwork(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

    void updateEquationEditor(QTextEdit* editor);
    QString buildTerm(const QString& from,
                      const QString& weight,
                      const QString& function,
                      double val);

public slots:
    void computeResults();
    void clearNetwork();
    void setSolverMode(const QString& mode);
    void setTimeLimit(int t);

    // NEW: alpha2 scan UI bindings (optional)
    void setAlpha2ScanRange(double minVal, double maxVal, double stepVal);
    void setAlpha2ScanSampling(int transientPercent, int sampleStride);

    void showGraph();
    void showTable();
    void showOutputTable();

    // ============================================================
    // alpha2 scan (surface + bifurcation projection)
    // - alpha2_scan_3d.dat : columns alpha2 t y1 y2 y3 y4 y5
    // - alpha2_scan_2d.dat : columns alpha2 y1 y2 y3 y4 y5 (many rows per alpha2 + blank line)
    // - gnuplot -> alpha2_y1.png ... alpha2_y5.png (dots)
    // ============================================================
    void scanAlpha2();
    void runAutoTestNode5Preset();   // NEW: Auto Test (Node5 preset) 1회 실행
    void scanAlpha2ReuseCurrentRun();   // NEW: 현재 currentRunDir 재사용 (새 run 생성 안 함)

signals:
    void fileSaved(const QString& filePath);

private slots:
    void buttonClicked();
    void showFunctionDialog(QPushButton* start, QPushButton* end);

private:
    QVector<QPushButton*> buttons;
    QVector<Connection> connections;
    QMap<QString, double> weightValues;
    QPushButton* firstSelected = nullptr;
    int maxNodes = 5;
    QString solverMode = "ODE";
    int tMax = 5000;
    QTextEdit* equationEditor = nullptr;

    double alpha1 = 2.2, alpha2 = 2.0, alpha3 = 1.2;
    double nu = 0.7;
    void ensurePresetNodes5(); // nodes 1~5 없으면 자동 생성(위치 고정)
    void addOrUpdateConnection(int from, int to, double w, const QString& fn); // 연결 추가/갱신
    bool copyOverwrite(const QString& src, const QString& dst) const; // test_* 복제용

    // ------------------------------------------------------------
    // alpha2 scan parameters (can be driven by UI; defaults mirror dialogs)
    // ------------------------------------------------------------
    double scanAlpha2Min = -10.0;
    double scanAlpha2Max = 10.0;
    double scanAlpha2Step = 0.5;
    // transientPercent: discard first N% of steps (bifurcation diagram)
    int scanTransientPercent = 70;
    // sampleStride: take every k-th point after transient
    int scanSampleStride = 20;

    // ============================================================
    // Gate model (G1/G2) for node 4 and 5 self-loops
    // Node4(index 3): G2 = base - coeff * fn(y4)
    // Node5(index 4): G1 = base - coeff * fn(y5)
    // ============================================================
    struct GateConfig {
        bool enabled = false;
        QString baseType = "alpha2"; // "const", "alpha1", "alpha2", "alpha3"
        double baseConst = 1.0;      // used if baseType=="const"
        double coeff = 1.2;          // multiplier for fn(y)
        QString fn = "sin";          // "sin", "tanh", "relu"
    };

    GateConfig gateNode4; // node4 self-loop => G2
    GateConfig gateNode5; // node5 self-loop => G1

    double baseValueFromType(const QString& baseType, double baseConst) const;
    double applyFn(const QString& fn, double x) const;
    double evalGateForNode(int nodeIndex, double yValue) const; // nodeIndex is 3 or 4

    // ------------------------------------------------------------
    // Auto-save folder system
    // ------------------------------------------------------------
    QString baseResultDir;
    QString currentRunDir;

    bool ensureBaseResultDir();
    bool createNewRunDir();
    QString runPath(const QString& filename) const;
    void writeRunInfoFile() const;

    void saveParams(const QString& path);

    // ------------------------------------------------------------
    // Plot / solvers
    // ------------------------------------------------------------
    void generateGnuplotScript();
    void runODE();
    void runGamma();

    double sinEFunction(double x);
    double tanhFunction(double x);
    double reluFunction(double x);
    double gammaWeight(int om, int r, double nu);
    void saveAndDisplayResult(const QVector<QVector<double>>& y, int steps);

    // ------------------------------------------------------------
    // alpha2 scan internals
    // ------------------------------------------------------------
    void generateAlpha2ScanGnuplotScripts() const;
    void generateAlpha2FinalLineGnuplotScripts() const;
    // ============================================================
    // Connection click-edit implementation
    // ============================================================
    double distancePointToSegment(const QPointF& p,
                                  const QPointF& a,
                                  const QPointF& b) const;
    int findClickedConnectionIndex(const QPoint& pos) const;
    void editConnectionAt(int index);

    int connectionHitRadiusPx = 10; // click tolerance (px)
};

#endif // BUTTONNETWORK_H
