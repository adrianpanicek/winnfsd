#include <windows.h>
#include <errno.h>
#include <string.h>

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

nfsstat3 CNFS3Prog::ProcedureRENAME(void)
{
    char pathFrom[MAXPATHLEN];
    wcc_data fromdir_wcc, todir_wcc;
    nfsstat3 stat;

    PrintLog("RENAME");

    std::string dirFromName;
    std::string fileFromName;
    ReadDirectory(dirFromName, fileFromName);
    char *tmpFrom = GetFullPath(dirFromName, fileFromName);

    std::string dirToName;
    std::string fileToName;
    ReadDirectory(dirToName, fileToName);
    char *pathTo = GetFullPath(dirToName, fileToName);

    if (tmpFrom == NULL || pathTo == NULL) {
        stat = NFS3ERR_NAMETOOLONG;
        fromdir_wcc.before.attributes_follow = false;
        fromdir_wcc.after.attributes_follow = false;
        todir_wcc.before.attributes_follow = false;
        todir_wcc.after.attributes_follow = false;
        Write(&stat);
        Write(&fromdir_wcc);
        Write(&todir_wcc);
        return stat;
    }

    strcpy_s(pathFrom, MAXPATHLEN, tmpFrom);

    fromdir_wcc.before.attributes_follow = GetFileAttributesForNFS(dirFromName.c_str(), &fromdir_wcc.before.attributes);
    todir_wcc.before.attributes_follow = GetFileAttributesForNFS(dirToName.c_str(), &todir_wcc.before.attributes);

    stat = CheckFile(dirFromName.c_str(), pathFrom);

    if (stat != NFS3_OK) {
        fromdir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirFromName.c_str(), &fromdir_wcc.after.attributes);
        todir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirToName.c_str(), &todir_wcc.after.attributes);
        Write(&stat);
        Write(&fromdir_wcc);
        Write(&todir_wcc);
        return stat;
    }

    if (FileExists(pathTo)) {
        stat = remove_entry(pathTo);
        if (stat != NFS3_OK) {
            fromdir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirFromName.c_str(), &fromdir_wcc.after.attributes);
            todir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirToName.c_str(), &todir_wcc.after.attributes);
            Write(&stat);
            Write(&fromdir_wcc);
            Write(&todir_wcc);
            return stat;
        }
    }

    errno_t errorNumber = RenameDirectory(pathFrom, pathTo);
    if (errorNumber != 0) {
        char buffer[1000];
        strerror_s(buffer, sizeof(buffer), errorNumber);
        PrintLog(buffer);
        stat = (errorNumber == EACCES) ? NFS3ERR_ACCES : NFS3ERR_IO;
    }

    fromdir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirFromName.c_str(), &fromdir_wcc.after.attributes);
    todir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirToName.c_str(), &todir_wcc.after.attributes);
    Write(&stat);
    Write(&fromdir_wcc);
    Write(&todir_wcc);
    return stat;
}
