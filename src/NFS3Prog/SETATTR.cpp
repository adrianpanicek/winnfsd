#include <windows.h>
#include <io.h>
#include <sys/stat.h>
#include <share.h>

#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedureSETATTR(void)
{
    std::string path;
    sattr3 new_attributes;
    sattrguard3 guard;
    wcc_data obj_wcc;
    nfsstat3 stat;

    PrintLog("SETATTR");
    bool validHandle = GetPath(path);
    const char* cStr = validHandle ? path.c_str() : NULL;
    Read(&new_attributes);
    Read(&guard);
    stat = CheckFile(cStr);
    obj_wcc.before.attributes_follow = GetFileAttributesForNFS(cStr, &obj_wcc.before.attributes);

    if (stat != NFS3_OK) {
        obj_wcc.after.attributes_follow = GetFileAttributesForNFS(cStr, &obj_wcc.after.attributes);
        Write(&stat);
        Write(&obj_wcc);
        return stat;
    }

    if (new_attributes.mode.set_it) {
        int nMode = 0;

        if ((new_attributes.mode.mode & 0x100) != 0) {
            nMode |= S_IREAD;
        }

        // Always set read and write permissions (deliberately implemented this way)
        nMode |= S_IWRITE;

        // S_IEXEC is not available on Windows

        if (_chmod(cStr, nMode) != 0) {
            stat = NFS3ERR_INVAL;
        }
    }

    // uid/gid cannot be reflected on Windows
    // mtime/atime SET_TO_CLIENT_TIME is not implemented

    if (new_attributes.mtime.set_it == SET_TO_SERVER_TIME || new_attributes.atime.set_it == SET_TO_SERVER_TIME) {
        HANDLE hFile = CreateFile(cStr, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
        if (hFile != INVALID_HANDLE_VALUE) {
            SYSTEMTIME systemTime;
            FILETIME fileTime;
            GetSystemTime(&systemTime);
            SystemTimeToFileTime(&systemTime, &fileTime);
            if (new_attributes.mtime.set_it == SET_TO_SERVER_TIME) {
                SetFileTime(hFile, NULL, NULL, &fileTime);
            }
            if (new_attributes.atime.set_it == SET_TO_SERVER_TIME) {
                SetFileTime(hFile, NULL, &fileTime, NULL);
            }
            CloseHandle(hFile);
        }
    }

    if (new_attributes.size.set_it) {
        FILE *pFile = _fsopen(cStr, "r+b", _SH_DENYWR);
        if (pFile != NULL) {
            _chsize_s(_fileno(pFile), new_attributes.size.size);
            fclose(pFile);
        }
    }

    obj_wcc.after.attributes_follow = GetFileAttributesForNFS(cStr, &obj_wcc.after.attributes);

    Write(&stat);
    Write(&obj_wcc);
    return stat;
}
