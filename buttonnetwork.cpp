#include "buttonnetwork.h"
#include "output/resultwriter.h"
#include "validation/validationrunner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
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
#include <QCheckBox>
#include <QComboBox>
#include <gsl/gsl_sf_gamma.h>
#include <cmath>

ButtonNetwork::ButtonNetwork(QWidget *parent) : QWidget(parent), model(params), solver(params, model)
{
    setMouseTracking(true);

    bool ok;
    int userInput = QInputDialog::getInt(
        this, "Number of Nodes", "How many nodes?", 5, 1, 100, 1, &ok);
    if (ok) maxNodes = userInput;

    baseResultDir = QDir::homePath() + "/ButtonNetwork/result";

    // Defaults for demo
    params.gateNode4.enabled = true;
    params.gateNode4.baseType = "alpha2";
    params.gateNode4.baseConst = 1.0;
    params.gateNode4.coeff = 1.2;
    params.gateNode4.fn = "sin";

    params.gateNode5.enabled = true;
    params.gateNode5.baseType = "const";
    params.gateNode5.baseConst = 1.0;
    params.gateNode5.coeff = 2.2;
    params.gateNode5.fn = "tanh";
}

void ButtonNetwork::updateEquationEditor(QTextEdit* editor)
{
    equationEditor = editor;
}

void ButtonNetwork::mousePressEvent(QMouseEvent *event)
{
    // 1) click connection to edit
    const int hit = findClickedConnectionIndex(event->pos());
    if (hit >= 0) {
        editConnectionAt(hit);
        return;
    }

    // 2) create node
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
        QString label = "s" + conn.start->text() + conn.end->text();
        double val = weightValues.value(label, 0.0);

        // Self-loop on node4 or node5 => show G2 / G1 label (visual)
        if (conn.start == conn.end && (sIdx == 4 || sIdx == 5)) {
            if (sIdx == 4) label = "G2";
            if (sIdx == 5) label = "G1";
        }

        p.setPen(QPen(conn.color, 3));

        if (conn.start == conn.end) {
            QRectF loop(start.x() - 20, start.y() - 40, 40, 40);
            p.drawArc(loop, 0, 360 * 16);

            if (sIdx == 4) {
                const double base = model.baseValueFromType(params.gateNode4.baseType, params.gateNode4.baseConst);
                p.drawText(start.x() - 80, start.y() - 50,
                           QString("G2=%1-%2*%3(y4)")
                               .arg(base, 0, 'f', 3)
                               .arg(params.gateNode4.coeff, 0, 'f', 3)
                               .arg(params.gateNode4.fn));
            } else if (sIdx == 5) {
                const double base = model.baseValueFromType(params.gateNode5.baseType, params.gateNode5.baseConst);
                p.drawText(start.x() - 80, start.y() - 50,
                           QString("G1=%1-%2*%3(y5)")
                               .arg(base, 0, 'f', 3)
                               .arg(params.gateNode5.coeff, 0, 'f', 3)
                               .arg(params.gateNode5.fn));
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

// ================= Run folder =================

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
    out << "Solver: " << params.solverMode << "\n";
    out << "tMax: " << params.tMax << "\n";
    out << "alpha1=" << params.alpha1 << " alpha2=" << params.alpha2 << " alpha3=" << params.alpha3 << "\n";
    out << "nu=" << params.nu << "\n";
    out << "Gate4(G2): enabled=" << params.gateNode4.enabled
        << " base=" << params.gateNode4.baseType << "(" << params.gateNode4.baseConst << ")"
        << " coeff=" << params.gateNode4.coeff << " fn=" << params.gateNode4.fn << "\n";
    out << "Gate5(G1): enabled=" << params.gateNode5.enabled
        << " base=" << params.gateNode5.baseType << "(" << params.gateNode5.baseConst << ")"
        << " coeff=" << params.gateNode5.coeff << " fn=" << params.gateNode5.fn << "\n\n";

    out << "Connections:\n";
    for (const auto& c : connections) {
        QString key = "s" + c.start->text() + c.end->text();
        out << " " << key << " = " << weightValues.value(key, 0.0)
            << " fn=" << c.function << "\n";
    }
    f.close();
}

void ButtonNetwork::saveParams(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);

    out << "solverMode=" << params.solverMode << "\n";
    out << "tMax=" << params.tMax << "\n";
    out << "alpha1=" << params.alpha1 << "\n";
    out << "alpha2=" << params.alpha2 << "\n";
    out << "alpha3=" << params.alpha3 << "\n";
    out << "nu=" << params.nu << "\n";

    out << "GateNode4.enabled=" << params.gateNode4.enabled << "\n";
    out << "GateNode4.baseType=" << params.gateNode4.baseType << "\n";
    out << "GateNode4.baseConst=" << params.gateNode4.baseConst << "\n";
    out << "GateNode4.coeff=" << params.gateNode4.coeff << "\n";
    out << "GateNode4.fn=" << params.gateNode4.fn << "\n";

    out << "GateNode5.enabled=" << params.gateNode5.enabled << "\n";
    out << "GateNode5.baseType=" << params.gateNode5.baseType << "\n";
    out << "GateNode5.baseConst=" << params.gateNode5.baseConst << "\n";
    out << "GateNode5.coeff=" << params.gateNode5.coeff << "\n";
    out << "GateNode5.fn=" << params.gateNode5.fn << "\n";

    out << "\n[weights]\n";
    for (auto it = weightValues.begin(); it != weightValues.end(); ++it) {
        out << it.key() << "=" << it.value() << "\n";
    }
    f.close();
}

// ================= Solver core =================



void ButtonNetwork::computeResults()
{
    if (!createNewRunDir()) return;

    saveParams(runPath("params.txt"));
    writeRunInfoFile();

    QVector<SolverConnection> solverConnections;
    solverConnections.reserve(connections.size());

    for (const auto& conn : connections) {
        SolverConnection sc;
        sc.firstNode  = conn.start->text().toInt();
        sc.secondNode = conn.end->text().toInt();
        sc.function   = conn.function;
        solverConnections.append(sc);
    }

    QVector<QVector<double>> y;

    if (params.solverMode == "ODE") {
        y = solver.runODE(solverConnections, weightValues);
    } else {
        y = solver.runGamma(buttons.size(),
                            solverConnections,
                            weightValues);
    }

    saveAndDisplayResult(y, params.tMax);
}


void ButtonNetwork::saveAndDisplayResult(const QVector<QVector<double>>& y, int steps)
{
    if (!ResultWriter::writeResults(currentRunDir, y, steps)) {
        QMessageBox::critical(this, "Error", "Cannot write result files");
        return;
    }

    emit fileSaved(runPath("result.dat"));
}


// ================= UI helpers =================

void ButtonNetwork::showTable()
{
    showOutputTable();
}

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

void ButtonNetwork::setSolverMode(const QString& mode) { params.solverMode = mode; }
void ButtonNetwork::setTimeLimit(int t) { params.tMax = t; }
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

// ================= gnuplot: y_all =================

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

    out << "set ylabel 'y1'\n";
    out << "plot 'result.dat' using 0:1 with lines linewidth 2 title 'y1'\n\n";
    out << "set ylabel 'y2'\n";
    out << "plot 'result.dat' using 0:2 with lines linewidth 2 title 'y2'\n\n";
    out << "set ylabel 'y3'\n";
    out << "plot 'result.dat' using 0:3 with lines linewidth 2 title 'y3'\n\n";
    out << "set ylabel 'y4'\n";
    out << "plot 'result.dat' using 0:4 with lines linewidth 2 title 'y4'\n\n";
    out << "set ylabel 'y5'\n";
    out << "plot 'result.dat' using 0:5 with lines linewidth 2 title 'y5'\n\n";

    out << "unset multiplot\n";
    out << "set output\n";

    script.close();
}

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
    proc.start("gnuplot", QStringList() << "plot.gnu");

    if (!proc.waitForStarted()) {
        QMessageBox::critical(this, "Error", "Failed to start gnuplot. Is it installed?");
        return;
    }

    if (!proc.waitForFinished(-1) || proc.exitCode() != 0) {
        QMessageBox::critical(this, "Error",
                              "gnuplot failed. Check if pngcairo is available.");
        return;
    }

    emit fileSaved(runPath("y_all.png"));

#ifdef Q_OS_LINUX
    QProcess::startDetached("xdg-open", QStringList() << runPath("y_all.png"));
#endif
}

// ================= Alpha2 scan =================

void ButtonNetwork::scanAlpha2()
{
    if (!createNewRunDir()) return;
    saveParams(runPath("params.txt"));
    writeRunInfoFile();
    scanAlpha2ReuseCurrentRun();
}

void ButtonNetwork::scanAlpha2ReuseCurrentRun()
{
    if (currentRunDir.isEmpty()) {
        QMessageBox::warning(this, "Error", "No run folder. Press Compute first (or Auto Test).");
        return;
    }

    double a2Min = scanAlpha2Min;
    double a2Max = scanAlpha2Max;
    double a2Step = scanAlpha2Step;
    int transientPercent = scanTransientPercent;
    int sampleStride = scanSampleStride;

    if (!(a2Step > 0.0) || a2Max < a2Min) {
        bool ok = true;
        a2Min = QInputDialog::getDouble(this, "Alpha2 scan", "alpha2 min:", -10.0, -1000, 1000, 4, &ok);
        if (!ok) return;
        a2Max = QInputDialog::getDouble(this, "Alpha2 scan", "alpha2 max:",  10.0, -1000, 1000, 4, &ok);
        if (!ok) return;
        a2Step = QInputDialog::getDouble(this, "Alpha2 scan", "alpha2 step:", 0.5, 0.0001, 1000, 4, &ok);
        if (!ok) return;
    }
    if (transientPercent < 0 || transientPercent > 99) transientPercent = 70;
    if (sampleStride < 1) sampleStride = 20;

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

    const int steps = params.tMax;
    const int transientStart =
        std::min(std::max(
            int(std::floor(steps * (transientPercent / 100.0))),
            0),
            steps);

    QVector<SolverConnection> solverConnections;
    solverConnections.reserve(connections.size());

    for (const auto& conn : connections) {
        SolverConnection sc;
        sc.firstNode  = conn.start->text().toInt();
        sc.secondNode = conn.end->text().toInt();
        sc.function   = conn.function;
        solverConnections.append(sc);
    }

    for (double a2 = a2Min;
         a2 <= a2Max + 1e-12;
         a2 += a2Step) {

        const double oldAlpha2 = params.alpha2;
        params.alpha2 = a2;

        QVector<QVector<double>> y;

        if (params.solverMode == "ODE") {
            y = solver.runODE(solverConnections, weightValues);
        } else {
            y = solver.runGamma(buttons.size(),
                                solverConnections,
                                weightValues);
        }

        for (int t = sampleStride; t <= steps; t += sampleStride) {
            out3d << a2 << " " << t << " "
                  << y[0][t] << " "
                  << y[1][t] << " "
                  << y[2][t] << " "
                  << y[3][t] << " "
                  << y[4][t] << "\n";
        }

        if (steps % sampleStride != 0) {
            out3d << a2 << " " << steps << " "
                  << y[0][steps] << " "
                  << y[1][steps] << " "
                  << y[2][steps] << " "
                  << y[3][steps] << " "
                  << y[4][steps] << "\n";
        }

        for (int t = transientStart;
             t <= steps;
             t += sampleStride) {

            out2d << a2 << " "
                  << y[0][t] << " "
                  << y[1][t] << " "
                  << y[2][t] << " "
                  << y[3][t] << " "
                  << y[4][t] << "\n";
        }

        params.alpha2 = oldAlpha2;

        out3d << "\n";
        out2d << "\n";
    }

    f3d.close();
    f2d.close();

    generateAlpha2ScanGnuplotScripts();

    QProcess proc;
    proc.setWorkingDirectory(currentRunDir);
    proc.start("gnuplot", QStringList() << "alpha2_scan.gnu");

    if (!proc.waitForStarted()) {
        QMessageBox::warning(this, "Gnuplot", "Failed to start gnuplot. Is it installed?");
        return;
    }

    const bool finished = proc.waitForFinished(-1);
    const QString gpStdout = QString::fromLocal8Bit(proc.readAllStandardOutput());
    const QString gpStderr = QString::fromLocal8Bit(proc.readAllStandardError());

    QFile gpLog(runPath("alpha2_gnuplot_log.txt"));
    if (gpLog.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream gl(&gpLog);
        gl << "=== gnuplot stdout ===\n" << gpStdout << "\n\n";
        gl << "=== gnuplot stderr ===\n" << gpStderr << "\n";
        gpLog.close();
    }

    if (equationEditor) {
        if (!gpStderr.trimmed().isEmpty()) equationEditor->append("\n[gnuplot stderr]\n" + gpStderr);
        if (!gpStdout.trimmed().isEmpty()) equationEditor->append("\n[gnuplot stdout]\n" + gpStdout);
    }

    if (!finished || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        QMessageBox::warning(this, "Gnuplot",
                             "alpha2 scan data saved, but gnuplot failed.\n"
                             "See alpha2_gnuplot_log.txt in the run folder.");
        return;
    }

    emit fileSaved(runPath("alpha2_y1.png"));
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
    script.close();
}

// ================= Click-edit connections =================

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

        // curved quad
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

    // self-loop on node4 or node5 => edit Gate
    if (conn.start == conn.end && (sIdx == 4 || sIdx == 5)) {

        GateConfig* gate = (sIdx == 4) ? &params.gateNode4 : &params.gateNode5;
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
        connect(&cancelBtn, &QPushButton::clicked, [&]() { dialog.reject(); });

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

// ================= Auto preset =================

bool ButtonNetwork::copyOverwrite(const QString& src, const QString& dst) const
{
    if (!QFileInfo::exists(src)) return false;
    QFile::remove(dst);
    return QFile::copy(src, dst);
}

void ButtonNetwork::ensurePresetNodes5()
{
    if (buttons.size() >= 5) return;

    const QPoint centers[5] = {
        QPoint(120, 120),
        QPoint(260, 220),
        QPoint(120, 320),
        QPoint(320, 120),
        QPoint(360, 320)
    };

    while (buttons.size() < 5) {
        int idx = buttons.size();
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
    if (from < 1 || from > buttons.size()) return;
    if (to < 1 || to > buttons.size()) return;

    QPushButton* start = buttons[from - 1];
    QPushButton* end   = buttons[to - 1];

    QColor color = Qt::yellow;
    if (fn == "tanh") color = Qt::black;
    else if (fn == "relu") color = Qt::blue;

    QString key = "s" + QString::number(from) + QString::number(to);
    weightValues[key] = w;

    for (Connection& c : connections) {
        if (c.start == start && c.end == end) {
            c.function = fn;
            c.color = color;
            update();
            return;
        }
    }

    connections.append({start, end, color, fn});
    update();
}

void ButtonNetwork::runAutoTestNode5Preset()
{
    // Prompt user for alpha2 value
    bool ok;
    double userAlpha2 = QInputDialog::getDouble(
        this,
        "Set Alpha2 for Test",
        "Enter alpha2 value (0.0, 1.0, 3.0, or 6.0):",
        0.01,      // default
        -100.0,   // min
        100.0,    // max
        2,        // decimals
        &ok
        );

    if (!ok) return;

    // SET ALPHA2 HERE!
    // alpha2 = userAlpha2;
    // alpha1 = 1.0;
    // alpha3 = 1.0;
    // nu = 0.70;
    // tMax = 5000;
    // solverMode = "GAMMA";
    ValidationRunner::configureFiveNodePreset(params, userAlpha2);

    ensurePresetNodes5();
    connections.clear();
    weightValues.clear();

    for (const auto& c : ValidationRunner::fiveNodeConnections()) {
        addOrUpdateConnection(c.from, c.to, c.weight, c.function);
    }

    if (equationEditor) {
        equationEditor->append(QString("\n[AUTO TEST] alpha2 = %1, nu = %2\n")
                                   .arg(params.alpha2).arg(params.nu));
    }

    computeResults();
    const QString runDirFixed = currentRunDir;
    showTable();
    showGraph();
    scanAlpha2ReuseCurrentRun();  // ADD THIS LINE - generates bifurcation diagrams

    // Copy to test files with alpha2 in filename
    QString a2str = QString::number(params.alpha2, 'f', 2).replace(".", "_");

    auto rp = [&](const QString& name){ return runDirFixed + "/" + name; };

    copyOverwrite(rp("y_all.png"),
                  rp(QString("test_y_all_a2_%1.png").arg(a2str)));
    copyOverwrite(rp("alpha2_y1.png"),
                  rp(QString("test_alpha2_y1.png")));  // ADD THIS
    copyOverwrite(rp("alpha2_y2.png"),
                  rp(QString("test_alpha2_y2.png")));  // ADD THIS
    copyOverwrite(rp("alpha2_y3.png"),
                  rp(QString("test_alpha2_y3.png")));  // ADD THIS
    copyOverwrite(rp("alpha2_y4.png"),
                  rp(QString("test_alpha2_y4.png")));  // ADD THIS
    copyOverwrite(rp("alpha2_y5.png"),
                  rp(QString("test_alpha2_y5.png")));  // ADD THIS

    if (equationEditor) {
        equationEditor->append(QString("\n[AUTO TEST] Saved bifurcation diagrams\n"));
    }
}
