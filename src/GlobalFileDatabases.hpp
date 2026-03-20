#pragma once

#include <array>
#include <atomic>
#include <cwctype>

#include "FileDatabase.hpp"

struct FileDatabaseSlot
{
    std::atomic<FileDatabase *> ptr{nullptr};
};

struct GlobalFileDatabases
{
    std::array<FileDatabaseSlot, 26> db_slots = {};

    bool isInitialized() const;
    void init();

    static
    u64 driveIndex(wc drive);

    FileDatabase *get(wc drive) const;

    void set(wc drive, FileDatabase *db);

private:
    bool initialized = false;
};

GlobalFileDatabases &getFileDatabases();
