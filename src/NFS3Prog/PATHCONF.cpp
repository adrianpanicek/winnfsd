#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedurePATHCONF(void)
{
    std::string path;
    post_op_attr obj_attributes = {};
    nfsstat3 stat;

    PrintLog("PATHCONF");
    bool validHandle = GetPath(path);
    const char* cStr = validHandle ? path.c_str() : NULL;
    stat = CheckFile(cStr);

    if (stat != NFS3_OK) {
        Write(&stat);
        Write(&obj_attributes);
        return stat;
    }

    obj_attributes.attributes_follow = GetFileAttributesForNFS(cStr, &obj_attributes.attributes);

    if (!obj_attributes.attributes_follow) {
        stat = NFS3ERR_SERVERFAULT;
        Write(&stat);
        Write(&obj_attributes);
        return stat;
    }

    uint32 linkmax        = MAXPATHLEN - 1;
    uint32 name_max       = MAXNAMELEN;
    bool no_trunc         = true;
    bool chown_restricted = true;
    bool case_insensitive = true;
    bool case_preserving  = true;

    Write(&stat);
    Write(&obj_attributes);
    Write(&linkmax);
    Write(&name_max);
    Write(&no_trunc);
    Write(&chown_restricted);
    Write(&case_insensitive);
    Write(&case_preserving);
    return stat;
}
