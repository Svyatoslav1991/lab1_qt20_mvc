#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mymodel.h"
#include "mydelegate.h"   // + делегат

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    MyModel*     m_model    = nullptr;
    MyDelegate*  m_delegate = nullptr; // + делегат
};
#endif // MAINWINDOW_H
