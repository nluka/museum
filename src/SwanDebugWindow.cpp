#include "SwanDebugWindow.hpp"
#include <QSizePolicy>
#include <QFrame>

SwanDebugWindow::SwanDebugWindow(SwanDockWindow *parent)
    : QWidget(parent)
{
    layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
}

void SwanDebugWindow::addDrive(wc driveLetter)
{
    if (rows.count(driveLetter)) {
        // Reset existing row
        Row *row = rows[driveLetter];
        row->timer1.invalidate();
        row->timer2.invalidate();
        row->bar1->setValue(0);
        row->bar2->setValue(0);
        row->bar1Speed->setText("0 MB/s");
        row->bar2Speed->setText("0 MB/s");
        row->bar1Seconds->setText("0.0s");
        row->bar2Seconds->setText("0.0s");
        row->parseDone = false;
        row->refreshButton->setEnabled(false);
        return;
    }

    Row *row = new Row();
    row->container = new QWidget(this);
    QGridLayout *grid = new QGridLayout(row->container);
    grid->setSpacing(5);
    grid->setContentsMargins(0, 0, 0, 0);

    // Column 1: Drive + refresh
    row->label = new QLabel(QString("%1:").arg(QChar(driveLetter)), row->container);
    row->refreshButton = new QPushButton("Refresh", row->container);
    row->refreshButton->setFixedHeight(25);
    row->refreshButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    grid->addWidget(row->label, 0, 0, 2, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(row->refreshButton, 0, 1, 2, 1, Qt::AlignLeft | Qt::AlignVCenter);

    // Spacer between column 1 and 2
    QSpacerItem *hSpacer = new QSpacerItem(15, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    grid->addItem(hSpacer, 0, 2, 2, 1);

    connect(row->refreshButton, &QPushButton::clicked, this, [this, driveLetter]() {
        if (!rows.count(driveLetter))
            return;

        Row *row = rows[driveLetter];
        row->parseDone = false;
        row->refreshButton->setEnabled(false);

        emit refreshOneRequested(driveLetter);
    });

    // Column 2: MB/s
    row->bar1Speed = new QLabel("0 MB/s", row->container);
    row->bar2Speed = new QLabel("0 MB/s", row->container);
    row->bar1Speed->setFixedWidth(80);
    row->bar2Speed->setFixedWidth(80);
    grid->addWidget(row->bar1Speed, 0, 3, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(row->bar2Speed, 1, 3, Qt::AlignLeft | Qt::AlignVCenter);

    // Column 3: seconds
    row->bar1Seconds = new QLabel("0.0s", row->container);
    row->bar2Seconds = new QLabel("0.0s", row->container);
    row->bar1Seconds->setFixedWidth(50);
    row->bar2Seconds->setFixedWidth(50);
    grid->addWidget(row->bar1Seconds, 0, 4, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(row->bar2Seconds, 1, 4, Qt::AlignLeft | Qt::AlignVCenter);

    // Column 4: progress bars with Read/Parse labels
    row->bar1 = new QProgressBar(row->container);
    row->bar1->setRange(0, 100);
    row->bar1->setFixedHeight(18);
    row->bar1->setMaximumWidth(200);

    row->bar2 = new QProgressBar(row->container);
    row->bar2->setRange(0, 100);
    row->bar2->setFixedHeight(18);
    row->bar2->setMaximumWidth(200);

    QLabel *readLabel = new QLabel("Read:", row->container);
    readLabel->setFixedWidth(40);
    QLabel *parseLabel = new QLabel("Parse:", row->container);
    parseLabel->setFixedWidth(40);

    QHBoxLayout *hBar1 = new QHBoxLayout();
    hBar1->addWidget(readLabel);
    hBar1->addWidget(row->bar1);
    hBar1->setContentsMargins(0, 0, 0, 0);
    grid->addLayout(hBar1, 0, 5, Qt::AlignLeft | Qt::AlignVCenter);

    QHBoxLayout *hBar2 = new QHBoxLayout();
    hBar2->addWidget(parseLabel);
    hBar2->addWidget(row->bar2);
    hBar2->setContentsMargins(0, 0, 0, 0);
    grid->addLayout(hBar2, 1, 5, Qt::AlignLeft | Qt::AlignVCenter);

    layout->addWidget(row->container);

    // Invalidate timers so they start fresh when progress begins
    row->timer1.invalidate(); // read timer
    row->timer2.invalidate(); // parse timer

    rows[driveLetter] = row;
}

void SwanDebugWindow::removeDrive(wc driveLetter)
{
    if (!rows.count(driveLetter)) return;

    Row *row = rows[driveLetter];
    layout->removeWidget(row->container);
    delete row->container;
    delete row;

    rows.erase(driveLetter);
}

void SwanDebugWindow::setProgress1(wc driveLetter, i32 value, f64 mbPerSec)
{
    if (!rows.count(driveLetter))
        return;
    Row *row = rows[driveLetter];
    if (!row->timer1.isValid()) {
        row->timer1.restart();
    }
    row->bar1->setValue(value);
    row->bar1Speed->setText(QString("%1 MB/s").arg(mbPerSec, 0, 'f', 2));
    row->bar1Seconds->setText(QString("%1s").arg(row->timer1.elapsed() / 1000.0, 0, 'f', 1));
}

void SwanDebugWindow::setProgress2(wc driveLetter, i32 value, f64 mbPerSec)
{
    if (!rows.count(driveLetter))
        return;
    Row *row = rows[driveLetter];
    if (!row->timer2.isValid()) {
        row->timer2.restart();
    }
    row->bar2->setValue(value);
    row->bar2Speed->setText(QString("%1 MB/s").arg(mbPerSec, 0, 'f', 2));
    row->bar2Seconds->setText(QString("%1s").arg(row->timer2.elapsed() / 1000.0, 0, 'f', 1));
}

void SwanDebugWindow::onDatabaseStarted(wc driveLetter)
{
    if (!rows.count(driveLetter))
        return;

    Row *row = rows[driveLetter];
    row->parseDone = false;
    row->refreshButton->setEnabled(false);
}

void SwanDebugWindow::onDatabaseFinished(wc driveLetter)
{
    if (!rows.count(driveLetter))
        return;

    Row *row = rows[driveLetter];
    row->parseDone = true;
    row->refreshButton->setEnabled(true);
}
