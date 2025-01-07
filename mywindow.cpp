#include "mywindow.h"
#include "ui_mywindow.h"
#include<QMessageBox>
MyWindow::MyWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MyWindow)
{
    ui->setupUi(this);
    connect(ui->horizontalSlider,SIGNAL(valueChanged(int)),
            ui->progressBar,SLOT(setValue(int)));
    /*    connect(ui->horizontalSlider,SIGNAL(valueChanged(int)),
            ui->progressBar,SLOT(setValue(int)));*/
}

MyWindow::~MyWindow()
{
    delete ui;
}

void MyWindow::on_pushButton_clicked()
{
    QMessageBox::about ()
}

