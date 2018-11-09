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

// TODO: links
// TODO: file reading exceptions
// TODO: great-reformating
// TODO: show adequate file names in front
// TODO: Column number hard code -> Enum

main_window::main_window(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    ui->rightTableWidget->setColumnCount(2);
    ui->rightTableWidget->setHorizontalHeaderLabels({"Files", "Delete"});
    ui->rightTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->rightTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);

    ui->leftTableWidget->setColumnCount(1);
    ui->leftTableWidget->setHorizontalHeaderLabels({"Diretories"});
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
    QString(QString::number(troubled.size())) + " file(s) troubled reading",
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
        if (*i == dir || dir.indexOf(*i) == 0) {
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
        ui->leftTableWidget->removeRow(i->row());
    }
}

void main_window::file_remove_dispatcher(int state) {
    QObject* check_box = sender();
    int row = check_box->property("row").toInt();

    QString file_name = ui->rightTableWidget->item(row, 0)->text();
    if (state == Qt::Checked) {
        files_to_remove.emplace(file_name, row);
    } else {
        files_to_remove.erase(files_to_remove.find({file_name, row}));
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

bool main_window::compare_files(QString const& first_name, QString const& second_name) {
    QFile first_file(first_name);
    QFile second_file(second_name);
    first_file.open(QFile::ReadOnly);
    second_file.open(QFile::ReadOnly);
    char* first_buffer = new char[1000];
    char* second_buffer = new char[1000];

    while (true) {
        qint64 count = first_file.read(first_buffer, 1000);
        //troubled reading
        if (count == -1 || count == 0) {
            break;
        }
        if (second_file.read(second_buffer, 1000) != count) {
            return false;
        }
        for (size_t i = 0; i < count; ++i) {
            if (first_buffer[i] != second_buffer[i]) {
                return false;
            }
        }
    }
    return second_file.bytesAvailable() == 0;
    // QFile destructor closes files
}

void main_window::scan_directories() {
    if (directories.empty()) {
        notification("Please, choose directories to scan");
        return;
    }

    files_to_remove.clear();
    ui->leftTableWidget->setRowCount(0);
    ui->rightTableWidget->setRowCount(0);
    std::map<QString, std::list<std::list<QString>>> hashes;
    std::list<QString> troubled;


    while (!directories.empty()) {
        QDir d(directories.front());
        QFileInfoList list = d.entryInfoList();

        for (QFileInfo file_info: list) {
            if (file_info.fileName() == "." || file_info.fileName() == "..") {
                continue;
            }
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
                    for (auto& i: hashes[current_hash]) {
                        if (compare_files(i.front(), path)) {
                            i.push_back(path);
                            add_new_file = false;
                            break;
                        }
                    }
                }

                if (add_new_file) {
                    hashes[current_hash].push_back(std::list<QString>({path}));
                }
            }
        }
        directories.pop_front();
    }


    // TO FIX front part:
    bool not_found = true;
    bool is_grey = true;

    for (auto i: hashes) {
        for (auto j: i.second) {
            if (j.size() > 1) {
                not_found = false;
                is_grey = !is_grey;

                for (auto k: j) {
                    ui->rightTableWidget->insertRow(ui->rightTableWidget->rowCount());
                    QTableWidgetItem* item = new QTableWidgetItem(k);
                    item->setBackground(QBrush(is_grey ? QColor(200, 200, 200) : QColor(255, 255, 255)));
                    ui->rightTableWidget->setItem(ui->rightTableWidget->rowCount() - 1, 0, item);

                    QCheckBox* check_box = new QCheckBox();
                    check_box->setProperty("row", ui->rightTableWidget->rowCount() - 1);
                    ui->rightTableWidget->setCellWidget(ui->rightTableWidget->rowCount() - 1, 1, check_box);
                    connect(check_box, SIGNAL(stateChanged(int)), this, SLOT(file_remove_dispatcher(int)));
                }
            }
        }
    }

    if (troubled.size() > 0) {
        file_trouble_message("Reading", troubled);
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
            if (!file.permissions().testFlag(QFileDevice::WriteUser)) {
                troubled.push_back(i->first);
                ++i;
            } else {
                file.remove();
                QTableWidgetItem* item = ui->rightTableWidget->item(i->second, 0);
                QFont font = item->font();
                font.setStrikeOut(true);
                item->setFont(font);

                ui->rightTableWidget->removeCellWidget(i->second, 1);
                i = files_to_remove.erase(i);
            }
        }

        if (troubled.size() > 0) {
            file_trouble_message("Deleting", troubled);
        }
    }

}

void main_window::show_about_dialog() {
    QMessageBox::aboutQt(this);
}
