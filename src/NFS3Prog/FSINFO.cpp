#include "../NFSProg.h"

#define NFS3_READ_MAX       65536
#define NFS3_READ_PREF      32768
#define NFS3_BLOCK_MULT      4096
#define NFS3_WRITE_MAX      65536
#define NFS3_WRITE_PREF     32768
#define NFS3_DIR_PREF        8192
#define NFS3_MAXFILESIZE    0x7FFFFFFFFFFFFFFF

nfsstat3 CNFS3Prog::ProcedureFSINFO(void)
{
    std::string path;
    post_op_attr obj_attributes = {};
    nfsstat3 stat;

    PrintLog("FSINFO");
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

    uint32 rtmax     = NFS3_READ_MAX;
    uint32 rtpref    = NFS3_READ_PREF;
    uint32 rtmult    = NFS3_BLOCK_MULT;
    uint32 wtmax     = NFS3_WRITE_MAX;
    uint32 wtpref    = NFS3_WRITE_PREF;
    uint32 wtmult    = NFS3_BLOCK_MULT;
    uint32 dtpref    = NFS3_DIR_PREF;
    size3  maxfilesize = NFS3_MAXFILESIZE;
    nfstime3 time_delta = { 1, 0 };
    uint32 properties = FSF3_LINK | FSF3_SYMLINK | FSF3_CANSETTIME;

    Write(&stat);
    Write(&obj_attributes);
    Write(&rtmax);
    Write(&rtpref);
    Write(&rtmult);
    Write(&wtmax);
    Write(&wtpref);
    Write(&wtmult);
    Write(&dtpref);
    Write(&maxfilesize);
    Write(&time_delta);
    Write(&properties);
    return stat;
}
