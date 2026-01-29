#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_FileSizeScanner.h"
#include <qpushbutton.h>
#include <qfiledialog.h>
#include <qtablewidget.h>
#include "ScanWorker.h"
#include <QThread>

class FileSizeScanner : public QMainWindow
{
    Q_OBJECT

public:
    FileSizeScanner(QWidget *parent = nullptr);
    ~FileSizeScanner();

private:
    void mySetupUI();
    void setupTable();
    void StartScanWorker(QString& path);
    bool fillTable();

    Ui::FileSizeScannerClass ui;
    QLineEdit* lineEditPath;
    QPushButton* btnSelectFolder;
    QPushButton* btnScan;
    QAction* cleanTable;
    QTableWidget* tableWidget;

    QThread* scanThread = nullptr;
    ScanWorker* worker = nullptr;

    QHash<quint64, QVector<FileInfo>> sizeMap;

private slots:
    void on_select_folder_clicked();
    void on_scan_clicked();
};

