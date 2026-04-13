#include <io.h>
#include <stdio.h>

#include "../NFSProg.h"
#include "../FileTable.h"

#define MAX_DIR_ENTRIES 10

// Advances the find handle by `cookie` entries to resume a previous listing.
static int seek_to_cookie(intptr_t handle, struct _finddata_t &fileinfo, cookie3 cookie)
{
    int nFound = 0;
    for (unsigned int i = (unsigned int)cookie; i > 0; i--)
        nFound = _findnext(handle, &fileinfo);
    return nFound;
}

nfsstat3 CNFS3Prog::ProcedureREADDIRPLUS(void)
{
    std::string path;
    cookie3 cookie;
    cookieverf3 cookieverf;
    count3 dircount, maxcount;
    post_op_attr dir_attributes;
    nfsstat3 stat;

    PrintLog("READDIRPLUS");
    bool validHandle = GetPath(path);
    const char* cStr = validHandle ? path.c_str() : NULL;
    Read(&cookie);
    Read(&cookieverf);
    Read(&dircount);
    Read(&maxcount);
    stat = CheckFile(cStr);

    if (stat == NFS3_OK) {
        dir_attributes.attributes_follow = GetFileAttributesForNFS(cStr, &dir_attributes.attributes);
        if (!dir_attributes.attributes_follow) {
            stat = NFS3ERR_IO;
        }
    }

    Write(&stat);
    Write(&dir_attributes);

    if (stat != NFS3_OK) {
        return stat;
    }

    char filePath[MAXPATHLEN];
    struct _finddata_t fileinfo;
    bool eof = true;

    Write(&cookieverf);
    sprintf_s(filePath, MAXPATHLEN, "%s\\*", cStr);
    intptr_t handle = _findfirst(filePath, &fileinfo);

    if (handle) {
        int nFound = seek_to_cookie(handle, fileinfo, cookie);

        // TODO: replace fixed threshold with proper count-based limiting
        if (nFound == 0) {
            bool bFollows = true;
            int remaining = MAX_DIR_ENTRIES;

            do {
                Write(&bFollows);
                sprintf_s(filePath, MAXPATHLEN, "%s\\%s", cStr, fileinfo.name);
                fileid3 fileid = GetFileID(filePath);
                Write(&fileid);
                filename3 name;
                name.Set(fileinfo.name);
                Write(&name);
                ++cookie;
                Write(&cookie);
                post_op_attr name_attributes;
                name_attributes.attributes_follow = GetFileAttributesForNFS(filePath, &name_attributes.attributes);
                Write(&name_attributes);
                post_op_fh3 name_handle;
                name_handle.handle_follows = GetFileHandle(filePath, &name_handle.handle);
                Write(&name_handle);

                if (--remaining == 0) {
                    eof = false;
                    break;
                }
            } while (_findnext(handle, &fileinfo) == 0);
        }

        _findclose(handle);
    }

    bool bFollows = false;
    Write(&bFollows);
    Write(&eof);
    return stat;
}
