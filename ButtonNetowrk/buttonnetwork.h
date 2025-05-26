// === UPDATED buttonnetwork.h ===
#ifndef BUTTONNETWORK_H
#define BUTTONNETWORK_H

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QPainter>
#include <QMouseEvent>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QPair>
#include <QTextEdit>
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
    QString buildTerm(const QString& from, const QString& weight, const QString& function, double val);//added


signals:
    void fileSaved(const QString& filePath);

public slots:
    void computeResults();
    void clearNetwork();

private slots:
    void buttonClicked();
    void showFunctionDialog(QPushButton* start, QPushButton* end);

private:
    QVector<QPushButton*> buttons;
    QVector<Connection> connections;
    QMap<QPushButton*, double> values;
    QMap<QString, double> weightValues;
    QPushButton* firstSelected = nullptr;
    int maxNodes = 5;
    QTextEdit* equationEditor = nullptr;  // Link to external text editor for live updates

    double sinEFunction(double x);
    double tanhFunction(double x);
    double reluFunction(double x);
};

#endif // BUTTONNETWORK_H
