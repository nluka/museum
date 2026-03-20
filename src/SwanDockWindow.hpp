#pragma once

#include <map>

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>

#include "FileDatabase.hpp"

class SwanDebugWindow;

class SwanDockWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SwanDockWindow(QWidget *parent = nullptr, QString const &title = "Swan File Explorer");

private:

    u64 dockCounter = 0;
    QList<QDockWidget*> docks;
    SwanDebugWindow *debugWindow;

signals:
    void refreshDatabases();
};
