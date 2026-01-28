#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_FileSizeScanner.h"
#include <qpushbutton.h>
#include <qfiledialog.h>
#include <qtablewidget.h>
#include <QString>
#include <QVector>
#include <QHash>

struct FileInfo
{
    QString fileName;
    QString filePath;
    quint64 fileSize;
};

class FileSizeScanner : public QMainWindow
{
    Q_OBJECT

public:
    FileSizeScanner(QWidget *parent = nullptr);
    ~FileSizeScanner();

private:
    void mySetupUI();
    void setupTable();
    void scanFolder(const QString& path);
    bool fillTable();

    Ui::FileSizeScannerClass ui;
    QLineEdit* lineEditPath;
    QPushButton* btnSelectFolder;
    QPushButton* btnScan;
    QTableWidget* tableWidget;

    QHash<quint64, QVector<FileInfo>> sizeMap;

private slots:
    void on_select_folder_clicked();
    void on_scan_clicked();
};

