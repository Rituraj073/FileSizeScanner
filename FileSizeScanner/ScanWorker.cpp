#include "stdafx.h"
#include "ScanWorker.h"
#include <QDirIterator>
#include <QFileInfo>

ScanWorker::ScanWorker(QObject* parent)
    : QObject(parent){}

void ScanWorker::scan(const QString& path, const QStringList& allowedExtensions)
{
    m_cancelRequested = false;

    // Normalize allowed extensions into a set (no leading dot, lower-case)
    QSet<QString> allowed;
    for (const QString& e : allowedExtensions)
    {
        QString n = e.trimmed().toLower();
        if (n.startsWith('.'))
            n.remove(0, 1);
        if (!n.isEmpty())
            allowed.insert(n);
    }

    // -------- Phase 1: Count files (respecting filter) --------
    int totalFiles = 0;
    {
        QDirIterator counter(path, QDir::Files, QDirIterator::Subdirectories);
        while (counter.hasNext())
        {
            if (m_cancelRequested)
                return;
            QString filePath = counter.next();
            if (!allowed.isEmpty())
            {
                QFileInfo info(filePath);
                QString ext = info.suffix().toLower();
                if (!allowed.contains(ext))
                    continue;
            }
            ++totalFiles;
        }
    }
    emit progressRange(totalFiles);

    // -------- Phase 2: Actual scan (respecting filter) --------
    std::map<quint64, std::vector<FileInfo>> localMap;
    int scanned = 0;

    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        if (m_cancelRequested)
        {
            emit scanFinished({});
            return;
        }

        QString filePath = it.next();
        QFileInfo info(filePath);

        if (!allowed.isEmpty())
        {
            QString ext = info.suffix().toLower();
            if (!allowed.contains(ext))
                continue;
        }

        FileInfo file;
        file.fileName = info.fileName();
        file.filePath = info.absoluteFilePath();
        file.fileSize = info.size();

        localMap[file.fileSize].push_back(file);

        ++scanned;

        // Throttle UI updates (every 50 files)
        if (scanned % 50 == 0)
        {
            emit progressValue(scanned);
            emit progressText(scanned, totalFiles);
        }
    }

    emit progressValue(scanned); // final update
    emit progressText(scanned, totalFiles);
    emit scanFinished(localMap);
}

void ScanWorker::cancel()
{
    m_cancelRequested = true;
}