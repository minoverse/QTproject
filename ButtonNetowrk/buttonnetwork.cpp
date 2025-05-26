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
#include <cmath>
#include <complex>

ButtonNetwork::ButtonNetwork(QWidget *parent) : QWidget(parent) {
    setFixedSize(800, 600);
    setMouseTracking(true);

    bool ok;
    int userInput = QInputDialog::getInt(
        this, "Number of Nodes", "How many nodes do you want to create?", 5, 1, 100, 1, &ok);
    if (ok) {
        maxNodes = userInput;
    }
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
    QRadioButton sinExpButton("1. Sin", &dialog);
    QRadioButton tanhButton("2. Tanh", &dialog);
    QRadioButton reluButton("3. ReLU", &dialog);
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
        double weightVal = QInputDialog::getDouble(this, "Weight Value",
                                                   "Enter value for " + weightName + ":", 0.0, -1000, 1000, 2, &ok);
        if (ok) {
            weightValues[weightName] = weightVal;
            connections.append({start, end, color, functionType});
            update();
        }
        dialog.accept();
    });

    dialog.exec();
}

void ButtonNetwork::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QFont font = painter.font();
    font.setPointSize(11);
    font.setBold(true);
    painter.setFont(font);

    for (int i = 0; i < connections.size(); ++i) {
        const auto& conn = connections[i];
        QPoint start = conn.start->geometry().center();
        QPoint end = conn.end->geometry().center();

        QString label = "s" + conn.start->text() + conn.end->text();
        double weight = weightValues.value(label, 0.0);
        painter.setPen(QPen(conn.color, 3));

        if (conn.start == conn.end) {
            int radius = 20;
            QRectF loopRect(start.x() - radius, start.y() - radius - 20, 2 * radius, 2 * radius);
            painter.drawArc(loopRect, 30 * 16, 300 * 16);
            painter.drawText(start.x() - 15, start.y() - 35, label + " = " + QString::number(weight));
        } else {
            QPainterPath path;
            QPointF control;

            int dx = end.x() - start.x();
            int dy = end.y() - start.y();
            QPointF normal(-dy, dx);
            double length = std::sqrt(normal.x() * normal.x() + normal.y() * normal.y());
            if (length != 0) normal /= length;

            int offsetCount = 0;
            for (int j = 0; j < i; ++j) {
                const auto& prev = connections[j];
                if ((prev.start == conn.start && prev.end == conn.end) ||
                    (prev.start == conn.end && prev.end == conn.start)) {
                    ++offsetCount;
                }
            }

            double curveHeight = 30.0 + offsetCount * 15.0;
            int directionFlag = (conn.start->text().toInt() < conn.end->text().toInt()) ? 1 : -1;
            QPointF offset = normal * curveHeight * directionFlag;

            control = (start + end) / 2 + offset;
            path.moveTo(start);
            path.quadTo(control, end);
            painter.drawPath(path);

            QPointF labelPos = (start + end) / 2 + offset + QPointF(10, -10);
            painter.drawText(labelPos, label + " = " + QString::number(weight));
        }
    }

    painter.setPen(Qt::darkRed);
    QFont labelFont = painter.font();
    labelFont.setPointSize(10);
    labelFont.setBold(true);
    painter.setFont(labelFont);

    for (auto btn : buttons) {
        QString node = btn->text();
        QPoint center = btn->geometry().center();
        for (const auto& conn : connections) {
            if (conn.start == conn.end) {
                if (node == "4" && conn.start->text() == "4") {
                    painter.drawText(center.x() - 60, center.y() - 50, "G₂ = α₂ - α₃ sin(y₄)");
                }
                if (node == "5" && conn.start->text() == "5") {
                    painter.drawText(center.x() - 60, center.y() - 50, "G₁ = 1 - α₁ tanh(y₅)");
                }
            }
        }
    }
}

void ButtonNetwork::computeResults() {
    bool ok;
    double alpha1 = QInputDialog::getDouble(this, "Alpha1", "Enter α1:", 2.2, -100, 100, 2, &ok);
    double alpha2 = QInputDialog::getDouble(this, "Alpha2", "Enter α2:", 2.0, -100, 100, 2, &ok);
    double alpha3 = QInputDialog::getDouble(this, "Alpha3", "Enter α3:", 1.2, -100, 100, 2, &ok);

    QMap<QString, QStringList> functionTerms;

    for (const auto& conn : connections) {
        QString from = "y" + conn.end->text();  // the influencing/source node
        QString to = "y" + conn.start->text();  // the node whose equation we're building
        QString weight = "s" + conn.end->text() + conn.start->text(); // sji
        //


        //
        double val = weightValues.value(weight, 1.0);

        QString term;
        if (conn.start->text() == "4" && conn.end->text() == "4") {
            term = QString("(%1 - %2*sin(y4))").arg(alpha2).arg(alpha3);
        } else if (conn.start->text() == "5" && conn.end->text() == "5") {
            term = QString("(1 - %1*tanh(y5))").arg(alpha1);
        } else {
            if (conn.function == "sin_exp")
                term = QString::number(val) + "*sin(" + from + ")";
            else if (conn.function == "tanh")
                term = QString::number(val) + "*tanh(" + from + ")";
            else if (conn.function == "relu")
                term = QString::number(val) + "*relu(" + from + ")";
            else
                term = QString::number(val) + "*" + from;
        }

        functionTerms[to].append(term);
    }

    QString filePath = QFileDialog::getSaveFileName(this, "Save Graph Equations", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);

        out << "=== Differential Equations ===\n";
        for (int i = 0; i < buttons.size(); ++i) {
            QString label = "y" + buttons[i]->text();
            out << "Dy" << buttons[i]->text() << "(w) = -" << label;
            if (functionTerms.contains(label)) {
                for (const QString& term : functionTerms[label]) {
                    out << " + " << term;
                }
            }
            out << "\n";
        }

        out << "\n=== Weight Values (sij) ===\n";
        for (auto it = weightValues.begin(); it != weightValues.end(); ++it) {
            out << it.key() << " = " << it.value() << "\n";
        }

        out << "\n=== Alpha Constants ===\n";
        out << "α1 = " << alpha1 << "\n";
        out << "α2 = " << alpha2 << "\n";
        out << "α3 = " << alpha3 << "\n";

        file.close();
        QMessageBox::information(this, "Saved", "Equations saved to:\n" + filePath);
        emit fileSaved(filePath);
        if (equationEditor) {
            QFile reload(filePath);
            if (reload.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&reload);
                equationEditor->setPlainText(in.readAll());
            }
        }
    }
}

void ButtonNetwork::clearNetwork() {
    for (auto btn : buttons) {
        delete btn;
    }
    buttons.clear();
    connections.clear();
    weightValues.clear();
    update();
    if (equationEditor) equationEditor->clear();
}

void ButtonNetwork::updateEquationEditor(QTextEdit* editor) {
    equationEditor = editor;
}

double ButtonNetwork::sinEFunction(double x) {
    std::complex<double> i(0, 1);
    return (exp(i * x) - exp(-i * x)).real() / (2.0 * i).real();
}

double ButtonNetwork::tanhFunction(double x) {
    double exp2x = exp(-2 * x);
    return (1 - exp2x) / (1 + exp2x);
}

double ButtonNetwork::reluFunction(double x) {
    return (x > 0) ? x : 0.0;
}
