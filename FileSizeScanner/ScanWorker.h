#pragma once
#include <QObject>
#include <QHash>
#include <QVector>
#include <QString>

struct FileInfo
{
    QString fileName;
    QString filePath;
    quint64 fileSize;
};

class ScanWorker : public QObject
{
    Q_OBJECT

public:
    ScanWorker(QObject* parent = nullptr);

public slots:
    void scan(const QString& path);

signals:
    void scanFinished(QHash<quint64, QVector<FileInfo>> result);
};