/*
 * Defines the COM interfaces and APIs related to IShellLink.
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

#ifndef __WINE_WINE_OBJ_SHELLLINK_H
#define __WINE_WINE_OBJ_SHELLLINK_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#define IShellLink WINELIB_NAME_AW(IShellLink)

/*****************************************************************************
 * Predeclare the interfaces
 */
typedef struct IShellLinkA IShellLinkA,*LPSHELLLINK;
typedef struct IShellLinkW IShellLinkW,*LPSHELLLINKW;

/*****************************************************************************
 *
 */
typedef enum
{	SLR_NO_UI	= 0x0001,
	SLR_ANY_MATCH	= 0x0002,
	SLR_UPDATE	= 0x0004
} SLR_FLAGS;

/*****************************************************************************
 * GetPath fFlags
 */
typedef enum
{	SLGP_SHORTPATH		= 0x0001,
	SLGP_UNCPRIORITY	= 0x0002
} SLGP_FLAGS;
/*****************************************************************************
 * IShellLink interface
 */
#define INTERFACE IShellLinkA
#define IShellLinkA_METHODS \
    IUnknown_METHODS \
    STDMETHOD(GetPath)(THIS_ LPSTR  pszFile, INT  cchMaxPath, WIN32_FIND_DATAA  * pfd, DWORD  fFlags) PURE; \
    STDMETHOD(GetIDList)(THIS_ LPITEMIDLIST  * ppidl) PURE; \
    STDMETHOD(SetIDList)(THIS_ LPCITEMIDLIST  pidl) PURE; \
    STDMETHOD(GetDescription)(THIS_ LPSTR  pszName, INT  cchMaxName) PURE; \
    STDMETHOD(SetDescription)(THIS_ LPCSTR  pszName) PURE; \
    STDMETHOD(GetWorkingDirectory)(THIS_ LPSTR  pszDir,INT  cchMaxPath) PURE; \
    STDMETHOD(SetWorkingDirectory)(THIS_ LPCSTR  pszDir) PURE; \
    STDMETHOD(GetArguments)(THIS_ LPSTR  pszArgs, INT  cchMaxPath) PURE; \
    STDMETHOD(SetArguments)(THIS_ LPCSTR  pszArgs) PURE; \
    STDMETHOD(GetHotkey)(THIS_ WORD * pwHotkey) PURE; \
    STDMETHOD(SetHotkey)(THIS_ WORD  wHotkey) PURE; \
    STDMETHOD(GetShowCmd)(THIS_ INT * piShowCmd) PURE; \
    STDMETHOD(SetShowCmd)(THIS_ INT  iShowCmd) PURE; \
    STDMETHOD(GetIconLocation)(THIS_ LPSTR  pszIconPath, INT  cchIconPath,INT  * piIcon) PURE; \
    STDMETHOD(SetIconLocation)(THIS_ LPCSTR  pszIconPath,INT  iIcon) PURE; \
    STDMETHOD(SetRelativePath)(THIS_ LPCSTR  pszPathRel, DWORD  dwReserved) PURE; \
    STDMETHOD(Resolve)(THIS_ HWND  hwnd, DWORD  fFlags) PURE; \
    STDMETHOD(SetPath)(THIS_ LPCSTR  pszFile) PURE;
ICOM_DEFINE(IShellLinkA,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IShellLinkA_QueryInterface(p,a,b)       (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellLinkA_AddRef(p)                   (p)->lpVtbl->AddRef(p)
#define IShellLinkA_Release(p)                  (p)->lpVtbl->Release(p)
/*** IShellLink methods ***/
#define IShellLinkA_GetPath(p,a,b,c,d)          (p)->lpVtbl->GetPath(p,a,b,c,d)
#define IShellLinkA_GetIDList(p,a)              (p)->lpVtbl->GetIDList(p,a)
#define IShellLinkA_SetIDList(p,a)              (p)->lpVtbl->SetIDList(p,a)
#define IShellLinkA_GetDescription(p,a,b)       (p)->lpVtbl->GetDescription(p,a,b)
#define IShellLinkA_SetDescription(p,a)         (p)->lpVtbl->SetDescription(p,a)
#define IShellLinkA_GetWorkingDirectory(p,a,b)  (p)->lpVtbl->GetWorkingDirectory(p,a,b)
#define IShellLinkA_SetWorkingDirectory(p,a)    (p)->lpVtbl->SetWorkingDirectory(p,a)
#define IShellLinkA_GetArguments(p,a,b)         (p)->lpVtbl->GetArguments(p,a,b)
#define IShellLinkA_SetArguments(p,a)           (p)->lpVtbl->SetArguments(p,a)
#define IShellLinkA_GetHotkey(p,a)              (p)->lpVtbl->GetHotkey(p,a)
#define IShellLinkA_SetHotkey(p,a)              (p)->lpVtbl->SetHotkey(p,a)
#define IShellLinkA_GetShowCmd(p,a)             (p)->lpVtbl->GetShowCmd(p,a)
#define IShellLinkA_SetShowCmd(p,a)             (p)->lpVtbl->SetShowCmd(p,a)
#define IShellLinkA_GetIconLocation(p,a,b,c)    (p)->lpVtbl->GetIconLocation(p,a,b,c)
#define IShellLinkA_SetIconLocation(p,a,b)      (p)->lpVtbl->SetIconLocation(p,a,b)
#define IShellLinkA_SetRelativePath(p,a,b)      (p)->lpVtbl->SetRelativePath(p,a,b)
#define IShellLinkA_Resolve(p,a,b)              (p)->lpVtbl->Resolve(p,a,b)
#define IShellLinkA_SetPath(p,a)                (p)->lpVtbl->SetPath(p,a)
#endif

/*****************************************************************************
 * IShellLinkW interface
 */
#define INTERFACE IShellLinkW
#define IShellLinkW_METHODS \
    IUnknown_METHODS \
    STDMETHOD(GetPath)(THIS_ LPWSTR  pszFile, INT  cchMaxPath, WIN32_FIND_DATAA  * pfd, DWORD  fFlags) PURE; \
    STDMETHOD(GetIDList)(THIS_ LPITEMIDLIST  * ppidl) PURE; \
    STDMETHOD(SetIDList)(THIS_ LPCITEMIDLIST  pidl) PURE; \
    STDMETHOD(GetDescription)(THIS_ LPWSTR  pszName, INT  cchMaxName) PURE; \
    STDMETHOD(SetDescription)(THIS_ LPCWSTR  pszName) PURE; \
    STDMETHOD(GetWorkingDirectory)(THIS_ LPWSTR  pszDir,INT  cchMaxPath) PURE; \
    STDMETHOD(SetWorkingDirectory)(THIS_ LPCWSTR  pszDir) PURE; \
    STDMETHOD(GetArguments)(THIS_ LPWSTR  pszArgs, INT  cchMaxPath) PURE; \
    STDMETHOD(SetArguments)(THIS_ LPCWSTR  pszArgs) PURE; \
    STDMETHOD(GetHotkey)(THIS_ WORD * pwHotkey) PURE; \
    STDMETHOD(SetHotkey)(THIS_ WORD  wHotkey) PURE; \
    STDMETHOD(GetShowCmd)(THIS_ INT * piShowCmd) PURE; \
    STDMETHOD(SetShowCmd)(THIS_ INT  iShowCmd) PURE; \
    STDMETHOD(GetIconLocation)(THIS_ LPWSTR  pszIconPath, INT  cchIconPath,INT  * piIcon) PURE; \
    STDMETHOD(SetIconLocation)(THIS_ LPCWSTR  pszIconPath,INT  iIcon) PURE; \
    STDMETHOD(SetRelativePath)(THIS_ LPCWSTR  pszPathRel, DWORD  dwReserved) PURE; \
    STDMETHOD(Resolve)(THIS_ HWND  hwnd, DWORD  fFlags) PURE; \
    STDMETHOD(SetPath)(THIS_ LPCWSTR  pszFile) PURE;
ICOM_DEFINE(IShellLinkW,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IShellLinkW_QueryInterface(p,a,b)       (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellLinkW_AddRef(p)                   (p)->lpVtbl->AddRef(p)
#define IShellLinkW_Release(p)                  (p)->lpVtbl->Release(p)
/*** IShellLinkW methods ***/
#define IShellLinkW_GetPath(p,a,b,c,d)          (p)->lpVtbl->GetPath(p,a,b,c,d)
#define IShellLinkW_GetIDList(p,a)              (p)->lpVtbl->GetIDList(p,a)
#define IShellLinkW_SetIDList(p,a)              (p)->lpVtbl->SetIDList(p,a)
#define IShellLinkW_GetDescription(p,a,b)       (p)->lpVtbl->GetDescription(p,a,b)
#define IShellLinkW_SetDescription(p,a)         (p)->lpVtbl->SetDescription(p,a)
#define IShellLinkW_GetWorkingDirectory(p,a,b)  (p)->lpVtbl->GetWorkingDirectory(p,a,b)
#define IShellLinkW_SetWorkingDirectory(p,a)    (p)->lpVtbl->SetWorkingDirectory(p,a)
#define IShellLinkW_GetArguments(p,a,b)         (p)->lpVtbl->GetArguments(p,a,b)
#define IShellLinkW_SetArguments(p,a)           (p)->lpVtbl->SetArguments(p,a)
#define IShellLinkW_GetHotkey(p,a)              (p)->lpVtbl->GetHotkey(p,a)
#define IShellLinkW_SetHotkey(p,a)              (p)->lpVtbl->SetHotkey(p,a)
#define IShellLinkW_GetShowCmd(p,a)             (p)->lpVtbl->GetShowCmd(p,a)
#define IShellLinkW_SetShowCmd(p,a)             (p)->lpVtbl->SetShowCmd(p,a)
#define IShellLinkW_GetIconLocation(p,a,b,c)    (p)->lpVtbl->GetIconLocation(p,a,b,c)
#define IShellLinkW_SetIconLocation(p,a,b)      (p)->lpVtbl->SetIconLocation(p,a,b)
#define IShellLinkW_SetRelativePath(p,a,b)      (p)->lpVtbl->SetRelativePath(p,a,b)
#define IShellLinkW_Resolve(p,a,b)              (p)->lpVtbl->Resolve(p,a,b)
#define IShellLinkW_SetPath(p,a)                (p)->lpVtbl->SetPath(p,a)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_SHELLLINK_H */
