#include <cassert>

#include "Windows.h"

#include "util_windows.hpp"

#include "NtfsDatabase.hpp"
#include "GlobalFileDatabases.hpp"

u64 GlobalFileDatabases::driveIndex(wc drive)
{
    wc c = (wc)std::towupper(drive);
    assert(c >= L'A' && c <= L'Z');
    return (c >= L'A' && c <= L'Z') ? (c - L'A') : -1;
}

FileDatabase *GlobalFileDatabases::get(wc drive) const
{
    u64 idx = driveIndex(drive);
    if (idx < 0) return nullptr;
    return db_slots[idx].ptr.load(std::memory_order_acquire);
}

void GlobalFileDatabases::set(wc drive, FileDatabase *db)
{
    i32 idx = driveIndex(drive);
    if (idx < 0) return;

    db_slots[idx].ptr.store(db, std::memory_order_release);
}

GlobalFileDatabases &getFileDatabases()
{
    static GlobalFileDatabases g;
    return g;
}

static
std::vector<wc> enumerateDrives()
{
    std::vector<wc> drives;

    DWORD mask = GetLogicalDrives(); // bitmask, bit 0 = A:, bit 1 = B:, etc.
    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        if (mask & (1 << (letter - 'A'))) {
            drives.push_back(letter);
        }
    }
    return drives;
}

static
std::wstring getFilesystemType(wc driveLetter)
{
    std::wstring rootPath = std::wstring(1, driveLetter) + L":\\";
    wc fsName[MAX_PATH] = {0};

    BOOL success = GetVolumeInformationW(
        rootPath.c_str(),
        nullptr,
        0,
        nullptr,
        nullptr,
        nullptr,
        fsName,
        MAX_PATH
    );

    if (success) {
        return fsName;
    }
    else {
        auto [code, msg] = get_last_winapi_error();
        return L"";
    }
}

bool GlobalFileDatabases::isInitialized() const
{
    return initialized;
}

void GlobalFileDatabases::init()
{
    if (initialized) {
        return;
    }

    auto &dbs = getFileDatabases();

    for (wc drive : enumerateDrives()) {
        std::wstring fsType = getFilesystemType(drive);

        if (fsType == L"NTFS") {
            FileDatabase *db = new NtfsDatabase();
            dbs.set(drive, db);
        }
        else if (fsType == L"FAT32" || fsType == L"exFAT") {
        }
        else {
        }
    }

    initialized = true;
}
