/*
 * Defines the COM interfaces and APIs related to IShellFolder
 *
 * Depends on 'obj_base.h'.
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

#ifndef __WINE_WINE_OBJ_SHELLFOLDER_H
#define __WINE_WINE_OBJ_SHELLFOLDER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IPersistFolder3, 0xcef04fdf, 0xfe72, 0x11d2, 0x87, 0xa5, 0x0, 0xc0, 0x4f, 0x68, 0x37, 0xcf);
typedef struct IPersistFolder3 IPersistFolder3, *LPPERSISTFOLDER3;

/* GetDetailsEx */
#define PID_FINDDATA		0
#define PID_NETRESOURCE		1
#define PID_DESCRIPTIONID	2


/*****************************************************************************
 * IPersistFolder3 interface
 */

typedef struct {
	LPITEMIDLIST	pidlTargetFolder;
	WCHAR		szTargetParsingName[MAX_PATH];
	WCHAR		szNetworkProvider[MAX_PATH];
	DWORD		dwAttributes;
	int		csidl;
} PERSIST_FOLDER_TARGET_INFO;

#define INTERFACE IPersistFolder3
#define IPersistFolder3_METHODS \
    IPersistFolder2_METHODS \
    STDMETHOD(InitializeEx)(THIS_ IBindCtx * pbc, LPCITEMIDLIST  pidlRoot, const PERSIST_FOLDER_TARGET_INFO * ppfti) PURE;\
    STDMETHOD(GetFolderTargetInfo)(THIS_ PERSIST_FOLDER_TARGET_INFO * ppfti) PURE;
ICOM_DEFINE(IPersistFolder3, IPersistFolder2)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IPersistFolder3_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IPersistFolder3_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define IPersistFolder3_Release(p)              (p)->lpVtbl->Release(p)
/*** IPersist methods ***/
#define IPersistFolder3_GetClassID(p,a)         (p)->lpVtbl->GetClassID(p,a)
/*** IPersistFolder methods ***/
#define IPersistFolder3_Initialize(p,a)         (p)->lpVtbl->Initialize(p,a)
/*** IPersistFolder2 methods ***/
#define IPersistFolder3_GetCurFolder(p,a)       (p)->lpVtbl->GetCurFolder(p,a)
/*** IPersistFolder3 methods ***/
#define IPersistFolder3_InitializeEx(p,a,b,c)   (p)->lpVtbl->InitializeEx(p,a,b,c)
#define IPersistFolder3_GetFolderTargetInfo(p,a) (p)->lpVtbl->InitializeEx(p,a)
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_SHELLFOLDER_H */
