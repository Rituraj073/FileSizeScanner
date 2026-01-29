#include "stdafx.h"
#include "ScanWorker.h"
#include <QDirIterator>
#include <QFileInfo>

ScanWorker::ScanWorker(QObject* parent)
    : QObject(parent){}

void ScanWorker::scan(const QString& path)
{
    m_cancelRequested = false;

    // -------- Phase 1: Count files --------
    int totalFiles = 0;
    {
        QDirIterator counter(path, QDir::Files, QDirIterator::Subdirectories);
        while (counter.hasNext())
        {
            if (m_cancelRequested)
                return;
            counter.next();
            ++totalFiles;
        }
    }
    emit progressRange(totalFiles);

    // -------- Phase 2: Actual scan --------
    QHash<quint64, QVector<FileInfo>> localMap;
    int scanned = 0;

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

        ++scanned;

        // Throttle UI updates (every 50 files)
        if (scanned % 50 == 0)
            emit progressValue(scanned);
    }

    emit progressValue(scanned); // final update
    emit scanFinished(localMap);
}

void ScanWorker::cancel()
{
    m_cancelRequested = true;
}