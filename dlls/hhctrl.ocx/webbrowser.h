/*
 * WebBrowser Include
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

#ifndef __WINE_WEBBROWSER_H
#define __WINE_WEBBROWSER_H

#include "exdisp.h"
#include "mshtml.h"
#include "mshtmhst.h"

#define WB_GOBACK       0
#define WB_GOFORWARD    1
#define WB_GOHOME       2
#define WB_SEARCH       3
#define WB_REFRESH      4
#define WB_STOP         5

typedef struct WBInfo
{
    IOleClientSite *pOleClientSite;
    IWebBrowser2 *pWebBrowser2;
    IOleObject *pBrowserObject;
    HWND hwndParent;
} WBInfo;

BOOL WB_EmbedBrowser(WBInfo *pWBInfo, HWND hwndParent);
void WB_UnEmbedBrowser(WBInfo *pWBInfo);
BOOL WB_Navigate(WBInfo *pWBInfo, LPCWSTR szUrl);
void WB_ResizeBrowser(WBInfo *pWBInfo, DWORD dwWidth, DWORD dwHeight);
void WB_DoPageAction(WBInfo *pWBInfo, DWORD dwAction);

#endif /* __WINE_WEBBROWSER_H */
