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

private:
    std::atomic_bool m_cancelRequested{ false };

public slots:
    void scan(const QString& path);
    void cancel();

signals:
    void scanFinished(QHash<quint64, QVector<FileInfo>> result);
    void progressRange(int max);
    void progressValue(int current);
    void progressText(int current, int total);
};