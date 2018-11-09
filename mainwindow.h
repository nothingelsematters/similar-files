#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

    void add_directory(QString const& dir);

    QString get_hash(QString const& file_name);
    bool compare_files(QString const& first_name, QString const& second_name);

    void notification(const char* window_title, const char* message, int time);
    void file_trouble_message(const char* message, std::list<QString> const& troubled);

private slots:
    void select_directory();
    void scan_directories();
    void remove_directories_from_list();

    void file_remove_dispatcher(int state);
    void remove_files();

    void show_about_dialog();

private:
    std::list<QString> directories;
    std::set<std::pair<QString, int>> files_to_remove;
    std::unique_ptr<Ui::MainWindow> ui;
};

#endif // MAINWINDOW_H
