#include "stdafx.h"
#include "FileSizeScanner.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>

/* ---------- Helper: Human-readable file size ---------- */
static QString formatFileSize(quint64 bytes)
{
    const double kb = 1024.0;
    const double mb = kb * 1024.0;
    const double gb = mb * 1024.0;


    if (bytes < kb)
        return QString("%1 B").arg(bytes);
    else if (bytes < mb)
        return QString("%1 KB").arg(bytes / kb, 0, 'f', 2);
    else if (bytes < gb)
        return QString("%1 MB").arg(bytes / mb, 0, 'f', 2);
    else
        return QString("%1 GB").arg(bytes / gb, 0, 'f', 2);
}


/* ---------- Helper: Group header row ---------- */
static QTableWidgetItem* createGroupItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(Qt::ItemIsEnabled);
    item->setBackground(QColor(230, 230, 230));
    item->setFont(QFont("Segoe UI", 9, QFont::Bold));
    return item;
}

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
    cleanTable = ui.actionCleanTable;

    setupTable();

    connect(btnSelectFolder, &QPushButton::clicked, this, &FileSizeScanner::on_select_folder_clicked);
    connect(btnScan, &QPushButton::clicked, this, &FileSizeScanner::on_scan_clicked);
    connect(cleanTable, &QAction::triggered, this, [=]() { tableWidget->setRowCount(0); });
}

void FileSizeScanner::setupTable()
{
    tableWidget->clear();
    tableWidget->setColumnCount(3);

    QStringList headers;
    headers << "File Name" << "File Size" << "Full Path";
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
        QMessageBox::warning(this, "Scaned", "Path is Empty, Select folder");
        return;
    }

    StartScanWorker(path);
}


void FileSizeScanner::StartScanWorker(QString& path)
{
    QProgressDialog* progressDialog = new QProgressDialog("Scanning files...", "Cancel", 0, 0, this);
    progressDialog->setWindowTitle("Scanning");
    progressDialog->setWindowModality(Qt::ApplicationModal);
    progressDialog->setAutoClose(false);
    progressDialog->setAutoReset(false);
    progressDialog->show();

    btnScan->setEnabled(false);
    tableWidget->setRowCount(0);

    scanThread = new QThread(this);
    worker = new ScanWorker();
    worker->moveToThread(scanThread);
    
    // Set progress range when total count is known
    connect(worker, &ScanWorker::progressRange,
        progressDialog, &QProgressDialog::setMaximum);

    // Update progress value
    connect(worker, &ScanWorker::progressValue,
        progressDialog, &QProgressDialog::setValue);

    connect(worker, &ScanWorker::progressText,
        this, [=](int current, int total)
        {
            progressDialog->setLabelText(
                QString("Scanning files...\n%1 / %2 files scanned")
                .arg(current)
                .arg(total)
            );
        });

    connect(scanThread, &QThread::started,
        [=]() { worker->scan(path); });

    connect(progressDialog, &QProgressDialog::canceled,
        this, [=]()
        {
            if (worker) worker->cancel();
        });

    connect(worker, &ScanWorker::scanFinished,
        this, [=](QHash<quint64, QVector<FileInfo>> result)
        {
            sizeMap = result;
            bool hasDuplicates = fillTable();

            progressDialog->close();
            progressDialog->deleteLater();

            btnScan->setEnabled(true);

            QMessageBox::information(this, "Scan Completed", hasDuplicates ? "Duplicate files found" : "No duplicate files");

            scanThread->quit();
        });

    connect(scanThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(scanThread, &QThread::finished, scanThread, &QObject::deleteLater);

    scanThread->start();
}


bool FileSizeScanner::fillTable()
{
    bool hasDuplicates = false;
    tableWidget->setRowCount(0);

    for (auto it = sizeMap.begin(); it != sizeMap.end(); ++it)
    {
        const QVector<FileInfo>& files = it.value();
        if (files.size() < 2)
            continue;
        hasDuplicates = true;

        // Group header
        int headerRow = tableWidget->rowCount();
        tableWidget->insertRow(headerRow);
        tableWidget->setSpan(headerRow, 0, 1, 3);

        tableWidget->setItem( headerRow, 0,
            createGroupItem(QString("Size: %1 (%2 files)")
            .arg(formatFileSize(it.key()))
            .arg(files.size())));

        // File rows
        for (const FileInfo& file : files)
        {
            int row = tableWidget->rowCount();
            tableWidget->insertRow(row);

            tableWidget->setItem(row, 0, new QTableWidgetItem(file.fileName));
            tableWidget->setItem(row, 1, new QTableWidgetItem(formatFileSize(file.fileSize)));
            tableWidget->setItem(row, 2, new QTableWidgetItem(file.filePath));
        }
    }
    return hasDuplicates;
}