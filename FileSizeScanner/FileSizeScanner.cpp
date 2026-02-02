#include "stdafx.h"
#include "FileSizeScanner.h"
#include "TableHelper.h"
#include <QFileDialog>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

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
    lineEditPath = ui.lineEditPath;
    btnSelectFolder = ui.selectFolder;
    btnScan = ui.ScanBtn;
    tableWidget = ui.tableWidget;
    cleanTable = ui.actionCleanTable;

    setupTable();

    tableWidget->setContextMenuPolicy(Qt::CustomContextMenu); // to start menu in table by right click

    connect(tableWidget, &QTableWidget::customContextMenuRequested, this, &FileSizeScanner::onTableContextMenu);
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
    tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
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
        QMessageBox::information(this, "Select Folder or Drive", "Selected Successfully, Now click on Scan.");
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


void FileSizeScanner::StartScanWorker(const QString& path)
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
        this, [=](std::map<quint64, std::vector<FileInfo>> result)
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
        const std::vector<FileInfo>& files = it->second;
        if (files.size() < 2)
            continue;
        hasDuplicates = true;

        // Group header
        int headerRow = tableWidget->rowCount();
        tableWidget->insertRow(headerRow);
        tableWidget->setSpan(headerRow, 0, 1, 3);

        tableWidget->setItem( headerRow, 0,
            createGroupItem(QString("Size: %1 (%2 files)")
            .arg(formatFileSize(it->first))
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


void FileSizeScanner::onTableContextMenu(const QPoint& pos)
{
    QTableWidgetItem* item = tableWidget->itemAt(pos);
    if (!item)
        return;

    int row = item->row();

    // Ignore group header rows (they span all columns)
    if (tableWidget->columnSpan(row, 0) > 1)
        return;

    QString filePath = tableWidget->item(row, 2)->text();
    QFileInfo fileInfo(filePath);

    QMenu menu(this);

    QAction* openLocation = menu.addAction("Open File Location");
    QAction* copyFile = menu.addAction("Copy File Name");
    QAction* deleteFile = menu.addAction("Delete File");
    QAction* deleteOthers = menu.addAction("Delete All Except This");
    QAction* deleteSelected = menu.addAction("Delete Selected Files");

    QAction* selectedAction = menu.exec(tableWidget->viewport()->mapToGlobal(pos));

    if (selectedAction == openLocation)
    {
        // Open containing folder
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
    }
    else if (selectedAction == copyFile)
    {
        QApplication::clipboard()->setText(fileInfo.fileName());
        QMessageBox::information(this, "File Info", "File Name copied");
    }
    else if (selectedAction == deleteFile)
    {
        // ---- Safety confirmation ----
        QMessageBox::StandardButton reply =
            QMessageBox::warning(
                this,
                "Delete File",
                QString("Are you sure you want to permanently delete:\n\n%1")
                .arg(fileInfo.fileName()),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

        if (reply != QMessageBox::Yes)
            return;

        // ---- Delete file ----
        if (QFile::remove(filePath))
        {
            int headerRow = findGroupHeaderRow(tableWidget, row);
            if (headerRow == -1)
            {
                tableWidget->removeRow(row);
                return;
            }

            // Count how many file rows belong to this group
            int fileCount = 0;
            int r = headerRow + 1;

            while (r < tableWidget->rowCount() &&
                tableWidget->columnSpan(r, 0) == 1)
            {
                ++fileCount;
                ++r;
            }

            if (fileCount <= 2)
            {
                // Remove entire group (header + files)
                for (int i = 0; i < fileCount + 1; ++i)
                    tableWidget->removeRow(headerRow);
            }
            else
            {
                // Remove only selected file
                tableWidget->removeRow(row);
            }
        }

        else
        {
            QMessageBox::critical(
                this,
                "Delete Failed",
                "Unable to delete the file.\n"
                "It may be in use or you may not have permission.");
        }
    }
    else if (selectedAction == deleteOthers)
    {
        int headerRow = findGroupHeaderRow(tableWidget, row);
        if (headerRow == -1)
            return;

        QMessageBox::StandardButton reply =
            QMessageBox::warning(
                this,
                "Smart Delete",
                "This will permanently delete all other duplicate files in group\n"
                "and keep only the selected file.\n\n"
                "Do you want to continue?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

        if (reply != QMessageBox::Yes)
            return;

        // Identify group range
        int startRow = headerRow + 1;
        int endRow = startRow;

        while (endRow < tableWidget->rowCount() &&
            tableWidget->columnSpan(endRow, 0) == 1)
        {
            ++endRow;
        }

        // Delete all files except selected row
        for (int r = startRow; r < endRow; ++r)
        {
            if (r == row)
                continue;

            QString path = tableWidget->item(r, 2)->text();
            QFile::remove(path);
        }

        // Remove entire group from table
        for (int r = endRow - 1; r >= headerRow; --r)
            tableWidget->removeRow(r);
    }

    else if (selectedAction == deleteSelected)
    {
        QList<QTableWidgetSelectionRange> ranges =
            tableWidget->selectedRanges();

        if (ranges.isEmpty())
            return;

        QMessageBox::StandardButton reply =
            QMessageBox::warning(
                this,
                "Bulk Delete",
                "This will permanently delete all selected files.\n\n"
                "Do you want to continue?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

        if (reply != QMessageBox::Yes)
            return;

        // Collect rows to delete (file rows only)
        QSet<int> rowsToDelete;

        for (const auto& range : ranges)
        {
            for (int r = range.topRow(); r <= range.bottomRow(); ++r)
            {
                // Skip group headers
                if (tableWidget->columnSpan(r, 0) > 1)
                    continue;

                rowsToDelete.insert(r);
            }
        }

        // Delete files from disk
        for (int row : rowsToDelete)
        {
            QString path = tableWidget->item(row, 2)->text();
            QFile::remove(path);
        }

        // Remove rows from table (bottom-up!)
        QList<int> sortedRows = rowsToDelete.values();
        std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());

        for (int row : sortedRows)
            tableWidget->removeRow(row);

        // Cleanup invalid groups (reuse smart logic)
        // Scan table and remove headers with <2 files
        for (int r = tableWidget->rowCount() - 1; r >= 0; --r)
        {
            if (tableWidget->columnSpan(r, 0) > 1)
            {
                int count = 0;
                int i = r + 1;

                while (i < tableWidget->rowCount() &&
                    tableWidget->columnSpan(i, 0) == 1)
                {
                    ++count;
                    ++i;
                }

                if (count < 2)
                {
                    for (int j = i - 1; j >= r; --j)
                        tableWidget->removeRow(j);
                }
            }
        }
    }
}