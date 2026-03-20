Help me figure out how to design my FileDatabase and SwanExplorerWindow according to these requirements:

- One global FileDatabase per drive, e.g. std::map<wchar_t, FileDatabase> which will probably be refreshed by a separate thread per FileDatabase using ReadDirectoryChangesW. The FileDatabase should be designed so that FILE_NOTIFY_CHANGE_ records can be efficiently applied (finding the right entry to update name/size/etc should be quick, and removing the entry if it's deleted). Initially I will probably just lock the FileDatabase to update it, and buffer notifications from ReadDirectoryChangesW to limit locking to once per second or so, but I am open to the concept of triple buffering so that it can be lock free.

- Many SwanExplorerWindow instances which have one focal directory at any moment, but have two modes: folder and recursive
  - Folder mode is like traditional file explorer, only direct children (file/dirs) are shown
  - Recursive mode shows direct and nested children to a certain depth (could be infinite). Nested children have their path shown as `path/to/file` whereas direct children are just `file`

- I don't want to store a full copy of the MFT (or equivalent for FAT) due to high memory requirement, ideally I would parse out only the information relevant to my app and store it compactly. I would like some kind of pool/arena or alternative to store the filenames so that allocation overhead is low

- The most important operations to optimize data structures for are navigation of big folders (remember, recursive mode) and searching/filtering and sorting the focal directory in Qt5 (C++)

- The data structure should enable effective parallel parsing of the MFT (or equivalent for FAT) and if possible, usage of SIMD for string comparison

- The data structure should enable efficient rendering of table entries in Qt5 C++, for good scrolling performance
