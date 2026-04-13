#include "../NFSProg.h"

nfsstat3 CNFS3Prog::ProcedureNOIMP(void)
{
    PrintLog("NOIMP");
    m_nResult = PRC_NOTIMP;

    return NFS3_OK;
}
