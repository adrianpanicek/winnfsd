#include <direct.h>
#include <errno.h>

#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedureMKDIR(void)
{
    post_op_fh3 obj;
    post_op_attr obj_attributes;
    wcc_data dir_wcc;
    sattr3 attributes;
    nfsstat3 stat;

    PrintLog("MKDIR");

    std::string dirName;
    std::string fileName;
    ReadDirectory(dirName, fileName);
    char *path = GetFullPath(dirName, fileName);
    Read(&attributes);

    dir_wcc.before.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.before.attributes);

    if (path == NULL) {
        stat = NFS3ERR_NAMETOOLONG;
        dir_wcc.after.attributes_follow = false;
        Write(&stat);
        Write(&dir_wcc);
        return stat;
    }

    if (_mkdir(path) != 0) {
        if (errno == EEXIST) {
            PrintLog("Directory already exists.");
            stat = NFS3ERR_EXIST;
        } else if (errno == ENOENT) {
            stat = NFS3ERR_NOENT;
        } else {
            stat = (CheckFile(path) == NFS3_OK) ? NFS3_OK : NFS3ERR_IO;
        }

        dir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.after.attributes);
        Write(&stat);
        Write(&dir_wcc);
        return stat;
    }

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
