#include "../NFSProg.h"
#include "../FileTable.h"

nfsstat3 CNFS3Prog::ProcedureCOMMIT(void)
{
    std::string path;
    wcc_data file_wcc;
    nfs_fh3 file;
    writeverf3 verf = 0;
    nfsstat3 stat;

    PrintLog("COMMIT");
    Read(&file);
    bool validHandle = GetFilePath(file.contents, path);
    const char* cStr = validHandle ? path.c_str() : NULL;

    if (validHandle) {
        PrintLog(" %s ", path.c_str());
    }

    // offset and count are present in the request but unused — COMMIT here
    // operates on the full file tracked by handle, not a byte range.
    // To fully conform to the spec this should be improved.
    offset3 offset;
    count3 count;
    Read(&offset);
    Read(&count);

    file_wcc.before.attributes_follow = GetFileAttributesForNFS(cStr, &file_wcc.before.attributes);

    int handleId = *(unsigned int *)file.contents;

    if (unstableStorageFile.count(handleId) != 0) {
        if (unstableStorageFile[handleId] == NULL) {
            stat = NFS3ERR_IO;
            file_wcc.after.attributes_follow = GetFileAttributesForNFS(cStr, &file_wcc.after.attributes);
            Write(&stat);
            Write(&file_wcc);
            Write(&verf);
            return stat;
        }
        fclose(unstableStorageFile[handleId]);
        unstableStorageFile.erase(handleId);
    }

    stat = NFS3_OK;
    file_wcc.after.attributes_follow = GetFileAttributesForNFS(cStr, &file_wcc.after.attributes);

    Write(&stat);
    Write(&file_wcc);
    // verf should be the server start timestamp to detect reboots
    Write(&verf);
    return stat;
}
