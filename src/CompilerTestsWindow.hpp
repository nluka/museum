#pragma once

#include <QMainWindow>
#include <QFileSystemWatcher>
#include <QTableWidget>

#include "FilePicker.hpp"
#include "DirectoryPicker.hpp"

#include "CompilationFlowWindow.hpp"

class CompilerTestsWindow : public QMainWindow
{
    Q_OBJECT

public:
    CompilerTestsWindow(QWidget *parent = nullptr, QString const &title = "Compiler Tests");

    void loadCsv(QString const &csvPath);

private slots:
    void onCsvPathChanged(QString const &path);
    void onCsvFileModified(QString const &path);
    void onTableItemChanged(QTableWidgetItem *item);

private:
    QFileSystemWatcher *watcher;
    FilePicker *csvFilePicker;
    DirectoryPicker *dataDirectoryPicker;

    QTableWidget *testsTable;
    bool testsTableIsPopulating = false;

    struct RowState {
        QString status;
        QStringList fields;
    };
    QVector<RowState> testRows;
};
