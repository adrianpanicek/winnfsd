#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedureLOOKUP(void)
{
    nfs_fh3 object;
    post_op_attr obj_attributes;
    post_op_attr dir_attributes;
    nfsstat3 stat;

    PrintLog("LOOKUP");

    std::string dirName;
    std::string fileName;
    ReadDirectory(dirName, fileName);

    char *path = GetFullPath(dirName, fileName);
    dir_attributes.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_attributes.attributes);

    if (path == NULL) {
        stat = NFS3ERR_NAMETOOLONG;
        Write(&stat);
        Write(&dir_attributes);
        return stat;
    }

    stat = CheckFile(dirName.c_str(), path);

    if (stat != NFS3_OK) {
        Write(&stat);
        Write(&dir_attributes);
        return stat;
    }

    GetFileHandle(path, &object);
    obj_attributes.attributes_follow = GetFileAttributesForNFS(path, &obj_attributes.attributes);

    Write(&stat);
    Write(&object);
    Write(&obj_attributes);
    Write(&dir_attributes);
    return stat;
}
