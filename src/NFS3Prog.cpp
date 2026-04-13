#pragma comment(lib, "Shlwapi.lib")
#include "NFSProg.h"
#include "FileTable.h"
#include <string.h>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#include <assert.h>
#include <string>
#include <windows.h>
#include <time.h>
#include <share.h>
#include "shlwapi.h"
enum
{
    NFSPROC3_NULL = 0,
    NFSPROC3_GETATTR = 1,
    NFSPROC3_SETATTR = 2,
    NFSPROC3_LOOKUP = 3,
    NFSPROC3_ACCESS = 4,
    NFSPROC3_READLINK = 5,
    NFSPROC3_READ = 6,
    NFSPROC3_WRITE = 7,
    NFSPROC3_CREATE = 8,
    NFSPROC3_MKDIR = 9,
    NFSPROC3_SYMLINK = 10,
    NFSPROC3_MKNOD = 11,
    NFSPROC3_REMOVE = 12,
    NFSPROC3_RMDIR = 13,
    NFSPROC3_RENAME = 14,
    NFSPROC3_LINK = 15,
    NFSPROC3_READDIR = 16,
    NFSPROC3_READDIRPLUS = 17,
    NFSPROC3_FSSTAT = 18,
    NFSPROC3_FSINFO = 19,
    NFSPROC3_PATHCONF = 20,
    NFSPROC3_COMMIT = 21
};

opaque::opaque()
{
    length = 0;
    contents = NULL;
}

opaque::opaque(uint32 len)
{
    contents = NULL;
    SetSize(len);
}

opaque::~opaque()
{
    delete[] contents;
}

void opaque::SetSize(uint32 len)
{
    delete[] contents;
    length = len;
    contents = new unsigned char[length];
    memset(contents, 0, length);
}

void opaque::SetSizeNoRealloc(uint32 len)
{
    length = len;
}

nfs_fh3::nfs_fh3() : opaque(NFS3_FHSIZE)
{
}

nfs_fh3::~nfs_fh3()
{
}

filename3::filename3() : opaque()
{
    name = NULL;
}

filename3::~filename3()
{
}

void filename3::SetSize(uint32 len)
{
    opaque::SetSize(len + 1);
    length = len;
    name = (char *)contents;
}

void filename3::Set(char *str)
{
    SetSize(strlen(str));
    strcpy_s(name, (strlen(str) + 1), str);
}

nfspath3::nfspath3() : opaque()
{
    path = NULL;
}

nfspath3::~nfspath3()
{
}

void nfspath3::SetSize(size_t len)
{
    opaque::SetSize(len + 1);
    length = len;
    path = (char *)contents;
}

void nfspath3::Set(const char *str)
{
    SetSize(strlen(str));
    strcpy_s(path, (strlen(str) + 1), str); // size_t + 1, possible overflow
}

typedef nfsstat3(CNFS3Prog::*PPROC)(void);

CNFS3Prog::CNFS3Prog() : CRPCProg()
{
    m_nUID = m_nGID = 0;
}

CNFS3Prog::~CNFS3Prog()
{
}

void CNFS3Prog::SetUserID(unsigned int nUID, unsigned int nGID)
{
    m_nUID = nUID;
    m_nGID = nGID;
}

int CNFS3Prog::Process(IInputStream *pInStream, IOutputStream *pOutStream, ProcessParam *pParam)
{
    static PPROC pf[] = { 
        &CNFS3Prog::ProcedureNULL, &CNFS3Prog::ProcedureGETATTR, &CNFS3Prog::ProcedureSETATTR, 
        &CNFS3Prog::ProcedureLOOKUP, &CNFS3Prog::ProcedureACCESS, &CNFS3Prog::ProcedureREADLINK, 
        &CNFS3Prog::ProcedureREAD, &CNFS3Prog::ProcedureWRITE, &CNFS3Prog::ProcedureCREATE, 
        &CNFS3Prog::ProcedureMKDIR, &CNFS3Prog::ProcedureSYMLINK, &CNFS3Prog::ProcedureMKNOD, 
        &CNFS3Prog::ProcedureREMOVE, &CNFS3Prog::ProcedureRMDIR, &CNFS3Prog::ProcedureRENAME, 
        &CNFS3Prog::ProcedureLINK, &CNFS3Prog::ProcedureREADDIR, &CNFS3Prog::ProcedureREADDIRPLUS,
        &CNFS3Prog::ProcedureFSSTAT, &CNFS3Prog::ProcedureFSINFO, &CNFS3Prog::ProcedurePATHCONF,
        &CNFS3Prog::ProcedureCOMMIT
    };

    nfsstat3 stat = NFS3_OK;

	struct tm current;
	time_t now;

	time(&now);
	localtime_s(&current, &now);

	PrintLog("[%02d:%02d:%02d] NFS ", current.tm_hour, current.tm_min, current.tm_sec);

    if (pParam->nProc >= sizeof(pf) / sizeof(PPROC)) {
        ProcedureNOIMP();
        PrintLog("\n");

        return PRC_NOTIMP;
    }

    m_pInStream = pInStream;
    m_pOutStream = pOutStream;
    m_pParam = pParam;
    m_nResult = PRC_OK;

    try {
        stat = (this->*pf[pParam->nProc])();
    } catch (const std::runtime_error& re) {
        m_nResult = PRC_FAIL;
        PrintLog("Runtime error: ");
        PrintLog(re.what());
    } catch (const std::exception& ex) {
        m_nResult = PRC_FAIL;
        PrintLog("Exception: ");
        PrintLog(ex.what());
    } catch (...) {
        m_nResult = PRC_FAIL;
        PrintLog("Unknown failure: Possible memory corruption");
    }

    PrintLog(" ");

    if (stat != NFS3_OK) {
        switch (stat) {
            case NFS3_OK:
                PrintLog("OK");
                break;
            case NFS3ERR_PERM:
                PrintLog("PERM");
                break;
            case NFS3ERR_NOENT:
                PrintLog("NOENT");
                break;
            case NFS3ERR_IO:
                PrintLog("IO");
                break;
            case NFS3ERR_NXIO:
                PrintLog("NXIO");
                break;
            case NFS3ERR_ACCES:
                PrintLog("ACCESS");
                break;
            case NFS3ERR_EXIST:
                PrintLog("EXIST");
                break;
            case NFS3ERR_XDEV:
                PrintLog("XDEV");
                break;
            case NFS3ERR_NODEV:
                PrintLog("NODEV");
                break;
            case NFS3ERR_NOTDIR:
                PrintLog("NOTDIR");
                break;
            case NFS3ERR_ISDIR:
                PrintLog("ISDIR");
                break;
            case NFS3ERR_INVAL:
                PrintLog("INVAL");
                break;
            case NFS3ERR_FBIG:
                PrintLog("FBIG");
                break;
            case NFS3ERR_NOSPC:
                PrintLog("NOSPC");
                break;
            case NFS3ERR_ROFS:
                PrintLog("ROFS");
                break;
            case NFS3ERR_MLINK:
                PrintLog("MLINK");
                break;
            case NFS3ERR_NAMETOOLONG:
                PrintLog("NAMETOOLONG");
                break;
            case NFS3ERR_NOTEMPTY:
                PrintLog("NOTEMPTY");
                break;
            case NFS3ERR_DQUOT:
                PrintLog("DQUOT");
                break;
            case NFS3ERR_STALE:
                PrintLog("STALE");
                break;
            case NFS3ERR_REMOTE:
                PrintLog("REMOTE");
                break;
            case NFS3ERR_BADHANDLE:
                PrintLog("BADHANDLE");
                break;
            case NFS3ERR_NOT_SYNC:
                PrintLog("NOT_SYNC");
                break;
            case NFS3ERR_BAD_COOKIE:
                PrintLog("BAD_COOKIE");
                break;
            case NFS3ERR_NOTSUPP:
                PrintLog("NOTSUPP");
                break;
            case NFS3ERR_TOOSMALL:
                PrintLog("TOOSMALL");
                break;
            case NFS3ERR_SERVERFAULT:
                PrintLog("SERVERFAULT");
                break;
            case NFS3ERR_BADTYPE:
                PrintLog("BADTYPE");
                break;
            case NFS3ERR_JUKEBOX:
                PrintLog("JUKEBOX");
                break;
            default:
                assert(false);
                break;
        }
    }

    PrintLog("\n");

    return m_nResult;
}

void CNFS3Prog::Read(bool *pBool)
{
    uint32 b;

    if (m_pInStream->Read(&b) < sizeof(uint32)) {
        throw __LINE__;
    }
        
    *pBool = b == 1;
}

void CNFS3Prog::Read(uint32 *pUint32)
{
    if (m_pInStream->Read(pUint32) < sizeof(uint32)) {
        throw __LINE__;
    }       
}

void CNFS3Prog::Read(uint64 *pUint64)
{
    if (m_pInStream->Read8(pUint64) < sizeof(uint64)) {
        throw __LINE__;
    }        
}

void CNFS3Prog::Read(sattr3 *pAttr)
{
    Read(&pAttr->mode.set_it);

    if (pAttr->mode.set_it) {
        Read(&pAttr->mode.mode);
    }
        
    Read(&pAttr->uid.set_it);

    if (pAttr->uid.set_it) {
        Read(&pAttr->uid.uid);
    }  

    Read(&pAttr->gid.set_it);

    if (pAttr->gid.set_it) {
        Read(&pAttr->gid.gid);
    }       

    Read(&pAttr->size.set_it);

    if (pAttr->size.set_it) {
        Read(&pAttr->size.size);
    }       

    Read(&pAttr->atime.set_it);

    if (pAttr->atime.set_it == SET_TO_CLIENT_TIME) {
        Read(&pAttr->atime.atime);
    }
        
    Read(&pAttr->mtime.set_it);

    if (pAttr->mtime.set_it == SET_TO_CLIENT_TIME) {
        Read(&pAttr->mtime.mtime);
    }        
}

void CNFS3Prog::Read(sattrguard3 *pGuard)
{
    Read(&pGuard->check);

    if (pGuard->check) {
        Read(&pGuard->obj_ctime);
    }        
}

void CNFS3Prog::Read(diropargs3 *pDir)
{
    Read(&pDir->dir);
    Read(&pDir->name);
}

void CNFS3Prog::Read(opaque *pOpaque)
{
    uint32 len, byte;

    Read(&len);
    pOpaque->SetSize(len);

    if (m_pInStream->Read(pOpaque->contents, len) < len) {
        throw __LINE__;
    }        

    len = 4 - (len & 3);

    if (len != 4) {
        if (m_pInStream->Read(&byte, len) < len) {
            throw __LINE__;
        }            
    }
}

void CNFS3Prog::Read(nfstime3 *pTime)
{
    Read(&pTime->seconds);
    Read(&pTime->nseconds);
}

void CNFS3Prog::Read(createhow3 *pHow)
{
    Read(&pHow->mode);

    if (pHow->mode == UNCHECKED || pHow->mode == GUARDED) {
        Read(&pHow->obj_attributes);
    } else {
        Read(&pHow->verf);
    }       
}

void CNFS3Prog::Read(symlinkdata3 *pSymlink)
{
	Read(&pSymlink->symlink_attributes);
	Read(&pSymlink->symlink_data);
}

void CNFS3Prog::Write(bool *pBool)
{
    m_pOutStream->Write(*pBool ? 1 : 0);
}

void CNFS3Prog::Write(uint32 *pUint32)
{
    m_pOutStream->Write(*pUint32);
}

void CNFS3Prog::Write(uint64 *pUint64)
{
    m_pOutStream->Write8(*pUint64);
}

void CNFS3Prog::Write(fattr3 *pAttr)
{
    Write(&pAttr->type);
    Write(&pAttr->mode);
    Write(&pAttr->nlink);
    Write(&pAttr->uid);
    Write(&pAttr->gid);
    Write(&pAttr->size);
    Write(&pAttr->used);
    Write(&pAttr->rdev);
    Write(&pAttr->fsid);
    Write(&pAttr->fileid);
    Write(&pAttr->atime);
    Write(&pAttr->mtime);
    Write(&pAttr->ctime);
}

void CNFS3Prog::Write(opaque *pOpaque)
{
    uint32 len, byte;

    Write(&pOpaque->length);
    m_pOutStream->Write(pOpaque->contents, pOpaque->length);
    len = pOpaque->length & 3;

    if (len != 0) {
        byte = 0;
        m_pOutStream->Write(&byte, 4 - len);
    }
}

void CNFS3Prog::Write(wcc_data *pWcc)
{
    Write(&pWcc->before);
    Write(&pWcc->after);
}

void CNFS3Prog::Write(post_op_attr *pAttr)
{
    Write(&pAttr->attributes_follow);

    if (pAttr->attributes_follow) {
        Write(&pAttr->attributes);
    }      
}

void CNFS3Prog::Write(pre_op_attr *pAttr)
{
    Write(&pAttr->attributes_follow);

    if (pAttr->attributes_follow) {
        Write(&pAttr->attributes);
    }    
}

void CNFS3Prog::Write(post_op_fh3 *pObj)
{
    Write(&pObj->handle_follows);

    if (pObj->handle_follows) {
        Write(&pObj->handle);
    }     
}

void CNFS3Prog::Write(nfstime3 *pTime)
{
    Write(&pTime->seconds);
    Write(&pTime->nseconds);
}

void CNFS3Prog::Write(specdata3 *pSpec)
{
    Write(&pSpec->specdata1);
    Write(&pSpec->specdata2);
}

void CNFS3Prog::Write(wcc_attr *pAttr)
{
    Write(&pAttr->size);
    Write(&pAttr->mtime);
    Write(&pAttr->ctime);
}

bool CNFS3Prog::GetPath(std::string &path)
{
    nfs_fh3 object;

    Read(&object);
    bool valid = GetFilePath(object.contents, path);
    if (valid) {
        PrintLog(" %s ", path.c_str());
    } else {
        PrintLog(" File handle is invalid ");
    }

    return valid;
}

bool CNFS3Prog::ReadDirectory(std::string &dirName, std::string &fileName)
{
    diropargs3 fileRequest;
    Read(&fileRequest);

    if (GetFilePath(fileRequest.dir.contents, dirName)) {
        fileName = std::string(fileRequest.name.name);
        return true;
    } else {
        return false;
    }

    //PrintLog(" %s | %s ", dirName.c_str(), fileName.c_str());
}

char *CNFS3Prog::GetFullPath(std::string &dirName, std::string &fileName)
{
    //TODO: Return std::string
    static char fullPath[MAXPATHLEN + 1];

    if (dirName.size() + 1 + fileName.size() > MAXPATHLEN) {
        return NULL;
    }

    sprintf_s(fullPath, "%s\\%s", dirName.c_str(), fileName.c_str()); //concate path and filename
    PrintLog(" %s ", fullPath);

    return fullPath;
}

nfsstat3 CNFS3Prog::CheckFile(const char *fullPath)
{
    if (fullPath == NULL) {
        return NFS3ERR_STALE;
    }

	//if (!FileExists(fullPath)) {
	if (_access(fullPath, 0) != 0) {
        return NFS3ERR_NOENT;
    }

    return NFS3_OK;
}

nfsstat3 CNFS3Prog::CheckFile(const char *directory, const char *fullPath)
{
    // FileExists will not work for the root of a drive, e.g. \\?\D:\, therefore check if it is a drive root with GetDriveType
    if (directory == NULL || (!FileExists(directory) && GetDriveType(directory) < 2) || fullPath == NULL) {
        return NFS3ERR_STALE;
    }
        
    if (!FileExists(fullPath)) {
        return NFS3ERR_NOENT;
    }
        
    return NFS3_OK;
}

bool CNFS3Prog::GetFileHandle(const char *path, nfs_fh3 *pObject)
{
	if (!::GetFileHandle(path)) {
		PrintLog("no filehandle(path %s)", path);
		return false;
	}
    auto err = memcpy_s(pObject->contents, NFS3_FHSIZE, ::GetFileHandle(path), pObject->length);
	if (err != 0) {
		PrintLog(" err %d ", err);
		return false;
	}

    return true;
}

bool CNFS3Prog::GetFileAttributesForNFS(const char *path, wcc_attr *pAttr)
{
    struct stat data;

    if (path == NULL) {
        return false;
    }

    if (stat(path, &data) != 0) {
        return false;
    }

    pAttr->size = data.st_size;
    pAttr->mtime.seconds = (unsigned int)data.st_mtime;
    pAttr->mtime.nseconds = 0;
	// TODO: This needs to be tested (not called on my setup)
	// This seems to be the changed time, not creation time.
    //pAttr->ctime.seconds = data.st_ctime;
    pAttr->ctime.seconds = (unsigned int)data.st_mtime;
    pAttr->ctime.nseconds = 0;

    return true;
}

bool CNFS3Prog::GetFileAttributesForNFS(const char *path, fattr3 *pAttr)
{
    DWORD fileAttr;
    BY_HANDLE_FILE_INFORMATION lpFileInformation;
    HANDLE hFile;
    DWORD dwFlagsAndAttributes;

    fileAttr = GetFileAttributes(path);

    if (path == NULL || fileAttr == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }

    dwFlagsAndAttributes = 0;
    if (fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
        pAttr->type = NF3DIR;
        dwFlagsAndAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_FLAG_BACKUP_SEMANTICS;
    }
    else if (fileAttr & FILE_ATTRIBUTE_ARCHIVE) {
        pAttr->type = NF3REG;
        dwFlagsAndAttributes = FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_OVERLAPPED;
    }
    else if (fileAttr & FILE_ATTRIBUTE_NORMAL) {
        pAttr->type = NF3REG;
        dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
    }
    else {
        pAttr->type = 0;
    }

    if (fileAttr & FILE_ATTRIBUTE_REPARSE_POINT) {
        pAttr->type = NF3LNK;
		dwFlagsAndAttributes = FILE_ATTRIBUTE_REPARSE_POINT | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS;
    }

	hFile = CreateFile(path, FILE_READ_EA, FILE_SHARE_READ, NULL, OPEN_EXISTING, dwFlagsAndAttributes, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    GetFileInformationByHandle(hFile, &lpFileInformation);
	CloseHandle(hFile);
    pAttr->mode = 0;

	// Set execution right for all
    pAttr->mode |= 0x49;

    // Set read right for all
    pAttr->mode |= 0x124;

    //if ((lpFileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0) {
        pAttr->mode |= 0x92;
    //}

    ULONGLONG fileSize = lpFileInformation.nFileSizeHigh;
    fileSize <<= sizeof(lpFileInformation.nFileSizeHigh) * 8;
    fileSize |= lpFileInformation.nFileSizeLow;

    pAttr->nlink = lpFileInformation.nNumberOfLinks;
    pAttr->uid = m_nUID;
    pAttr->gid = m_nGID;
    pAttr->size = fileSize;
    pAttr->used = pAttr->size;
    pAttr->rdev.specdata1 = 0;
    pAttr->rdev.specdata2 = 0;
    pAttr->fsid = 7; //NTFS //4; 
    pAttr->fileid = ((uint64) lpFileInformation.nFileIndexHigh << 32) | lpFileInformation.nFileIndexLow;
    pAttr->atime.seconds = FileTimeToPOSIX(lpFileInformation.ftLastAccessTime);
    pAttr->atime.nseconds = 0;
    pAttr->mtime.seconds = FileTimeToPOSIX(lpFileInformation.ftLastWriteTime);
    pAttr->mtime.nseconds = 0;
	// This seems to be the changed time, not creation time
	pAttr->ctime.seconds = FileTimeToPOSIX(lpFileInformation.ftLastWriteTime);
    pAttr->ctime.nseconds = 0;

    return true;
}

UINT32 CNFS3Prog::FileTimeToPOSIX(FILETIME ft)
{
    // takes the last modified date
    LARGE_INTEGER date, adjust;
    date.HighPart = ft.dwHighDateTime;
    date.LowPart = ft.dwLowDateTime;

    // 100-nanoseconds = milliseconds * 10000
    adjust.QuadPart = 11644473600000 * 10000;

    // removes the diff between 1970 and 1601
    date.QuadPart -= adjust.QuadPart;

    // converts back from 100-nanoseconds to seconds
    return (unsigned int)(date.QuadPart / 10000000);
}
