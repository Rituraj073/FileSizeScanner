#pragma once
#include <QObject>
#include <vector>
#include <map>
#include "FileInfo.h"

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
    void scanFinished(std::map<quint64, std::vector<FileInfo>> result);
    void progressRange(int max);
    void progressValue(int current);
    void progressText(int current, int total);
};