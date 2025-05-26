#include <QApplication>
#include <QSplitter>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include "buttonnetwork.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    qputenv("QT_ACCESSIBILITY", "0");

    QWidget* mainWindow = new QWidget;
    mainWindow->setWindowTitle("Fractional Hopfield Network Builder");
    mainWindow->resize(1200, 600);  // 2x wider

    QSplitter* splitter = new QSplitter(Qt::Horizontal, mainWindow);
    splitter->setHandleWidth(2);

    ButtonNetwork* builder = new ButtonNetwork;
    QTextEdit* outputView = new QTextEdit;
    outputView->setReadOnly(true);
    outputView->setMinimumWidth(500);
    outputView->setStyleSheet("background-color: #f4f4f4; font-family: Consolas; font-size: 12px;");

    builder->updateEquationEditor(outputView);

    splitter->addWidget(builder);
    splitter->addWidget(outputView);

    QPushButton* clearBtn = new QPushButton("Clear");
    QPushButton* computeBtn = new QPushButton("Compute");

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(clearBtn);
    buttonLayout->addWidget(computeBtn);

    QVBoxLayout* layout = new QVBoxLayout(mainWindow);
    layout->addWidget(splitter);
    layout->addLayout(buttonLayout);

    QObject::connect(clearBtn, &QPushButton::clicked, builder, &ButtonNetwork::clearNetwork);
    QObject::connect(computeBtn, &QPushButton::clicked, builder, &ButtonNetwork::computeResults);

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
