/*
 * SetupX .inf file parsing functions
 *
 * FIXME:
 * - return values ???
 * - this should be reimplemented at some point to have its own
 *   file parsing instead of using profile functions,
 *   as some SETUPX exports probably demand that
 *   (IpSaveRestorePosition, IpFindNextMatchLine, ...).
 */

#include "debugtools.h"
#include "windef.h"
#include "winbase.h"
#include "heap.h"
#include "wine/winbase16.h"
#include "setupx16.h"
#include "setupx_private.h"

DEFAULT_DEBUG_CHANNEL(setupx);

WORD InfNumEntries = 0;
INF_FILE *InfList = NULL;
HINF16 IP_curr_handle = 0;

RETERR16 IP_OpenInf(LPCSTR lpInfFileName, HINF16 *lphInf)
{
    HFILE hFile = _lopen(lpInfFileName, OF_READ);

    if (!lphInf)
	return IP_ERROR;

    /* this could be improved by checking for already freed handles */
    if (IP_curr_handle == 0xffff) 
	return ERR_IP_OUT_OF_HANDLES; 

    if (hFile != HFILE_ERROR)
    {
	InfList = HeapReAlloc(GetProcessHeap(), 0, InfList, InfNumEntries+1);
	InfList[InfNumEntries].hInf = IP_curr_handle++;
	InfList[InfNumEntries].hInfFile = hFile;
	InfList[InfNumEntries].lpInfFileName = HeapAlloc( GetProcessHeap(), 0,
                                                          strlen(lpInfFileName)+1);
        strcpy( InfList[InfNumEntries].lpInfFileName, lpInfFileName );
        *lphInf = InfList[InfNumEntries].hInf;
	InfNumEntries++;
	TRACE("ret handle %d.\n", *lphInf);
	return OK;
    }
    *lphInf = 0xffff;
    return ERR_IP_INVALID_INFFILE;
}

BOOL IP_FindInf(HINF16 hInf, WORD *ret)
{
    WORD n;

    for (n=0; n < InfNumEntries; n++)
	if (InfList[n].hInf == hInf)
	{
	    *ret = n;
	    return TRUE;
	}
    return FALSE;
}
	

LPCSTR IP_GetFileName(HINF16 hInf)
{
    WORD n;
    if (IP_FindInf(hInf, &n))
    {
	return InfList[n].lpInfFileName;
    }
    return NULL;
}

RETERR16 IP_CloseInf(HINF16 hInf)
{
    int i;
    WORD n;
    RETERR16 res = ERR_IP_INVALID_HINF;

    if (IP_FindInf(hInf, &n))
    {
	_lclose(InfList[n].hInfFile);
	HeapFree(GetProcessHeap(), 0, InfList[n].lpInfFileName);
	for (i=n; i < InfNumEntries-1; i++)
	    InfList[i] = InfList[i+1];
	InfNumEntries--;
	InfList = HeapReAlloc(GetProcessHeap(), 0, InfList, InfNumEntries);
	res = OK;
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
    TRACE("'%s': section '%s' entry '%s'\n", IP_GetFileName(hInf), section, entry);
    GetPrivateProfileStringA(section, entry, "", buffer, buflen, IP_GetFileName(hInf));
    return 0;
}
