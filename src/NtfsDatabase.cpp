#include <fstream>
#include <stack>
#include <string>
#include <shared_mutex>
#include <locale>
#include <codecvt>
#include <cassert>

#include <Windows.h>

#include <QElapsedTimer>

#include "NtfsDatabase.hpp"

#pragma pack(push, 1)
struct FILE_RECORD_HEADER
{
    char Signature[4];         // "FILE"
    u16 UpdateSequenceOffset;
    u16 UpdateSequenceSize;
    u64 LogFileSequenceNumber;
    u16 SequenceNumber;
    u16 HardLinkCount;
    u16 FirstAttributeOffset; // offset to first attribute
    u16 Flags;
    u32 UsedSize;             // size of MFT entry used
    u32 AllocatedSize;        // size of MFT entry allocated
    u64 BaseFileRecord;
    u16 NextAttributeID;
    u16 Padding;
    u32 MFTRecordNumber;
    // ... ignore remaining padding
};
struct ATTRIBUTE_HEADER
{
    u32 Type;
    u32 Length;
    u8  NonResident;   // 0 = resident, 1 = non-resident
    u8  NameLength;
    u16 NameOffset;
    u16 Flags;
    u16 Instance;

    union AttributeData
    {
        struct ResidentData
        {
            u32 ValueLength;
            u16 ValueOffset;
            u8  IndexedFlag;
            u8  Padding;
        } Resident;

        struct NonResidentData
        {
            u64 StartVCN;
            u64 LastVCN;
            u16 DataRunOffset;
            u16 CompressionUnit;
            u32 Padding1;
            u64 AllocatedSize;
            u64 DataSize;
            u64 InitializedSize;
            u64 CompressedSize;
        } NonResident;
    } AttrData;
};
// $FILE_NAME attribute structure
struct FILE_NAME_ATTR {
    u64 ParentDirectory;   // File reference to parent MFT entry
    u64 CreationTime;      // FILETIME
    u64 ModificationTime;  // FILETIME
    u64 MFTChangeTime;     // FILETIME
    u64 AccessTime;        // FILETIME
    u64 AllocatedSize;     // on disk
    u64 DataSize;          // logical size
    u32 Flags;             // file flags
    u32 Reparse;           // reparse point
    u8  NameLength;        // in characters
    u8  NameType;          // 0 = POSIX, 1 = Win32, 2 = DOS, 3 = Win32+DOS
    wc  Name[1];           // actual filename (variable length)
};
// $STANDARD_INFORMATION attribute structure
struct STANDARD_INFORMATION_ATTR {
    u64 CreationTime;
    u64 ModificationTime;
    u64 MFTChangeTime;
    u64 AccessTime;
    u32 FileAttributes;
    u32 Reserved;
};
#pragma pack(pop)

inline u64 makeFrn(const FILE_RECORD_HEADER *frh)
{
    return (u64(frh->SequenceNumber) << 48) | u64(frh->MFTRecordNumber);
}

inline u64 normalizeFrn(u64 frn)
{
    return frn & 0x0000FFFFFFFFFFFFULL;
}

void NtfsDatabase::readRawDiskData(wc driveLetter)
{
    QElapsedTimer timer; timer.start();

    wc volumePath[] = { L'\\', L'\\', L'.', L'\\', driveLetter, L':', L'\0' };
    HANDLE hVolume = CreateFileW(
        volumePath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );
    assert(hVolume != INVALID_HANDLE_VALUE);

    DWORD bytesReturned;
    BOOL success = DeviceIoControl(
        hVolume,
        FSCTL_GET_NTFS_VOLUME_DATA,
        nullptr,
        0,
        &ntfsData,
        sizeof(ntfsData),
        &bytesReturned,
        nullptr
    );
    assert(success && bytesReturned > 0);

    u64 mftSize = ntfsData.MftValidDataLength.QuadPart;
    u64 mftOffset = ntfsData.MftStartLcn.QuadPart * ntfsData.BytesPerCluster;
    {
        LARGE_INTEGER mftOffsetLi;
        mftOffsetLi.QuadPart = mftOffset;
        success = SetFilePointerEx(hVolume, mftOffsetLi, nullptr, FILE_BEGIN);
        assert(success);
    }

    rawMftBytes.resize(mftSize);
    u64 bytesRemaining = mftSize;
    u8 *bufPtr = rawMftBytes.data();

    while (bytesRemaining > 0)
    {
        DWORD chunkSize = static_cast<DWORD>(std::min<u64>(bytesRemaining, 512ULL << 20)); // MiB chunks
        success = ReadFile(hVolume, bufPtr, chunkSize, &bytesReturned, nullptr);
        assert(success && bytesReturned > 0);

        bufPtr += bytesReturned;
        bytesRemaining -= bytesReturned;

        u64 readSoFar = mftSize - bytesRemaining;
        i32 percent = i32( ( f64(readSoFar) / f64(mftSize) ) * f64(100) );
        f64 mbPerSec = f64(readSoFar) / 1024.0 / 1024.0 / (timer.elapsed() / 1000.0);

        QMetaObject::invokeMethod(this, [=]() {
            emit readProgress(driveLetter, percent, mbPerSec);
        }, Qt::QueuedConnection);
    }

    assert(bytesRemaining == 0);

    CloseHandle(hVolume);
}

#if 0
void NtfsDatabase::parseDiskDataSequential(wc driveLetter)
{
    QElapsedTimer timer;
    timer.start();

    u64 recordSizeBytes = ntfsData.BytesPerFileRecordSegment;
    u64 totalRecords = rawMftBytes.size() / recordSizeBytes;
    u8 *mftData = rawMftBytes.data();

    constexpr u32 percentPerChunk = 10; // e.g. 1 = 100 chunks, 5 = 20 chunks, 10 = 10 chunks
    constexpr u32 totalPercent = 100;
    const u32 chunkCount = totalPercent / percentPerChunk;

    u64 recordsPerChunk = totalRecords / chunkCount;
    u64 remainder = totalRecords % chunkCount;

    u64 startIndex = 0;

    for (u32 chunk = 0; chunk < chunkCount; ++chunk)
    {
        // Distribute remainder into last chunk (simple + cache-friendly)
        u64 recordsToParse = recordsPerChunk + (chunk == chunkCount - 1 ? remainder : 0);

        for (u64 i = 0; i < recordsToParse; ++i)
        {
            u64 recordIndex = startIndex + i;
            u8 *recordPtr = mftData + recordIndex * recordSizeBytes;

            FILE_RECORD_HEADER *fr = reinterpret_cast<FILE_RECORD_HEADER*>(recordPtr);
            if (memcmp(fr->Signature, "FILE", 4) != 0)
                continue;

            ATTRIBUTE_HEADER *attr = reinterpret_cast<ATTRIBUTE_HEADER*>(recordPtr + fr->FirstAttributeOffset);

            while (true)
            {
                u64 attrOffset = u64(reinterpret_cast<u8 const *>(attr) - recordPtr);
                u64 attrLength = u64(attr->Length);

                if (attr->Type == 0xFFFFFFFF || attrLength == 0 || (attrOffset + attrLength) > recordSizeBytes)
                    break;

                if (attr->Type == 0x30 && attr->NonResident == 0)
                {
                    if (attr->AttrData.Resident.ValueLength < sizeof(FILE_NAME_ATTR))
                        goto next_attr;

                    FILE_NAME_ATTR *fname = reinterpret_cast<FILE_NAME_ATTR*>(
                        reinterpret_cast<u8*>(attr) + attr->AttrData.Resident.ValueOffset);

                    bool isDir = (fname->Flags & FILE_ATTRIBUTE_DIRECTORY) != 0;

                    std::pmr::wstring name(&arena);
                    name.assign(fname->Name, fname->NameLength);

                    insertNode(FileNode{
                        .id = normalizeFrn(makeFrn(fr)),
                        .parentId = normalizeFrn(fname->ParentDirectory),
                        .name = std::move(name),
                        .isDirectory = isDir,
                        .size = fname->DataSize,
                        .creationTime = fname->CreationTime,
                        .modificationTime = fname->ModificationTime,
                        .accessTime = fname->AccessTime,
                    });
                }

            next_attr:
                attr = reinterpret_cast<ATTRIBUTE_HEADER *>(reinterpret_cast<u8 *>(attr) + attrLength);
            }
        }

        startIndex += recordsToParse;

        // Accurate percent based on chunk index
        i32 percent = i32(((chunk + 1) * percentPerChunk));

        // Throughput based on total elapsed time so far
        f64 seconds = timer.elapsed() / 1000.0;
        f64 mbProcessed = f64(startIndex * recordSizeBytes) / (1024.0 * 1024.0);
        f64 mbPerSec = seconds > 0.0 ? (mbProcessed / seconds) : 0.0;

        QMetaObject::invokeMethod(this, [=]() {
            emit parseProgress(driveLetter, percent, mbPerSec);
        }, Qt::QueuedConnection);
    }

    // Build parent-child relationships
    for (auto &[id, node] : nodes)
    {
        u64 parent = normalizeFrn(node.parentId);

        auto it = nodes.find(parent);
        if (it != nodes.end())
        {
            it->second.children.push_back(id);
        }
    }
}
#endif
void NtfsDatabase::parseDiskDataSequential(wc driveLetter)
{
    QElapsedTimer timer;
    timer.start();

    u64 recordSizeBytes = ntfsData.BytesPerFileRecordSegment;
    u64 totalRecords = rawMftBytes.size() / recordSizeBytes;
    u8 *mftData = rawMftBytes.data();

    constexpr u32 percentPerChunk = 10;
    constexpr u32 totalPercent = 100;
    const u32 chunkCount = totalPercent / percentPerChunk;

    u64 recordsPerChunk = totalRecords / chunkCount;
    u64 remainder = totalRecords % chunkCount;

    u64 startIndex = 0;

    for (u32 chunk = 0; chunk < chunkCount; ++chunk)
    {
        u64 recordsToParse = recordsPerChunk + (chunk == chunkCount - 1 ? remainder : 0);

        for (u64 i = 0; i < recordsToParse; ++i)
        {
            u64 recordIndex = startIndex + i;
            u8 *recordPtr = mftData + recordIndex * recordSizeBytes;

            FILE_RECORD_HEADER *fr = reinterpret_cast<FILE_RECORD_HEADER *>(recordPtr);
            if (memcmp(fr->Signature, "FILE", 4) != 0)
                continue;

            if (fr->FirstAttributeOffset >= recordSizeBytes)
                continue;

            ATTRIBUTE_HEADER *attr = reinterpret_cast<ATTRIBUTE_HEADER *>(recordPtr + fr->FirstAttributeOffset);

            FILE_NAME_ATTR const *bestName = nullptr;
            u8 bestNamespace = 0xFF;

            while (true)
            {
                u64 attrOffset = u64(reinterpret_cast<u8 const *>(attr) - recordPtr);
                u64 attrLength = u64(attr->Length);

                if (attr->Type == 0xFFFFFFFF)
                    break;

                if (attrLength < sizeof(ATTRIBUTE_HEADER))
                    break;

                if ((attrOffset + attrLength) > recordSizeBytes)
                    break;

                if (attr->Type == 0x30 && attr->NonResident == 0)
                {
                    auto const &res = attr->AttrData.Resident;

                    if (res.ValueOffset + res.ValueLength > attrLength)
                        goto next_attr;

                    if (res.ValueLength < sizeof(FILE_NAME_ATTR))
                        goto next_attr;

                    FILE_NAME_ATTR const *fname =
                        reinterpret_cast<FILE_NAME_ATTR const *>(
                            reinterpret_cast<u8 const *>(attr) + res.ValueOffset);

                    u8 ns = fname->NameType;

                    // Namespace priority
                    // 1 = Win32, 3 = Win32+DOS, 0 = POSIX, 2 = DOS
                    auto priority = [](u8 n) -> u8 {
                        switch (n)
                        {
                        case 1: return 0;
                        case 3: return 1;
                        case 0: return 2;
                        case 2: return 3;
                        default: return 4;
                        }
                    };

                    if (!bestName || priority(ns) < priority(bestNamespace))
                    {
                        bestName = fname;
                        bestNamespace = ns;
                    }
                }

            next_attr:
                attr = reinterpret_cast<ATTRIBUTE_HEADER *>(reinterpret_cast<u8 *>(attr) + attrLength);
            }

            if (bestName)
            {
                std::pmr::wstring name(&arena);
                name.assign(bestName->Name, bestName->NameLength);

                bool isDir = (bestName->Flags & FILE_ATTRIBUTE_DIRECTORY) != 0;

                insertNode(FileNode{
                    .id = normalizeFrn(makeFrn(fr)),
                    .parentId = normalizeFrn(bestName->ParentDirectory),
                    .name = std::move(name),
                    .isDirectory = isDir,
                    .size = bestName->DataSize,
                    .creationTime = bestName->CreationTime,
                    .modificationTime = bestName->ModificationTime,
                    .accessTime = bestName->AccessTime,
                });
            }
        }

        startIndex += recordsToParse;

        i32 percent = i32((chunk + 1) * percentPerChunk);

        f64 seconds = timer.elapsed() / 1000.0;
        f64 mbProcessed = f64(startIndex * recordSizeBytes) / (1024.0 * 1024.0);
        f64 mbPerSec = seconds > 0.0 ? (mbProcessed / seconds) : 0.0;

        QMetaObject::invokeMethod(this, [=]() {
            emit parseProgress(driveLetter, percent, mbPerSec);
        }, Qt::QueuedConnection);
    }

    for (auto &[id, node] : nodes)
    {
        u64 parent = normalizeFrn(node.parentId);
        auto it = nodes.find(parent);
        if (it != nodes.end())
        {
            it->second.children.push_back(id);
        }
    }

    emit finished(driveLetter);
}

u64 NtfsDatabase::findNodeByPath(std::wstring const &path_) const
{
    if (path_.empty())
        return 0;

    std::pmr::wstring path(path_);

    // Split path into components
    std::vector<std::pmr::wstring> components;
    u64 start = 0;
    while (start < path.size()) {
        u64 end = path.find(L'\\', start);
        if (end == std::wstring::npos) end = path.size();
        if (end > start)
            components.push_back(path.substr(start, end - start));
        start = end + 1;
    }

    if (components.empty())
        return 0;

    // For each node, check if its path matches
    for (auto const &kv : nodes)
    {
        u64 candidateId = kv.first;
        std::vector<std::pmr::wstring> candidatePath;

        // Walk up parent chain
        while (true)
        {
            auto it = nodes.find(candidateId);
            if (it == nodes.end())
                break; // broken chain

            const FileNode &node = it->second;
            candidatePath.push_back(node.name);
            if (node.parentId == candidateId || nodes.find(node.parentId) == nodes.end())
                break; // reached root or missing parent

            candidateId = node.parentId;
        }

        // candidatePath is bottom-up, reverse it
        std::reverse(candidatePath.begin(), candidatePath.end());

        if (candidatePath == components)
            return kv.first; // found
    }

    return 0; // not found
}
