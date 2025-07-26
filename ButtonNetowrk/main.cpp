#include <QApplication>
#include <QSplitter>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include "buttonnetwork.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    qputenv("QT_ACCESSIBILITY", "0");

    QWidget* mainWindow = new QWidget;
    mainWindow->setWindowTitle("Hopfield Fractional Network");
    mainWindow->resize(1200, 600);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, mainWindow);
    ButtonNetwork* builder = new ButtonNetwork;

    QTextEdit* outputView = new QTextEdit;
    outputView->setReadOnly(true);
    outputView->setMinimumWidth(500);
    builder->updateEquationEditor(outputView);

    splitter->addWidget(builder);
    splitter->addWidget(outputView);

    QPushButton* clearBtn = new QPushButton("Clear");
    QPushButton* computeBtn = new QPushButton("Compute");
    QPushButton* graphBtn = new QPushButton("Show Graph");
    QPushButton* tableBtn = new QPushButton("Show Output Table");

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

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(clearBtn);
    buttonLayout->addWidget(computeBtn);
    buttonLayout->addWidget(graphBtn);
    buttonLayout->addWidget(tableBtn);

    QVBoxLayout* layout = new QVBoxLayout(mainWindow);
    layout->addLayout(topOptions);
    layout->addWidget(splitter);
    layout->addLayout(buttonLayout);

    QObject::connect(clearBtn, &QPushButton::clicked, builder, &ButtonNetwork::clearNetwork);
    QObject::connect(computeBtn, &QPushButton::clicked, builder, &ButtonNetwork::computeResults);
    QObject::connect(graphBtn, &QPushButton::clicked, builder, &ButtonNetwork::showGraph);
    QObject::connect(tableBtn, &QPushButton::clicked, builder, &ButtonNetwork::showTable);
    QObject::connect(solverMode, &QComboBox::currentTextChanged, builder, &ButtonNetwork::setSolverMode);
    QObject::connect(tmaxInput, qOverload<int>(&QSpinBox::valueChanged), builder, &ButtonNetwork::setTimeLimit);

    QObject::connect(builder, &ButtonNetwork::fileSaved, [=](const QString& path) {
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
