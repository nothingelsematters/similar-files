#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>
#include <QCryptographicHash>
#include <QCheckBox>
#include <QFlag>

#include <map>
#include <ctime>


main_window::main_window(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    ui->leftTableWidget->horizontalHeader()->setStretchLastSection(true);

    QCommonStyle style;
    ui->actionAdd_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogApplyButton));
    ui->actionRemove_Directories_From_List->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionRemove_Files->setIcon(style.standardIcon(QCommonStyle::SP_TrashIcon));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));

    connect(ui->actionAdd_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::scan_directories);
    connect(ui->actionRemove_Directories_From_List, &QAction::triggered, this, &main_window::remove_directories_from_list);
    connect(ui->actionRemove_Files, &QAction::triggered, this, &main_window::remove_files);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
}

main_window::~main_window() {}

void main_window::notification(const char* message,
        const char* window_title = "Notification", int time = 2000) {
    QMessageBox* msgbox = new QMessageBox(QMessageBox::Information,
        QString(window_title), QString(message),
        QMessageBox::StandardButtons(QMessageBox::Ok), this);
    msgbox->open();

    QTimer* timer = new QTimer();
    connect(timer, SIGNAL(timeout()), msgbox, SLOT(close()));
    connect(timer, SIGNAL(timeout()), timer, SLOT(stop()));
    connect(timer, SIGNAL(timeout()), timer, SLOT(deleteLater()));
    timer->start(time);
}

void main_window::file_trouble_message(const char* message, std::list<QString> const& troubled) {
    QMessageBox* msgBox = new QMessageBox(QMessageBox::Warning, QString("Troubled ") + message,
    QString(QString::number(troubled.size())) + " file(s) troubled " + message,
    QMessageBox::StandardButtons({QMessageBox::Ok}),
    this);
    QPushButton* show = msgBox->addButton("Show troubled files", QMessageBox::ButtonRole::AcceptRole);

    if (msgBox->exec() == 0) {
        QString message;
        for (auto i: troubled) {
            message += i + '\n';
        }
        msgBox->setText(message);
        msgBox->removeButton((QAbstractButton*) show);
        msgBox->exec();
    }
}

void main_window::select_directory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != nullptr) {
        add_directory(dir);
    }
}

void main_window::add_directory(QString const& dir) {
    for (auto i = directories.begin(); i != directories.end(); ++i) {
        if (*i == dir || dir.indexOf(*i + '/') == 0) {
            notification("This directory is already on the list");
            return;
        }

        if ((*i).indexOf(dir) == 0) {
            for (auto j: ui->leftTableWidget->findItems(dir, Qt::MatchStartsWith)) {
                ui->leftTableWidget->removeRow(j->row());
            }
            directories.remove_if([&dir] (QString const& d) {
                return (d.indexOf(dir) == 0);
            });
            notification("Subdirectories were replaced with the directory chosen");
            break;
        }
    }

    directories.push_back(dir);
    QTableWidgetItem* item = new QTableWidgetItem(dir);
    ui->leftTableWidget->insertRow(ui->leftTableWidget->rowCount());
    ui->leftTableWidget->setItem(ui->leftTableWidget->rowCount() - 1, 0, item);
}

void main_window::remove_directories_from_list() {
    QList<QTableWidgetItem*> directory_list = ui->leftTableWidget->selectedItems();
    for (auto i: directory_list) {
        directories.erase(std::find(directories.begin(), directories.end(), i->text()));
        ui->leftTableWidget->removeRow(i->row());
    }
}

void main_window::file_remove_dispatcher(QTreeWidgetItem* check_box) {
    QString file_name = check_box->text(0);

    if (check_box->checkState(0) == Qt::Checked) {
        files_to_remove.emplace(file_name, check_box);
    } else {
        files_to_remove.erase(files_to_remove.find({file_name, check_box}));
    }
}

QString main_window::get_hash(QString const& file_name) {
    QFile file(file_name);
    if (file.open(QFile::ReadOnly)) {
        QCryptographicHash hasher(QCryptographicHash::Sha256);
        if (hasher.addData(&file)) {
            return QString(hasher.result().toHex());
        }
    }
    return nullptr;
}

void main_window::scan_directories() {
    if (directories.empty()) {
        notification("Please, choose directories to scan");
        return;
    }

    files_to_remove.clear();
    ui->leftTableWidget->setRowCount(0);
    ui->rightTreeWidget->clear();
    std::map<QString, size_t> sizes;
    std::map<QString, std::list<std::list<QString>>> hashes;
    std::list<QString> troubled;

    while (!directories.empty()) {
        QDir d(directories.front());
        QFlags<QDir::Filter> flags({QDir::NoDotAndDotDot, QDir::Dirs, QDir::Files}); // TODO: add hidden
        d.setFilter(flags);
        QFileInfoList list = d.entryInfoList();

        for (QFileInfo file_info: list) {
            QString path = file_info.absoluteFilePath();

            if (file_info.isDir()) {
                directories.push_back(path);
            } else {
                QFile current_file(path);
                if (!current_file.permissions().testFlag(QFileDevice::ReadUser)) {
                    troubled.push_back(path);
                    continue;
                }

                QString current_hash = get_hash(path);
                bool add_new_file = true;

                if (hashes[current_hash].size() != 0) {
                    for (auto i = hashes[current_hash].begin();
                            i != hashes[current_hash].end() && add_new_file; ++i) {

                        if (sizes[i->front()] == current_file.size()) {
                            i->push_back(path);
                            add_new_file = false;
                        }
                    }
                }

                if (add_new_file) {
                    hashes[current_hash].push_back(std::list<QString>({path}));
                    sizes[path] = current_file.size();
                }
            }
        }
        directories.pop_front();
    }
    bool not_found = true;

    for (auto i: hashes) {
        for (auto j: i.second) {
            if (j.size() > 1) {
                not_found = false;

                // TODO: remove copy paste

                QTreeWidgetItem* parent = new QTreeWidgetItem();
                QString item_string = (QFileInfo(*(j.begin())).isSymLink() ? "Symbolic: " : "") + *(j.begin());
                parent->setText(0, item_string);
                ui->rightTreeWidget->insertTopLevelItem(0, parent);

                for (auto k: j) {
                    QString item_string = (QFileInfo(k).isSymLink() ? "Symbolic: " : "") + k;
                    QTreeWidgetItem* item = new QTreeWidgetItem();
                    item->setText(0, item_string);
                    item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
                    item->setCheckState(0, Qt::Unchecked);

                    parent->insertChild(0, item);
                    connect(ui->rightTreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
                        this, SLOT(file_remove_dispatcher(QTreeWidgetItem*)));
                }
            }
        }
    }
    if (troubled.size() > 0) {
        file_trouble_message("reading", troubled);
    } else if (not_found) {
        notification("Similar Files Not Found");
    }

}

void main_window::remove_files() {
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
                    ui->rightTreeWidget->invisibleRootItem()->removeChild(parent_item);
                }
                i = files_to_remove.erase(i);
            }
        }

        if (troubled.size() > 0) {
            file_trouble_message("deleting", troubled);
        }
    }

}

void main_window::show_about_dialog() {
    QMessageBox::aboutQt(this);
}
