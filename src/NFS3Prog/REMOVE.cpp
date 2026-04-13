#include <windows.h>

#include "../NFSProg.h"
#include "../FileTable.h"

// Removes a file or a directory symlink/junction at the given path.
static nfsstat3 remove_entry(const char *path)
{
    DWORD fileAttr = GetFileAttributes(path);
    if ((fileAttr & FILE_ATTRIBUTE_DIRECTORY) && (fileAttr & FILE_ATTRIBUTE_REPARSE_POINT)) {
        unsigned long returnCode = RemoveFolder(path);
        if (returnCode == ERROR_DIR_NOT_EMPTY) return NFS3ERR_NOTEMPTY;
        if (returnCode != 0) return NFS3ERR_IO;
        return NFS3_OK;
    }
    return RemoveFile(path) ? NFS3_OK : NFS3ERR_IO;
}

nfsstat3 CNFS3Prog::ProcedureREMOVE(void)
{
    wcc_data dir_wcc;
    nfsstat3 stat;

    PrintLog("REMOVE");

    std::string dirName;
    std::string fileName;
    ReadDirectory(dirName, fileName);
    char *path = GetFullPath(dirName, fileName);

    dir_wcc.before.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.before.attributes);

    if (path == NULL) {
        stat = NFS3ERR_NAMETOOLONG;
        dir_wcc.after.attributes_follow = false;
        Write(&stat);
        Write(&dir_wcc);
        return stat;
    }

    stat = CheckFile(dirName.c_str(), path);

    if (stat != NFS3_OK) {
        dir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.after.attributes);
        Write(&stat);
        Write(&dir_wcc);
        return stat;
    }

    stat = remove_entry(path);

    dir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.after.attributes);
    Write(&stat);
    Write(&dir_wcc);
    return stat;
}
