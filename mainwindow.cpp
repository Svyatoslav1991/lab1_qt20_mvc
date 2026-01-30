#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_model = new MyModel(this);
    ui->tableView->setModel(m_model);

    // Заполняем модель тестовыми данными (через slotAddData внутри test()).
    m_model->test();
}

MainWindow::~MainWindow()
{
    delete ui;
}

