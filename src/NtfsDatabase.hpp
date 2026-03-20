#pragma once

#include <memory_resource>
#include <unordered_map>
#include <shared_mutex>

#include "primitives.hpp"

#include "FileDatabase.hpp"

struct NtfsDatabase : public FileDatabase {
    std::vector<u8> rawMftBytes;
    NTFS_VOLUME_DATA_BUFFER ntfsData;

    void readRawDiskData(wc driveLetter) override;

    void parseDiskDataSequential(wc driveLetter) override;

    u64 findNodeByPath(std::wstring const &path) const override;
};
