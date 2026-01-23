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
#include <QDoubleSpinBox>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <cmath>
#include <complex>
#include <QCheckBox>
#include <QComboBox>
ButtonNetwork::ButtonNetwork(QWidget *parent) : QWidget(parent)
{
    // setFixedSize(800, 600);
    setMouseTracking(true);

    bool ok;
    int userInput = QInputDialog::getInt(
        this, "Number of Nodes", "How many nodes?", 5, 1, 100, 1, &ok);
    if (ok) maxNodes = userInput;

    // Default base folder (if user cancels QFileDialog later)
    // ex) ~/ButtonNetwork/result
    baseResultDir = QDir::homePath() + "/ButtonNetwork/result";

    // Default gate configs (so professor demo works immediately)
    // Node4: G2 = alpha2 - 1.2*sin(y4)
    gateNode4.enabled = true;
    gateNode4.baseType = "alpha2";
    gateNode4.baseConst = 1.0;
    gateNode4.coeff = 1.2;
    gateNode4.fn = "sin";

    // Node5: G1 = 1.0 - 2.2*tanh(y5)
    gateNode5.enabled = true;
    gateNode5.baseType = "const";
    gateNode5.baseConst = 1.0;
    gateNode5.coeff = 2.2;
    gateNode5.fn = "tanh";
}

void ButtonNetwork::updateEquationEditor(QTextEdit* editor)
{
    equationEditor = editor;
}

/*
 * IMPORTANT:
 * - First check if the user clicked an existing connection line.
 * - If yes -> open edit dialog (weight/function) OR gate config if (4->4 / 5->5)
 * - Else -> create new node (original behavior)
 *
 * Also: Even if buttons.size() >= maxNodes, editing must still work.
 */
void ButtonNetwork::mousePressEvent(QMouseEvent *event)
{
    // 1) Try clicking a connection first
    const int hit = findClickedConnectionIndex(event->pos());
    if (hit >= 0) {
        editConnectionAt(hit);
        return;
    }

    // 2) Original behavior: create a node
    if (buttons.size() >= maxNodes) return;

    QPushButton* btn = new QPushButton(QString::number(buttons.size() + 1), this);
    btn->setGeometry(event->pos().x(), event->pos().y(), 40, 40);
    btn->setStyleSheet("border-radius: 20px; background-color: lightgray;");
    btn->show();
    connect(btn, &QPushButton::clicked, this, &ButtonNetwork::buttonClicked);
    buttons.append(btn);
}

void ButtonNetwork::buttonClicked()
{
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

/*
 * Create connection:
 * key = s(start)(end)
 * Example: click 4 then 5 => s45
 */
void ButtonNetwork::showFunctionDialog(QPushButton* start, QPushButton* end)
{
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
        else {
            QMessageBox::warning(this, "Select", "Please select a function.");
            return;
        }

        QString key = "s" + start->text() + end->text();

        bool ok;
        double weightVal = QInputDialog::getDouble(
            this, "Weight Value",
            "Enter value for " + key + ":", 0.0, -1000, 1000, 6, &ok);

        if (ok) {
            weightValues[key] = weightVal;
            connections.append({start, end, color, functionType});
            update();
        }
        dialog.accept();
    });

    dialog.exec();
}

void ButtonNetwork::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QFont font = p.font();
    font.setBold(true);
    font.setPointSize(10);
    p.setFont(font);

    for (const auto& conn : connections) {
        QPoint start = conn.start->geometry().center();
        QPoint end   = conn.end->geometry().center();

        const int sIdx = conn.start->text().toInt(); // 1-based
        const int eIdx = conn.end->text().toInt();   // 1-based
        (void)eIdx;

        QString label = "s" + conn.start->text() + conn.end->text();
        double val = weightValues.value(label, 0.0);

        // Self-loop on node4 or node5 => show G2 / G1 label (visual only)
        if (conn.start == conn.end && (sIdx == 4 || sIdx == 5)) {
            if (sIdx == 4) label = "G2";
            if (sIdx == 5) label = "G1";
        }

        p.setPen(QPen(conn.color, 3));

        if (conn.start == conn.end) {
            QRectF loop(start.x() - 20, start.y() - 40, 40, 40);
            p.drawArc(loop, 0, 360 * 16);

            if (sIdx == 4) {
                const double base = baseValueFromType(gateNode4.baseType, gateNode4.baseConst);
                p.drawText(start.x() - 80, start.y() - 50,
                           QString("G2=%1-%2*%3(y4)")
                               .arg(base, 0, 'f', 3)
                               .arg(gateNode4.coeff, 0, 'f', 3)
                               .arg(gateNode4.fn));
            } else if (sIdx == 5) {
                const double base = baseValueFromType(gateNode5.baseType, gateNode5.baseConst);
                p.drawText(start.x() - 80, start.y() - 50,
                           QString("G1=%1-%2*%3(y5)")
                               .arg(base, 0, 'f', 3)
                               .arg(gateNode5.coeff, 0, 'f', 3)
                               .arg(gateNode5.fn));
            } else {
                p.drawText(start.x() - 30, start.y() - 50,
                           label + "=" + QString::number(val));
            }
        } else {
            QPainterPath path;
            QPointF mid = (start + end) / 2.0;
            QPointF offset(-(end.y() - start.y()), end.x() - start.x());
            if (offset.manhattanLength() > 0)
                offset /= std::sqrt(offset.x()*offset.x() + offset.y()*offset.y());

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
    if (function == "sin_exp") return QString::number(val) + "*sin(" + from + ")";
    if (function == "tanh")    return QString::number(val) + "*tanh(" + from + ")";
    if (function == "relu")    return QString::number(val) + "*relu(" + from + ")";
    return QString::number(val) + "*" + from;
}

/*
 * ============================================================
 * Gate helpers
 * ============================================================
 */
double ButtonNetwork::baseValueFromType(const QString& baseType, double baseConst) const
{
    if (baseType == "alpha1") return alpha1;
    if (baseType == "alpha2") return alpha2;
    if (baseType == "alpha3") return alpha3;
    if (baseType == "const")  return baseConst;
    return baseConst;
}

double ButtonNetwork::applyFn(const QString& fn, double x) const
{
    if (fn == "sin")  return std::sin(x);
    if (fn == "tanh") return std::tanh(x);
    if (fn == "relu") return (x > 0.0) ? x : 0.0;
    return std::sin(x);
}

double ButtonNetwork::evalGateForNode(int nodeIndex, double yValue) const
{
    const GateConfig* gate = nullptr;
    if (nodeIndex == 3) gate = &gateNode4;
    if (nodeIndex == 4) gate = &gateNode5;
    if (!gate) return 0.0;

    const double base = baseValueFromType(gate->baseType, gate->baseConst);
    const double fnv  = applyFn(gate->fn, yValue);
    return base - gate->coeff * fnv;
}

/*
 * ============================================================
 * Auto-save folder
 * ============================================================
 */
bool ButtonNetwork::ensureBaseResultDir()
{
    if (!baseResultDir.isEmpty()) {
        QDir d(baseResultDir);
        if (!d.exists()) {
            if (!d.mkpath(".")) {
                QMessageBox::critical(this, "Error",
                                      "Cannot create base result dir:\n" + baseResultDir);
                return false;
            }
        }
        return true;
    }

    QString selected = QFileDialog::getExistingDirectory(
        this, "Select base result folder (choose once)", QDir::homePath());

    if (selected.isEmpty()) {
        baseResultDir = QDir::homePath() + "/ButtonNetwork/result";
    } else {
        baseResultDir = selected;
    }

    QDir d(baseResultDir);
    if (!d.exists()) {
        if (!d.mkpath(".")) {
            QMessageBox::critical(this, "Error",
                                  "Cannot create base result dir:\n" + baseResultDir);
            return false;
        }
    }
    return true;
}

bool ButtonNetwork::createNewRunDir()
{
    if (!ensureBaseResultDir()) return false;

    const QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    currentRunDir = baseResultDir + "/run_" + ts;

    QDir d;
    if (!d.mkpath(currentRunDir)) {
        QMessageBox::critical(this, "Error",
                              "Cannot create run folder:\n" + currentRunDir);
        currentRunDir.clear();
        return false;
    }
    return true;
}

QString ButtonNetwork::runPath(const QString& filename) const
{
    if (currentRunDir.isEmpty()) return filename;
    return currentRunDir + "/" + filename;
}

void ButtonNetwork::writeRunInfoFile() const
{
    if (currentRunDir.isEmpty()) return;
    QFile f(runPath("run_info.txt"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&f);
    out << "=== Hopfield Fractional Network Run Info ===\n";
    out << "RunDir: " << currentRunDir << "\n";
    out << "Solver: " << solverMode << "\n";
    out << "tMax: " << tMax << "\n";
    out << "alpha1=" << alpha1 << " alpha2=" << alpha2 << " alpha3=" << alpha3 << "\n";
    out << "nu=" << nu << "\n";
    out << "Gate4(G2): enabled=" << gateNode4.enabled
        << " base=" << gateNode4.baseType << "(" << gateNode4.baseConst << ")"
        << " coeff=" << gateNode4.coeff << " fn=" << gateNode4.fn << "\n";
    out << "Gate5(G1): enabled=" << gateNode5.enabled
        << " base=" << gateNode5.baseType << "(" << gateNode5.baseConst << ")"
        << " coeff=" << gateNode5.coeff << " fn=" << gateNode5.fn << "\n\n";

    out << "Connections:\n";
    for (const auto& c : connections) {
        QString key = "s" + c.start->text() + c.end->text();
        out << " " << key << " = " << weightValues.value(key, 0.0)
            << " fn=" << c.function << "\n";
    }
    out << "\nOutputs:\n";
    out << " - result.dat\n";
    out << " - result_stream.csv\n";
    out << " - result_final.csv\n";
    out << " - params.txt\n";
    out << " - table.txt\n";
    out << " - plot.gnu\n";
    out << " - y_all.png\n";
    out << " - alpha2_scan_3d.dat / alpha2_scan_2d.dat (when scanAlpha2)\n";
    out << " - alpha2_y1.png ... alpha2_y5.png (when scanAlpha2)\n";
    f.close();
}

void ButtonNetwork::saveParams(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);

    out << "solverMode=" << solverMode << "\n";
    out << "tMax=" << tMax << "\n";
    out << "alpha1=" << alpha1 << "\n";
    out << "alpha2=" << alpha2 << "\n";
    out << "alpha3=" << alpha3 << "\n";
    out << "nu=" << nu << "\n";

    out << "GateNode4.enabled=" << gateNode4.enabled << "\n";
    out << "GateNode4.baseType=" << gateNode4.baseType << "\n";
    out << "GateNode4.baseConst=" << gateNode4.baseConst << "\n";
    out << "GateNode4.coeff=" << gateNode4.coeff << "\n";
    out << "GateNode4.fn=" << gateNode4.fn << "\n";

    out << "GateNode5.enabled=" << gateNode5.enabled << "\n";
    out << "GateNode5.baseType=" << gateNode5.baseType << "\n";
    out << "GateNode5.baseConst=" << gateNode5.baseConst << "\n";
    out << "GateNode5.coeff=" << gateNode5.coeff << "\n";
    out << "GateNode5.fn=" << gateNode5.fn << "\n";

    out << "\n[weights]\n";
    for (auto it = weightValues.begin(); it != weightValues.end(); ++it) {
        out << it.key() << "=" << it.value() << "\n";
    }
    f.close();
}

/*
 * ============================================================
 * Functions / solver core
 * ============================================================
 */
double ButtonNetwork::sinEFunction(double x) { return std::sin(x); }
double ButtonNetwork::tanhFunction(double x) { return std::tanh(x); }
double ButtonNetwork::reluFunction(double x) { return (x > 0.0) ? x : 0.0; }

double ButtonNetwork::gammaWeight(int om, int r, double nu)
{
    // (om-r+1)^nu - (om-r)^nu
    double a = std::pow(double(om - r + 1), nu);
    double b = std::pow(double(om - r), nu);
    return a - b;
}

/*
 * ============================================================
 * Main compute
 * ============================================================
 */
void ButtonNetwork::computeResults()
{
    if (!createNewRunDir()) return;

    saveParams(runPath("params.txt"));
    writeRunInfoFile();

    if (solverMode == "ODE") runODE();
    else runGamma();
}

void ButtonNetwork::runODE()
{
    const int steps = tMax;
    const double h = 0.01;

    QVector<QVector<double>> y(5, QVector<double>(steps + 1));
    y[0][0] = 0.8;
    y[1][0] = 0.3;
    y[2][0] = 0.4;
    y[3][0] = 0.6;
    y[4][0] = 0.7;

    for (int t = 1; t <= steps; ++t) {
        if (t % 400 == 0) QCoreApplication::processEvents();

        for (int i = 0; i < 5; ++i) {
            double sum = -y[i][t - 1];

            for (const auto& conn : connections) {
                int from = conn.start->text().toInt() - 1;
                int to   = conn.end->text().toInt() - 1;
                if (to != i) continue;

                QString key = "s" + conn.start->text() + conn.end->text();
                double w = weightValues.value(key, 0.0);
                double in = y[from][t - 1];

                if (conn.function == "sin_exp") sum += w * sinEFunction(in);
                else if (conn.function == "tanh") sum += w * tanhFunction(in);
                else if (conn.function == "relu") sum += w * reluFunction(in);
            }

            if (i == 3) {
                if (gateNode4.enabled) {
                    const double G2 = evalGateForNode(3, y[3][t - 1]);
                    sum += G2 * tanhFunction(y[3][t - 1]);
                } else {
                    sum += (alpha2 - alpha3 * sinEFunction(y[4][t - 1]))
                    * tanhFunction(y[3][t - 1]);
                }
            }

            if (i == 4) {
                if (gateNode5.enabled) {
                    const double G1 = evalGateForNode(4, y[4][t - 1]);
                    sum += G1 * tanhFunction(y[4][t - 1]);
                } else {
                    sum += (1 - alpha1 * tanhFunction(y[2][t - 1]))
                    * tanhFunction(y[4][t - 1]);
                }
            }

            y[i][t] = y[i][t - 1] + h * sum;
        }
    }

    saveAndDisplayResult(y, steps);
}

void ButtonNetwork::runGamma()
{
    const int steps = tMax;

    QVector<QVector<double>> y(5, QVector<double>(steps + 1));
    y[0][0] = 0.8;
    y[1][0] = 0.3;
    y[2][0] = 0.4;
    y[3][0] = 0.6;
    y[4][0] = 0.7;

    for (int om = 1; om <= steps; ++om) {
        if (om % 80 == 0) QCoreApplication::processEvents();

        for (int i = 0; i < 5; ++i) {
            double acc = 0.0;

            for (int r = 1; r <= om; ++r) {
                double sum = -y[i][r - 1];

                for (const auto& conn : connections) {
                    int from = conn.start->text().toInt() - 1;
                    int to   = conn.end->text().toInt() - 1;
                    if (to != i) continue;

                    QString key = "s" + conn.start->text() + conn.end->text();
                    double w = weightValues.value(key, 0.0);
                    double in = y[from][r - 1];

                    if (conn.function == "sin_exp") sum += w * sinEFunction(in);
                    else if (conn.function == "tanh") sum += w * tanhFunction(in);
                    else if (conn.function == "relu") sum += w * reluFunction(in);
                }

                if (i == 3) {
                    if (gateNode4.enabled) {
                        const double G2 = evalGateForNode(3, y[3][r - 1]);
                        sum += G2 * tanhFunction(y[3][r - 1]);
                    } else {
                        sum += (alpha2 - alpha3 * sinEFunction(y[4][r - 1]))
                        * tanhFunction(y[3][r - 1]);
                    }
                }

                if (i == 4) {
                    if (gateNode5.enabled) {
                        const double G1 = evalGateForNode(4, y[4][r - 1]);
                        sum += G1 * tanhFunction(y[4][r - 1]);
                    } else {
                        sum += (1 - alpha1 * tanhFunction(y[2][r - 1]))
                        * tanhFunction(y[4][r - 1]);
                    }
                }

                acc += sum * gammaWeight(om, r, nu);
            }

            y[i][om] = y[i][0] + acc;
        }
    }

    saveAndDisplayResult(y, steps);
}

void ButtonNetwork::saveAndDisplayResult(const QVector<QVector<double>>& y, int steps)
{
    QFile f(runPath("result.dat"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot write result.dat");
        return;
    }

    QTextStream out(&f);
    for (int t = 0; t <= steps; ++t) {
        out << y[0][t] << " " << y[1][t] << " " << y[2][t] << " " << y[3][t] << " " << y[4][t] << "\n";
    }
    f.close();

    QFile stream(runPath("result_stream.csv"));
    if (stream.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&stream);
        s << "t,y1,y2,y3,y4,y5\n";
        for (int t = 0; t <= steps; ++t) {
            s << t << "," << y[0][t] << "," << y[1][t] << "," << y[2][t] << "," << y[3][t] << "," << y[4][t] << "\n";
        }
        stream.close();
    }

    QFile fin(runPath("result_final.csv"));
    if (fin.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&fin);
        s << "y1,y2,y3,y4,y5\n";
        s << y[0][steps] << "," << y[1][steps] << "," << y[2][steps] << "," << y[3][steps] << "," << y[4][steps] << "\n";
        fin.close();
    }

    QFile table(runPath("table.txt"));
    if (table.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream t(&table);
        t << "Output Table (result.dat)\n";
        t << "Rows: " << (steps + 1) << "\n\n";
        t << "y1 y2 y3 y4 y5\n";
        for (int i = 0; i <= steps; ++i) {
            t << y[0][i] << " " << y[1][i] << " " << y[2][i] << " " << y[3][i] << " " << y[4][i] << "\n";
        }
        table.close();
    }

    QFile info(runPath("compute_info.txt"));
    if (info.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out2(&info);
        out2 << "Compute finished.\n";
        out2 << "Saved:\n";
        out2 << " - " << runPath("result.dat") << "\n";
        out2 << " - " << runPath("result_stream.csv") << "\n";
        out2 << " - " << runPath("result_final.csv") << "\n";
        out2 << " - " << runPath("params.txt") << "\n";
        out2 << " - " << runPath("table.txt") << "\n";
        info.close();
    }

    emit fileSaved(runPath("result.dat"));
}

/*
 * ============================================================
 * Extra UI helpers
 * ============================================================
 */
void ButtonNetwork::clearNetwork()
{
    for (QPushButton* btn : buttons) {
        if (btn) btn->deleteLater();
    }
    buttons.clear();
    connections.clear();
    weightValues.clear();
    firstSelected = nullptr;

    if (equationEditor) equationEditor->clear();
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

// ------------------------------------------------------------
// NEW: alpha2 scan UI bindings (optional)
// ------------------------------------------------------------
void ButtonNetwork::setAlpha2ScanRange(double minVal, double maxVal, double stepVal)
{
    scanAlpha2Min = minVal;
    scanAlpha2Max = maxVal;
    scanAlpha2Step = stepVal;
}

void ButtonNetwork::setAlpha2ScanSampling(int transientPercent, int sampleStride)
{
    scanTransientPercent = transientPercent;
    scanSampleStride = sampleStride;
}

/*
 * Show Graph:
 * - Generate gnuplot script INSIDE current run folder
 * - Save y_all.png INSIDE current run folder
 * - Open image viewer (optional)
 */
void ButtonNetwork::showGraph()
{
    if (currentRunDir.isEmpty()) {
        QMessageBox::warning(this, "Error", "No run folder. Press Compute first.");
        return;
    }

    QFile f(runPath("result.dat"));
    if (!f.exists()) {
        QMessageBox::warning(this, "Error",
                             "No result.dat file found in run folder.\nRun Compute first.");
        return;
    }

    generateGnuplotScript();

    QProcess proc;
    proc.setWorkingDirectory(currentRunDir);
    proc.start("gnuplot", QStringList() << runPath("plot.gnu"));
    if (!proc.waitForFinished()) {
        QMessageBox::critical(this, "Error",
                              "Failed to run gnuplot. Is it installed?");
        return;
    }

    QFile info(runPath("graph_info.txt"));
    if (info.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&info);
        out << "Graph saved: " << runPath("y_all.png") << "\n";
        info.close();
    }
    emit fileSaved(runPath("graph_info.txt"));

#ifdef Q_OS_LINUX
    QProcess::startDetached("xdg-open", QStringList() << runPath("y_all.png"));
#elif defined(Q_OS_MAC)
    QProcess::startDetached("open", QStringList() << runPath("y_all.png"));
#elif defined(Q_OS_WIN)
    QProcess::startDetached(runPath("y_all.png"));
#endif
}

void ButtonNetwork::showTable()
{
    showOutputTable();
}

/*
 * Show Output Table:
 * - Read result.dat from run folder
 * - Show it in right panel
 */
void ButtonNetwork::showOutputTable()
{
    if (currentRunDir.isEmpty()) {
        QMessageBox::warning(this, "Error", "No run folder. Press Compute first.");
        return;
    }

    QFile f(runPath("table.txt"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot open table.txt");
        return;
    }
    if (equationEditor) {
        QTextStream in(&f);
        equationEditor->setPlainText(in.readAll());
    }
    f.close();
}

// void ButtonNetwork::showTable()
// {
//     showOutputTable();
// }

/*
 * ============================================================
 * gnuplot script for y_all.png (multiplot)
 * ============================================================
 */
void ButtonNetwork::generateGnuplotScript()
{
    if (currentRunDir.isEmpty()) return;

    QFile script(runPath("plot.gnu"));
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&script);

    out << "set terminal pngcairo size 1200,900\n";
    out << "set output 'y_all.png'\n";
    out << "set multiplot layout 5,1 title 'Hopfield Network Results'\n";
    out << "set grid\n";
    out << "set key left\n";
    out << "set xlabel 't (step)'\n";
    out << "set xrange [0:*]\n";

    out << "set xlabel 't (step)'\n";
    out << "set ylabel 'y1'\n";
    out << "plot 'result.dat' using 0:1 with lines lc rgb 'blue' linewidth 2 title 'y1'\n\n";

    out << "set xlabel 't (step)'\n";
    out << "set ylabel 'y2'\n";
    out << "plot 'result.dat' using 0:2 with lines lc rgb 'red' linewidth 2 title 'y2'\n\n";

    out << "set xlabel 't (step)'\n";
    out << "set ylabel 'y3'\n";
    out << "plot 'result.dat' using 0:3 with lines lc rgb 'green' linewidth 2 title 'y3'\n\n";

    out << "set xlabel 't (step)'\n";
    out << "set ylabel 'y4'\n";
    out << "plot 'result.dat' using 0:4 with lines lc rgb 'black' linewidth 2 title 'y4'\n\n";

    out << "set xlabel 't (step)'\n";
    out << "set ylabel 'y5'\n";
    out << "plot 'result.dat' using 0:5 with lines lc rgb 'purple' linewidth 2 title 'y5'\n\n";

    out << "unset multiplot\n";
    script.close();
}

/*
 * ============================================================
 * alpha2 scan (bifurcation diagram)
 * - alpha2_scan_3d.dat : alpha2 t y1 y2 y3 y4 y5   (surface data)
 * - alpha2_scan_2d.dat : alpha2 y1 y2 y3 y4 y5     (many rows per alpha2 + blank line)
 * - gnuplot -> alpha2_y1.png ... alpha2_y5.png (dots)
 * ============================================================
 */
void ButtonNetwork::scanAlpha2()
{
    // 새 run 폴더를 만들고, 그 안에 scan 결과를 저장 (요청사항: 자동저장)
    if (!createNewRunDir()) return;

    // scan 설정 (기본: UI로 set된 값 사용)
    // - scanAlpha2Min/Max/Step
    // - scanTransientPercent / scanSampleStride
    // NOTE: 기존 QInputDialog 기반 흐름은 구조 변경 없이 "추가"만으로 유지하기 위해,
    //       값이 비정상일 때만 다이얼로그로 보정 입력을 받는다.
    bool ok = true;
    double a2Min = scanAlpha2Min;
    double a2Max = scanAlpha2Max;
    double a2Step = scanAlpha2Step;
    int transientPercent = scanTransientPercent;
    int sampleStride = scanSampleStride;

    if (!(a2Step > 0.0) || a2Max < a2Min) {
        ok = false;
        a2Min = QInputDialog::getDouble(this, "Alpha2 scan", "alpha2 min:", -10.0, -1000, 1000, 4, &ok);
        if (!ok) return;
        a2Max = QInputDialog::getDouble(this, "Alpha2 scan", "alpha2 max:",  10.0, -1000, 1000, 4, &ok);
        if (!ok) return;
        a2Step = QInputDialog::getDouble(this, "Alpha2 scan", "alpha2 step:", 0.5, 0.0001, 1000, 4, &ok);
        if (!ok) return;
    }
    if (transientPercent < 0 || transientPercent > 99) transientPercent = 70;
    if (sampleStride < 1) sampleStride = 20;

    // 저장: params + info
    saveParams(runPath("params.txt"));
    writeRunInfoFile();

    // data files
    QFile f3d(runPath("alpha2_scan_3d.dat"));
    QFile f2d(runPath("alpha2_scan_2d.dat"));
    if (!f3d.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot write alpha2_scan_3d.dat");
        return;
    }
    if (!f2d.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot write alpha2_scan_2d.dat");
        return;
    }

    QTextStream out3d(&f3d);
    QTextStream out2d(&f2d);

    // NOTE: alpha2_scan_2d.dat 포맷은 교수님 bifurcation diagram용 "정답 포맷" 그대로 쓴다.
    //       (헤더 없이)  alpha2 y1 y2 y3 y4 y5  가 여러 줄 + blank line

    const int steps = tMax;
    const double h = 0.01; // ODE fixed-step

    const int transientStart = std::min(std::max(int(std::floor(steps * (transientPercent / 100.0))), 0), steps);

    // Scan loop
    for (double a2 = a2Min; a2 <= a2Max + 1e-12; a2 += a2Step) {
        // alpha2를 임시로 바꿔서 시뮬레이션
        const double oldAlpha2 = alpha2;
        alpha2 = a2;

        QVector<QVector<double>> y(5, QVector<double>(steps + 1));
        y[0][0] = 0.8;
        y[1][0] = 0.3;
        y[2][0] = 0.4;
        y[3][0] = 0.6;
        y[4][0] = 0.7;

        if (solverMode == "ODE") {
            for (int t = 1; t <= steps; ++t) {
                if (t % 400 == 0) QCoreApplication::processEvents();

                for (int i = 0; i < 5; ++i) {
                    double sum = -y[i][t - 1];

                    for (const auto& conn : connections) {
                        int from = conn.start->text().toInt() - 1;
                        int to   = conn.end->text().toInt() - 1;
                        if (to != i) continue;

                        QString key = "s" + conn.start->text() + conn.end->text();
                        double w = weightValues.value(key, 0.0);
                        double in = y[from][t - 1];

                        if (conn.function == "sin_exp") sum += w * sinEFunction(in);
                        else if (conn.function == "tanh") sum += w * tanhFunction(in);
                        else if (conn.function == "relu") sum += w * reluFunction(in);
                    }

                    if (i == 3) {
                        if (gateNode4.enabled) {
                            const double G2 = evalGateForNode(3, y[3][t - 1]);
                            sum += G2 * tanhFunction(y[3][t - 1]);
                        } else {
                            sum += (alpha2 - alpha3 * sinEFunction(y[4][t - 1]))
                            * tanhFunction(y[3][t - 1]);
                        }
                    }

                    if (i == 4) {
                        if (gateNode5.enabled) {
                            const double G1 = evalGateForNode(4, y[4][t - 1]);
                            sum += G1 * tanhFunction(y[4][t - 1]);
                        } else {
                            sum += (1 - alpha1 * tanhFunction(y[2][t - 1]))
                            * tanhFunction(y[4][t - 1]);
                        }
                    }

                    y[i][t] = y[i][t - 1] + h * sum;
                }

                if (t % sampleStride == 0 || t == steps) {
                    out3d << a2 << " " << t << " "
                          << y[0][t] << " " << y[1][t] << " " << y[2][t] << " "
                          << y[3][t] << " " << y[4][t] << "\n";
                }
            }
        } else {
            // Fractional(Gamma) scan은 매우 느릴 수 있음. 그래도 "옵션"대로 동작하게 구현.
            for (int om = 1; om <= steps; ++om) {
                if (om % 80 == 0) QCoreApplication::processEvents();

                for (int i = 0; i < 5; ++i) {
                    double acc = 0.0;

                    for (int r = 1; r <= om; ++r) {
                        double sum = -y[i][r - 1];

                        for (const auto& conn : connections) {
                            int from = conn.start->text().toInt() - 1;
                            int to   = conn.end->text().toInt() - 1;
                            if (to != i) continue;

                            QString key = "s" + conn.start->text() + conn.end->text();
                            double w = weightValues.value(key, 0.0);
                            double in = y[from][r - 1];

                            if (conn.function == "sin_exp") sum += w * sinEFunction(in);
                            else if (conn.function == "tanh") sum += w * tanhFunction(in);
                            else if (conn.function == "relu") sum += w * reluFunction(in);
                        }

                        if (i == 3) {
                            if (gateNode4.enabled) {
                                const double G2 = evalGateForNode(3, y[3][r - 1]);
                                sum += G2 * tanhFunction(y[3][r - 1]);
                            } else {
                                sum += (alpha2 - alpha3 * sinEFunction(y[4][r - 1]))
                                * tanhFunction(y[3][r - 1]);
                            }
                        }

                        if (i == 4) {
                            if (gateNode5.enabled) {
                                const double G1 = evalGateForNode(4, y[4][r - 1]);
                                sum += G1 * tanhFunction(y[4][r - 1]);
                            } else {
                                sum += (1 - alpha1 * tanhFunction(y[2][r - 1]))
                                * tanhFunction(y[4][r - 1]);
                            }
                        }

                        acc += sum * gammaWeight(om, r, nu);
                    }

                    y[i][om] = y[i][0] + acc;
                }

                if (om % sampleStride == 0 || om == steps) {
                    out3d << a2 << " " << om << " "
                          << y[0][om] << " " << y[1][om] << " " << y[2][om] << " "
                          << y[3][om] << " " << y[4][om] << "\n";
                }
            }
        }

        // 2D bifurcation projection: transient 이후의 (alpha2, y(t)) 점들을 기록
        for (int t = transientStart; t <= steps; t += sampleStride) {
            out2d << a2 << " "
                  << y[0][t] << " " << y[1][t] << " " << y[2][t] << " "
                  << y[3][t] << " " << y[4][t] << "\n";
        }

        // alpha2 복원
        alpha2 = oldAlpha2;

        // 블록 구분 (gnuplot에서 surface 줄바꿈 효과)
        out3d << "\n";
        out2d << "\n"; // blank line block separator
    }

    f3d.close();
    f2d.close();

    // gnuplot scripts 생성 + 실행 + png 저장
    generateAlpha2ScanGnuplotScripts();

    QProcess proc;
    proc.setWorkingDirectory(currentRunDir);
    proc.start("gnuplot", QStringList() << runPath("alpha2_scan.gnu"));
    const bool finished = proc.waitForFinished();

    const QString gpStdout = QString::fromLocal8Bit(proc.readAllStandardOutput());
    const QString gpStderr = QString::fromLocal8Bit(proc.readAllStandardError());

    QFile gpLog(runPath("alpha2_gnuplot_log.txt"));
    if (gpLog.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream gl(&gpLog);
        gl << "=== gnuplot stdout ===\n" << gpStdout << "\n\n";
        gl << "=== gnuplot stderr ===\n" << gpStderr << "\n";
        gpLog.close();
    }

    // gnuplot 에러를 GUI 오른쪽 QTextEdit에도 보여주기 (재발 방지 규칙)
    if (equationEditor) {
        if (!gpStderr.trimmed().isEmpty()) {
            equationEditor->append("\n[gnuplot stderr]\n" + gpStderr);
        }
        if (!gpStdout.trimmed().isEmpty()) {
            equationEditor->append("\n[gnuplot stdout]\n" + gpStdout);
        }
    }

    if (!finished || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        QMessageBox::warning(this, "Gnuplot",
                             "alpha2 scan data saved, but gnuplot failed.\n"
                             "See alpha2_gnuplot_log.txt in the run folder.");
    }

    // 오른쪽 패널에 요약 표시 + fileSaved로 갱신
    QFile info(runPath("alpha2_scan_info.txt"));
    if (info.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream o(&info);
        o << "=== alpha2 scan finished ===\n";
        o << "RunDir: " << currentRunDir << "\n";
        o << "Scan range: [" << a2Min << ", " << a2Max << "] step=" << a2Step << "\n";
        o << "Transient discard: first " << transientPercent << "% (startIndex=" << transientStart << ")\n";
        o << "Sampling stride: " << sampleStride << "\n\n";
        o << "Data files:\n";
        o << " - " << runPath("alpha2_scan_3d.dat") << "\n";
        o << " - " << runPath("alpha2_scan_2d.dat") << "\n\n";
        o << "PNGs (bifurcation, 2D projection):\n";
        o << " - alpha2_y1.png\n - alpha2_y2.png\n - alpha2_y3.png\n - alpha2_y4.png\n - alpha2_y5.png\n\n";
        o << "Gnuplot log:\n";
        o << " - alpha2_gnuplot_log.txt\n";
        info.close();
    }
    emit fileSaved(runPath("alpha2_scan_info.txt"));
}

// void ButtonNetwork::generateAlpha2ScanGnuplotScripts() const
// {
//     if (currentRunDir.isEmpty()) return;

//     QFile script(runPath("alpha2_scan.gnu"));
//     if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) return;

//     QTextStream g(&script);

//     // ============================================================
//     // Professor-style bifurcation diagram (2D projection only)
//     // x-axis: alpha2
//     // y-axis: yk (k=1..5)
//     // points: sampled y(t) values after transient, written in alpha2_scan_2d.dat
//     // ============================================================
//     g << "set term pngcairo size 900,700\n";
//     g << "set grid\n";
//     g << "set xlabel 'alpha2'\n";
//     g << "unset key\n";

//     // 2D file columns: (1)alpha2 (2)y1 (3)y2 (4)y3 (5)y4 (6)y5
//     const char* yNames[5] = {"y1","y2","y3","y4","y5"};
//     const int col2d[5] = {2,3,4,5,6};

//     for (int i = 0; i < 5; ++i) {
//         g << "set output 'alpha2_" << yNames[i] << ".png'\n";
//         g << "set ylabel '" << yNames[i] << "'\n";
//         g << "plot 'alpha2_scan_2d.dat' using 1:" << col2d[i] << " with dots\n\n";
//     }

//     g << "set output\n";
//     script.close();
// }

/*
 * ============================================================
 * Connection click-edit implementation
 * ============================================================
 */
double ButtonNetwork::distancePointToSegment(const QPointF& p,
                                             const QPointF& a,
                                             const QPointF& b) const
{
    const double dx = b.x() - a.x();
    const double dy = b.y() - a.y();
    const double len2 = dx*dx + dy*dy;
    if (len2 <= 1e-12) {
        const double px = p.x() - a.x();
        const double py = p.y() - a.y();
        return std::sqrt(px*px + py*py);
    }

    double t = ((p.x() - a.x())*dx + (p.y() - a.y())*dy) / len2;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    const double projx = a.x() + t*dx;
    const double projy = a.y() + t*dy;
    const double ex = p.x() - projx;
    const double ey = p.y() - projy;
    return std::sqrt(ex*ex + ey*ey);
}

int ButtonNetwork::findClickedConnectionIndex(const QPoint& pos) const
{
    const QPointF p(pos);

    for (int idx = 0; idx < connections.size(); ++idx) {
        const Connection& conn = connections[idx];

        QPointF a = conn.start->geometry().center();
        QPointF b = conn.end->geometry().center();

        // self-loop
        if (conn.start == conn.end) {
            QRectF loop(a.x() - 20.0, a.y() - 40.0, 40.0, 40.0);
            QRectF hit = loop.adjusted(-connectionHitRadiusPx,
                                       -connectionHitRadiusPx,
                                       connectionHitRadiusPx,
                                       connectionHitRadiusPx);
            if (hit.contains(p)) return idx;
            continue;
        }

        // curved quadTo (same math as paintEvent)
        QPointF mid = (a + b) / 2.0;
        QPointF offset(-(b.y() - a.y()), b.x() - a.x());
        if (std::abs(offset.x()) > 1e-9 || std::abs(offset.y()) > 1e-9) {
            double norm = std::sqrt(offset.x()*offset.x() + offset.y()*offset.y());
            if (norm > 1e-9) offset /= norm;
        }
        offset *= 40.0;
        const QPointF c = mid + offset;

        const int N = 24;
        QPointF prev = a;

        double best = 1e18;
        for (int k = 1; k <= N; ++k) {
            const double t = double(k) / double(N);
            const double u = 1.0 - t;
            QPointF cur = (u*u)*a + (2*u*t)*c + (t*t)*b;

            best = std::min(best, distancePointToSegment(p, prev, cur));
            prev = cur;

            if (best <= connectionHitRadiusPx) return idx;
        }
    }

    return -1;
}

void ButtonNetwork::editConnectionAt(int index)
{
    if (index < 0 || index >= connections.size()) return;

    Connection& conn = connections[index];

    const int sIdx = conn.start->text().toInt(); // 1-based

    // self-loop on node4 or node5 => edit Gate (G2/G1)
    if (conn.start == conn.end && (sIdx == 4 || sIdx == 5)) {

        GateConfig* gate = (sIdx == 4) ? &gateNode4 : &gateNode5;
        const QString gateName = (sIdx == 4) ? "G2 (Node4)" : "G1 (Node5)";

        QDialog dialog(this);
        dialog.setWindowTitle("Edit " + gateName);

        QVBoxLayout layout(&dialog);

        QCheckBox enabledBox("Gate Enabled", &dialog);
        enabledBox.setChecked(gate->enabled);

        QComboBox baseTypeCombo(&dialog);
        baseTypeCombo.addItem("const");
        baseTypeCombo.addItem("alpha1");
        baseTypeCombo.addItem("alpha2");
        baseTypeCombo.addItem("alpha3");
        baseTypeCombo.setCurrentText(gate->baseType);

        QDoubleSpinBox baseConstSpin(&dialog);
        baseConstSpin.setRange(-1000, 1000);
        baseConstSpin.setDecimals(6);
        baseConstSpin.setValue(gate->baseConst);

        QDoubleSpinBox coeffSpin(&dialog);
        coeffSpin.setRange(-1000, 1000);
        coeffSpin.setDecimals(6);
        coeffSpin.setValue(gate->coeff);

        QComboBox fnCombo(&dialog);
        fnCombo.addItem("sin");
        fnCombo.addItem("tanh");
        fnCombo.addItem("relu");
        fnCombo.setCurrentText(gate->fn);

        QPushButton okBtn("OK", &dialog);
        QPushButton cancelBtn("Cancel", &dialog);

        layout.addWidget(&enabledBox);

        layout.addWidget(new QLabel("Base type:", &dialog));
        layout.addWidget(&baseTypeCombo);

        layout.addWidget(new QLabel("Base const (used if baseType==const):", &dialog));
        layout.addWidget(&baseConstSpin);

        layout.addWidget(new QLabel("Coeff:", &dialog));
        layout.addWidget(&coeffSpin);

        layout.addWidget(new QLabel("fn(y):", &dialog));
        layout.addWidget(&fnCombo);

        QHBoxLayout btns;
        btns.addWidget(&okBtn);
        btns.addWidget(&cancelBtn);
        layout.addLayout(&btns);

        connect(&okBtn, &QPushButton::clicked, [&]() {
            gate->enabled = enabledBox.isChecked();
            gate->baseType = baseTypeCombo.currentText();
            gate->baseConst = baseConstSpin.value();
            gate->coeff = coeffSpin.value();
            gate->fn = fnCombo.currentText();
            dialog.accept();
        });
        connect(&cancelBtn, &QPushButton::clicked, [&]() {
            dialog.reject();
        });

        dialog.exec();
        update();
        return;
    }

    // normal connection edit
    QString key = "s" + conn.start->text() + conn.end->text();

    bool ok;
    double newVal = QInputDialog::getDouble(
        this, "Edit Weight",
        "Enter value for " + key + ":", weightValues.value(key, 0.0),
        -1000, 1000, 6, &ok);
    if (!ok) return;

    weightValues[key] = newVal;

    // function edit
    QStringList items;
    items << "sin_exp" << "tanh" << "relu";
    QString fn = QInputDialog::getItem(
        this, "Edit Function", "Select function:", items, items.indexOf(conn.function),
        false, &ok);
    if (ok) {
        conn.function = fn;
        update();
    }
}
bool ButtonNetwork::copyOverwrite(const QString& src, const QString& dst) const
{
    if (!QFileInfo::exists(src)) return false;
    QFile::remove(dst);
    return QFile::copy(src, dst);
}

void ButtonNetwork::ensurePresetNodes5()
{
    // 이미 노드가 5개 이상 있으면 그대로 사용 (기존 유저 구성 보존)
    if (buttons.size() >= 5) return;

    // 부족한 만큼만 생성 (기존 동작 변경 없음)
    // 고정 위치(원형 배치 느낌). 화면 크기/사용자 클릭과 무관하게 테스트는 항상 같은 배치.
    const QPoint centers[5] = {
        QPoint(120, 120),  // node1
        QPoint(260, 220),  // node2
        QPoint(120, 320),  // node3
        QPoint(320, 120),  // node4
        QPoint(360, 320)   // node5
    };

    while (buttons.size() < 5) {
        int idx = buttons.size(); // 0..4
        QPushButton* btn = new QPushButton(QString::number(idx + 1), this);
        btn->setGeometry(centers[idx].x(), centers[idx].y(), 40, 40);
        btn->setStyleSheet("border-radius: 20px; background-color: lightgray;");
        btn->show();
        connect(btn, &QPushButton::clicked, this, &ButtonNetwork::buttonClicked);
        buttons.append(btn);
    }
    update();
}

void ButtonNetwork::addOrUpdateConnection(int from, int to, double w, const QString& fn)
{
    // from,to are 1-based
    if (from < 1 || from > buttons.size()) return;
    if (to < 1 || to > buttons.size()) return;

    QPushButton* start = buttons[from - 1];
    QPushButton* end   = buttons[to - 1];

    // function -> color mapping (기존 UI 규칙과 동일)
    QColor color = Qt::yellow;
    if (fn == "tanh") color = Qt::black;
    else if (fn == "relu") color = Qt::blue;
    else color = Qt::yellow; // sin_exp

    QString key = "s" + QString::number(from) + QString::number(to);
    weightValues[key] = w;

    // 이미 존재하는 연결이면 갱신
    for (Connection& c : connections) {
        if (c.start == start && c.end == end) {
            c.function = fn;
            c.color = color;
            update();
            return;
        }
    }

    // 없으면 추가
    connections.append({start, end, color, fn});
    update();
}

void ButtonNetwork::runAutoTestNode5Preset()
{
    ensurePresetNodes5();

    addOrUpdateConnection(1, 4, -0.6, "sin_exp");
    addOrUpdateConnection(4, 1,  0.7, "tanh");
    addOrUpdateConnection(1, 3, -0.8, "sin_exp");
    addOrUpdateConnection(3, 1,  1.7, "tanh");
    addOrUpdateConnection(2, 3,  2.0, "sin_exp");
    addOrUpdateConnection(3, 2, -0.4, "tanh");
    addOrUpdateConnection(1, 2, -0.3, "tanh");
    addOrUpdateConnection(2, 1, -3.0, "sin_exp");
    addOrUpdateConnection(2, 5,  0.4, "sin_exp");
    addOrUpdateConnection(5, 2,  1.7, "tanh");

    if (equationEditor) {
        equationEditor->append("\n[AUTO TEST Node5] preset network applied. Running 1 time...\n");
    }

    // 1) Compute (creates run folder A)
    computeResults();

    // ✅ runDir을 여기서 고정해두면, 혹시라도 내부에서 바뀌어도 복제/로그가 안전함
    const QString runDirFixed = currentRunDir;

    // 2) Graph in same folder
    showGraph();

    // 3) ✅ Alpha2 scan in SAME folder (no new run folder)
    scanAlpha2ReuseCurrentRun();

    // 4) test_* 복제도 고정된 runDir 기준으로 수행
    auto rp = [&](const QString& name){ return runDirFixed + "/" + name; };

    copyOverwrite(rp("result.dat"),             rp("test_result.dat"));
    copyOverwrite(rp("result_stream.csv"),      rp("test_result_stream.csv"));
    copyOverwrite(rp("result_final.csv"),       rp("test_result_final.csv"));
    copyOverwrite(rp("params.txt"),             rp("test_params.txt"));
    copyOverwrite(rp("table.txt"),              rp("test_table.txt"));
    copyOverwrite(rp("plot.gnu"),               rp("test_plot.gnu"));
    copyOverwrite(rp("y_all.png"),              rp("test_y_all.png"));

    copyOverwrite(rp("alpha2_scan_3d.dat"),     rp("test_alpha2_scan_3d.dat"));
    copyOverwrite(rp("alpha2_scan_2d.dat"),     rp("test_alpha2_scan_2d.dat"));
    copyOverwrite(rp("alpha2_y1.png"),          rp("test_alpha2_y1.png"));
    copyOverwrite(rp("alpha2_y2.png"),          rp("test_alpha2_y2.png"));
    copyOverwrite(rp("alpha2_y3.png"),          rp("test_alpha2_y3.png"));
    copyOverwrite(rp("alpha2_y4.png"),          rp("test_alpha2_y4.png"));
    copyOverwrite(rp("alpha2_y5.png"),          rp("test_alpha2_y5.png"));

    QFile info(rp("test_info.txt"));
    if (info.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream o(&info);
        o << "AUTO TEST Node5 preset finished.\n";
        o << "RunDir: " << runDirFixed << "\n\n";
        o << "Preset edges:\n";
        o << "s14=-0.6 sin_exp\ns41=0.7 tanh\ns13=-0.8 sin_exp\ns31=1.7 tanh\n";
        o << "s23=2.0 sin_exp\ns32=-0.4 tanh\ns12=-0.3 tanh\ns21=-3.0 sin_exp\n";
        o << "s25=0.4 sin_exp\ns52=1.7 tanh\n\n";
        o << "Copied outputs as test_* files.\n";
        info.close();
    }

    emit fileSaved(rp("test_info.txt"));

    if (equationEditor) {
        equationEditor->append("\n[AUTO TEST Node5] done. Saved test_* files in:\n" + runDirFixed + "\n");
    }
}

void ButtonNetwork::scanAlpha2ReuseCurrentRun()
{
    // ✅ 핵심: 새 run 폴더 만들지 않고 currentRunDir을 그대로 사용
    if (currentRunDir.isEmpty()) {
        QMessageBox::warning(this, "Error", "No run folder. Press Compute first (or Auto Test).");
        return;
    }

    // scan 설정 (UI로 set된 값 사용)
    bool ok = true;
    double a2Min = scanAlpha2Min;
    double a2Max = scanAlpha2Max;
    double a2Step = scanAlpha2Step;
    int transientPercent = scanTransientPercent;
    int sampleStride = scanSampleStride;

    if (!(a2Step > 0.0) || a2Max < a2Min) {
        ok = false;
        a2Min = QInputDialog::getDouble(this, "Alpha2 scan", "alpha2 min:", -10.0, -1000, 1000, 4, &ok);
        if (!ok) return;
        a2Max = QInputDialog::getDouble(this, "Alpha2 scan", "alpha2 max:",  10.0, -1000, 1000, 4, &ok);
        if (!ok) return;
        a2Step = QInputDialog::getDouble(this, "Alpha2 scan", "alpha2 step:", 0.5, 0.0001, 1000, 4, &ok);
        if (!ok) return;
    }
    if (transientPercent < 0 || transientPercent > 99) transientPercent = 70;
    if (sampleStride < 1) sampleStride = 20;

    // ✅ 현재 run 폴더에 params/info 저장 (scan도 같은 폴더에 남게)
    saveParams(runPath("params.txt"));
    writeRunInfoFile();

    QFile f3d(runPath("alpha2_scan_3d.dat"));
    QFile f2d(runPath("alpha2_scan_2d.dat"));
    if (!f3d.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot write alpha2_scan_3d.dat");
        return;
    }
    if (!f2d.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot write alpha2_scan_2d.dat");
        return;
    }

    QTextStream out3d(&f3d);
    QTextStream out2d(&f2d);

    const int steps = tMax;
    const double h = 0.01;
    const int transientStart = std::min(std::max(int(std::floor(steps * (transientPercent / 100.0))), 0), steps);

    for (double a2 = a2Min; a2 <= a2Max + 1e-12; a2 += a2Step) {
        const double oldAlpha2 = alpha2;
        alpha2 = a2;

        QVector<QVector<double>> y(5, QVector<double>(steps + 1));
        y[0][0] = 0.8;
        y[1][0] = 0.3;
        y[2][0] = 0.4;
        y[3][0] = 0.6;
        y[4][0] = 0.7;

        if (solverMode == "ODE") {
            for (int t = 1; t <= steps; ++t) {
                if (t % 400 == 0) QCoreApplication::processEvents();

                for (int i = 0; i < 5; ++i) {
                    double sum = -y[i][t - 1];

                    for (const auto& conn : connections) {
                        int from = conn.start->text().toInt() - 1;
                        int to   = conn.end->text().toInt() - 1;
                        if (to != i) continue;

                        QString key = "s" + conn.start->text() + conn.end->text();
                        double w = weightValues.value(key, 0.0);
                        double in = y[from][t - 1];

                        if (conn.function == "sin_exp") sum += w * sinEFunction(in);
                        else if (conn.function == "tanh") sum += w * tanhFunction(in);
                        else if (conn.function == "relu") sum += w * reluFunction(in);
                    }

                    if (i == 3) {
                        if (gateNode4.enabled) {
                            const double G2 = evalGateForNode(3, y[3][t - 1]);
                            sum += G2 * tanhFunction(y[3][t - 1]);
                        } else {
                            sum += (alpha2 - alpha3 * sinEFunction(y[4][t - 1]))
                            * tanhFunction(y[3][t - 1]);
                        }
                    }

                    if (i == 4) {
                        if (gateNode5.enabled) {
                            const double G1 = evalGateForNode(4, y[4][t - 1]);
                            sum += G1 * tanhFunction(y[4][t - 1]);
                        } else {
                            sum += (1 - alpha1 * tanhFunction(y[2][t - 1]))
                            * tanhFunction(y[4][t - 1]);
                        }
                    }

                    y[i][t] = y[i][t - 1] + h * sum;
                }

                if (t % sampleStride == 0 || t == steps) {
                    out3d << a2 << " " << t << " "
                          << y[0][t] << " " << y[1][t] << " " << y[2][t] << " "
                          << y[3][t] << " " << y[4][t] << "\n";
                }
            }
        } else {
            for (int om = 1; om <= steps; ++om) {
                if (om % 80 == 0) QCoreApplication::processEvents();

                for (int i = 0; i < 5; ++i) {
                    double acc = 0.0;

                    for (int r = 1; r <= om; ++r) {
                        double sum = -y[i][r - 1];

                        for (const auto& conn : connections) {
                            int from = conn.start->text().toInt() - 1;
                            int to   = conn.end->text().toInt() - 1;
                            if (to != i) continue;

                            QString key = "s" + conn.start->text() + conn.end->text();
                            double w = weightValues.value(key, 0.0);
                            double in = y[from][r - 1];

                            if (conn.function == "sin_exp") sum += w * sinEFunction(in);
                            else if (conn.function == "tanh") sum += w * tanhFunction(in);
                            else if (conn.function == "relu") sum += w * reluFunction(in);
                        }

                        if (i == 3) {
                            if (gateNode4.enabled) {
                                const double G2 = evalGateForNode(3, y[3][r - 1]);
                                sum += G2 * tanhFunction(y[3][r - 1]);
                            } else {
                                sum += (alpha2 - alpha3 * sinEFunction(y[4][r - 1]))
                                * tanhFunction(y[3][r - 1]);
                            }
                        }

                        if (i == 4) {
                            if (gateNode5.enabled) {
                                const double G1 = evalGateForNode(4, y[4][r - 1]);
                                sum += G1 * tanhFunction(y[4][r - 1]);
                            } else {
                                sum += (1 - alpha1 * tanhFunction(y[2][r - 1]))
                                * tanhFunction(y[4][r - 1]);
                            }
                        }

                        acc += sum * gammaWeight(om, r, nu);
                    }

                    y[i][om] = y[i][0] + acc;
                }

                if (om % sampleStride == 0 || om == steps) {
                    out3d << a2 << " " << om << " "
                          << y[0][om] << " " << y[1][om] << " " << y[2][om] << " "
                          << y[3][om] << " " << y[4][om] << "\n";
                }
            }
        }

        for (int t = transientStart; t <= steps; t += sampleStride) {
            out2d << a2 << " "
                  << y[0][t] << " " << y[1][t] << " " << y[2][t] << " "
                  << y[3][t] << " " << y[4][t] << "\n";
        }

        alpha2 = oldAlpha2;
        out3d << "\n";
        out2d << "\n";
    }

    f3d.close();
    f2d.close();

    generateAlpha2ScanGnuplotScripts();

    QProcess proc;
    proc.setWorkingDirectory(currentRunDir);
    proc.start("gnuplot", QStringList() << "alpha2_scan.gnu");
    proc.waitForFinished();

}

void ButtonNetwork::generateAlpha2ScanGnuplotScripts() const
{
    if (currentRunDir.isEmpty()) return;

    QFile script(runPath("alpha2_scan.gnu"));
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream g(&script);

    g << "set term pngcairo size 900,700\n";
    g << "set grid\n";
    g << "set xlabel 'alpha2'\n";
    g << "unset key\n";
    g << "set pointsize 0.6\n";

    const char* yNames[5] = {"y1","y2","y3","y4","y5"};
    const int col2d[5] = {2,3,4,5,6};

    for (int i = 0; i < 5; ++i) {
        g << "set output 'alpha2_" << yNames[i] << ".png'\n";
        g << "set ylabel '" << yNames[i] << "'\n";
        g << "plot 'alpha2_scan_2d.dat' using 1:" << col2d[i]
          << " with points pt 7 ps 0.4\n\n";
    }

    g << "set output\n";
}


