/*
 * CHM Utility API
 *
 * Copyright 2005 James Hawkins
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "htmlhelp.h"

#include "initguid.h"
#include "chm.h"

static LPWSTR CHM_ANSIToUnicode(LPCSTR ansi)
{
    LPWSTR unicode;
    int count;

    count = MultiByteToWideChar(CP_ACP, 0, ansi, -1, NULL, 0);
    unicode = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ansi, -1, unicode, count);

    return unicode;
}

/* Reads a string from the #STRINGS section in the CHM file */
static LPWSTR CHM_ReadString(CHMInfo *pChmInfo, DWORD dwOffset)
{
    LARGE_INTEGER liOffset;
    IStorage *pStorage = pChmInfo->pStorage;
    IStream *pStream;
    DWORD cbRead;
    ULONG iPos;
    DWORD dwSize;
    LPSTR szString;
    LPWSTR stringW;
    
    const int CB_READ_BLOCK = 64;
    static const WCHAR stringsW[] = {'#','S','T','R','I','N','G','S',0};

    dwSize = CB_READ_BLOCK;
    szString = HeapAlloc(GetProcessHeap(), 0, dwSize);

    if (FAILED(IStorage_OpenStream(pStorage, stringsW, NULL, STGM_READ, 0, &pStream)))
        return NULL;

    liOffset.QuadPart = dwOffset;
    
    if (FAILED(IStream_Seek(pStream, liOffset, STREAM_SEEK_SET, NULL)))
    {
        IStream_Release(pStream);
        return NULL;
    }

    while (SUCCEEDED(IStream_Read(pStream, szString, CB_READ_BLOCK, &cbRead)))
    {
        if (!cbRead)
            return NULL;

        for (iPos = 0; iPos < cbRead; iPos++)
        {
            if (!szString[iPos])
            {
                stringW = CHM_ANSIToUnicode(szString);
                HeapFree(GetProcessHeap(), 0, szString);
                return stringW;
            }
        }

        dwSize *= 2;
        szString = HeapReAlloc(GetProcessHeap(), 0, szString, dwSize);
        szString += cbRead;
    }

    /* didn't find a string */
    return NULL;
}

/* Loads the HH_WINTYPE data from the CHM file
 *
 * FIXME: There may be more than one window type in the file, so
 *        add the ability to choose a certain window type
 */
BOOL CHM_LoadWinTypeFromCHM(CHMInfo *pChmInfo, HH_WINTYPEW *pHHWinType)
{
    LARGE_INTEGER liOffset;
    IStorage *pStorage = pChmInfo->pStorage;
    IStream *pStream;
    HRESULT hr;
    DWORD cbRead;

    static const WCHAR windowsW[] = {'#','W','I','N','D','O','W','S',0};

    hr = IStorage_OpenStream(pStorage, windowsW, NULL, STGM_READ, 0, &pStream);
    if (FAILED(hr))
        return FALSE;

    /* jump past the #WINDOWS header */
    liOffset.QuadPart = sizeof(DWORD) * 2;

    hr = IStream_Seek(pStream, liOffset, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto done;

    /* read the HH_WINTYPE struct data */
    hr = IStream_Read(pStream, pHHWinType, sizeof(*pHHWinType), &cbRead);
    if (FAILED(hr)) goto done;

    /* convert the #STRINGS offsets to actual strings */
    pHHWinType->pszType = CHM_ReadString(pChmInfo, (DWORD)pHHWinType->pszType);
    pHHWinType->pszCaption = CHM_ReadString(pChmInfo, (DWORD)pHHWinType->pszCaption);
    pHHWinType->pszToc = CHM_ReadString(pChmInfo, (DWORD)pHHWinType->pszToc);
    pHHWinType->pszIndex = CHM_ReadString(pChmInfo, (DWORD)pHHWinType->pszIndex);
    pHHWinType->pszFile = CHM_ReadString(pChmInfo, (DWORD)pHHWinType->pszFile);
    pHHWinType->pszHome = CHM_ReadString(pChmInfo, (DWORD)pHHWinType->pszHome);
    pHHWinType->pszJump1 = CHM_ReadString(pChmInfo, (DWORD)pHHWinType->pszJump1);
    pHHWinType->pszJump2 = CHM_ReadString(pChmInfo, (DWORD)pHHWinType->pszJump2);
    pHHWinType->pszUrlJump1 = CHM_ReadString(pChmInfo, (DWORD)pHHWinType->pszUrlJump1);
    pHHWinType->pszUrlJump2 = CHM_ReadString(pChmInfo, (DWORD)pHHWinType->pszUrlJump2);
    
    /* FIXME: pszCustomTabs is a list of multiple zero-terminated strings so ReadString won't
     * work in this case
     */
#if 0
    pHHWinType->pszCustomTabs = CHM_ReadString(pChmInfo, (DWORD)pHHWinType->pszCustomTabs);
#endif

done:
    IStream_Release(pStream);

    return SUCCEEDED(hr);
}

/* Opens the CHM file for reading */
BOOL CHM_OpenCHM(CHMInfo *pChmInfo, LPCWSTR szFile)
{
    pChmInfo->szFile = szFile;

    if (FAILED(CoCreateInstance(&CLSID_ITStorage, NULL, CLSCTX_INPROC_SERVER,
                                &IID_IITStorage, (void **) &pChmInfo->pITStorage)))
        return FALSE;

    if (FAILED(IITStorage_StgOpenStorage(pChmInfo->pITStorage, szFile, NULL,
                                         STGM_READ | STGM_SHARE_DENY_WRITE,
                                         NULL, 0, &pChmInfo->pStorage)))
        return FALSE;

    return TRUE;
}

void CHM_CloseCHM(CHMInfo *pCHMInfo)
{
    IITStorage_Release(pCHMInfo->pITStorage);
    IStorage_Release(pCHMInfo->pStorage);
}

/* Creates a Url of a CHM file that can be used with WB_Navigate */
void CHM_CreateITSUrl(CHMInfo *pChmInfo, LPCWSTR szIndex, LPWSTR szUrl)
{
    static const WCHAR formatW[] = {
        'i','t','s',':','%','s',':',':','%','s',0
    };

    wsprintfW(szUrl, formatW, pChmInfo->szFile, szIndex);
}
