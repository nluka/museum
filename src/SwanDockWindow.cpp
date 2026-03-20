// TODO parse all MFT entries (non-resident, best names, etc.)

// TODO reduce memory footprint by not storing entire MFT
// TODO determine if double/triple buffering is practical - How much memory is needed for FileDatabase if not storing the entire MFT, but copying only the relevant values (name, size, etc)

// TODO background thread with ReadDirectoryChangesW and periodic resync
// TODO parse MFT in parallel

#include <Windows.h>
#include <winioctl.h>
#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <unordered_map>

#include <QTableWidget>
#include <QHeaderView>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QDateTime>
#include <QTextEdit>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QFontDatabase>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QtConcurrent/QtConcurrentRun>
#include <QTime>
#include <QTimer>

#include "util.hpp"
#include "util_windows.hpp"
#include "scoped_timer.hpp"
#include "primitives.hpp"

#include "populateCommonMenuBar.hpp"
#include "SwanDockWindow.hpp"
#include "NtfsDatabase.hpp"
#include "SwanDebugWindow.hpp"
#include "GlobalFileDatabases.hpp"

class FileDatabaseTableModel : public QAbstractTableModel {
    FileDatabase *db;
    std::vector<u64> visible; // snapshot of current directory

public:
    FileDatabaseTableModel(FileDatabase *db, QObject *parent = nullptr)
        : QAbstractTableModel(parent), db(db) {}

    void setDirectory(u64 parentFrn)
    {
        std::shared_lock lock(db->mutex);

        auto it = db->nodes.find(parentFrn);
        if (it == db->nodes.end())
            return;

        beginResetModel();
        visible = std::vector<u64>(it->second.children.begin(), it->second.children.end()); // snapshot
        endResetModel();
    }

    i32 rowCount(const QModelIndex &) const override {
        return i32(visible.size());
    }

    i32 columnCount(const QModelIndex &) const override {
        return 1; // just name for now
    }

    QVariant data(const QModelIndex &index, i32 role) const override
    {
        if (role != Qt::DisplayRole)
            return {};

        if (!index.isValid() || index.row() < 0 || index.row() >= (i32)visible.size())
            return {};

        std::shared_lock lock(db->mutex);

        u64 frn = visible[index.row()];
        auto it = db->nodes.find(frn);
        if (it == db->nodes.end())
            return {};

        const auto &node = it->second;

        if (!node.name.empty())
            return QString::fromWCharArray(node.name.data(), i32(node.name.size()));

        // fallback so you SEE something meaningful
        return QString("[FRN %1]").arg(QString::number(frn));
    }
};

class FileDatabaseFlatModel : public QAbstractTableModel {
    FileDatabase *db;
    std::vector<u64> rows; // snapshot of ALL nodes

public:
    FileDatabaseFlatModel(FileDatabase *db, QObject *parent = nullptr)
        : QAbstractTableModel(parent), db(db) {}

    void rebuild()
    {
        std::shared_lock lock(db->mutex);

        beginResetModel();

        rows.clear();
        rows.reserve(db->nodes.size());

        for (auto const &[id, node] : db->nodes)
            rows.push_back(id);

        endResetModel();
    }

    i32 rowCount(const QModelIndex &) const override {
        return i32(rows.size());
    }

    i32 columnCount(const QModelIndex &) const override {
        return 5; // name, id, parent, size, type
    }

    QVariant data(const QModelIndex &index, i32 role) const override
    {
        if (role != Qt::DisplayRole && role != Qt::EditRole)
            return {};

        if (!index.isValid() || index.row() < 0 || index.row() >= (i32)rows.size())
            return {};

        std::shared_lock lock(db->mutex);

        u64 id = rows[index.row()];
        auto it = db->nodes.find(id);
        if (it == db->nodes.end())
            return {};

        const FileNode &node = it->second;

        switch (index.column())
        {
            case 0: // Name
                if (!node.name.empty())
                    return QString::fromWCharArray(node.name.data(), i32(node.name.size()));
                return QString("[unnamed]");

            case 1: // ID
                return QString::number(node.id);

            case 2: // Parent
                return QString::number(node.parentId);

            case 3: // Size
                return QString::number(node.size);

            case 4: // Type
                return node.isDirectory ? "Dir" : "File";
        }

        return {};
    }

    QVariant headerData(i32 section, Qt::Orientation orientation, i32 role) const override
    {
        if (role != Qt::DisplayRole)
            return {};

        if (orientation == Qt::Horizontal)
        {
            switch (section)
            {
                case 0: return "Name";
                case 1: return "ID";
                case 2: return "Parent";
                case 3: return "Size";
                case 4: return "Type";
            }
        }

        return {};
    }
};

SwanDockWindow::SwanDockWindow(QWidget *parent, QString const &title)
    : QMainWindow(nullptr)
{
    Q_UNUSED(parent);
    setWindowTitle(title);
    populateCommonMenuBar(this->menuBar());

    debugWindow = new SwanDebugWindow(this);

    QDockWidget *debugDock = new QDockWidget("Debug", this);
    debugDock->setWidget(debugWindow);

    addDockWidget(Qt::BottomDockWidgetArea, debugDock);

    {
        GlobalFileDatabases &dbs = getFileDatabases();

        if (!dbs.isInitialized()) {
            // The first Swan being opened, need to initialize
            dbs.init();
        }
    }

    constexpr wc allDrives = L'\0';

    auto refresh = [this, allDrives](wc requestedDriveLetter) {
        GlobalFileDatabases &dbs = getFileDatabases();

        auto readAndParseConcurrently = [this](wc driveLetter, FileDatabase *db) {
            debugWindow->addDrive(driveLetter);

            connect(db, &FileDatabase::readProgress, this, [this](wc letter, i32 value, f64 mbPerSec) {
                debugWindow->setProgress1(letter, value, mbPerSec);
            });
            connect(db, &FileDatabase::parseProgress, this, [this](wc letter, i32 value, f64 mbPerSec) {
                debugWindow->setProgress2(letter, value, mbPerSec);
            });
            connect(db, &FileDatabase::finished, this, [this](wc letter) {
                debugWindow->onDatabaseFinished(letter);
            }, Qt::UniqueConnection);

            debugWindow->onDatabaseStarted(driveLetter);

            QtConcurrent::run([db, driveLetter]() {
                db->readRawDiskData(driveLetter);
                db->parseDiskDataSequential(driveLetter);

                QTimer::singleShot(0, db, [db, driveLetter]() {
                    emit db->finished(driveLetter);
                });
            });
        };

        if (requestedDriveLetter == allDrives) {
            for (wc driveLetter = 'A'; driveLetter <= 'Z'; ++driveLetter) {
                auto db = dbs.get(driveLetter);
                if (db != nullptr) {
                    readAndParseConcurrently(driveLetter, db);
                }
            }
        }
        else {
            auto db = dbs.get(requestedDriveLetter);
            readAndParseConcurrently(requestedDriveLetter, db);
        }
    };

    connect(debugWindow, &SwanDebugWindow::refreshOneRequested, this, [this, refresh](wc driveLetter) {
        refresh(driveLetter);
    });
    connect(debugWindow, &SwanDebugWindow::refreshAllRequested, this, [this, refresh]() {
        refresh(allDrives);
    });

    refresh(allDrives);

    // QTableView *hexView = createHexView(mftBuffer.data(), mftBuffer.size(), this);
}
