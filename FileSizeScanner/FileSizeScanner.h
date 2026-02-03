#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_FileSizeScanner.h"
#include <QPushButton>
#include <QTableWidget>
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
    void StartScanWorker(const QString& path);
    bool fillTable();
    bool isDarkThemeEnabled = false;

    Ui::FileSizeScannerClass ui;
    QLineEdit* lineEditPath;
    QPushButton* btnSelectFolder;
    QPushButton* btnScan;
    QAction* cleanTable;
    QAction* themeAction;
    QTableWidget* tableWidget;

    QThread* scanThread = nullptr;
    ScanWorker* worker = nullptr;

    std::map<quint64, std::vector<FileInfo>> sizeMap;

private slots:
    void on_select_folder_clicked();
    void on_scan_clicked();
    void onTableContextMenu(const QPoint& pos);
    void toggleTheme();
};