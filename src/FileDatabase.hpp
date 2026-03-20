#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <fstream>
#include <stack>
#include <memory_resource>

#include <QObject>

#include "primitives.hpp"

// Generic file node for any filesystem
struct FileNode {
    u64 id;                       // unique ID (FRN, inode, or virtual)
    u64 parentId;                 // parent node ID
    std::pmr::wstring name;       // filename
    bool isDirectory;
    u64 size = 0;                 // logical size
    u64 creationTime = 0;
    u64 modificationTime = 0;
    u64 accessTime = 0;

    std::pmr::vector<u64> children; // child IDs
};

// Generic database interface
struct FileDatabase : public QObject {
    Q_OBJECT

public:
    std::pmr::monotonic_buffer_resource arena;
    std::pmr::unordered_map<u64, FileNode> nodes;
    std::shared_mutex mutex;

    FileDatabase(/*std::pmr::memory_resource *mr = std::pmr::get_default_resource()*/)
        : nodes(&arena) {}

    virtual ~FileDatabase() = default;

    virtual void readRawDiskData(wc driveLetter) = 0;

    virtual void parseDiskDataSequential(wc driveLetter) = 0;

    virtual void insertNode(FileNode &&node);

    virtual u64 findNodeByPath(std::wstring const &path) const = 0;

    virtual u64 findRootNode() const;

    void dumpToCsv(std::wstring const &filename) const;

signals:
    void readProgress(wc driveLetter, i32 percent, f64 mbPerSec);
    void parseProgress(wc driveLetter, i32 percent, f64 mbPerSec);
    void finished(wc driveLetter);
};
