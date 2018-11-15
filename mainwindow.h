#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <memory>
#include <list>
#include <set>

namespace Ui {
class MainWindow;
}

class main_window : public QMainWindow {
    Q_OBJECT

public:
    explicit main_window(QWidget *parent = 0);
    ~main_window();


private slots:
    void select_directory();
    void scan_directories();
    void remove_directories_from_list();

    void file_remove_dispatcher(QTreeWidgetItem* check_box);
    void remove_files();

    void show_about_dialog();

private:
    void add_directory(QString const& dir);

    void notification(const char* window_title, const char* message, int time);
    void file_trouble_message(const char* message, std::list<QString> const& troubled);

    QString get_hash(QString const& file_name);

    std::list<QString> directories;
    std::set<std::pair<QString, QTreeWidgetItem*>> files_to_remove;
    std::unique_ptr<Ui::MainWindow> ui;
};

#endif // MAINWINDOW_H
