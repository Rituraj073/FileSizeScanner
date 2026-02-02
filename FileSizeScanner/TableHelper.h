#pragma once
#include <QString>
#include <QTableWidget>

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

/* ---------- Find: Group header row ---------- */
static int findGroupHeaderRow(QTableWidget* table, int row)
{
    for (int r = row; r >= 0; --r)
    {
        if (table->columnSpan(r, 0) > 1)
            return r;
    }
    return -1;
}