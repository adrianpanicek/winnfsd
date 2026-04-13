#include <errno.h>
#include <share.h>

#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedureCREATE(void)
{
    post_op_fh3 obj;
    post_op_attr obj_attributes;
    wcc_data dir_wcc;
    createhow3 how;
    nfsstat3 stat;

    PrintLog("CREATE");
    std::string dirName;
    std::string fileName;
    ReadDirectory(dirName, fileName);
    char *path = GetFullPath(dirName, fileName);
    Read(&how);

    dir_wcc.before.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.before.attributes);

    if (path == NULL) {
        stat = NFS3ERR_NAMETOOLONG;
        dir_wcc.after.attributes_follow = false;
        Write(&stat);
        Write(&dir_wcc);
        return stat;
    }

    FILE *pFile = _fsopen(path, "wb", _SH_DENYWR);

    if (pFile == NULL) {
        errno_t errorNumber = errno;
        char buffer[1000];
        strerror_s(buffer, sizeof(buffer), errorNumber);
        PrintLog(buffer);

        if (errorNumber == ENOENT) {
            stat = NFS3ERR_STALE;
        } else if (errorNumber == EACCES) {
            stat = NFS3ERR_ACCES;
        } else {
            stat = NFS3ERR_IO;
        }

        dir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.after.attributes);
        Write(&stat);
        Write(&dir_wcc);
        return stat;
    }

    fclose(pFile);

    obj.handle_follows = GetFileHandle(path, &obj.handle);
    obj_attributes.attributes_follow = GetFileAttributesForNFS(path, &obj_attributes.attributes);
    dir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.after.attributes);

    stat = NFS3_OK;
    Write(&stat);
    Write(&obj);
    Write(&obj_attributes);
    Write(&dir_wcc);
    return stat;
}
