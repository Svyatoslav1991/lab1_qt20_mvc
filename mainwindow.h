#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "mymodel.h"     // модель
#include "mydelegate.h"  // делегат

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief Главное окно приложения.
 *
 * @details
 * - Создаёт модель MyModel и привязывает её к QTableView.
 * - Создаёт делегат MyDelegate и устанавливает его в QTableView.
 * - Создаёт меню "File" (или "Файл") с пунктами:
 *     - Open...  -> загрузка модели из TSV
 *     - Save...  -> сохранение модели в TSV
 * - Для выбора имени файла использует стандартные диалоги QFileDialog.
 */
class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    /**
     * @brief Слот: сохранить данные модели в TSV-файл.
     *
     * @details
     * - Открывает диалог выбора имени файла (QFileDialog::getSaveFileName()).
     * - Если имя выбрано — вызывает MyModel::saveToTsv().
     * - При ошибке показывает QMessageBox.
     */
    void slotSaveToFile();

    /**
     * @brief Слот: загрузить данные модели из TSV-файла.
     *
     * @details
     * - Открывает диалог выбора файла (QFileDialog::getOpenFileName()).
     * - Если файл выбран — вызывает MyModel::loadFromTsv().
     * - При ошибке показывает QMessageBox.
     */
    void slotLoadFromFile();

private:
    /**
     * @brief Настраивает меню и действия (QAction).
     *
     * @details
     * Создаёт пункты меню и связывает их со слотами сохранения/загрузки.
     */
    void setupFileMenu();

private:
    Ui::MainWindow* ui = nullptr;
    MyModel* m_model = nullptr;
};

#endif // MAINWINDOW_H
