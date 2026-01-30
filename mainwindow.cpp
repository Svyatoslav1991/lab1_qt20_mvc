// ======================= mainwindow.cpp =======================
#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

/**
 * @brief Конструктор MainWindow.
 *
 * @details
 * - Инициализирует UI.
 * - Создаёт модель и привязывает к tableView.
 * - Устанавливает делегат (редактирование стиля/цвета).
 * - Заполняет тестовыми данными (можно убрать, когда начнёшь работать с файлами).
 * - Настраивает растягивание столбцов.
 * - Создаёт меню "File" и связывает действия со слотами.
 */
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1) Модель
    m_model = new MyModel(this);
    ui->tableView->setModel(m_model);

    // 2) Делегат
    // Родитель = tableView, чтобы делегат жил столько же, сколько и таблица.
    ui->tableView->setItemDelegate(new MyDelegate(ui->tableView));

    // 3) Тестовые данные (по желанию можно отключить)
    m_model->test();

    // 4) Отображение
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 5) Меню "File" / "Файл"
    setupFileMenu();
}

/**
 * @brief Деструктор MainWindow.
 *
 * @details
 * UI удаляем вручную (Qt Creator генерирует такой шаблон).
 * m_model удалится автоматически, т.к. parent = this.
 * Делегат удалится автоматически, т.к. parent = tableView.
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief Настраивает меню "File" и привязывает действия.
 */
void MainWindow::setupFileMenu()
{
    // Можно "File", можно "Файл" — как тебе требуется по лабораторной.
    QMenu* fileMenu = menuBar()->addMenu("Файл");

    QAction* actOpen = fileMenu->addAction("Открыть...");
    QAction* actSave = fileMenu->addAction("Сохранить...");

    // Горячие клавиши (опционально)
    actOpen->setShortcut(QKeySequence::Open);
    actSave->setShortcut(QKeySequence::Save);

    connect(actOpen, &QAction::triggered, this, &MainWindow::slotLoadFromFile);
    connect(actSave, &QAction::triggered, this, &MainWindow::slotSaveToFile);
}

/**
 * @brief Сохранение модели в TSV-файл.
 */
void MainWindow::slotSaveToFile()
{
    const QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save data"),
        QString(),
        tr("TSV files (*.tsv);;Text files (*.txt);;All files (*.*)")
    );

    if (fileName.isEmpty())
        return;

    QString error;
    if (!m_model->saveToTsv(fileName, &error))
    {
        QMessageBox::critical(this, tr("Save failed"), error);
    }
}

/**
 * @brief Загрузка модели из TSV-файла.
 */
void MainWindow::slotLoadFromFile()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open data"),
        QString(),
        tr("TSV files (*.tsv);;Text files (*.txt);;All files (*.*)")
    );

    if (fileName.isEmpty())
        return;

    QString error;
    if (!m_model->loadFromTsv(fileName, &error))
    {
        QMessageBox::critical(this, tr("Open failed"), error);
    }
}
