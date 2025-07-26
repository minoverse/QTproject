#include "buttonnetwork.h"
#include <QVBoxLayout>
#include <QDialog>
#include <QLabel>
#include <QRadioButton>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QInputDialog>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QProcess>
#include <cmath>
#include <complex>

ButtonNetwork::ButtonNetwork(QWidget *parent) : QWidget(parent) {
    setFixedSize(800, 600);
    setMouseTracking(true);

    bool ok;
    int userInput = QInputDialog::getInt(this, "Number of Nodes", "How many nodes?", 5, 1, 100, 1, &ok);
    if (ok) maxNodes = userInput;
}

void ButtonNetwork::mousePressEvent(QMouseEvent *event) {
    if (buttons.size() >= maxNodes) return;

    QPushButton* btn = new QPushButton(QString::number(buttons.size() + 1), this);
    btn->setGeometry(event->pos().x(), event->pos().y(), 40, 40);
    btn->setStyleSheet("border-radius: 20px; background-color: lightgray;");
    btn->show();
    connect(btn, &QPushButton::clicked, this, &ButtonNetwork::buttonClicked);
    buttons.append(btn);
}

void ButtonNetwork::buttonClicked() {
    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton) return;

    if (!firstSelected) {
        firstSelected = clickedButton;
        firstSelected->setStyleSheet("border-radius: 20px; background-color: yellow;");
    } else {
        showFunctionDialog(firstSelected, clickedButton);
        firstSelected->setStyleSheet("border-radius: 20px; background-color: lightgray;");
        firstSelected = nullptr;
    }
}

void ButtonNetwork::showFunctionDialog(QPushButton* start, QPushButton* end) {
    QDialog dialog(this);
    dialog.setWindowTitle("Select Function");

    QVBoxLayout layout(&dialog);
    QLabel label("Choose function:", &dialog);
    QRadioButton sinExpButton("Sin", &dialog);
    QRadioButton tanhButton("Tanh", &dialog);
    QRadioButton reluButton("ReLU", &dialog);
    QPushButton confirmButton("OK", &dialog);

    layout.addWidget(&label);
    layout.addWidget(&sinExpButton);
    layout.addWidget(&tanhButton);
    layout.addWidget(&reluButton);
    layout.addWidget(&confirmButton);

    connect(&confirmButton, &QPushButton::clicked, [&]() {
        QColor color;
        QString functionType;
        if (sinExpButton.isChecked()) { color = Qt::yellow; functionType = "sin_exp"; }
        else if (tanhButton.isChecked()) { color = Qt::black; functionType = "tanh"; }
        else if (reluButton.isChecked()) { color = Qt::blue; functionType = "relu"; }

        QString weightName = "s" + start->text() + end->text();
        bool ok;
        double weightVal = QInputDialog::getDouble(this, "Weight Value", "Enter value for " + weightName + ":", 0.0, -1000, 1000, 2, &ok);
        if (ok) {
            weightValues[weightName] = weightVal;
            connections.append({start, end, color, functionType});
            update();
        }
        dialog.accept();
    });

    dialog.exec();
}

void ButtonNetwork::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QFont font = p.font();
    font.setBold(true);
    font.setPointSize(10);
    p.setFont(font);

    for (const auto& conn : connections) {
        QPoint start = conn.start->geometry().center();
        QPoint end = conn.end->geometry().center();
        QString label = "s" + conn.start->text() + conn.end->text();
        double val = weightValues.value(label, 0.0);
        p.setPen(QPen(conn.color, 3));

        if (conn.start == conn.end) {
            QRectF loop(start.x() - 20, start.y() - 40, 40, 40);
            p.drawArc(loop, 0, 360 * 16);
            p.drawText(start.x() - 30, start.y() - 50, label + "=" + QString::number(val));
        } else {
            QPainterPath path;
            QPointF mid = (start + end) / 2.0;
            QPointF offset(-(end.y() - start.y()), end.x() - start.x());
            if (offset.manhattanLength() > 0)
                offset /= std::sqrt(offset.x() * offset.x() + offset.y() * offset.y());

            offset *= 40;
            path.moveTo(start);
            path.quadTo(mid + offset, end);
            p.drawPath(path);
            p.drawText(mid + offset + QPointF(10, -10), label + "=" + QString::number(val));
        }
    }
}

QString ButtonNetwork::buildTerm(const QString& from, const QString&, const QString& function, double val) {
    if (function == "sin_exp") return QString::number(val) + "*sin(" + from + ")";
    if (function == "tanh") return QString::number(val) + "*tanh(" + from + ")";
    if (function == "relu") return QString::number(val) + "*relu(" + from + ")";
    return QString::number(val) + "*" + from;
}

void ButtonNetwork::computeResults() {
    bool ok;
    alpha1 = QInputDialog::getDouble(this, "Alpha1", "Enter α1:", 2.2, -100, 100, 2, &ok);
    if (!ok) return;
    alpha2 = QInputDialog::getDouble(this, "Alpha2", "Enter α2:", 2.0, -100, 100, 2, &ok);
    if (!ok) return;
    alpha3 = QInputDialog::getDouble(this, "Alpha3", "Enter α3:", 1.2, -100, 100, 2, &ok);
    if (!ok) return;

    if (solverMode == "ODE") runODE();
    else runGamma();
}

double ButtonNetwork::sinEFunction(double x) {
    return sin(x);
}

double ButtonNetwork::tanhFunction(double x) {
    return tanh(x);
}

double ButtonNetwork::reluFunction(double x) {
    return (x > 0) ? x : 0;
}

double ButtonNetwork::gammaWeight(int om, int r, double nu) {
    if (om - r == 0) return 1.0;
    double num = tgamma(om - r + nu);
    double denom = tgamma(om - r + 1) * tgamma(nu);
    return num / denom;
}

void ButtonNetwork::runODE() {
    const double h = 0.01;
    const int steps = tMax;
    QVector<QVector<double>> y(5, QVector<double>(steps + 1));
    y[0][0] = 0.8;
    y[1][0] = 0.3;
    y[2][0] = 0.4;
    y[3][0] = 0.6;
    y[4][0] = 0.7;

    for (int t = 1; t <= steps; ++t) {
        for (int i = 0; i < 5; ++i) {
            double sum = -y[i][t - 1];

            for (const auto& conn : connections) {
                int from = conn.start->text().toInt() - 1;
                int to = conn.end->text().toInt() - 1;
                if (to != i) continue;

                QString key = "s" + conn.start->text() + conn.end->text();
                double weight = weightValues.value(key, 0.0);
                double input = y[from][t - 1];

                if (conn.function == "sin_exp")
                    sum += weight * sinEFunction(input);
                else if (conn.function == "tanh")
                    sum += weight * tanhFunction(input);
                else if (conn.function == "relu")
                    sum += weight * reluFunction(input);
            }

            if (i == 3)
                sum += (alpha2 - alpha3 * sinEFunction(y[4][t - 1])) * tanhFunction(y[3][t - 1]);
            if (i == 4)
                sum += (1 - alpha1 * tanhFunction(y[2][t - 1])) * tanhFunction(y[4][t - 1]);

            y[i][t] = y[i][t - 1] + h * sum;
        }
    }

    saveAndDisplayResult(y, steps);
}

void ButtonNetwork::runGamma() {
    const int steps = tMax;
    const double nu = 0.7;
    QVector<QVector<double>> y(5, QVector<double>(steps + 1));
    y[0][0] = 0.8;
    y[1][0] = 0.3;
    y[2][0] = 0.4;
    y[3][0] = 0.6;
    y[4][0] = 0.7;

    for (int om = 1; om <= steps; ++om) {
        for (int i = 0; i < 5; ++i) {
            double acc = 0.0;

            for (int r = 1; r <= om; ++r) {
                double sum = -y[i][r - 1];

                for (const auto& conn : connections) {
                    int from = conn.start->text().toInt() - 1;
                    int to = conn.end->text().toInt() - 1;
                    if (to != i) continue;

                    QString key = "s" + conn.start->text() + conn.end->text();
                    double weight = weightValues.value(key, 0.0);
                    double input = y[from][r - 1];

                    if (conn.function == "sin_exp")
                        sum += weight * sinEFunction(input);
                    else if (conn.function == "tanh")
                        sum += weight * tanhFunction(input);
                    else if (conn.function == "relu")
                        sum += weight * reluFunction(input);
                }

                if (i == 3)
                    sum += (alpha2 - alpha3 * sinEFunction(y[4][r - 1])) * tanhFunction(y[3][r - 1]);
                if (i == 4)
                    sum += (1 - alpha1 * tanhFunction(y[2][r - 1])) * tanhFunction(y[4][r - 1]);

                acc += sum * gammaWeight(om, r, nu);
            }

            y[i][om] = y[i][0] + acc;
        }
    }

    saveAndDisplayResult(y, steps);
}

void ButtonNetwork::saveAndDisplayResult(const QVector<QVector<double>>& y, int steps) {
    QFile file("result.dat");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not write result.dat");
        return;
    }

    QTextStream out(&file);
    for (int t = 0; t <= steps; ++t) {
        for (int i = 0; i < 5; ++i) {
            out << QString::number(y[i][t], 'f', 6) << (i < 4 ? " " : "\n");
        }
    }
    file.close();

    if (equationEditor) {
        QFile f("result.dat");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f);
            QString content = "=== Computation Result ===\n";
            content += in.readAll();
            equationEditor->setPlainText(content);
            f.close();
        }
    }
}
