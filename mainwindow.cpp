#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils/directoryscanner.h"

#include <QCommonStyle>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QCheckBox>
#include <QDirIterator>
#include <QDir>
#include <QProgressBar>
#include <QFileInfo>
#include <QLabel>
#include <QtCore/QThread>
#include <QMovie>
#include <QDebug>

#include <map>
#include <vector>
#include <list>
#include <string>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    ui->detailsList->setHidden(true);
    ui->cancelButton->setHidden(true);
    ui->selectButton->setHidden(true);

    QCommonStyle style;
    ui->actionAdd_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionRemove_Directories_From_List->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionRemove_Files->setIcon(style.standardIcon(QCommonStyle::SP_TrashIcon));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));

    connect(ui->actionAdd_Directory, &QAction::triggered, this, &MainWindow::select_directory);
    connect(ui->actionRemove_Directories_From_List, &QAction::triggered,
            this, &MainWindow::remove_directories_from_list);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->recursiveCheckbox, &QCheckBox::toggled, this, &MainWindow::normalize_directories);
    connect(ui->scanButton, &QAbstractButton::clicked, this, &MainWindow::directories_scan);
    connect(ui->actionRemove_Files, &QAction::triggered, this, &MainWindow::remove_files);
    connect(ui->selectButton, &QPushButton::clicked, this, &MainWindow::select_items);
}

MainWindow::~MainWindow() {
//    delete preprocessing;
//    for (auto i: ui->directoriesTable->children()) delete i;
//    for (auto i: ui->stringsList->children()) delete i;
}

std::map<parameters, bool> MainWindow::get_parameters() {
    std::map<parameters, bool> result;
    result[parameters::Hidden] = ui->hiddenCheckbox->checkState();
    result[parameters::Recursive] = ui->recursiveCheckbox->checkState();

    return std::move(result);
}

void MainWindow::notification(const char* content,
                              const char* window_title = "Notification",
                              int time = 4000) {
    QMessageBox* msgbox = new QMessageBox(QMessageBox::Information,
        QString(window_title), QString(content),
        QMessageBox::StandardButtons(QMessageBox::Ok), this, Qt::WindowType::Popup);
    msgbox->open();

    QTimer* timer = new QTimer();
    connect(timer, &QTimer::timeout, msgbox, &QMessageBox::close);
    connect(timer, &QTimer::timeout, timer, &QTimer::stop);
    connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
    timer->start(time);
}

void MainWindow::details_manager() {
    QPushButton* sender = static_cast<QPushButton*>(QObject::sender());
    if (sender->text() == "hide details") {
        ui->detailsList->setHidden(true);
        sender->setText("show details");
    } else {
        ui->detailsList->setHidden(false);
        sender->setText("hide details");
    }
}

void MainWindow::select_directory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
        QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != nullptr) {
        add_directory(dir);
    }
}

void MainWindow::add_directory(QString const& dir) {
    bool recursive = get_parameters()[parameters::Recursive];
    for (int i = 0; i < ui->directoriesTable->rowCount(); ++i) {
        QString cur_dir = ui->directoriesTable->item(i, 0)->text();
        if (cur_dir == dir || (recursive && dir.indexOf(cur_dir + '/') == 0)) {
            notification("This directory is already on the list");
            return;
        }

        if (recursive && cur_dir.indexOf(dir + '/') == 0) {
            while (i < ui->directoriesTable->rowCount()) {
                if (static_cast<QProgressBar*>(ui->directoriesTable->
                       cellWidget(i, 0))->format().indexOf(dir) == 0) {
                    ui->directoriesTable->removeRow(i);
                } else {
                    ++i;
                }
            }
            notification("Subdirectories were replaced with the directory chosen\n"
                         "If you want to scan exactly these directories, you can "
                         "turn off the \"Recursive\" option");
            break;
        }
    }

    ui->directoriesTable->insertRow(ui->directoriesTable->rowCount());
    QTableWidgetItem* item = new QTableWidgetItem(dir);
    ui->directoriesTable->setItem(ui->directoriesTable->rowCount() - 1, 0, item);
}

void MainWindow::remove_directories_from_list() {
    for (auto i: ui->directoriesTable->selectedItems()) {
        ui->directoriesTable->removeRow(i->row());
    }
}

void MainWindow::normalize_directories(bool checkbox_state) {
    if (!checkbox_state) {
        return;
    }

    for (int i = 0; i < ui->directoriesTable->rowCount(); ++i) {
        QString dir_name = ui->directoriesTable->item(i, 0)->text();
        for (int j = i + 1; j < ui->directoriesTable->rowCount(); ++j) {
            QString second_dir_name = ui->directoriesTable->item(j, 0)->text();
            if (dir_name.indexOf(second_dir_name + '/') == 0) {
                ui->directoriesTable->item(i--, 0)->setText(second_dir_name);
                ui->directoriesTable->removeRow(j);
                break;
            } else if (second_dir_name.indexOf(dir_name + '/') == 0) {
                ui->directoriesTable->removeRow(j--);
            }
        }
    }
}

std::pair<DirectoryScanner*, QThread*> MainWindow::new_dir_scanner() {
    ui->detailsList->clear();
    ui->detailsList->setHidden(true);
    emit clear_details();

    ui->scanButton->setDisabled(true);
    ui->cancelButton->setHidden(false);
    ui->actionRemove_Directories_From_List->setDisabled(true);
    ui->actionAdd_Directory->setDisabled(true);

    std::list<QString> directories;
    for (int i = 0; i < ui->directoriesTable->rowCount(); ++i) {
        QString directory_name = ui->directoriesTable->item(i, 0)->text();
        directories.push_back(directory_name);
    }
    QThread* worker_thread = new QThread();
    DirectoryScanner* dir_scanner = new DirectoryScanner(directories, get_parameters());
    dir_scanner->moveToThread(worker_thread);

    connect(dir_scanner, &DirectoryScanner::new_match, this, &MainWindow::catch_match);
    connect(dir_scanner, &DirectoryScanner::new_error, this, &MainWindow::catch_error);
    connect(dir_scanner, &DirectoryScanner::progress, this, &MainWindow::set_progress);
    connect(dir_scanner, &DirectoryScanner::finished, worker_thread, &QThread::quit);
    connect(dir_scanner, &DirectoryScanner::finished, dir_scanner, &DirectoryScanner::deleteLater);
    connect(worker_thread, &QThread::finished, worker_thread, &QThread::deleteLater);
    connect(ui->cancelButton, &QPushButton::clicked, worker_thread, &QThread::requestInterruption);

    return {dir_scanner, worker_thread};
}

void MainWindow::directories_scan() {
    ui->selectButton->setHidden(true);
    ui->filesList->clear();
    auto [dir_scanner, worker_thread] = new_dir_scanner();
    connect(worker_thread, &QThread::started, dir_scanner, &DirectoryScanner::scan_directories);
    connect(dir_scanner, &DirectoryScanner::finished, this, &MainWindow::result_ready);
    worker_thread->start();
}

void MainWindow::result_ready() {
    ui->actionRemove_Directories_From_List->setDisabled(false);
    ui->actionAdd_Directory->setDisabled(false);
    ui->cancelButton->setHidden(true);
    ui->directoriesTable->setStyleSheet("");
    ui->scanButton->setDisabled(false);
    ui->progressBar->setStyleSheet(" QProgressBar { color: green } ");
    ui->progressBar->setValue(0);
    int count = ui->filesList->invisibleRootItem()->childCount();
    if (count == 0) {
        notification("No matches found");
    } else {
        ui->selectButton->setHidden(false);
        notification((std::to_string(count) + " matches found").c_str());
    }
}

void MainWindow::catch_match(std::list<QString> const& files) {
    QTreeWidgetItem* parent = new QTreeWidgetItem(ui->filesList);
    parent->setText(0, files.front());
    ui->filesList->insertTopLevelItem(0, parent);
    for (auto i: files) {
        QTreeWidgetItem* item = new QTreeWidgetItem(parent);
        item->setText(0, i);
        item->setCheckState(0, Qt::Unchecked);
        connect(ui->filesList, &QTreeWidget::itemChanged,
            this, &MainWindow::file_remove_dispatcher);
    }
}

void MainWindow::catch_error(QString const& file_name) {
    ui->progressBar->setStyleSheet(" QProgressBar { color: red } ");
    if (ui->detailsList->count() == 0) {
        QPushButton* button = new QPushButton("show details");
        ui->mainGrid->addWidget(button);
        connect(button, &QPushButton::clicked, this, &MainWindow::details_manager);
        connect(this, &MainWindow::clear_details, button, &QPushButton::deleteLater);
        connect(this, &MainWindow::clear_details, ui->statusBar, &QStatusBar::clearMessage);
    }
    ui->detailsList->addItem(new QListWidgetItem(file_name));
    ui->statusBar->showMessage(QString("Troubled reading: ") +
                                QString(QString::number(ui->detailsList->count())) + " file(s) troubled reading");
}

void MainWindow::set_progress(double progress) {
    ui->progressBar->setValue(progress);
}

void MainWindow::select_items() {
    auto root = ui->filesList->invisibleRootItem();
    for (int i = 0; i < root->childCount(); ++i) {
        root->child(i)->child(0)->setCheckState(0, Qt::Unchecked);
        for (int j = 1; j < root->child(i)->childCount(); ++j) {
            root->child(i)->child(j)->setCheckState(0, Qt::Checked);
        }
    }
}


void MainWindow::file_remove_dispatcher(QTreeWidgetItem* check_box) {
    QString file_name = check_box->text(0);

    if (check_box->checkState(0) == Qt::Checked) {
        files_to_remove.emplace(file_name, check_box);
    } else if (files_to_remove.find({file_name, check_box}) != files_to_remove.end()) {
        files_to_remove.erase(files_to_remove.find({file_name, check_box}));
    }
}

void MainWindow::remove_files() {
    if (files_to_remove.size() == 0) {
        notification("Please, choose files to delete");
        return;
    }

    QMessageBox* msgBox = new QMessageBox(QMessageBox::Warning, QString("Notification"),
        QString("Are you sure you want to delete ") + QString::number(files_to_remove.size()) + " file(s)",
        QMessageBox::StandardButtons({QMessageBox::Ok, QMessageBox::Cancel}),
        this);


    if (msgBox->exec() == QMessageBox::Ok) {
        std::list<QString> troubled;

        for (auto i = files_to_remove.begin(); i != files_to_remove.end(); ) {
            QFile file(i->first);

            if (!file.permissions().testFlag(QFileDevice::WriteUser) || !file.remove()) {
                i->second->setForeground(0, QBrush(QColor(200, 0, 0)));
                troubled.push_back(i->first);
                ++i;
            } else {
                QTreeWidgetItem* parent_item = i->second->parent();
                parent_item->removeChild(i->second);
                if (parent_item->childCount() == 0) {
                    ui->filesList->invisibleRootItem()->removeChild(parent_item);
                }
                i = files_to_remove.erase(i);
            }
        }
    }
}
