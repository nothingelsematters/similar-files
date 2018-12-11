#include "directoryscanner.h"

#include <QtCore/QThread>
#include <QCryptographicHash>


DirectoryScanner::DirectoryScanner(std::list<QString> const& directories,
                                   std::map<parameters, bool> const& params)
    : directories(directories) {
    iterator_flags = params.at(parameters::Recursive) ?
         QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    directory_flags = {QDir::NoDotAndDotDot, QDir::Files};
    if (params.at(parameters::Hidden)) {
        directory_flags |= QDir::Hidden;
    }
    qRegisterMetaType<std::list<QString>>("matches");
}

DirectoryScanner::~DirectoryScanner() {}

QString DirectoryScanner::get_hash(QString const& file_name) {
    QFile file(file_name);
    if (file.open(QFile::ReadOnly)) {
        QCryptographicHash hasher(QCryptographicHash::Sha256);
        if (hasher.addData(&file)) {
            return QString(hasher.result().toHex());
        } else {
            return nullptr;
        }
    }
    return nullptr;
}

void DirectoryScanner::handle_file(QString const& file_path, std::map<QString, std::list<QString>>& hashes) {
    QFile current_file(file_path);
    if (!current_file.permissions().testFlag(QFileDevice::ReadUser)) {
        emit new_error(file_path);
        return;
    }
    QString current_hash = get_hash(file_path);
    if (current_hash == nullptr) {
        emit new_error(file_path);
        return;
    }
    hashes[current_hash].push_back(file_path);
}

void DirectoryScanner::scan_directories() {
    size_t directory_size = 0;
    std::map<size_t, std::list<QString>> sizes;
    for (auto i: directories) {
        for (QDirIterator it(i, directory_flags, iterator_flags); it.hasNext(); ) {
            it.next();
            size_t size = it.fileInfo().size();
            sizes[size].push_back(it.filePath());
            directory_size += size;
        }
        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }
    }

    size_t current_size = 0;
    for (auto i: sizes) {
        if (i.second.size() < 2) {
            if (i.second.size() == 1) {
                emit progress((double) (current_size += QFileInfo(i.second.front()).size()) * 100 / directory_size);
            }
            continue;
        }

        std::map<QString, std::list<QString>> hashes;
        for (auto j: i.second) {
            handle_file(j, hashes);
            emit progress((double) (current_size += QFileInfo(j).size()) * 100 / directory_size);
            if (QThread::currentThread()->isInterruptionRequested()) {
                break;
            }
        }
        for (auto j: hashes) {
            if (j.second.size() > 1) {
                emit new_match(std::move(j.second));
            }
        }
        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }
    }

    emit finished();
}
