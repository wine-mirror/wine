/*
 *    IShellExtInit
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

#ifndef __WINE_WINE_OBJ_SHELLEXTINIT_H
#define __WINE_WINE_OBJ_SHELLEXTINIT_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef struct 	IShellExtInit IShellExtInit, *LPSHELLEXTINIT;

#define INTERFACE IShellExtInit
#define IShellExtInit_METHODS \
	STDMETHOD(Initialize)(THIS_ LPCITEMIDLIST  pidlFolder, LPDATAOBJECT  lpdobj, HKEY  hkeyProgID) PURE;
#define IShellExtInit_IMETHODS \
	IUnknown_IMETHODS \
	IShellExtInit_METHODS
ICOM_DEFINE(IShellExtInit,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IShellExtInit_QueryInterface(p,a,b)     (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellExtInit_AddRef(p)                 (p)->lpVtbl->AddRef(p)
#define IShellExtInit_Release(p)                (p)->lpVtbl->Release(p)
/*** IShellExtInit methods ***/
#define IShellExtInit_Initialize(p,a,b,c)       (p)->lpVtbl->Initialize(p,a,b,c)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_SHELLEXTINIT_H */
