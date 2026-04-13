#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedureGETATTR(void)
{
    std::string path;
    fattr3 obj_attributes;
    nfsstat3 stat;

    PrintLog("GETATTR");
    bool validHandle = GetPath(path);
    const char* cStr = validHandle ? path.c_str() : NULL;
    stat = CheckFile(cStr);

    if (stat == NFS3ERR_NOENT) {
        stat = NFS3ERR_STALE;
    }

    if (stat != NFS3_OK) {
        Write(&stat);
        return stat;
    }

    if (!GetFileAttributesForNFS(cStr, &obj_attributes)) {
        stat = NFS3ERR_IO;
        Write(&stat);
        return stat;
    }

    Write(&stat);
    Write(&obj_attributes);
    return stat;
}
