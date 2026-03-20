#pragma once

#include <map>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QElapsedTimer>

#include "primitives.hpp"

#include "SwanDockWindow.hpp"

class SwanDebugWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SwanDebugWindow(SwanDockWindow *parent = nullptr);

    void addDrive(wc driveLetter);
    void removeDrive(wc driveLetter);

    void onDatabaseStarted(wc driveLetter);
    void setProgress1(wc driveLetter, i32 value, f64 mbPerSec = 0.0);
    void setProgress2(wc driveLetter, i32 value, f64 mbPerSec = 0.0);
    void onDatabaseFinished(wc driveLetter);

signals:
    void refreshOneRequested(wc driveLetter);
    void refreshAllRequested();

private:
    struct Row {
        QWidget *container = nullptr;

        QLabel *label = nullptr;
        QPushButton *refreshButton = nullptr;

        QLabel *bar1Speed = nullptr;
        QLabel *bar2Speed = nullptr;

        QLabel *bar1Seconds = nullptr;
        QLabel *bar2Seconds = nullptr;

        QProgressBar *bar1 = nullptr;
        QProgressBar *bar2 = nullptr;

        QElapsedTimer timer1;
        QElapsedTimer timer2;

        b8 parseDone = false;
    };

    QVBoxLayout *layout;
    std::map<wc, Row*> rows;

    QPushButton *refreshButton; // Refresh all
};
