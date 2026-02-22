#include "buttonnetwork.h"

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
#include <QCoreApplication> //long loop 동안 UI 멈추는 것 방지
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QCheckBox> // 옵션 ON/OFF 체크
#include <QComboBox> //함수 선택

#include <cmath>

ButtonNetwork::ButtonNetwork(QWidget *parent) : QWidget(parent) //passing parent ensures proper Qt ownership and event propagation.
{
    setMouseTracking(true); //마우스를 누르지 않아도 mouse move 이벤트를 받을 수 있게

    bool ok;
    int userInput = QInputDialog::getInt(
        this, "Number of Nodes", "How many nodes?", 5, 1, 100, 1, &ok);
    if (ok) maxNodes = userInput;

    baseResultDir = QDir::homePath() + "/ButtonNetwork/result";

    // Defaults for demo
    gateNode4.enabled = true;
    gateNode4.baseType = "alpha2";
    gateNode4.baseConst = 1.0;
    gateNode4.coeff = 1.2;
    gateNode4.fn = "sin";

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

    QPushButton* btn = new QPushButton(QString::number(buttons.size() + 1), this); //버튼에 노드 번호 label 붙이는 역할,
    //부모가 삭제되면 자식 위젯도 자동으로 삭제됨 (Qt parent-child ownership)
    btn->setGeometry(event->pos().x(), event->pos().y(), 40, 40);
    btn->setStyleSheet("border-radius: 20px; background-color: lightgray;");
    btn->show();
    connect(btn, &QPushButton::clicked, this, &ButtonNetwork::buttonClicked);
    buttons.append(btn);
}

void ButtonNetwork::buttonClicked() // 노드 버튼을 두 번 선택해서 연결(Edge)을 만들기 위한 로직
{
    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender()); //sender :signal을 보낸 객체를 얻음,qobject_cast : Qt-safe cast, 실패 시 nullptr
    //캐스팅은 데이터 타입을 강제로 변환
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

    connect(&confirmButton, &QPushButton::clicked, [&]() { //필요한 외부 변수를 참조로 가져다 쓰겠다”
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

// ================= Gate helpers =================

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
//nodeIndex가 node4 또는 node5일 때, 그 노드의 Gate(G2 또는 G1)를 yValue(현재 상태 값)에기반해서계산해 반환.
//다른 노드면 0.0을 반환
double ButtonNetwork::evalGateForNode(int nodeIndex, double yValue) const
{
    const GateConfig* gate = nullptr;
    if (nodeIndex == 3) gate = &gateNode4; // node4
    if (nodeIndex == 4) gate = &gateNode5; // node5
    if (!gate) return 0.0;

    const double base = baseValueFromType(gate->baseType, gate->baseConst);
    const double fnv  = applyFn(gate->fn, yValue);
    return base - gate->coeff * fnv;
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
}  //warning:문제는있지만 계속진행 critical: 문제가 심각 작업실패, 중단”

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

    QTextStream out(&f); //f는 QFile 객체,TextStream writes formatted text to a QIODevice
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

// ================= Solver core =================

double ButtonNetwork::sinEFunction(double x) { return std::sin(x); }
double ButtonNetwork::tanhFunction(double x) { return std::tanh(x); }
double ButtonNetwork::reluFunction(double x) { return (x > 0.0) ? x : 0.0; }


double ButtonNetwork::gammaWeight(int om, int r, double nu)
{
    double k = om - r;
    return std::pow(k + 1.0, nu) - std::pow(k, nu);
}


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
    y[0][0] = 0.8; y[1][0] = 0.3; y[2][0] = 0.4; y[3][0] = 0.6; y[4][0] = 0.7;

    for (int t = 1; t <= steps; ++t) {
        if (t % 400 == 0) QCoreApplication::processEvents();

        for (int i = 0; i < 5; ++i) {
            double sum = -y[i][t - 1];

            for (const auto& conn : connections) { //버튼에 적힌 노드 번호가 1부터 시작인데, 배열/벡터 인덱스는 0부터 시작
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

            y[i][t] = y[i][t - 1] + h * sum; // Euler
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

        for (int i = 0; i < 5; ++i)
            y[i][om] = 0.0;  // important (same as C code)

        for (int r = 1; r <= om; ++r) {

            double bg = gammaWeight(om, r, nu);

            // y0
            y[0][om] += (
                            -y[0][r-1]
                            + weightValues.value("s12",0.0)*tanhFunction(y[1][r-1])
                            + weightValues.value("s13",0.0)*sinEFunction(y[2][r-1])
                            + weightValues.value("s14",0.0)*sinEFunction(y[3][r-1])
                            ) * bg;

            // y1
            y[1][om] += (
                            -y[1][r-1]
                            + weightValues.value("s21",0.0)*sinEFunction(y[0][r-1])
                            + weightValues.value("s23",0.0)*sinEFunction(y[2][r-1])
                            + weightValues.value("s25",0.0)*sinEFunction(y[4][r-1])
                            ) * bg;

            // y2
            y[2][om] += (
                            -y[2][r-1]
                            + weightValues.value("s31",0.0)*tanhFunction(y[0][r-1])
                            + weightValues.value("s32",0.0)*tanhFunction(y[1][r-1])
                            + weightValues.value("s33",0.0)*sinEFunction(y[2][r-1])
                            ) * bg;

            // y3 (node4)
            double gate4Term;
            if (gateNode4.enabled) {
                const double G2 = evalGateForNode(3, y[3][r-1]);
                gate4Term = G2 * tanhFunction(y[3][r-1]);
            } else {
                gate4Term = (alpha2 - alpha3*sinEFunction(y[4][r-1]))
                * tanhFunction(y[3][r-1]);
            }

            y[3][om] += (
                            -y[3][r-1]
                            + weightValues.value("s41",0.0)*tanhFunction(y[0][r-1])
                            + gate4Term
                            ) * bg;

            // y4 (node5)
            double gate5Term;
            if (gateNode5.enabled) {
                const double G1 = evalGateForNode(4, y[4][r-1]);
                gate5Term = G1 * tanhFunction(y[4][r-1]);
            } else {
                gate5Term = (1.0 - alpha1*tanhFunction(y[2][r-1]))
                * tanhFunction(y[4][r-1]);
            }

            y[4][om] += (
                            -y[4][r-1]
                            + weightValues.value("s52",0.0)*tanhFunction(y[1][r-1])
                            + gate5Term
                            ) * bg;
        }

        // add initial condition (same as C code)
        for (int i = 0; i < 5; ++i)
            y[i][om] += y[i][0];
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

void ButtonNetwork::setSolverMode(const QString& mode) { solverMode = mode; }
void ButtonNetwork::setTimeLimit(int t) { tMax = t; }
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
    //Qt에서 외부 프로그램(gnuplot)을 실행해서 PNG 그래프를 만든 다음, 성공하면 신호(signal)를 보내고, Linux에서는 파일을 열어주는 흐름
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
//currentRunDir를 그대로 사용해 alpha2 값을 여러 개로 바꿔가며 시뮬레이션을 반복 실행,  결과파일로 저장(gnuplot으로 PNG도 만들려고 시도)하는alpha2 파라미터 스윕/스캔 함수
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
        a2Min = QInputDialog::getDouble(this, "Alpha2 scan", "alpha2 min:", -10.0, -1000, 1000, 4, &ok); //text is default value
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

    const int steps = tMax;
    const double h = 0.01;
    const int transientStart = std::min(std::max(int(std::floor(steps * (transientPercent / 100.0))), 0), steps);

    for (double a2 = a2Min; a2 <= a2Max + 1e-12; a2 += a2Step) {
        const double oldAlpha2 = alpha2;
        alpha2 = a2;

        QVector<QVector<double>> y(5, QVector<double>(steps + 1));
        y[0][0] = 0.8; y[1][0] = 0.3; y[2][0] = 0.4; y[3][0] = 0.6; y[4][0] = 0.7;

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
                        const double G2 = gateNode4.enabled ? evalGateForNode(3, y[3][t - 1])
                                                            : (alpha2 - alpha3 * sinEFunction(y[4][t - 1]));
                        sum += G2 * tanhFunction(y[3][t - 1]);
                    }
                    if (i == 4) {
                        const double G1 = gateNode5.enabled ? evalGateForNode(4, y[4][t - 1])
                                                            : (1 - alpha1 * tanhFunction(y[2][t - 1]));
                        sum += G1 * tanhFunction(y[4][t - 1]);
                    }

                    y[i][t] = y[i][t - 1] + h * sum;
                }

                if (t % sampleStride == 0 || t == steps) {
                    out3d << a2 << " " << t << " "
                          << y[0][t] << " " << y[1][t] << " " << y[2][t] << " "
                          << y[3][t] << " " << y[4][t] << "\n";
                }
            }
        }
        else {
            // GAMMA MODE
            for (int om = 1; om <= steps; ++om) {
                if (om % 100 == 0) QCoreApplication::processEvents();

                for (int i = 0; i < 5; ++i)
                    y[i][om] = 0.0;

                for (int r = 1; r <= om; ++r) {
                    double bg = gammaWeight(om, r, nu);

                    y[0][om] += (-y[0][r-1] + weightValues.value("s12",0.0)*tanhFunction(y[1][r-1])
                                 + weightValues.value("s13",0.0)*sinEFunction(y[2][r-1])
                                 + weightValues.value("s14",0.0)*sinEFunction(y[3][r-1])) * bg;

                    y[1][om] += (-y[1][r-1] + weightValues.value("s21",0.0)*sinEFunction(y[0][r-1])
                                 + weightValues.value("s23",0.0)*sinEFunction(y[2][r-1])
                                 + weightValues.value("s25",0.0)*sinEFunction(y[4][r-1])) * bg;

                    y[2][om] += (-y[2][r-1] + weightValues.value("s31",0.0)*tanhFunction(y[0][r-1])
                                 + weightValues.value("s32",0.0)*tanhFunction(y[1][r-1])
                                 + weightValues.value("s33",0.0)*sinEFunction(y[2][r-1])) * bg;

                    double gate4Term;
                    if (gateNode4.enabled) {
                        const double G2 = evalGateForNode(3, y[3][r-1]);
                        gate4Term = G2 * tanhFunction(y[3][r-1]);
                    } else {
                        gate4Term = (alpha2 - alpha3*sinEFunction(y[4][r-1])) * tanhFunction(y[3][r-1]);
                    }
                    y[3][om] += (-y[3][r-1] + weightValues.value("s41",0.0)*tanhFunction(y[0][r-1]) + gate4Term) * bg;

                    double gate5Term;
                    if (gateNode5.enabled) {
                        const double G1 = evalGateForNode(4, y[4][r-1]);
                        gate5Term = G1 * tanhFunction(y[4][r-1]);
                    } else {
                        gate5Term = (1.0 - alpha1*tanhFunction(y[2][r-1])) * tanhFunction(y[4][r-1]);
                    }
                    y[4][om] += (-y[4][r-1] + weightValues.value("s52",0.0)*tanhFunction(y[1][r-1]) + gate5Term) * bg;
                }

                for (int i = 0; i < 5; ++i)
                    y[i][om] += y[i][0];

                if (om % sampleStride == 0 || om == steps) {
                    out3d << a2 << " " << om << " " << y[0][om] << " " << y[1][om] << " "
                          << y[2][om] << " " << y[3][om] << " " << y[4][om] << "\n";
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

    if (!proc.waitForStarted()) {
        QMessageBox::warning(this, "Gnuplot", "Failed to start gnuplot. Is it installed?");
        return;
    }
//nuplot 실행 결과가 실패인지 성공인지 판단해서, 실패면 경고 띄우고 종료 / 성공이면 파일 저장 완료 signal을 emit
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
//선(연결선)을 클릭했는지 판단하는 hit-testing 용도
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
//같은 이름이 이미 있으면 새로 만들 때 덮어써야 하니까
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
    nu = 0.70;
    solverMode = "GAMMA";
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

    if (equationEditor) equationEditor->append("\n[AUTO TEST Node5] preset applied. Running...\n");

    computeResults();
    const QString runDirFixed = currentRunDir;

    showGraph();
    scanAlpha2ReuseCurrentRun();

    auto rp = [&](const QString& name){ return runDirFixed + "/" + name; };

    copyOverwrite(rp("result.dat"),        rp("test_result.dat"));
    copyOverwrite(rp("result_stream.csv"), rp("test_result_stream.csv"));
    copyOverwrite(rp("result_final.csv"),  rp("test_result_final.csv"));
    copyOverwrite(rp("params.txt"),        rp("test_params.txt"));
    copyOverwrite(rp("table.txt"),         rp("test_table.txt"));
    copyOverwrite(rp("plot.gnu"),          rp("test_plot.gnu"));
    copyOverwrite(rp("y_all.png"),         rp("test_y_all.png"));

    copyOverwrite(rp("alpha2_scan_3d.dat"), rp("test_alpha2_scan_3d.dat"));
    copyOverwrite(rp("alpha2_scan_2d.dat"), rp("test_alpha2_scan_2d.dat"));
    copyOverwrite(rp("alpha2_y1.png"),      rp("test_alpha2_y1.png"));
    copyOverwrite(rp("alpha2_y2.png"),      rp("test_alpha2_y2.png"));
    copyOverwrite(rp("alpha2_y3.png"),      rp("test_alpha2_y3.png"));
    copyOverwrite(rp("alpha2_y4.png"),      rp("test_alpha2_y4.png"));
    copyOverwrite(rp("alpha2_y5.png"),      rp("test_alpha2_y5.png"));

    if (equationEditor) equationEditor->append("\n[AUTO TEST Node5] done. Saved test_* in:\n" + runDirFixed + "\n");
}
