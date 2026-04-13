#include <windows.h>
#include <string>
#include <algorithm>

#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedureSYMLINK(void)
{
    PrintLog("SYMLINK");

    post_op_fh3 obj;
    post_op_attr obj_attributes;
    wcc_data dir_wcc;
    symlinkdata3 symlink;
    nfsstat3 stat;

    std::string dirName;
    std::string fileName;
    ReadDirectory(dirName, fileName);
    char *path = GetFullPath(dirName, fileName);
    Read(&symlink);

    dir_wcc.before.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.before.attributes);

    if (path == NULL) {
        stat = NFS3ERR_NAMETOOLONG;
        dir_wcc.after.attributes_follow = false;
        Write(&stat);
        Write(&dir_wcc);
        return stat;
    }

    // TODO: Maybe revisit this later for a cleaner solution.
    // Convert target path to Windows format. Without this conversion,
    // nested folder symlinks do not work cross-platform.
    std::string strFromChar;
    symlink.symlink_data.path
        ? strFromChar.append(symlink.symlink_data.path)
        : strFromChar.append((const char *)symlink.symlink_data.contents, (size_t)symlink.symlink_data.length);
    std::replace(strFromChar.begin(), strFromChar.end(), '/', '\\');
    const char *lpTargetFileName = strFromChar.c_str();

    // Relative paths don't work with GetFileAttributes for directories,
    // so normalise to absolute before querying the target type.
    std::string fullTargetPath = dirName + "\\" + strFromChar;
    char fullTargetPathNormalized[MAX_PATH];
    GetFullPathNameA(fullTargetPath.c_str(), MAX_PATH, fullTargetPathNormalized, NULL);
    DWORD targetFileAttr = GetFileAttributes(fullTargetPathNormalized);

    DWORD dwFlags = (targetFileAttr & FILE_ATTRIBUTE_DIRECTORY) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;

    if (CreateSymbolicLink(path, lpTargetFileName, dwFlags) == 0) {
        stat = (CheckFile(path) == NFS3_OK) ? NFS3ERR_EXIST : NFS3ERR_IO;
        PrintLog("CreateSymbolicLink failed.");
        dir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.after.attributes);
        Write(&stat);
        Write(&dir_wcc);
        return stat;
    }

    obj.handle_follows = GetFileHandle(path, &obj.handle);
    obj_attributes.attributes_follow = GetFileAttributesForNFS(path, &obj_attributes.attributes);
    dir_wcc.after.attributes_follow = GetFileAttributesForNFS(dirName.c_str(), &dir_wcc.after.attributes);

    stat = NFS3_OK;
    Write(&stat);
    Write(&obj);
    Write(&obj_attributes);
    Write(&dir_wcc);
    return stat;
}
