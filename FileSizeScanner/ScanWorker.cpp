#include "stdafx.h"
#include "ScanWorker.h"
#include <QDirIterator>
#include <QFileInfo>

ScanWorker::ScanWorker(QObject* parent)
    : QObject(parent){}

void ScanWorker::scan(const QString& path)
{
    m_cancelRequested = false;

    QHash<quint64, QVector<FileInfo>> localMap;
    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext())
    {
        if (m_cancelRequested)
            break;
        QFileInfo info(it.next());

        FileInfo file;
        file.fileName = info.fileName();
        file.filePath = info.absoluteFilePath();
        file.fileSize = info.size();

        localMap[file.fileSize].push_back(file);
        emit progress();
    }

    emit scanFinished(localMap);
}

void ScanWorker::cancel()
{
    m_cancelRequested = true;
}