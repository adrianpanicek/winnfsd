#include <windows.h>

#include "../NFSProg.h"
#include "../FileTable.h"

nfsstat3 CNFS3Prog::ProcedureRMDIR(void)
{
    wcc_data dir_wcc;
    nfsstat3 stat;

    PrintLog("RMDIR");

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

    unsigned long returnCode = RemoveFolder(path);
    if (returnCode == ERROR_DIR_NOT_EMPTY) {
        stat = NFS3ERR_NOTEMPTY;
    } else if (returnCode != 0) {
        stat = NFS3ERR_IO;
    } else {
        stat = NFS3_OK;
    }

    dir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.after.attributes);
    Write(&stat);
    Write(&dir_wcc);
    return stat;
}
