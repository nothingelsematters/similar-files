#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "utils/parameters.h"
#include "utils/directoryscanner.h"

#include <QMainWindow>
#include <QTreeWidget>
#include <QProgressBar>
#include <memory>
#include <set>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
    void select_directory();
    void remove_directories_from_list();
    void normalize_directories(bool checkbox_state);

    void details_manager();

    std::pair<DirectoryScanner*, QThread*> new_dir_scanner();
    void directories_scan();
    void result_ready();
    void file_remove_dispatcher(QTreeWidgetItem* check_box);
    void remove_files();
    void select_items();

    void catch_match(std::list<QString> const& files);
    void catch_error(QString const& file_name);
    void set_progress(double progress);

signals:
    void clear_details();

private:
    void add_directory(QString const& dir);
    std::map<parameters, bool> get_parameters();

    void notification(const char* content, const char *window_title, int time);

    std::set<std::pair<QString, QTreeWidgetItem*>> files_to_remove;
    std::unique_ptr<Ui::MainWindow> ui;
};

#endif // MAINWINDOW_H
