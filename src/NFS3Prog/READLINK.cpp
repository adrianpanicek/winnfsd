#undef UNICODE
#include <windows.h>
#include <string>
#include <algorithm>
#include <cstdlib>
#include "shlwapi.h"

#include "../NFSProg.h"

// REPARSE_DATA_BUFFER specifies header and expects a user to allocate enough memory for the rest
// NOTE: This is not thread safe
struct {
    REPARSE_DATA_BUFFER buffer;
    char* _padding[MAXIMUM_REPARSE_DATA_BUFFER_SIZE - sizeof(REPARSE_DATA_BUFFER)];
} widened_allocation;

REPARSE_DATA_BUFFER* lpOutBuffer = (REPARSE_DATA_BUFFER*) &widened_allocation.buffer;

void collapse_double_backslashes(std::string& s)
{
    std::size_t pos = 0;
    while ((pos = s.find("\\\\", pos)) != std::string::npos) {
        s.replace(pos, 2, "\\");
        ++pos;
    }
}

void build_relative_path(std::string &relativePath, std::string basePath, std::string targetPath)
{
    std::string tempBase(basePath);
    std::string tempTarget(targetPath);

    // Sanitize paths by removing weird \\\\?\\ prefix
    tempBase.erase(0, tempBase.find_first_of(":") - 1);
    tempTarget.erase(0, tempTarget.find_first_of(":") - 1);

    collapse_double_backslashes(tempBase);
    collapse_double_backslashes(tempTarget);

    // Remove last folder from base path to get the parent directory
    size_t lastFolderIndex = tempBase.find_last_of('\\');
    if (lastFolderIndex != std::string::npos) {
        tempBase = tempBase.substr(0, lastFolderIndex);
    }

    char szOut[MAX_PATH];
    PathRelativePathTo(szOut, tempBase.c_str(), FILE_ATTRIBUTE_DIRECTORY, tempTarget.c_str(), FILE_ATTRIBUTE_DIRECTORY);

    relativePath.assign(szOut);
}

void parse_symlink(std::string &symlink, std::string path) 
{
    size_t nlen = lpOutBuffer->SymbolicLinkReparseBuffer.PrintNameLength / sizeof(WCHAR);
    auto szName = std::vector<WCHAR>(nlen + 1);
    std::string tempPath;

    wcsncpy_s(
        &szName[0], 
        nlen + 1, 
        &lpOutBuffer->SymbolicLinkReparseBuffer.PathBuffer[
            lpOutBuffer->SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(WCHAR)
        ], 
        nlen
    );
    szName[nlen] = '\0'; // Add null terminator

    std::wstring wStringTemp(&szName[0]);
    tempPath.append(wStringTemp.begin(), wStringTemp.end());

    if (!PathIsRelative(tempPath.c_str()))
    {
        std::string relativePath;
        build_relative_path(relativePath, path, tempPath);

        symlink.assign(relativePath);

        return;
    }

    symlink.assign(tempPath);
}

void parse_lx_symlink(std::string &symlink, std::string path)
{
    const size_t LX_SYMLINK_TYPE = 4; // This is always present but we can ignore these bytes

    // Remove header of REPARSE_DATA_BUFFER so we get the raw data
    size_t nlen = (MAXIMUM_REPARSE_DATA_BUFFER_SIZE - sizeof(REPARSE_DATA_BUFFER));
    auto szName = std::vector<WCHAR>(nlen / sizeof(WCHAR));

    MultiByteToWideChar(
        CP_UTF8, 
        MB_ERR_INVALID_CHARS, 
        (LPCCH) &lpOutBuffer->GenericReparseBuffer.DataBuffer + LX_SYMLINK_TYPE, 
        static_cast<int>(nlen), 
        &szName[0], 
        static_cast<int>(nlen / sizeof(WCHAR))
    );

    std::wstring tempWPath(&szName[0]);
    std::string tempPath;
    tempPath.append(tempWPath.begin(), tempWPath.end());

    symlink.assign(tempPath);
}

void parse_mount_point(std::string &symlink, std::string path)
{
    size_t nlen = lpOutBuffer->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
    auto szName = std::vector<WCHAR>(nlen + 1);

    wcsncpy_s(
        &szName[0], 
        nlen + 1, 
        &lpOutBuffer->MountPointReparseBuffer.PathBuffer[
            lpOutBuffer->MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR)
        ], 
        nlen
    );
    szName[nlen] = '\0';

    std::wstring tempWSymlink(&szName[0]);
    std::string tempSymlink;
    std::string tempRoot(path);
    tempSymlink.append(tempWSymlink.begin(), tempWSymlink.end());

    std::string relativePath;
    build_relative_path(relativePath, path, tempSymlink);

    symlink.assign(relativePath);
}

nfsstat3 CNFS3Prog::ProcedureREADLINK(void)
{
    PrintLog("READLINK");
    
    std::string path;
    std::wstring wpath;

    post_op_attr symlink_attributes;
    nfspath3 data = nfspath3();
    nfsstat3 stat;

    HANDLE hFile;
    
    if (!GetPath(path)) {
        stat = NFS3ERR_STALE;
        Write(&stat);
        Write(&symlink_attributes);

        return stat;
    }

    stat = CheckFile(path.c_str());
    symlink_attributes.attributes_follow = GetFileAttributesForNFS(path.c_str(), &symlink_attributes.attributes);

    wpath.append(path.begin(), path.end());

    if (stat != NFS3_OK) {
        Write(&stat);
        Write(&symlink_attributes);

        return stat;
    }

    hFile = CreateFile(
        path.c_str(), 
        GENERIC_READ, 
        FILE_SHARE_READ, 
        NULL, 
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_REPARSE_POINT | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, 
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        stat = NFS3ERR_IO;
        Write(&stat);
        Write(&symlink_attributes);

        return stat;
    }

    // Reset buffer
    memset(lpOutBuffer, 0, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    DWORD bytesReturned;
    if (!DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, NULL, 0, lpOutBuffer, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &bytesReturned, NULL)) {
        DWORD errorCode = GetLastError();
        PrintLog("DeviceIoControl failed with error code: %lu", errorCode);

        stat = NFS3ERR_IO;
        Write(&stat);
        Write(&symlink_attributes);
        CloseHandle(hFile);

        return stat;
    }

    std::string resultingSymlink;
    switch (lpOutBuffer->ReparseTag) {
        case IO_REPARSE_TAG_SYMLINK:
            parse_symlink(resultingSymlink, path);
            break;
        case IO_REPARSE_TAG_LX_SYMLINK:
            parse_lx_symlink(resultingSymlink, path);
            break;
        case IO_REPARSE_TAG_MOUNT_POINT:
            parse_mount_point(resultingSymlink, path);
            break;
    }

    CloseHandle(hFile);

    std::string result;
    result.append(resultingSymlink.begin(), resultingSymlink.end());
    // write path always with / separator, so windows created symlinks work too
    std::replace(result.begin(), result.end(), '\\', '/');
    data.Set(result.c_str());  
    Write(&stat);
    Write(&symlink_attributes);              
    Write(&data);

    return stat;
}