/*
 *    IExtractIconA, IExtractIconW
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

#ifndef __WINE_WINE_OBJ_EXTRACTICON_H
#define __WINE_WINE_OBJ_EXTRACTICON_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#define IID_IExtractIcon WINELIB_NAME_AW(IID_IExtractIcon)

typedef struct IExtractIconA IExtractIconA,*LPEXTRACTICONA;
typedef struct IExtractIconW IExtractIconW,*LPEXTRACTICONW;
#define LPEXTRACTICON WINELIB_NAME_AW(LPEXTRACTICON)

/* GetIconLocation() input flags*/
#define GIL_OPENICON     0x0001      /* allows containers to specify an "open" look */
#define GIL_FORSHELL     0x0002      /* icon is to be displayed in a ShellFolder */
#define GIL_ASYNC        0x0020      /* this is an async extract, return E_ASYNC */

/* GetIconLocation() return flags */
#define GIL_SIMULATEDOC  0x0001      /* simulate this document icon for this */
#define GIL_PERINSTANCE  0x0002      /* icons from this class are per instance (each file has its own) */
#define GIL_PERCLASS     0x0004      /* icons from this class per class (shared for all files of this type) */
#define GIL_NOTFILENAME  0x0008      /* location is not a filename, must call ::ExtractIcon */
#define GIL_DONTCACHE    0x0010      /* this icon should not be cached */


#define INTERFACE IExtractIconA
#define IExtractIconA_METHODS \
	STDMETHOD(GetIconLocation)(THIS_ UINT  uFlags, LPSTR  szIconFile, UINT  cchMax, INT * piIndex, UINT  * pwFlags) PURE; \
	STDMETHOD(Extract)(THIS_ LPCSTR  pszFile, UINT  nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT  nIconSize) PURE;
#define IExtractIconA_IMETHODS \
	IUnknown_IMETHODS \
	IExtractIconA_METHODS
ICOM_DEFINE(IExtractIconA,IUnknown)
#undef INTERFACE

#define IExtractIconA_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IExtractIconA_AddRef(p)			ICOM_CALL(AddRef,p)
#define IExtractIconA_Release(p)		ICOM_CALL(Release,p)
#define IExtractIconA_GetIconLocation(p,a,b,c,d,e)	ICOM_CALL5(GetIconLocation,p,a,b,c,d,e)
#define IExtractIconA_Extract(p,a,b,c,d,e)	ICOM_CALL5(Extract,p,a,b,c,d,e)


#define INTERFACE IExtractIconW
#define IExtractIconW_METHODS \
	STDMETHOD(GetIconLocation)(THIS_ UINT  uFlags, LPWSTR  szIconFile, UINT  cchMax, INT * piIndex, UINT  * pwFlags) PURE; \
	STDMETHOD(Extract)(THIS_ LPCWSTR  pszFile, UINT  nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT  nIconSize) PURE;
#define IExtractIconW_IMETHODS \
	IUnknown_IMETHODS \
	IExtractIconW_METHODS
ICOM_DEFINE(IExtractIconW,IUnknown)
#undef INTERFACE

#define IExtractIconW_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IExtractIconW_AddRef(p)			ICOM_CALL(AddRef,p)
#define IExtractIconW_Release(p)		ICOM_CALL(Release,p)
#define IExtractIconW_GetIconLocation(p,a,b,c,d,e)	ICOM_CALL5(GetIconLocation,p,a,b,c,d,e)
#define IExtractIconW_Extract(p,a,b,c,d,e)	ICOM_CALL5(Extract,p,a,b,c,d,e)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_EXTRACTICON_H */
