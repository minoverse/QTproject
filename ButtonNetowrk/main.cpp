#include <QApplication>
#include <QSplitter>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include "buttonnetwork.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    qputenv("QT_ACCESSIBILITY", "0");

    QWidget* mainWindow = new QWidget;
    mainWindow->setWindowTitle("Hopfield Fractional Network");
    mainWindow->resize(1500, 800);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, mainWindow);
    ButtonNetwork* builder = new ButtonNetwork;

    QTextEdit* outputView = new QTextEdit;
    outputView->setReadOnly(true);
    outputView->setMinimumWidth(700);
    builder->updateEquationEditor(outputView);

    splitter->addWidget(builder);
    splitter->addWidget(outputView);

    QList<int> sizes;
    sizes << 600 << 900;
    splitter->setSizes(sizes);

    QPushButton* clearBtn    = new QPushButton("Clear");
    QPushButton* computeBtn  = new QPushButton("Compute");
    QPushButton* graphBtn    = new QPushButton("Show Graph");
    QPushButton* tableBtn    = new QPushButton("Show Output Table");
    QPushButton* scanBtn     = new QPushButton("Scan Alpha2 (3D/2D)");

    // ✅ NEW: Auto Test button (Node5 preset)
    QPushButton* autoTestBtn = new QPushButton("Auto Test (Node5 preset)");

    QComboBox* solverMode = new QComboBox;
    solverMode->addItem("ODE");
    solverMode->addItem("Fractional (Gamma)");

    QSpinBox* tmaxInput = new QSpinBox;
    tmaxInput->setRange(100, 100000);
    tmaxInput->setValue(5000);
    tmaxInput->setSingleStep(500);

    QHBoxLayout* topOptions = new QHBoxLayout;
    topOptions->addWidget(new QLabel("Solver:"));
    topOptions->addWidget(solverMode);
    topOptions->addWidget(new QLabel("t_max:"));
    topOptions->addWidget(tmaxInput);

    // ------------------------------------------------------------
    // alpha2 scan inputs (UI)
    // ------------------------------------------------------------
    QDoubleSpinBox* a2Min = new QDoubleSpinBox;
    QDoubleSpinBox* a2Max = new QDoubleSpinBox;
    QDoubleSpinBox* a2Step = new QDoubleSpinBox;
    a2Min->setRange(-1000, 1000); a2Min->setDecimals(4); a2Min->setValue(-10.0);
    a2Max->setRange(-1000, 1000); a2Max->setDecimals(4); a2Max->setValue(10.0);
    a2Step->setRange(0.0001, 1000); a2Step->setDecimals(4); a2Step->setValue(0.5);

    QSpinBox* transientPct = new QSpinBox;
    transientPct->setRange(0, 99); transientPct->setValue(70);
    QSpinBox* sampleStride = new QSpinBox;
    sampleStride->setRange(1, 1000000); sampleStride->setValue(20);

    QHBoxLayout* scanOptions = new QHBoxLayout;
    scanOptions->addWidget(new QLabel("alpha2 min:"));
    scanOptions->addWidget(a2Min);
    scanOptions->addWidget(new QLabel("max:"));
    scanOptions->addWidget(a2Max);
    scanOptions->addWidget(new QLabel("step:"));
    scanOptions->addWidget(a2Step);
    scanOptions->addWidget(new QLabel("transient %:"));
    scanOptions->addWidget(transientPct);
    scanOptions->addWidget(new QLabel("stride:"));
    scanOptions->addWidget(sampleStride);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(clearBtn);
    buttonLayout->addWidget(computeBtn);
    buttonLayout->addWidget(graphBtn);
    buttonLayout->addWidget(tableBtn);
    buttonLayout->addWidget(scanBtn);

    // ✅ NEW: add Auto Test button (does not affect existing buttons)
    buttonLayout->addWidget(autoTestBtn);

    QVBoxLayout* layout = new QVBoxLayout(mainWindow);
    layout->addLayout(topOptions);
    layout->addLayout(scanOptions);
    layout->addWidget(splitter);
    layout->addLayout(buttonLayout);

    QObject::connect(clearBtn,  &QPushButton::clicked, builder, &ButtonNetwork::clearNetwork);
    QObject::connect(computeBtn,&QPushButton::clicked, builder, &ButtonNetwork::computeResults);
    QObject::connect(graphBtn,  &QPushButton::clicked, builder, &ButtonNetwork::showGraph);
    QObject::connect(tableBtn,  &QPushButton::clicked, builder, &ButtonNetwork::showTable);
    QObject::connect(scanBtn,   &QPushButton::clicked, builder, &ButtonNetwork::scanAlpha2);

    // ✅ NEW: Auto Test (Node5 preset) 1-time run
    QObject::connect(autoTestBtn, &QPushButton::clicked, builder, &ButtonNetwork::runAutoTestNode5Preset);

    QObject::connect(solverMode,&QComboBox::currentTextChanged, builder, &ButtonNetwork::setSolverMode);
    QObject::connect(tmaxInput, qOverload<int>(&QSpinBox::valueChanged),
                     builder, &ButtonNetwork::setTimeLimit);

    // alpha2 scan UI -> builder
    auto pushScanParams = [=]() {
        builder->setAlpha2ScanRange(a2Min->value(), a2Max->value(), a2Step->value());
        builder->setAlpha2ScanSampling(transientPct->value(), sampleStride->value());
    };
    QObject::connect(a2Min, qOverload<double>(&QDoubleSpinBox::valueChanged), pushScanParams);
    QObject::connect(a2Max, qOverload<double>(&QDoubleSpinBox::valueChanged), pushScanParams);
    QObject::connect(a2Step, qOverload<double>(&QDoubleSpinBox::valueChanged), pushScanParams);
    QObject::connect(transientPct, qOverload<int>(&QSpinBox::valueChanged), pushScanParams);
    QObject::connect(sampleStride, qOverload<int>(&QSpinBox::valueChanged), pushScanParams);
    pushScanParams();

    QObject::connect(builder, &ButtonNetwork::fileSaved,
                     [=](const QString& path) {
                         QFile file(path);
                         if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                             QTextStream in(&file);
                             outputView->setPlainText(in.readAll());
                         }
                     });

    mainWindow->setLayout(layout);
    mainWindow->show();
    return app.exec();
}
