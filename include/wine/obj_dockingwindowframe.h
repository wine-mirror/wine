/*
 *    IDockingWindowFrame
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

#ifndef __WINE_WINE_OBJ_DOCKINGWINDOWFRAME_H
#define __WINE_WINE_OBJ_DOCKINGWINDOWFRAME_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef struct IDockingWindowFrame IDockingWindowFrame,	*LPDOCKINGWINDOWFRAME;
DEFINE_GUID (IID_IDockingWindowFrame,	0x47D2657AL, 0x7B27, 0x11D0, 0x8C, 0xA9, 0x00, 0xA0, 0xC9, 0x2D, 0xBF, 0xE8);

#define DWFRF_NORMAL		0x0000  /* femove toolbar flags*/
#define DWFRF_DELETECONFIGDATA	0x0001
#define DWFAF_HIDDEN		0x0001   /* add tolbar*/

#define INTERFACE IDockingWindowFrame
#define IDockingWindowFrame_METHODS \
	STDMETHOD(AddToolbar)(THIS_ IUnknown * punkSrc, LPCWSTR  pwszItem, DWORD  dwAddFlags) PURE; \
	STDMETHOD(RemoveToolbar)(THIS_ IUnknown * punkSrc, DWORD  dwRemoveFlags) PURE; \
	STDMETHOD(FindToolbar)(THIS_ LPCWSTR  pwszItem, REFIID  riid, LPVOID * ppvObj) PURE;
#define IDockingWindowFrame_IMETHODS \
	IOleWindow_IMETHODS \
	IDockingWindowFrame_METHODS
ICOM_DEFINE(IDockingWindowFrame,IOleWindow)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDockingWindowFrame_QueryInterface(p,a,b)	(p)->lpVtbl->QueryInterface(p,a,b)
#define IDockingWindowFrame_AddRef(p)	(p)->lpVtbl->AddRef(p)
#define IDockingWindowFrame_Release(p)	(p)->lpVtbl->Release(p)
/*** IDockingWindowFrame methods ***/
#define IDockingWindowFrame_GetWindow(p,a)	(p)->lpVtbl->GetWindow(p,a)
#define IDockingWindowFrame_ContextSensitiveHelp(p,a)	(p)->lpVtbl->ContextSensitiveHelp(p,a)
#define IDockingWindowFrame_AddToolbar(p,a,b,c)	(p)->lpVtbl->AddToolbar(p,a,b,c)
#define IDockingWindowFrame_RemoveToolbar(p,a,b)	(p)->lpVtbl->RemoveToolbar(p,a,b)
#define IDockingWindowFrame_FindToolbar(p,a,b,c)	(p)->lpVtbl->FindToolbar(p,a,b,c)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_DOCKINGWINDOWFRAME_H */
