/*
 *    IShellBrowser
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

#ifndef __WINE_WINE_OBJ_SHELLBROWSER_H
#define __WINE_WINE_OBJ_SHELLBROWSER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/* it's ok commented out, see obj_shellview.h
   typedef struct IShellBrowser IShellBrowser, *LPSHELLBROWSER;
*/

#define SID_SShellBrowser IID_IShellBrowser

DEFINE_GUID(SID_STopLevelBrowser, 0x4C96BE40L, 0x915C, 0x11CF, 0x99, 0xD3, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37);

/* targets for GetWindow/SendControlMsg */
#define FCW_STATUS		0x0001
#define FCW_TOOLBAR		0x0002
#define FCW_TREE		0x0003
#define FCW_INTERNETBAR		0x0006
#define FCW_PROGRESS		0x0008

/* wFlags for BrowseObject*/
#define SBSP_DEFBROWSER		0x0000
#define SBSP_SAMEBROWSER	0x0001
#define SBSP_NEWBROWSER		0x0002

#define SBSP_DEFMODE		0x0000
#define SBSP_OPENMODE		0x0010
#define SBSP_EXPLOREMODE	0x0020

#define SBSP_ABSOLUTE		0x0000
#define SBSP_RELATIVE		0x1000
#define SBSP_PARENT		0x2000
#define SBSP_NAVIGATEBACK	0x4000
#define SBSP_NAVIGATEFORWARD	0x8000

#define SBSP_ALLOW_AUTONAVIGATE		0x10000

#define SBSP_INITIATEDBYHLINKFRAME	0x80000000
#define SBSP_REDIRECT			0x40000000
#define SBSP_WRITENOHISTORY		0x08000000

/* uFlage for SetToolbarItems */
#define FCT_MERGE       0x0001
#define FCT_CONFIGABLE  0x0002
#define FCT_ADDTOEND    0x0004

#define INTERFACE IShellBrowser
#define IShellBrowser_METHODS \
	STDMETHOD(InsertMenusSB)(THIS_ HMENU  hmenuShared, LPOLEMENUGROUPWIDTHS  lpMenuWidths) PURE; \
	STDMETHOD(SetMenuSB)(THIS_ HMENU  hmenuShared, HOLEMENU  holemenuReserved, HWND  hwndActiveObject) PURE; \
	STDMETHOD(RemoveMenusSB)(THIS_ HMENU  hmenuShared) PURE; \
	STDMETHOD(SetStatusTextSB)(THIS_ LPCOLESTR  lpszStatusText) PURE; \
	STDMETHOD(EnableModelessSB)(THIS_ BOOL  fEnable) PURE; \
	STDMETHOD(TranslateAcceleratorSB)(THIS_ LPMSG  lpmsg, WORD  wID) PURE; \
	STDMETHOD(BrowseObject)(THIS_ LPCITEMIDLIST  pidl, UINT  wFlags) PURE; \
	STDMETHOD(GetViewStateStream)(THIS_ DWORD  grfMode, LPSTREAM * ppStrm) PURE; \
	STDMETHOD(GetControlWindow)(THIS_ UINT  id, HWND * lphwnd) PURE; \
	STDMETHOD(SendControlMsg)(THIS_ UINT  id, UINT  uMsg, WPARAM  wParam, LPARAM  lParam, LRESULT * pret) PURE; \
	STDMETHOD(QueryActiveShellView)(THIS_ IShellView ** IShellView) PURE; \
	STDMETHOD(OnViewWindowActive)(THIS_ IShellView * IShellView) PURE; \
	STDMETHOD(SetToolbarItems)(THIS_ LPTBBUTTON  lpButtons, UINT  nButtons, UINT  uFlags) PURE;
#define IShellBrowser_IMETHODS \
	IOleWindow_IMETHODS \
	IShellBrowser_METHODS
ICOM_DEFINE(IShellBrowser,IOleWindow)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IShellBrowser_QueryInterface(p,a,b)             (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellBrowser_AddRef(p)                         (p)->lpVtbl->AddRef(p)
#define IShellBrowser_Release(p)                        (p)->lpVtbl->Release(p)
/*** IShellBrowser methods ***/
#define IShellBrowser_GetWindow(p,a)                    (p)->lpVtbl->GetWindow(p,a)
#define IShellBrowser_ContextSensitiveHelp(p,a)         (p)->lpVtbl->ContextSensitiveHelp(p,a)
#define IShellBrowser_InsertMenusSB(p,a,b)              (p)->lpVtbl->InsertMenusSB(p,a,b)
#define IShellBrowser_SetMenuSB(p,a,b,c)                (p)->lpVtbl->SetMenuSB(p,a,b,c)
#define IShellBrowser_RemoveMenusSB(p,a)                (p)->lpVtbl->RemoveMenusSB(p,a)
#define IShellBrowser_SetStatusTextSB(p,a)              (p)->lpVtbl->SetStatusTextSB(p,a)
#define IShellBrowser_EnableModelessSB(p,a)             (p)->lpVtbl->EnableModelessSB(p,a)
#define IShellBrowser_TranslateAcceleratorSB(p,a,b)     (p)->lpVtbl->TranslateAcceleratorSB(p,a,b)
#define IShellBrowser_BrowseObject(p,a,b)               (p)->lpVtbl->BrowseObject(p,a,b)
#define IShellBrowser_GetViewStateStream(p,a,b)         (p)->lpVtbl->GetViewStateStream(p,a,b)
#define IShellBrowser_GetControlWindow(p,a,b)           (p)->lpVtbl->GetControlWindow(p,a,b)
#define IShellBrowser_SendControlMsg(p,a,b,c,d,e)       (p)->lpVtbl->SendControlMsg(p,a,b,c,d,e)
#define IShellBrowser_QueryActiveShellView(p,a)         (p)->lpVtbl->QueryActiveShellView(p,a)
#define IShellBrowser_OnViewWindowActive(p,a)           (p)->lpVtbl->OnViewWindowActive(p,a)
#define IShellBrowser_SetToolbarItems(p,a,b,c)          (p)->lpVtbl->SetToolbarItems(p,a,b,c)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_SHELLBROWSER_H */
