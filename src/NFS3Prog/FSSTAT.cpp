#include <windows.h>

#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedureFSSTAT(void)
{
    std::string path;
    post_op_attr obj_attributes = {};
    nfsstat3 stat;

    PrintLog("FSSTAT");
    bool validHandle = GetPath(path);
    const char* cStr = validHandle ? path.c_str() : NULL;
    stat = CheckFile(cStr);

    if (stat != NFS3_OK) {
        Write(&stat);
        Write(&obj_attributes);
        return stat;
    }

    obj_attributes.attributes_follow = GetFileAttributesForNFS(cStr, &obj_attributes.attributes);

    size3 tbytes, fbytes, abytes, tfiles = 0, ffiles = 0, afiles = 0;
    uint32 invarsec = 0;

    if (!obj_attributes.attributes_follow ||
        !GetDiskFreeSpaceEx(cStr, (PULARGE_INTEGER)&fbytes, (PULARGE_INTEGER)&tbytes, (PULARGE_INTEGER)&abytes)) {
        stat = NFS3ERR_IO;
        Write(&stat);
        Write(&obj_attributes);
        return stat;
    }

    Write(&stat);
    Write(&obj_attributes);
    Write(&tbytes);
    Write(&fbytes);
    Write(&abytes);
    Write(&tfiles);
    Write(&ffiles);
    Write(&afiles);
    Write(&invarsec);
    return stat;
}
