#include "buttonnetwork.h"

#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QDoubleSpinBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QWidget window;
    window.setWindowTitle("Hopfield Button Network (Debian/Qt)");
    window.resize(1200, 700);

    auto *root = new QHBoxLayout(&window);

    // Left: network canvas
    auto *net = new ButtonNetwork();
    net->setMinimumSize(700, 650);

    // Right: controls + log/table viewer
    auto *right = new QVBoxLayout();
    auto *log = new QTextEdit();
    log->setReadOnly(true);
    log->setPlaceholderText("Logs / output table / gnuplot errors...");

    net->updateEquationEditor(log);

    // Controls box
    auto *box = new QGroupBox("Controls");
    auto *boxL = new QVBoxLayout(box);

    auto *solverCombo = new QComboBox();
    solverCombo->addItem("ODE");
    solverCombo->addItem("GAMMA");

    auto *stepsSpin = new QSpinBox();
    stepsSpin->setRange(10, 50000);
    stepsSpin->setValue(800);

    auto *a2Min = new QDoubleSpinBox(); a2Min->setRange(-1000, 1000); a2Min->setValue(-10.0);
    auto *a2Max = new QDoubleSpinBox(); a2Max->setRange(-1000, 1000); a2Max->setValue( 10.0);
    auto *a2Step= new QDoubleSpinBox(); a2Step->setRange(0.0001, 1000); a2Step->setDecimals(4); a2Step->setValue(0.5);

    auto *transientSpin = new QSpinBox(); transientSpin->setRange(0, 99); transientSpin->setValue(70);
    auto *strideSpin    = new QSpinBox(); strideSpin->setRange(1, 10000); strideSpin->setValue(20);

    auto *btnCompute = new QPushButton("Compute");
    auto *btnGraph   = new QPushButton("Graph (y_all.png)");
    auto *btnTable   = new QPushButton("Show Table");
    auto *btnScanA2  = new QPushButton("Alpha2 Scan (PNG)");
    auto *btnAuto    = new QPushButton("AUTO Test Preset");
    auto *btnClear   = new QPushButton("Clear Network");

    boxL->addWidget(new QLabel("Solver"));
    boxL->addWidget(solverCombo);

    boxL->addWidget(new QLabel("tMax (steps)"));
    boxL->addWidget(stepsSpin);

    boxL->addWidget(new QLabel("alpha2 scan min / max / step"));
    boxL->addWidget(a2Min);
    boxL->addWidget(a2Max);
    boxL->addWidget(a2Step);

    boxL->addWidget(new QLabel("scan transient% / stride"));
    boxL->addWidget(transientSpin);
    boxL->addWidget(strideSpin);

    boxL->addSpacing(8);
    boxL->addWidget(btnCompute);
    boxL->addWidget(btnGraph);
    boxL->addWidget(btnTable);
    boxL->addWidget(btnScanA2);
    boxL->addWidget(btnAuto);
    boxL->addWidget(btnClear);

    right->addWidget(box);
    right->addWidget(log, 1);

    root->addWidget(net, 1);
    root->addLayout(right, 0);

    // Wiring
    QObject::connect(solverCombo, &QComboBox::currentTextChanged,
                     net, &ButtonNetwork::setSolverMode);
    QObject::connect(stepsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                     net, &ButtonNetwork::setTimeLimit);

    auto applyScanSettings = [&]() {
        net->setAlpha2ScanRange(a2Min->value(), a2Max->value(), a2Step->value());
        net->setAlpha2ScanSampling(transientSpin->value(), strideSpin->value());
    };

    QObject::connect(a2Min,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](){ applyScanSettings(); });
    QObject::connect(a2Max,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](){ applyScanSettings(); });
    QObject::connect(a2Step, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](){ applyScanSettings(); });
    QObject::connect(transientSpin, QOverload<int>::of(&QSpinBox::valueChanged), [&](){ applyScanSettings(); });
    QObject::connect(strideSpin,    QOverload<int>::of(&QSpinBox::valueChanged), [&](){ applyScanSettings(); });

    applyScanSettings();

    QObject::connect(btnCompute, &QPushButton::clicked, net, &ButtonNetwork::computeResults);
    QObject::connect(btnGraph,   &QPushButton::clicked, net, &ButtonNetwork::showGraph);
    QObject::connect(btnTable,   &QPushButton::clicked, net, &ButtonNetwork::showTable);
    QObject::connect(btnScanA2,  &QPushButton::clicked, net, &ButtonNetwork::scanAlpha2);
    QObject::connect(btnAuto,    &QPushButton::clicked, net, &ButtonNetwork::runAutoTestNode5Preset);
    QObject::connect(btnClear,   &QPushButton::clicked, net, &ButtonNetwork::clearNetwork);

    QObject::connect(net, &ButtonNetwork::fileSaved, [&](const QString& p){
        log->append("[saved] " + p);
    });

    window.show();
    return a.exec();
}
