#include "FileDatabase.hpp"

static
std::string wstrToUtf8(std::pmr::wstring const &wstr)
{
    std::string utf8;
    utf8.reserve(wstr.size() * 3); // rough max
    for (wc c : wstr)
    {
        if (c <= 0x7F)
            utf8.push_back(char(c));
        else if (c <= 0x7FF)
        {
            utf8.push_back(0xC0 | ((c >> 6) & 0x1F));
            utf8.push_back(0x80 | (c & 0x3F));
        }
        else
        {
            utf8.push_back(0xE0 | ((c >> 12) & 0x0F));
            utf8.push_back(0x80 | ((c >> 6) & 0x3F));
            utf8.push_back(0x80 | (c & 0x3F));
        }
    }
    return utf8;
};

void FileDatabase::dumpToCsv(std::wstring const &filename) const {
    std::ofstream out(filename, std::ios::out);
    if (!out.is_open())
        return;

    out << "ID,ParentID,IsDirectory,Size,Creation,Modification,Access,NumChildren,Name\n";

    for (auto const &kv : nodes) {
        const FileNode &n = kv.second;
        std::string nameUtf8 = wstrToUtf8(n.name);

        // escape quotes
        size_t pos = 0;
        while ((pos = nameUtf8.find('"', pos)) != std::string::npos) {
            nameUtf8.insert(pos, 1, '"');
            pos += 2;
        }

        out << n.id << "," << n.parentId << "," << (n.isDirectory ? "1" : "0") << ",";
        out << n.size << "," << n.creationTime << "," << n.modificationTime << "," << n.accessTime << ",";
        out << n.children.size() << ",";
        out << "\"" << nameUtf8 << "\"\n";
    }

    out.close();
}

void FileDatabase::insertNode(FileNode &&node)
{
    nodes.emplace(node.id, std::move(node));
    // update parent's children
    auto pit = nodes.find(node.parentId);
    if (pit != nodes.end())
        pit->second.children.push_back(node.id);
}

u64 FileDatabase::findRootNode() const
{
    for (auto const &[id, node] : nodes) {
        if (nodes.find(node.parentId) == nodes.end())
            return id;
    }
    return 0; // no root found
}
