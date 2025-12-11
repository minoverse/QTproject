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
    void showGraph();
    void showTable();
    void showOutputTable();

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
    void generateGnuplotScript();
    void runODE();
    void runGamma();

    double sinEFunction(double x);
    double tanhFunction(double x);
    double reluFunction(double x);
    double gammaWeight(int om, int r, double nu);
    void saveAndDisplayResult(const QVector<QVector<double>>& y, int steps);
};

#endif // BUTTONNETWORK_H
