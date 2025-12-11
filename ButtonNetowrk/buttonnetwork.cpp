#include "buttonnetwork.h"

#include <QVBoxLayout>
#include <QDialog>
#include <QLabel>
#include <QRadioButton>
#include <QInputDialog>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QProcess>
#include <QPainter>
#include <QPainterPath>
#include <cmath>
#include <complex>

ButtonNetwork::ButtonNetwork(QWidget *parent) : QWidget(parent) {
    setFixedSize(800, 600);
    setMouseTracking(true);

    bool ok;
    int userInput = QInputDialog::getInt(
        this, "Number of Nodes", "How many nodes?", 5, 1, 100, 1, &ok);
    if (ok)
        maxNodes = userInput;
}

void ButtonNetwork::updateEquationEditor(QTextEdit* editor)
{
    equationEditor = editor;
}

void ButtonNetwork::mousePressEvent(QMouseEvent *event) {
    if (buttons.size() >= maxNodes)
        return;

    QPushButton* btn = new QPushButton(QString::number(buttons.size() + 1), this);
    btn->setGeometry(event->pos().x(), event->pos().y(), 40, 40);
    btn->setStyleSheet("border-radius: 20px; background-color: lightgray;");
    btn->show();
    connect(btn, &QPushButton::clicked, this, &ButtonNetwork::buttonClicked);
    buttons.append(btn);
}

void ButtonNetwork::buttonClicked() {
    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton)
        return;

    if (!firstSelected) {
        firstSelected = clickedButton;
        firstSelected->setStyleSheet(
            "border-radius: 20px; background-color: yellow;");
    } else {
        showFunctionDialog(firstSelected, clickedButton);
        firstSelected->setStyleSheet(
            "border-radius: 20px; background-color: lightgray;");
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

        // NOTE: key is s(end)(start), consistent with runODE/runGamma
        QString weightName = "s" + end->text() + start->text();
        bool ok;
        double weightVal = QInputDialog::getDouble(
            this, "Weight Value",
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
        QString label = "s" + conn.end->text() + conn.start->text();
        double val = weightValues.value(label, 0.0);

        p.setPen(QPen(conn.color, 3));

        if (conn.start == conn.end) {
            QRectF loop(start.x() - 20, start.y() - 40, 40, 40);
            p.drawArc(loop, 0, 360 * 16);
            p.drawText(start.x() - 30, start.y() - 50,
                       label + "=" + QString::number(val));
        } else {
            QPainterPath path;
            QPointF mid = (start + end) / 2.0;
            QPointF offset(-(end.y() - start.y()), end.x() - start.x());
            if (offset.manhattanLength() > 0)
                offset /= std::sqrt(offset.x() * offset.x()
                                    + offset.y() * offset.y());

            offset *= 40;
            path.moveTo(start);
            path.quadTo(mid + offset, end);
            p.drawPath(path);
            p.drawText(mid + offset + QPointF(10, -10),
                       label + "=" + QString::number(val));
        }
    }
}

QString ButtonNetwork::buildTerm(const QString& from,
                                 const QString&,
                                 const QString& function,
                                 double val)
{
    if (function == "sin_exp")
        return QString::number(val) + "*sin(" + from + ")";
    if (function == "tanh")
        return QString::number(val) + "*tanh(" + from + ")";
    if (function == "relu")
        return QString::number(val) + "*relu(" + from + ")";
    return QString::number(val) + "*" + from;
}

void ButtonNetwork::computeResults() {
    bool ok;
    alpha1 = QInputDialog::getDouble(this, "Alpha1", "Enter α1:",
                                     2.2, -100, 100, 2, &ok);
    if (!ok) return;
    alpha2 = QInputDialog::getDouble(this, "Alpha2", "Enter α2:",
                                     2.0, -100, 100, 2, &ok);
    if (!ok) return;
    alpha3 = QInputDialog::getDouble(this, "Alpha3", "Enter α3:",
                                     1.2, -100, 100, 2, &ok);
    if (!ok) return;

    if (solverMode == "ODE")
        runODE();
    else
        runGamma();
}

double ButtonNetwork::sinEFunction(double x) { return std::sin(x); }
double ButtonNetwork::tanhFunction(double x) { return std::tanh(x); }
double ButtonNetwork::reluFunction(double x) { return (x > 0) ? x : 0; }

double ButtonNetwork::gammaWeight(int om, int r, double nu) {
    if (om - r == 0) return 1.0;
    double num = std::tgamma(om - r + nu);
    double denom = std::tgamma(om - r + 1) * std::tgamma(nu);
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
            QCoreApplication::processEvents();QCoreApplication::processEvents(); //add
            double sum = -y[i][t - 1];

            for (const auto& conn : connections) {
                int from = conn.start->text().toInt() - 1;
                int to   = conn.end->text().toInt() - 1;
                if (to != i) continue;

                QString key = "s" + conn.end->text() + conn.start->text();
                double weight = weightValues.value(key, 0.0);
                double input  = y[from][t - 1];

                if (conn.function == "sin_exp")
                    sum += weight * sinEFunction(input);
                else if (conn.function == "tanh")
                    sum += weight * tanhFunction(input);
                else if (conn.function == "relu")
                    sum += weight * reluFunction(input);
            }

            if (i == 3)
                sum += (alpha2 - alpha3 * sinEFunction(y[4][t - 1]))
                       * tanhFunction(y[3][t - 1]);
            if (i == 4)
                sum += (1 - alpha1 * tanhFunction(y[2][t - 1]))
                       * tanhFunction(y[4][t - 1]);

            y[i][t] = y[i][t - 1] + h * sum;
        }
    }

    saveAndDisplayResult(y, steps);
}

void ButtonNetwork::runGamma() {
    const int steps = tMax;
    const double nu = 0.7; // uup to user decision
    QVector<QVector<double>> y(5, QVector<double>(steps + 1));
    y[0][0] = 0.8;
    y[1][0] = 0.3;
    y[2][0] = 0.4;
    y[3][0] = 0.6;
    y[4][0] = 0.7;

    for (int om = 1; om <= steps; ++om) {
        QCoreApplication::processEvents(); //add
        for (int i = 0; i < 5; ++i) {
            double acc = 0.0;

            for (int r = 1; r <= om; ++r) {
                double sum = -y[i][r - 1];

                for (const auto& conn : connections) {
                    int from = conn.start->text().toInt() - 1;
                    int to   = conn.end->text().toInt() - 1;
                    if (to != i) continue;

                    QString key = "s" + conn.end->text() + conn.start->text();
                    double weight = weightValues.value(key, 0.0);
                    double input  = y[from][r - 1];

                    if (conn.function == "sin_exp")
                        sum += weight * sinEFunction(input);
                    else if (conn.function == "tanh")
                        sum += weight * tanhFunction(input);
                    else if (conn.function == "relu")
                        sum += weight * reluFunction(input);
                }

                if (i == 3)
                    sum += (alpha2 - alpha3 * sinEFunction(y[4][r - 1]))
                           * tanhFunction(y[3][r - 1]);
                if (i == 4)
                    sum += (1 - alpha1 * tanhFunction(y[2][r - 1]))
                           * tanhFunction(y[4][r - 1]);

                acc += sum * gammaWeight(om, r, nu);
            }

            y[i][om] = y[i][0] + acc;
        }
    }

    saveAndDisplayResult(y, steps);
}

void ButtonNetwork::saveAndDisplayResult(const QVector<QVector<double>>& y,
                                         int steps)
{
    QFile file("result.dat");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not write result.dat");
        return;
    }

    QTextStream out(&file);
    for (int t = 0; t <= steps; ++t) {
        for (int i = 0; i < 5; ++i) {
            out << QString::number(y[i][t], 'f', 6)
                << (i < 4 ? " " : "\n");
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

// --------- extra UI helpers ---------

void ButtonNetwork::clearNetwork()
{
    for (QPushButton* btn : buttons) {
        if (btn)
            btn->deleteLater();
    }
    buttons.clear();
    connections.clear();
    weightValues.clear();
    firstSelected = nullptr;

    if (equationEditor)
        equationEditor->clear();

    update();
}

void ButtonNetwork::setSolverMode(const QString& mode)
{
    solverMode = mode;
}

void ButtonNetwork::setTimeLimit(int t)
{
    tMax = t;
}

void ButtonNetwork::showGraph()
{
    QFile f("result.dat");
    if (!f.exists()) {
        QMessageBox::warning(this, "Error",
                             "No result.dat file found. Run Compute first.");
        return;
    }

    generateGnuplotScript();

    QProcess proc;
    proc.start("gnuplot", QStringList() << "plot.gnu");
    if (!proc.waitForFinished()) {
        QMessageBox::critical(this, "Error",
                              "Failed to run gnuplot. Is it installed?");
        return;
    }

#ifdef Q_OS_LINUX
    QProcess::startDetached("xdg-open", QStringList() << "y_all.png");
#elif defined(Q_OS_MAC)
    QProcess::startDetached("open", QStringList() << "y_all.png");
#elif defined(Q_OS_WIN)
    QProcess::startDetached("y_all.png");
#endif
}

void ButtonNetwork::showTable()
{
    showOutputTable();
}

void ButtonNetwork::showOutputTable()
{
    if (!equationEditor) {
        QMessageBox::warning(this, "Error",
                             "Output panel is not connected.");
        return;
    }

    QFile file("result.dat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error",
                              "Could not open result.dat");
        return;
    }

    QTextStream in(&file);

    QString output;
    output += "t_step\t   y1\t\t   y2\t\t   y3\t\t   y4\t\t   y5\n";
    output += "-------------------------------------------------------------\n";

    int step = 0;
    while (!in.atEnd()) {
        double y1, y2, y3, y4, y5;
        in >> y1 >> y2 >> y3 >> y4 >> y5;
        if (in.status() != QTextStream::Ok)
            break;

        output += QString("%1\t%2\t%3\t%4\t%5\t%6\n")
                      .arg(step)
                      .arg(y1, 0, 'f', 6)
                      .arg(y2, 0, 'f', 6)
                      .arg(y3, 0, 'f', 6)
                      .arg(y4, 0, 'f', 6)
                      .arg(y5, 0, 'f', 6);
        ++step;
    }

    file.close();
    equationEditor->setPlainText(output);
}

void ButtonNetwork::generateGnuplotScript()
{
    QFile script("plot.gnu");
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot write plot.gnu");
        return;
    }

    QTextStream out(&script);

    // ONE PNG with 5 stacked plots (multiplot)
    out << "set term png size 800,1200\n";
    out << "set output 'y_all.png'\n";
    out << "set multiplot layout 5,1 title 'States y1..y5'\n";
    out << "set xrange [0:*]\n";

    // y1 - blue
    out << "set xlabel 't (step)'\n";
    out << "set ylabel 'y1'\n";
    out << "plot 'result.dat' using 0:1 with lines lc rgb 'blue' linewidth 2 title 'y1'\n\n";

    // y2 - red
    out << "set xlabel 't (step)'\n";
    out << "set ylabel 'y2'\n";
    out << "plot 'result.dat' using 0:2 with lines lc rgb 'red' linewidth 2 title 'y2'\n\n";

    // y3 - green
    out << "set xlabel 't (step)'\n";
    out << "set ylabel 'y3'\n";
    out << "plot 'result.dat' using 0:3 with lines lc rgb 'green' linewidth 2 title 'y3'\n\n";

    // y4 - black
    out << "set xlabel 't (step)'\n";
    out << "set ylabel 'y4'\n";
    out << "plot 'result.dat' using 0:4 with lines lc rgb 'black' linewidth 2 title 'y4'\n\n";

    // y5 - purple
    out << "set xlabel 't (step)'\n";
    out << "set ylabel 'y5'\n";
    out << "plot 'result.dat' using 0:5 with lines lc rgb 'purple' linewidth 2 title 'y5'\n\n";

    out << "unset multiplot\n";

    script.close();
}
