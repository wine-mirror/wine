/*
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

#ifndef HHCTRL_H
#define HHCTRL_H

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "htmlhelp.h"
#include "ole2.h"
#include "exdisp.h"
#include "mshtmhst.h"

#ifdef INIT_GUID
#include "initguid.h"
#endif

#include "wine/itss.h"

#define WB_GOBACK     0
#define WB_GOFORWARD  1
#define WB_GOHOME     2
#define WB_SEARCH     3
#define WB_REFRESH    4
#define WB_STOP       5

typedef struct CHMInfo
{
    IITStorage *pITStorage;
    IStorage *pStorage;
    LPCWSTR szFile;
} CHMInfo;


typedef struct WBInfo
{
    IOleClientSite *pOleClientSite;
    IWebBrowser2 *pWebBrowser2;
    IOleObject *pBrowserObject;
    HWND hwndParent;
} WBInfo;

BOOL WB_EmbedBrowser(WBInfo *pWBInfo, HWND hwndParent);
void WB_UnEmbedBrowser(WBInfo *pWBInfo);
void WB_ResizeBrowser(WBInfo *pWBInfo, DWORD dwWidth, DWORD dwHeight);
void WB_DoPageAction(WBInfo *pWBInfo, DWORD dwAction);

BOOL CHM_OpenCHM(CHMInfo *pCHMInfo, LPCWSTR szFile);
BOOL CHM_LoadWinTypeFromCHM(CHMInfo *pCHMInfo, HH_WINTYPEW *pHHWinType);
void CHM_CloseCHM(CHMInfo *pCHMInfo);

/* memory allocation functions */

static inline void *hhctrl_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *hhctrl_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void *hhctrl_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline BOOL hhctrl_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR strdupAtoW(LPCSTR str)
{
    LPWSTR ret;
    DWORD len;

    if(!str)
        return NULL;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = hhctrl_alloc(len*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

#endif
