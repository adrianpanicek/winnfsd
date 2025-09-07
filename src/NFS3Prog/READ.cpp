#include <windows.h>

#include "../NFSProg.h"

#define ERROR_BUFFER_SIZE 1000

bool load_data(opaque &data, bool &eof, count3 &dataLength, offset3 offset, count3 readLength, std::string path)
{
    FILE *pFile = _fsopen(path.c_str(), "rb", _SH_DENYWR);

    if (pFile == NULL) {
        return false;
    }

    _fseeki64(pFile, offset, SEEK_SET) ;
    dataLength = (count3) fread(data.contents, sizeof(char), readLength, pFile);
    data.SetSizeNoRealloc(dataLength);
    eof = fgetc(pFile) == EOF;

    fclose(pFile);

    return eof;
}

nfsstat3 CNFS3Prog::ProcedureREAD(void)
{
    PrintLog("READ");

    std::string path;
    offset3 offset;
    count3 readLength;
    count3 dataLength;
    post_op_attr file_attributes;
    bool eof;
    opaque data;
    nfsstat3 stat;

    bool validHandle = GetPath(path);
    const char* cStr = validHandle ? path.c_str() : NULL;
    Read(&offset);
    Read(&readLength);
    stat = CheckFile(cStr);

    file_attributes.attributes_follow = GetFileAttributesForNFS(cStr, &file_attributes.attributes);

    if (stat != NFS3_OK) {
        Write(&stat);
        Write(&file_attributes);

        return stat;
    }

    data.SetSize(readLength);
    if (!load_data(data, eof, dataLength, offset, readLength, path)) {
        char buffer[ERROR_BUFFER_SIZE];
        errno_t errorNumber = errno;
        strerror_s(buffer, ERROR_BUFFER_SIZE, errorNumber);
        PrintLog(buffer);

        stat = errorNumber == 13 ? NFS3ERR_ACCES : NFS3ERR_IO;

        Write(&stat);
        Write(&file_attributes);

        return stat;
    }

    Write(&stat);
    Write(&file_attributes);
    Write(&dataLength);
    Write(&eof);
    Write(&data);

    return stat;
}