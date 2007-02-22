/*
 * CHM Utility API
 *
 * Copyright 2005 James Hawkins
 * Copyright 2007 Jacek Caban
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

#include "hhctrl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(htmlhelp);

#define BLOCK_BITS 12
#define BLOCK_SIZE (1 << BLOCK_BITS)
#define BLOCK_MASK (BLOCK_SIZE-1)

/* Reads a string from the #STRINGS section in the CHM file */
static LPCSTR GetChmString(CHMInfo *chm, DWORD offset)
{
    if(!chm->strings_stream)
        return NULL;

    if(chm->strings_size <= (offset >> BLOCK_BITS)) {
        if(chm->strings)
            chm->strings = hhctrl_realloc_zero(chm->strings,
                    chm->strings_size = ((offset >> BLOCK_BITS)+1)*sizeof(char*));
        else
            chm->strings = hhctrl_alloc_zero(
                    chm->strings_size = ((offset >> BLOCK_BITS)+1)*sizeof(char*));

    }

    if(!chm->strings[offset >> BLOCK_BITS]) {
        LARGE_INTEGER pos;
        DWORD read;
        HRESULT hres;

        pos.QuadPart = offset & ~BLOCK_MASK;
        hres = IStream_Seek(chm->strings_stream, pos, STREAM_SEEK_SET, NULL);
        if(FAILED(hres)) {
            WARN("Seek failed: %08x\n", hres);
            return NULL;
        }

        chm->strings[offset >> BLOCK_BITS] = hhctrl_alloc(BLOCK_SIZE);

        hres = IStream_Read(chm->strings_stream, chm->strings[offset >> BLOCK_BITS],
                            BLOCK_SIZE, &read);
        if(FAILED(hres)) {
            WARN("Read failed: %08x\n", hres);
            hhctrl_free(chm->strings[offset >> BLOCK_BITS]);
            chm->strings[offset >> BLOCK_BITS] = NULL;
            return NULL;
        }
    }

    return chm->strings[offset >> BLOCK_BITS] + (offset & BLOCK_MASK);
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
    pHHWinType->pszType = strdupAtoW(GetChmString(pChmInfo, (DWORD)pHHWinType->pszType));
    pHHWinType->pszCaption = strdupAtoW(GetChmString(pChmInfo, (DWORD)pHHWinType->pszCaption));
    pHHWinType->pszToc = strdupAtoW(GetChmString(pChmInfo, (DWORD)pHHWinType->pszToc));
    pHHWinType->pszIndex = strdupAtoW(GetChmString(pChmInfo, (DWORD)pHHWinType->pszIndex));
    pHHWinType->pszFile = strdupAtoW(GetChmString(pChmInfo, (DWORD)pHHWinType->pszFile));
    pHHWinType->pszHome = strdupAtoW(GetChmString(pChmInfo, (DWORD)pHHWinType->pszHome));
    pHHWinType->pszJump1 = strdupAtoW(GetChmString(pChmInfo, (DWORD)pHHWinType->pszJump1));
    pHHWinType->pszJump2 = strdupAtoW(GetChmString(pChmInfo, (DWORD)pHHWinType->pszJump2));
    pHHWinType->pszUrlJump1 = strdupAtoW(GetChmString(pChmInfo, (DWORD)pHHWinType->pszUrlJump1));
    pHHWinType->pszUrlJump2 = strdupAtoW(GetChmString(pChmInfo, (DWORD)pHHWinType->pszUrlJump2));
    
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
    HRESULT hres;

    static const WCHAR wszSTRINGS[] = {'#','S','T','R','I','N','G','S',0};

    pChmInfo->szFile = szFile;

    if (FAILED(CoCreateInstance(&CLSID_ITStorage, NULL, CLSCTX_INPROC_SERVER,
                                &IID_IITStorage, (void **) &pChmInfo->pITStorage)))
        return FALSE;

    if (FAILED(IITStorage_StgOpenStorage(pChmInfo->pITStorage, szFile, NULL,
                                         STGM_READ | STGM_SHARE_DENY_WRITE,
                                         NULL, 0, &pChmInfo->pStorage)))
        return FALSE;

    hres = IStorage_OpenStream(pChmInfo->pStorage, wszSTRINGS, NULL, STGM_READ, 0,
                               &pChmInfo->strings_stream);
    if(FAILED(hres)) {
        WARN("Could not open #STRINGS stream: %08x\n", hres);
        return FALSE;
    }

    pChmInfo->strings = NULL;
    pChmInfo->strings_size = 0;

    return TRUE;
}

void CHM_CloseCHM(CHMInfo *pCHMInfo)
{
    IITStorage_Release(pCHMInfo->pITStorage);
    IStorage_Release(pCHMInfo->pStorage);
    IStream_Release(pCHMInfo->strings_stream);

    if(pCHMInfo->strings_size) {
        int i;

        for(i=0; i<pCHMInfo->strings_size; i++)
            hhctrl_free(pCHMInfo->strings[i]);
    }

    hhctrl_free(pCHMInfo->strings);
}
