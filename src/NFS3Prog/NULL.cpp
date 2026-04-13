#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedureNULL(void)
{
    PrintLog("NULL");
    return NFS3_OK;
}
