
/*
 *    Virtual Desktop Folder
 *
 *    Copyright 1997            Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "ole2.h"
#include "shlguid.h"

#include "wininet.h"
#include "pidl.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "shlwapi.h"
#include "shellfolder.h"
#include "wine/debug.h"
#include "debughlp.h"
#include "shfldr.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/* Undocumented functions from shdocvw */
extern HRESULT WINAPI IEParseDisplayNameWithBCW(DWORD codepage, LPCWSTR lpszDisplayName, LPBC pbc, LPITEMIDLIST *ppidl);

/***********************************************************************
*     Desktopfolder implementation
*/

typedef struct {
    IShellFolder2 IShellFolder2_iface;
    IPersistFolder2 IPersistFolder2_iface;
    LONG ref;

    /* both paths are parsible from the desktop */
    LPWSTR sPathTarget;     /* complete path to target used for enumeration and ChangeNotify */
    LPITEMIDLIST pidlRoot;  /* absolute pidl */

    UINT cfShellIDList;        /* clipboardformat for IDropTarget */
    BOOL fAcceptFmt;        /* flag for pending Drop */
} IDesktopFolderImpl;

static IDesktopFolderImpl *cached_sf;

static inline IDesktopFolderImpl *impl_from_IShellFolder2(IShellFolder2 *iface)
{
    return CONTAINING_RECORD(iface, IDesktopFolderImpl, IShellFolder2_iface);
}

static inline IDesktopFolderImpl *impl_from_IPersistFolder2( IPersistFolder2 *iface )
{
    return CONTAINING_RECORD(iface, IDesktopFolderImpl, IPersistFolder2_iface);
}

static const shvheader desktop_header[] =
{
    { &FMTID_Storage, PID_STG_NAME, IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 15 },
    { &FMTID_Storage, PID_STG_SIZE, IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 10 },
    { &FMTID_Storage, PID_STG_STORAGETYPE, IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 10 },
    { &FMTID_Storage, PID_STG_WRITETIME, IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12 },
    { &FMTID_Storage, PID_STG_ATTRIBUTES, IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 5  },
};

/**************************************************************************
 *    ISF_Desktop_fnQueryInterface
 *
 */
static HRESULT WINAPI ISF_Desktop_fnQueryInterface(
                IShellFolder2 * iface, REFIID riid, LPVOID * ppvObj)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    if (!ppvObj) return E_POINTER;

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IShellFolder) ||
        IsEqualIID (riid, &IID_IShellFolder2))
    {
        *ppvObj = &This->IShellFolder2_iface;
    }
    else if (IsEqualIID (riid, &IID_IPersist) ||
             IsEqualIID (riid, &IID_IPersistFolder) ||
             IsEqualIID (riid, &IID_IPersistFolder2))
    {
        *ppvObj = &This->IPersistFolder2_iface;
    }

    if (*ppvObj)
    {
        IUnknown_AddRef ((IUnknown *) (*ppvObj));
        TRACE ("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
        return S_OK;
    }
    TRACE ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ISF_Desktop_fnAddRef (IShellFolder2 * iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI ISF_Desktop_fnRelease (IShellFolder2 * iface)
{
    return 1; /* non-heap based object */
}

/**************************************************************************
 *    ISF_Desktop_fnParseDisplayName
 *
 * NOTES
 *    "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}" and "" binds
 *    to MyComputer
 */
static HRESULT WINAPI ISF_Desktop_fnParseDisplayName (IShellFolder2 * iface,
                HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
                DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    WCHAR szElement[MAX_PATH];
    LPCWSTR szNext = NULL;
    LPITEMIDLIST pidlTemp = NULL;
    PARSEDURLW urldata;
    HRESULT hr = S_OK;
    CLSID clsid;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
           This, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName),
           pchEaten, ppidl, pdwAttributes);

    if (!ppidl) return E_INVALIDARG;
    *ppidl = 0;

    if (!lpszDisplayName) return E_INVALIDARG;

    if (pchEaten)
        *pchEaten = 0;        /* strange but like the original */

    urldata.cbSize = sizeof(urldata);

    if (lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':')
    {
        szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
        TRACE ("-- element: %s\n", debugstr_w (szElement));
        SHCLSIDFromStringW (szElement + 2, &clsid);
        pidlTemp = _ILCreateGuid (PT_GUID, &clsid);
    }
    else if (PathGetDriveNumberW (lpszDisplayName) >= 0)
    {
        /* it's a filesystem path with a drive. Let MyComputer/UnixDosFolder parse it */
        pidlTemp = _ILCreateMyComputer ();
        szNext = lpszDisplayName;
    }
    else if (!wcsncmp( lpszDisplayName, L"\\\\?\\unix\\", 9 ))
    {
        pidlTemp = _ILCreateGuid(PT_GUID, &CLSID_UnixDosFolder);
        szNext = lpszDisplayName;
    }
    else if (PathIsUNCW(lpszDisplayName))
    {
        pidlTemp = _ILCreateNetwork();
        szNext = lpszDisplayName;
    }
    else if( (pidlTemp = SHELL32_CreatePidlFromBindCtx(pbc, lpszDisplayName)) )
    {
        *ppidl = pidlTemp;
        return S_OK;
    }
    else if (SUCCEEDED(ParseURLW(lpszDisplayName, &urldata)))
    {
        if (urldata.nScheme == URL_SCHEME_SHELL) /* handle shell: urls */
        {
            TRACE ("-- shell url: %s\n", debugstr_w(urldata.pszSuffix));
            SHCLSIDFromStringW (urldata.pszSuffix+2, &clsid);
            pidlTemp = _ILCreateGuid (PT_GUID, &clsid);
        }
        else
            return IEParseDisplayNameWithBCW(CP_ACP,lpszDisplayName,pbc,ppidl);
    }
    else
    {
        /* it's a filesystem path on the desktop. Let a FSFolder parse it */

        if (*lpszDisplayName)
        {
            if (*lpszDisplayName == '/')
            {
                /* UNIX paths should be parsed by unixfs */
                IShellFolder *unixFS;
                hr = UnixFolder_Constructor(NULL, &IID_IShellFolder, (LPVOID*)&unixFS);
                if (SUCCEEDED(hr))
                {
                    hr = IShellFolder_ParseDisplayName(unixFS, NULL, NULL,
                            lpszDisplayName, NULL, &pidlTemp, NULL);
                    IShellFolder_Release(unixFS);
                }
            }
            else
            {
                /* build a complete path to create a simple pidl */
                WCHAR szPath[MAX_PATH];
                LPWSTR pathPtr;

                lstrcpynW(szPath, This->sPathTarget, MAX_PATH);
                pathPtr = PathAddBackslashW(szPath);
                if (pathPtr)
                {
                    lstrcpynW(pathPtr, lpszDisplayName, MAX_PATH - (pathPtr - szPath));
                    hr = _ILCreateFromPathW(szPath, &pidlTemp);
                }
                else
                {
                    /* should never reach here, but for completeness */
                    hr = E_NOT_SUFFICIENT_BUFFER;
                }
            }
        }
        else
            pidlTemp = _ILCreateMyComputer();

        szNext = NULL;
    }

    if (SUCCEEDED(hr) && pidlTemp)
    {
        if (szNext && *szNext)
        {
            hr = SHELL32_ParseNextElement(iface, hwndOwner, pbc,
                    &pidlTemp, (LPOLESTR) szNext, pchEaten, pdwAttributes);
        }
        else
        {
            if (pdwAttributes && *pdwAttributes)
                hr = SHELL32_GetItemAttributes(iface, pidlTemp, pdwAttributes);
        }
    }

    *ppidl = pidlTemp;

    TRACE ("(%p)->(-- ret=0x%08lx)\n", This, hr);

    return hr;
}

static void add_shell_namespace_extensions(IEnumIDListImpl *list, HKEY root)
{
    WCHAR guid[39], clsidkeyW[60];
    DWORD size, i = 0;
    HKEY hkey;

    if (RegOpenKeyExW(root, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\Namespace", 0, KEY_READ, &hkey))
        return;

    size = ARRAY_SIZE(guid);
    while (!RegEnumKeyExW(hkey, i++, guid, &size, 0, NULL, NULL, NULL))
    {
        DWORD attributes, value_size = sizeof(attributes);

        /* Check if extension is configured as nonenumerable */
        swprintf(clsidkeyW, ARRAY_SIZE(clsidkeyW), L"CLSID\\%s\\ShellFolder", guid);
        RegGetValueW(HKEY_CLASSES_ROOT, clsidkeyW, L"Attributes", RRF_RT_REG_DWORD | RRF_ZEROONFAILURE,
            NULL, &attributes, &value_size);

        if (!(attributes & SFGAO_NONENUMERATED))
            AddToEnumList(list, _ILCreateGuidFromStrW(guid));
        size = ARRAY_SIZE(guid);
    }

    RegCloseKey(hkey);
}

/**************************************************************************
 *  CreateDesktopEnumList()
 */
static BOOL CreateDesktopEnumList(IEnumIDListImpl *list, DWORD dwFlags)
{
    BOOL ret = TRUE;
    WCHAR szPath[MAX_PATH];

    TRACE("(%p)->(flags=0x%08lx)\n", list, dwFlags);

    /* enumerate the root folders */
    if (dwFlags & SHCONTF_FOLDERS)
    {
        ret = AddToEnumList(list, _ILCreateMyComputer());
        add_shell_namespace_extensions(list, HKEY_LOCAL_MACHINE);
        add_shell_namespace_extensions(list, HKEY_CURRENT_USER);
    }

    /* enumerate the elements in %windir%\desktop */
    ret = ret && SHGetSpecialFolderPathW(0, szPath, CSIDL_DESKTOPDIRECTORY, FALSE);
    ret = ret && CreateFolderEnumList(list, szPath, dwFlags);

    return ret;
}

/**************************************************************************
 *        ISF_Desktop_fnEnumObjects
 */
static HRESULT WINAPI ISF_Desktop_fnEnumObjects (IShellFolder2 * iface,
                HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    IEnumIDListImpl *list;

    TRACE ("(%p)->(HWND=%p flags=0x%08lx pplist=%p)\n",
           This, hwndOwner, dwFlags, ppEnumIDList);

    if (!(list = IEnumIDList_Constructor()))
        return E_OUTOFMEMORY;
    CreateDesktopEnumList(list, dwFlags);
    *ppEnumIDList = &list->IEnumIDList_iface;

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return S_OK;
}

/**************************************************************************
 *        ISF_Desktop_fnBindToObject
 */
static HRESULT WINAPI ISF_Desktop_fnBindToObject (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n",
           This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild( This->pidlRoot, &CLSID_ShellFSFolder, This->sPathTarget, pidl, riid, ppvOut );
}

/**************************************************************************
 *    ISF_Desktop_fnBindToStorage
 */
static HRESULT WINAPI ISF_Desktop_fnBindToStorage (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n",
           This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
 *     ISF_Desktop_fnCompareIDs
 */
static HRESULT WINAPI ISF_Desktop_fnCompareIDs (IShellFolder2 *iface,
                        LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    HRESULT hr;

    TRACE ("(%p)->(0x%08Ix,pidl1=%p,pidl2=%p)\n", This, lParam, pidl1, pidl2);
    hr = SHELL32_CompareIDs(iface, lParam, pidl1, pidl2);
    TRACE ("-- 0x%08lx\n", hr);
    return hr;
}

/**************************************************************************
 *    ISF_Desktop_fnCreateViewObject
 */
static HRESULT WINAPI ISF_Desktop_fnCreateViewObject (IShellFolder2 * iface,
                              HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n",
           This, hwndOwner, shdebugstr_guid (riid), ppvOut);

    if (!ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IDropTarget))
    {
        WARN ("IDropTarget not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, &IID_IContextMenu))
    {
        hr = BackgroundMenu_Constructor((IShellFolder*)iface, TRUE, riid, ppvOut);
    }
    else if (IsEqualIID (riid, &IID_IShellView))
    {
        pShellView = IShellView_Constructor ((IShellFolder *) iface);
        if (pShellView)
        {
            hr = IShellView_QueryInterface (pShellView, riid, ppvOut);
            IShellView_Release (pShellView);
        }
    }
    TRACE ("-- (%p)->(interface=%p)\n", This, ppvOut);
    return hr;
}

/**************************************************************************
 *  ISF_Desktop_fnGetAttributesOf
 */
static HRESULT WINAPI ISF_Desktop_fnGetAttributesOf (IShellFolder2 * iface,
                UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    static const DWORD dwDesktopAttributes = 
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER;
    static const DWORD dwMyComputerAttributes = 
        SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HASPROPSHEET |
        SFGAO_DROPTARGET | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08lx))\n",
           This, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;
    
    if(cidl == 0) {
        *rgfInOut &= dwDesktopAttributes; 
    } else {
        while (cidl > 0 && *apidl) {
            pdump (*apidl);
            if (_ILIsDesktop(*apidl)) { 
                *rgfInOut &= dwDesktopAttributes;
            } else if (_ILIsMyComputer(*apidl)) {
                *rgfInOut &= dwMyComputerAttributes;
            } else {
                SHELL32_GetItemAttributes(iface, *apidl, rgfInOut);
            }
            apidl++;
            cidl--;
        }
    }
    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE ("-- result=0x%08lx\n", *rgfInOut);

    return S_OK;
}

/**************************************************************************
 *    ISF_Desktop_fnGetUIObjectOf
 *
 * PARAMETERS
 *  HWND           hwndOwner, //[in ] Parent window for any output
 *  UINT           cidl,      //[in ] array size
 *  LPCITEMIDLIST* apidl,     //[in ] simple pidl array
 *  REFIID         riid,      //[in ] Requested Interface
 *  UINT*          prgfInOut, //[   ] reserved
 *  LPVOID*        ppvObject) //[out] Resulting Interface
 *
 */
static HRESULT WINAPI ISF_Desktop_fnGetUIObjectOf (IShellFolder2 * iface,
                HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
       This, hwndOwner, cidl, apidl, shdebugstr_guid (riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IContextMenu))
    {
        if (cidl > 0)
            return ItemMenu_Constructor((IShellFolder*)iface, This->pidlRoot, apidl, cidl, riid, ppvOut);
        else
            return BackgroundMenu_Constructor((IShellFolder*)iface, TRUE, riid, ppvOut);
    }
    else if (IsEqualIID (riid, &IID_IDataObject) && (cidl >= 1))
    {
        pObj = (LPUNKNOWN) IDataObject_Constructor( hwndOwner,
                                                  This->pidlRoot, apidl, cidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IExtractIconA) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconA_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IExtractIconW) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconW_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IDropTarget) && (cidl >= 1))
    {
        hr = IShellFolder2_QueryInterface (iface,
                                          &IID_IDropTarget, (LPVOID *) & pObj);
    }
    else if ((IsEqualIID(riid,&IID_IShellLinkW) ||
              IsEqualIID(riid,&IID_IShellLinkA)) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        hr = IShellLink_ConstructFromFile(NULL, riid, pidl, &pObj);
        SHFree (pidl);
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj;
    TRACE ("(%p)->hr=0x%08lx\n", This, hr);
    return hr;
}

/**************************************************************************
 *    ISF_Desktop_fnGetDisplayNameOf
 *
 * NOTES
 *    special case: pidl = null gives desktop-name back
 */
static HRESULT WINAPI ISF_Desktop_fnGetDisplayNameOf (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    HRESULT hr = S_OK;
    LPWSTR pszPath;

    TRACE ("(%p)->(pidl=%p,0x%08lx,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    pszPath = CoTaskMemAlloc((MAX_PATH +1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    if (_ILIsDesktop (pidl))
    {
        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
            (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
            lstrcpyW(pszPath, This->sPathTarget);
        else
            HCR_GetClassNameW(&CLSID_ShellDesktop, pszPath, MAX_PATH);
    }
    else if (_ILIsPidlSimple (pidl))
    {
        GUID const *clsid;

        if ((clsid = _ILGetGUIDPointer (pidl)))
        {
            if ((GET_SHGDN_FOR (dwFlags) & (SHGDN_FORPARSING | SHGDN_FORADDRESSBAR)) == SHGDN_FORPARSING)
            {
                BOOL bWantsForParsing;

                /*
                 * We can only get a filesystem path from a shellfolder if the
                 *  value WantsFORPARSING in CLSID\\{...}\\shellfolder exists.
                 *
                 * Exception: The MyComputer folder doesn't have this key,
                 *   but any other filesystem backed folder it needs it.
                 */
                if (IsEqualIID (clsid, &CLSID_MyComputer))
                {
                    bWantsForParsing = TRUE;
                }
                else
                {
                    /* get the "WantsFORPARSING" flag from the registry */
                    WCHAR szRegPath[100];
                    LONG r;

                    lstrcpyW (szRegPath, L"CLSID\\");
                    SHELL32_GUIDToStringW (clsid, &szRegPath[6]);
                    lstrcatW (szRegPath, L"\\shellfolder");
                    r = SHGetValueW(HKEY_CLASSES_ROOT, szRegPath, L"WantsForParsing", NULL, NULL, NULL);
                    if (r == ERROR_SUCCESS)
                        bWantsForParsing = TRUE;
                    else
                        bWantsForParsing = FALSE;
                }

                if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                     bWantsForParsing)
                {
                    /*
                     * we need the filesystem path to the destination folder.
                     * Only the folder itself can know it
                     */
                    hr = SHELL32_GetDisplayNameOfChild (iface, pidl, dwFlags,
                                                        pszPath,
                                                        MAX_PATH);
                }
                else
                {
                    /* parsing name like ::{...} */
                    pszPath[0] = ':';
                    pszPath[1] = ':';
                    SHELL32_GUIDToStringW (clsid, &pszPath[2]);
                }
            }
            else
            {
                /* user friendly name */
                HCR_GetClassNameW (clsid, pszPath, MAX_PATH);
            }
        }
        else
        {
            int cLen = 0;

            /* file system folder or file rooted at the desktop */
            if ((GET_SHGDN_FOR(dwFlags) == SHGDN_FORPARSING) &&
                (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER))
            {
                lstrcpynW(pszPath, This->sPathTarget, MAX_PATH - 1);
                PathAddBackslashW(pszPath);
                cLen = lstrlenW(pszPath);
            }

            _ILSimpleGetTextW(pidl, pszPath + cLen, MAX_PATH - cLen);

            if (!_ILIsFolder(pidl))
                SHELL_FS_ProcessDisplayFilename(pszPath, dwFlags);
        }
    }
    else
    {
        /* a complex pidl, let the subfolder do the work */
        hr = SHELL32_GetDisplayNameOfChild (iface, pidl, dwFlags,
                                            pszPath, MAX_PATH);
    }

    if (SUCCEEDED(hr))
    {
        /* Win9x always returns ANSI strings, NT always returns Unicode strings */
        if (GetVersion() & 0x80000000)
        {
            strRet->uType = STRRET_CSTR;
            if (!WideCharToMultiByte(CP_ACP, 0, pszPath, -1, strRet->u.cStr, MAX_PATH,
                                     NULL, NULL))
                strRet->u.cStr[0] = '\0';
            CoTaskMemFree(pszPath);
        }
        else
        {
            strRet->uType = STRRET_WSTR;
            strRet->u.pOleStr = pszPath;
        }
    }
    else
        CoTaskMemFree(pszPath);

    TRACE ("-- (%p)->(%s,0x%08lx)\n", This,
    strRet->uType == STRRET_CSTR ? strRet->u.cStr :
    debugstr_w(strRet->u.pOleStr), hr);
    return hr;
}

/**************************************************************************
 *  ISF_Desktop_fnSetNameOf
 *  Changes the name of a file object or subfolder, possibly changing its item
 *  identifier in the process.
 *
 * PARAMETERS
 *  HWND          hwndOwner,  //[in ] Owner window for output
 *  LPCITEMIDLIST pidl,       //[in ] simple pidl of item to change
 *  LPCOLESTR     lpszName,   //[in ] the items new display name
 *  DWORD         dwFlags,    //[in ] SHGNO formatting flags
 *  LPITEMIDLIST* ppidlOut)   //[out] simple pidl returned
 */
static HRESULT WINAPI ISF_Desktop_fnSetNameOf (IShellFolder2 * iface,
                HWND hwndOwner, LPCITEMIDLIST pidl,    /* simple pidl */
                LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    FIXME ("(%p)->(%p,pidl=%p,%s,%lu,%p) stub\n", This, hwndOwner, pidl,
           debugstr_w (lpName), dwFlags, pPidlOut);

    return E_FAIL;
}

static HRESULT WINAPI ISF_Desktop_fnGetDefaultSearchGUID(IShellFolder2 *iface, GUID *guid)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    TRACE("(%p)->(%p)\n", This, guid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Desktop_fnEnumSearches (IShellFolder2 *iface,
                IEnumExtraSearch ** ppenum)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    FIXME ("(%p)->(%p) stub\n", This, ppenum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Desktop_fnGetDefaultColumn(IShellFolder2 *iface, DWORD reserved, ULONG *sort, ULONG *display)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(%#lx, %p, %p)\n", This, reserved, sort, display);

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Desktop_fnGetDefaultColumnState (
                IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(%d %p)\n", This, iColumn, pcsFlags);

    if (!pcsFlags || iColumn >= ARRAY_SIZE(desktop_header))
        return E_INVALIDARG;

    *pcsFlags = desktop_header[iColumn].pcsFlags;

    return S_OK;
}

static HRESULT WINAPI ISF_Desktop_fnGetDetailsEx (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);
    FIXME ("(%p)->(%p %p %p) stub\n", This, pidl, pscid, pv);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Desktop_fnGetDetailsOf (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(%p %i %p)\n", This, pidl, iColumn, psd);

    if (!psd || iColumn >= ARRAY_SIZE(desktop_header))
        return E_INVALIDARG;

    if (!pidl)
        return SHELL32_GetColumnDetails(desktop_header, iColumn, psd);

    return shellfolder_get_file_details( iface, pidl, desktop_header, iColumn, psd );
}

static HRESULT WINAPI ISF_Desktop_fnMapColumnToSCID(IShellFolder2 *iface, UINT column, SHCOLUMNID *scid)
{
    IDesktopFolderImpl *This = impl_from_IShellFolder2(iface);

    TRACE("(%p)->(%u %p)\n", This, column, scid);

    if (column >= ARRAY_SIZE(desktop_header))
        return E_INVALIDARG;

    return shellfolder_map_column_to_scid(desktop_header, column, scid);
}

static const IShellFolder2Vtbl vt_MCFldr_ShellFolder2 =
{
    ISF_Desktop_fnQueryInterface,
    ISF_Desktop_fnAddRef,
    ISF_Desktop_fnRelease,
    ISF_Desktop_fnParseDisplayName,
    ISF_Desktop_fnEnumObjects,
    ISF_Desktop_fnBindToObject,
    ISF_Desktop_fnBindToStorage,
    ISF_Desktop_fnCompareIDs,
    ISF_Desktop_fnCreateViewObject,
    ISF_Desktop_fnGetAttributesOf,
    ISF_Desktop_fnGetUIObjectOf,
    ISF_Desktop_fnGetDisplayNameOf,
    ISF_Desktop_fnSetNameOf,
    /* ShellFolder2 */
    ISF_Desktop_fnGetDefaultSearchGUID,
    ISF_Desktop_fnEnumSearches,
    ISF_Desktop_fnGetDefaultColumn,
    ISF_Desktop_fnGetDefaultColumnState,
    ISF_Desktop_fnGetDetailsEx,
    ISF_Desktop_fnGetDetailsOf,
    ISF_Desktop_fnMapColumnToSCID
};

/**************************************************************************
 *    IPersist
 */
static HRESULT WINAPI ISF_Desktop_IPersistFolder2_fnQueryInterface(
    IPersistFolder2 *iface, REFIID riid, LPVOID *ppvObj)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder2( iface );
    return IShellFolder2_QueryInterface(&This->IShellFolder2_iface, riid, ppvObj);
}

static ULONG WINAPI ISF_Desktop_IPersistFolder2_fnAddRef(
    IPersistFolder2 *iface)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder2( iface );
    return IShellFolder2_AddRef(&This->IShellFolder2_iface);
}

static ULONG WINAPI ISF_Desktop_IPersistFolder2_fnRelease(
    IPersistFolder2 *iface)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder2( iface );
    return IShellFolder2_Release(&This->IShellFolder2_iface);
}

static HRESULT WINAPI ISF_Desktop_IPersistFolder2_fnGetClassID(
    IPersistFolder2 *iface, CLSID *clsid)
{
    *clsid = CLSID_ShellDesktop;
    return S_OK;
}
static HRESULT WINAPI ISF_Desktop_IPersistFolder2_fnInitialize(
    IPersistFolder2 *iface, LPCITEMIDLIST pidl)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder2( iface );
    FIXME ("(%p)->(%p) stub\n", This, pidl);
    return E_NOTIMPL;
}
static HRESULT WINAPI ISF_Desktop_IPersistFolder2_fnGetCurFolder(
    IPersistFolder2 *iface, LPITEMIDLIST *ppidl)
{
    IDesktopFolderImpl *This = impl_from_IPersistFolder2( iface );
    *ppidl = ILClone(This->pidlRoot);
    return S_OK;
}

static const IPersistFolder2Vtbl vt_IPersistFolder2 =
{
    ISF_Desktop_IPersistFolder2_fnQueryInterface,
    ISF_Desktop_IPersistFolder2_fnAddRef,
    ISF_Desktop_IPersistFolder2_fnRelease,
    ISF_Desktop_IPersistFolder2_fnGetClassID,
    ISF_Desktop_IPersistFolder2_fnInitialize,
    ISF_Desktop_IPersistFolder2_fnGetCurFolder
};

void release_desktop_folder(void)
{
    if (!cached_sf) return;
    SHFree(cached_sf->pidlRoot);
    SHFree(cached_sf->sPathTarget);
    LocalFree(cached_sf);
}

/**************************************************************************
 *    ISF_Desktop_Constructor
 */
HRESULT WINAPI ISF_Desktop_Constructor (
                IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    WCHAR szMyPath[MAX_PATH];

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (!cached_sf)
    {
        IDesktopFolderImpl *sf;

        if (!SHGetSpecialFolderPathW( 0, szMyPath, CSIDL_DESKTOPDIRECTORY, FALSE ))
            return E_UNEXPECTED;

        sf = LocalAlloc( LMEM_ZEROINIT, sizeof (IDesktopFolderImpl) );
        if (!sf)
            return E_OUTOFMEMORY;

        sf->ref = 1;
        sf->IShellFolder2_iface.lpVtbl = &vt_MCFldr_ShellFolder2;
        sf->IPersistFolder2_iface.lpVtbl = &vt_IPersistFolder2;
        sf->pidlRoot = _ILCreateDesktop();    /* my qualified pidl */
        sf->sPathTarget = SHAlloc( (lstrlenW(szMyPath) + 1)*sizeof(WCHAR) );
        lstrcpyW( sf->sPathTarget, szMyPath );

        if (InterlockedCompareExchangePointer((void *)&cached_sf, sf, NULL) != NULL)
        {
            /* some other thread already been here */
            SHFree( sf->pidlRoot );
            SHFree( sf->sPathTarget );
            LocalFree( sf );
        }
    }

    return IShellFolder2_QueryInterface( &cached_sf->IShellFolder2_iface, riid, ppv );
}

static HRESULT WINAPI active_desktop_QueryInterface(IActiveDesktop *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IActiveDesktop)
            || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }

    if (*obj)
    {
        IUnknown_AddRef((IUnknown *)*obj);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI active_desktop_AddRef(IActiveDesktop *iface)
{
    return 2;
}

static ULONG WINAPI active_desktop_Release(IActiveDesktop *iface)
{
    return 1;
}

static HRESULT WINAPI active_desktop_ApplyChanges(IActiveDesktop *iface, DWORD flags)
{
    FIXME("%p, %#lx.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_GetWallpaper(IActiveDesktop *iface, PWSTR wallpaper, UINT length, DWORD flags)
{
    FIXME("%p, %p, %u, %#lx.\n", iface, wallpaper, length, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_SetWallpaper(IActiveDesktop *iface, PCWSTR wallpaper, DWORD reserved)
{
    FIXME("%p, %s, %#lx.\n", iface, debugstr_w(wallpaper), reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_GetWallpaperOptions(IActiveDesktop *iface, LPWALLPAPEROPT options, DWORD reserved)
{
    FIXME("%p, %p, %#lx.\n", iface, options, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_SetWallpaperOptions(IActiveDesktop *iface, LPCWALLPAPEROPT options, DWORD reserved)
{
    FIXME("%p, %p, %#lx.\n", iface, options, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_GetPattern(IActiveDesktop *iface, PWSTR pattern, UINT length, DWORD reserved)
{
    FIXME("%p, %p, %u, %#lx.\n", iface, pattern, length, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_SetPattern(IActiveDesktop *iface, PCWSTR pattern, DWORD reserved)
{
    FIXME("%p, %s, %#lx.\n", iface, debugstr_w(pattern), reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_GetDesktopItemOptions(IActiveDesktop *iface, LPCOMPONENTSOPT options, DWORD reserved)
{
    FIXME("%p, %p, %#lx.\n", iface, options, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_SetDesktopItemOptions(IActiveDesktop *iface, LPCCOMPONENTSOPT options, DWORD reserved)
{
    FIXME("%p, %p, %#lx.\n", iface, options, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_AddDesktopItem(IActiveDesktop *iface, LPCCOMPONENT component, DWORD reserved)
{
    FIXME("%p, %p, %#lx.\n", iface, component, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_AddDesktopItemWithUI(IActiveDesktop *iface, HWND hwnd, LPCOMPONENT component, DWORD reserved)
{
    FIXME("%p, %p, %p, %#lx.\n", iface, hwnd, component, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_ModifyDesktopItem(IActiveDesktop *iface, LPCCOMPONENT component, DWORD flags)
{
    FIXME("%p, %p, %#lx.\n", iface, component, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_RemoveDesktopItem(IActiveDesktop *iface, LPCCOMPONENT component, DWORD reserved)
{
    FIXME("%p, %p, %#lx.\n", iface, component, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_GetDesktopItemCount(IActiveDesktop *iface, int *count, DWORD reserved)
{
    FIXME("%p, %p, %#lx.\n", iface, count, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_GetDesktopItem(IActiveDesktop *iface, int index, LPCOMPONENT component, DWORD reserved)
{
    FIXME("%p, %d, %p, %#lx.\n", iface, index, component, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_GetDesktopItemByID(IActiveDesktop *iface, ULONG_PTR id, LPCOMPONENT component, DWORD reserved)
{
    FIXME("%p, %Ix, %p, %#lx.\n", iface, id, component, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_GenerateDesktopItemHtml(IActiveDesktop *iface, PCWSTR filename, LPCOMPONENT component, DWORD reserved)
{
    FIXME("%p, %s, %p, %#lx.\n", iface, debugstr_w(filename), component, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_AddUrl(IActiveDesktop *iface, HWND hwnd, PCWSTR source, LPCOMPONENT component, DWORD flags)
{
    FIXME("%p, %p, %s, %p, %#lx.\n", iface, hwnd, debugstr_w(source), component, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI active_desktop_GetDesktopItemBySource(IActiveDesktop *iface, PCWSTR source, LPCOMPONENT component, DWORD reserved)
{
    FIXME("%p, %s, %p, %#lx.\n", iface, debugstr_w(source), component, reserved);

    return E_NOTIMPL;
}

static const IActiveDesktopVtbl active_desktop_vtbl =
{
    active_desktop_QueryInterface,
    active_desktop_AddRef,
    active_desktop_Release,
    active_desktop_ApplyChanges,
    active_desktop_GetWallpaper,
    active_desktop_SetWallpaper,
    active_desktop_GetWallpaperOptions,
    active_desktop_SetWallpaperOptions,
    active_desktop_GetPattern,
    active_desktop_SetPattern,
    active_desktop_GetDesktopItemOptions,
    active_desktop_SetDesktopItemOptions,
    active_desktop_AddDesktopItem,
    active_desktop_AddDesktopItemWithUI,
    active_desktop_ModifyDesktopItem,
    active_desktop_RemoveDesktopItem,
    active_desktop_GetDesktopItemCount,
    active_desktop_GetDesktopItem,
    active_desktop_GetDesktopItemByID,
    active_desktop_GenerateDesktopItemHtml,
    active_desktop_AddUrl,
    active_desktop_GetDesktopItemBySource,
};

HRESULT WINAPI ActiveDesktop_Constructor(IUnknown *outer, REFIID riid, void **obj)
{
    static IActiveDesktop object = { &active_desktop_vtbl };
    HRESULT hr;

    TRACE("%p, %s, %p.\n", outer, debugstr_guid(riid), obj);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    hr = IUnknown_QueryInterface(&object, riid, obj);
    IUnknown_Release(&object);

    return hr;
}
