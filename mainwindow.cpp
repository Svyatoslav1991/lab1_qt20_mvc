#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QHeaderView>     // чтобы QHeaderView::Stretch был объявлен

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_model = new MyModel(this);
    ui->tableView->setModel(m_model);

    // Делегат: parent = tableView, чтобы жил столько же, сколько таблица
    m_delegate = new MyDelegate(ui->tableView);
    ui->tableView->setItemDelegate(m_delegate);

    m_model->test();

    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

MainWindow::~MainWindow()
{
    delete ui;
}
