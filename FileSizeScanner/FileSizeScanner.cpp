#include "stdafx.h"
#include "FileSizeScanner.h"
#include <QDirIterator>
#include <QFileInfo>
#include <qmessagebox.h>

FileSizeScanner::FileSizeScanner(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    mySetupUI();
}

FileSizeScanner::~FileSizeScanner()
{}

void FileSizeScanner::mySetupUI()
{
    lineEditPath = new QLineEdit();
    btnSelectFolder = ui.selectFolder;
    btnScan = ui.ScanBtn;
    tableWidget = ui.tableWidget;

    setupTable();

    connect(btnSelectFolder, &QPushButton::clicked, this, &FileSizeScanner::on_select_folder_clicked);
    connect(btnScan, &QPushButton::clicked, this, &FileSizeScanner::on_scan_clicked);
}

void FileSizeScanner::setupTable()
{
    tableWidget->clear();
    tableWidget->setColumnCount(3);

    QStringList headers;
    headers << "File Name" << "File Size (Bytes)" << "Full Path";
    tableWidget->setHorizontalHeaderLabels(headers);

    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    tableWidget->setRowCount(0);
}


void FileSizeScanner::on_select_folder_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(
        this,
        "Select Folder or Drive"
    );

    if (!folderPath.isEmpty())
    {
        lineEditPath->setText(folderPath);
        tableWidget->setRowCount(0); // Clear old results
        sizeMap.clear();
        QMessageBox::information(this, "Select Folder or Drive", "Selected Successfully, Now click on Scan Butten to Scan.");
    }
}


void FileSizeScanner::on_scan_clicked()
{
    QString path = lineEditPath->text();
    if (path.isEmpty())
    {
        QMessageBox::information(this, "Scaned", "No duplicates files present");
        return;
    } 

    scanFolder(path);
    QVector<FileInfo> duplicates;

    for (auto it = sizeMap.begin(); it != sizeMap.end(); ++it)
    {
        if (it.value().size() > 1)
            duplicates += it.value();
    }

    if (duplicates.isEmpty())
    {
        QMessageBox::information(this, "Scaned", "No duplicates files present");
        return;
    }
    fillTable(duplicates);
    QMessageBox::information(this, "Scaned", "Duplicates files present");
}


void FileSizeScanner::scanFolder(const QString& path)
{
    sizeMap.clear();
    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext())
    {
        QString filePath = it.next();
        QFileInfo info(filePath);

        FileInfo file;
        file.fileName = info.fileName();
        file.filePath = info.absoluteFilePath();
        file.fileSize = info.size();

        sizeMap[file.fileSize].push_back(file);
    }
}


void FileSizeScanner::fillTable(const QVector<FileInfo>& files)
{
    tableWidget->setRowCount(0);

    for (const FileInfo& file : files)
    {
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);

        tableWidget->setItem(row, 0,
            new QTableWidgetItem(file.fileName));

        tableWidget->setItem(row, 1,
            new QTableWidgetItem(QString::number(file.fileSize)));

        tableWidget->setItem(row, 2,
            new QTableWidgetItem(file.filePath));
    }
}