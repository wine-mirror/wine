/*
 * SetupX .inf file parsing functions
 *
 * FIXME: return values ???
 */

#include "debugtools.h"
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "setupx16.h"

DEFAULT_DEBUG_CHANNEL(setupx);

WORD InfNumEntries = 0;
INF_HANDLE *InfList = NULL;

#define GET_INF_ENTRY(x) ((InfList - x)/4)

RETERR16 IP_OpenInf(LPCSTR lpInfFileName, HINF16 *lphInf)
{
    HFILE16 hFile = _lopen16(lpInfFileName, OF_READ);

    if (!lphInf)
	return IP_ERROR;

    if (hFile != HFILE_ERROR16)
    {
	InfList = HeapReAlloc(GetProcessHeap(), 0, InfList, InfNumEntries+1);
	InfList[InfNumEntries].hInfFile = hFile;
	InfList[InfNumEntries].lpInfFileName = lpInfFileName;
	InfNumEntries++;
        *lphInf = &InfList[InfNumEntries-1] - InfList;
	return OK;
    }
    *lphInf = 0xffff;
    return ERR_IP_INVALID_INFFILE;
}

LPCSTR IP_GetFileName(HINF16 hInf)
{
    if ((hInf <= (InfNumEntries*sizeof(INF_HANDLE *)))
    && ((hInf & 3) == 0)) /* aligned ? */
    {
	return InfList[hInf/4].lpInfFileName;
    }
    return NULL;
}

RETERR16 IP_CloseInf(HINF16 hInf)
{
    int i;
    HFILE16 res = ERR_IP_INVALID_HINF;

    if ((hInf <= (InfNumEntries*sizeof(INF_HANDLE *)))
    && ((hInf & 3) == 0)) /* aligned ? */
    {
	_lclose16(InfList[hInf/4].hInfFile);
	res = OK;
	for (i=hInf/4; i < InfNumEntries-1; i++)
	    InfList[i] = InfList[i+1];
	InfNumEntries--;
	InfList = HeapReAlloc(GetProcessHeap(), 0, InfList, InfNumEntries);
    }
    return res;
}

/***********************************************************************
 *		IpOpen16
 *
 */
RETERR16 WINAPI IpOpen16(LPCSTR lpInfFileName, HINF16 *lphInf)
{
    TRACE("('%s', %p)\n", lpInfFileName, lphInf);
    return IP_OpenInf(lpInfFileName, lphInf);
}

/***********************************************************************
 *		IpClose16
 */
RETERR16 WINAPI IpClose16(HINF16 hInf)
{
    return IP_CloseInf(hInf);
}

/***********************************************************************
 *		IpGetProfileString16
 */
RETERR16 WINAPI IpGetProfileString16(HINF16 hInf, LPCSTR section, LPCSTR entry, LPSTR buffer, WORD buflen) 
{
    GetPrivateProfileString16(section, entry, "", buffer, buflen, IP_GetFileName(hInf));
    return 0;
}
