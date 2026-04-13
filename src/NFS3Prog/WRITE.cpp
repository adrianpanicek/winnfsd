#include <errno.h>
#include <share.h>

#include "../NFSProg.h"

#define BUFFER_SIZE 1000

// Performs a synchronous write and closes the file. Returns NFS error code.
static nfsstat3 write_to_file(const char *path, offset3 offset, const opaque &data, count3 &written)
{
    FILE *pFile = _fsopen(path, "r+b", _SH_DENYWR);
    if (pFile == NULL) {
        return (errno == EACCES) ? NFS3ERR_ACCES : NFS3ERR_IO;
    }
    _fseeki64(pFile, offset, SEEK_SET);
    written = fwrite(data.contents, sizeof(char), data.length, pFile);
    fclose(pFile);
    return NFS3_OK;
}

nfsstat3 CNFS3Prog::ProcedureWRITE(void)
{
    std::string path;
    offset3 offset;
    count3 count;
    stable_how stable;
    opaque data;
    wcc_data file_wcc;
    writeverf3 verf = 0;
    nfsstat3 stat;

    PrintLog("WRITE");
    bool validHandle = GetPath(path);
    const char* cStr = validHandle ? path.c_str() : NULL;
    Read(&offset);
    Read(&count);
    Read(&stable);
    Read(&data);
    stat = CheckFile(cStr);

    file_wcc.before.attributes_follow = GetFileAttributesForNFS(cStr, &file_wcc.before.attributes);

    if (stat != NFS3_OK) {
        Write(&stat);
        Write(&file_wcc);
        return stat;
    }

    if (stable == UNSTABLE) {
        nfs_fh3 handle;
        if (!GetFileHandle(cStr, &handle)) {
            stat = NFS3ERR_IO;
            Write(&stat);
            Write(&file_wcc);
            return stat;
        }
        int handleId = *(unsigned int *)handle.contents;

        FILE *pFile;
        if (unstableStorageFile.count(handleId) == 0) {
            pFile = _fsopen(cStr, "r+b", _SH_DENYWR);
            if (pFile != NULL) {
                unstableStorageFile.insert(std::make_pair(handleId, pFile));
            }
        } else {
            pFile = unstableStorageFile[handleId];
        }

        if (pFile == NULL) {
            stat = (errno == EACCES) ? NFS3ERR_ACCES : NFS3ERR_IO;
            Write(&stat);
            Write(&file_wcc);
            return stat;
        }

        _fseeki64(pFile, offset, SEEK_SET);
        count = fwrite(data.contents, sizeof(char), data.length, pFile);
        // verf should be process start time; using 0 as placeholder
        verf = 0;
        // no physical write yet — after attrs unchanged
        file_wcc.after.attributes_follow = false;
    } else {
        stat = write_to_file(cStr, offset, data, count);

        if (stat != NFS3_OK) {
            char buffer[BUFFER_SIZE];
            strerror_s(buffer, BUFFER_SIZE, errno);
            PrintLog(buffer);
            Write(&stat);
            Write(&file_wcc);
            return stat;
        }

        stable = FILE_SYNC;
        verf = 0;
        file_wcc.after.attributes_follow = GetFileAttributesForNFS(cStr, &file_wcc.after.attributes);
    }

    Write(&stat);
    Write(&file_wcc);
    Write(&count);
    Write(&stable);
    Write(&verf);
    return stat;
}
