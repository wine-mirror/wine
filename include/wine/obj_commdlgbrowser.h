/*
 *    ICommDlgBrowser
 *
 * Copyright (C) 1999 Juergen Schmied
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINE_OBJ_COMMDLGBROWSER_H
#define __WINE_WINE_OBJ_COMMDLGBROWSER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef struct 	ICommDlgBrowser ICommDlgBrowser, *LPCOMMDLGBROWSER;

/* for OnStateChange*/
#define CDBOSC_SETFOCUS     0x00000000
#define CDBOSC_KILLFOCUS    0x00000001
#define CDBOSC_SELCHANGE    0x00000002
#define CDBOSC_RENAME       0x00000003


#define INTERFACE ICommDlgBrowser
#define ICommDlgBrowser_METHODS \
	IUnknown_METHODS \
	STDMETHOD(OnDefaultCommand)(THIS_ IShellView * IShellView) PURE; \
	STDMETHOD(OnStateChange)(THIS_ IShellView * IShellView, ULONG  uChange) PURE; \
	STDMETHOD(IncludeObject)(THIS_ IShellView * IShellView, LPCITEMIDLIST  pidl) PURE;
ICOM_DEFINE(ICommDlgBrowser,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
#define ICommDlgBrowser_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define ICommDlgBrowser_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define ICommDlgBrowser_Release(p)              (p)->lpVtbl->Release(p)
#define ICommDlgBrowser_OnDefaultCommand(p,a)   (p)->lpVtbl->OnDefaultCommand(p,a)
#define ICommDlgBrowser_OnStateChange(p,a,b)    (p)->lpVtbl->OnStateChange(p,a,b)
#define ICommDlgBrowser_IncludeObject(p,a,b)    (p)->lpVtbl->IncludeObject(p,a,b)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_COMMDLGBROWSER_H */
