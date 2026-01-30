#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_model = new MyModel(this);
    ui->tableView->setModel(m_model);
    m_model->insertRows(0, 3); // добавим 3 прямоугольника
}

MainWindow::~MainWindow()
{
    delete ui;
}

