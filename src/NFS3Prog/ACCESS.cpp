#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedureACCESS(void)
{
    std::string path;
    uint32 access;
    post_op_attr obj_attributes;
    nfsstat3 stat;

    PrintLog("ACCESS");
    bool validHandle = GetPath(path);
    const char* cStr = validHandle ? path.c_str() : NULL;
    Read(&access);
    stat = CheckFile(cStr);

    if (stat == NFS3ERR_NOENT) {
        stat = NFS3ERR_STALE;
    }

    obj_attributes.attributes_follow = GetFileAttributesForNFS(cStr, &obj_attributes.attributes);

    if (stat != NFS3_OK) {
        Write(&stat);
        Write(&obj_attributes);
        return stat;
    }

    Write(&stat);
    Write(&obj_attributes);
    Write(&access);
    return stat;
}
