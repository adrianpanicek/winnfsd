#include <windows.h>

#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedureLINK(void)
{
    PrintLog("LINK");
    std::string path;
    std::string dirName;
    std::string fileName;
    post_op_attr obj_attributes = {};
    wcc_data dir_wcc;
    nfsstat3 stat;

    bool validHandle = GetPath(path);
    const char* cStr = validHandle ? path.c_str() : NULL;
    ReadDirectory(dirName, fileName);
    char *linkFullPath = GetFullPath(dirName, fileName);

    dir_wcc.before.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.before.attributes);

    if (linkFullPath == NULL) {
        stat = NFS3ERR_NAMETOOLONG;
        dir_wcc.after.attributes_follow = false;
        Write(&stat);
        Write(&obj_attributes);
        Write(&dir_wcc);
        return stat;
    }

    //TODO: Improve checks — cStr may be NULL if the source handle is invalid
    if (CreateHardLink(linkFullPath, cStr, NULL) == 0) {
        stat = (CheckFile(linkFullPath) == NFS3_OK) ? NFS3ERR_EXIST : NFS3ERR_IO;
        dir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.after.attributes);
        Write(&stat);
        Write(&obj_attributes);
        Write(&dir_wcc);
        return stat;
    }

    obj_attributes.attributes_follow = GetFileAttributesForNFS(cStr, &obj_attributes.attributes);
    if (!obj_attributes.attributes_follow) {
        stat = NFS3ERR_IO;
        dir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.after.attributes);
        Write(&stat);
        Write(&obj_attributes);
        Write(&dir_wcc);
        return stat;
    }

    dir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.after.attributes);

    stat = NFS3_OK;
    Write(&stat);
    Write(&obj_attributes);
    Write(&dir_wcc);
    return stat;
}
