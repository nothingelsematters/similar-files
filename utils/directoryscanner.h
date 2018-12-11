#ifndef DIRECTORYSCANNER_H
#define DIRECTORYSCANNER_H

#include "utils/parameters.h"

#include <QString>
#include <QObject>
#include <QFlags>
#include <QDir>
#include <QDirIterator>

#include <map>
#include <list>
#include <vector>


class DirectoryScanner : public QObject {
    Q_OBJECT

public:
    DirectoryScanner(std::list<QString> const& directories,
                     std::map<parameters, bool> const& params);
    ~DirectoryScanner();

public slots:
    void scan_directories();

signals:
    void new_match(std::list<QString> const& match);
    void new_error(QString const& file_name);
    void progress(double progress);
    void finished();

private:
    void scan_directory(QString const& directory_name, int index);
    void handle_file(QString const& file_path, std::map<QString, std::list<QString>>& hashes);
    QString get_hash(QString const& file_name);

    std::list<QString> directories;
    QFlags<QDirIterator::IteratorFlag> iterator_flags;
    QFlags<QDir::Filter> directory_flags;
};

#endif // DIRECTORYSCANNER_H
