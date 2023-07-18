/*
 * Control panel folder
 *
 * Copyright 2003 Martin Fuchs
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
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "ole2.h"
#include "shlguid.h"

#include "commctrl.h"
#include "cpanel.h"
#include "pidl.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "debughlp.h"
#include "shfldr.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/***********************************************************************
*   control panel implementation in shell namespace
*/

typedef struct {
    IShellFolder2      IShellFolder2_iface;
    IPersistFolder2    IPersistFolder2_iface;
    IShellExecuteHookW IShellExecuteHookW_iface;
    IShellExecuteHookA IShellExecuteHookA_iface;
    LONG               ref;

    IUnknown *pUnkOuter;	/* used for aggregation */

    /* both paths are parsible from the desktop */
    LPITEMIDLIST pidlRoot;	/* absolute pidl */
    int dwAttributes;		/* attributes returned by GetAttributesOf FIXME: use it */
} ICPanelImpl;

static const WCHAR name_spaceW[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace";

static const IShellFolder2Vtbl vt_ShellFolder2;
static const IPersistFolder2Vtbl vt_PersistFolder2;
static const IShellExecuteHookWVtbl vt_ShellExecuteHookW;
static const IShellExecuteHookAVtbl vt_ShellExecuteHookA;

static inline ICPanelImpl *impl_from_IShellFolder2(IShellFolder2 *iface)
{
    return CONTAINING_RECORD(iface, ICPanelImpl, IShellFolder2_iface);
}

static inline ICPanelImpl *impl_from_IPersistFolder2(IPersistFolder2 *iface)
{
    return CONTAINING_RECORD(iface, ICPanelImpl, IPersistFolder2_iface);
}

static inline ICPanelImpl *impl_from_IShellExecuteHookW(IShellExecuteHookW *iface)
{
    return CONTAINING_RECORD(iface, ICPanelImpl, IShellExecuteHookW_iface);
}

static inline ICPanelImpl *impl_from_IShellExecuteHookA(IShellExecuteHookA *iface)
{
    return CONTAINING_RECORD(iface, ICPanelImpl, IShellExecuteHookA_iface);
}


/***********************************************************************
*   IShellFolder [ControlPanel] implementation
*/

static const shvheader ControlPanelSFHeader[] =
{
    { &FMTID_Storage, PID_STG_NAME, IDS_SHV_COLUMN8, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15 },/*FIXME*/
    { NULL, 0, IDS_SHV_COLUMN9, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 80 },/*FIXME*/
};

#define CONROLPANELSHELLVIEWCOLUMNS 2

/**************************************************************************
*	IControlPanel_Constructor
*/
HRESULT WINAPI IControlPanel_Constructor(IUnknown* pUnkOuter, REFIID riid, LPVOID * ppv)
{
    ICPanelImpl *sf;

    TRACE("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid(riid));

    if (!ppv)
	return E_POINTER;
    if (pUnkOuter && !IsEqualIID (riid, &IID_IUnknown))
	return CLASS_E_NOAGGREGATION;

    sf = LocalAlloc(LMEM_ZEROINIT, sizeof(ICPanelImpl));
    if (!sf)
	return E_OUTOFMEMORY;

    sf->ref = 1;
    sf->IShellFolder2_iface.lpVtbl = &vt_ShellFolder2;
    sf->IPersistFolder2_iface.lpVtbl = &vt_PersistFolder2;
    sf->IShellExecuteHookW_iface.lpVtbl = &vt_ShellExecuteHookW;
    sf->IShellExecuteHookA_iface.lpVtbl = &vt_ShellExecuteHookA;
    sf->pidlRoot = _ILCreateControlPanel();	/* my qualified pidl */
    sf->pUnkOuter = pUnkOuter ? pUnkOuter : (IUnknown *)&sf->IShellFolder2_iface;

    if (FAILED(IShellFolder2_QueryInterface(&sf->IShellFolder2_iface, riid, ppv))) {
        IShellFolder2_Release(&sf->IShellFolder2_iface);
	return E_NOINTERFACE;
    }
    IShellFolder2_Release(&sf->IShellFolder2_iface);

    TRACE("--(%p)\n", sf);
    return S_OK;
}

/**************************************************************************
 *	ISF_ControlPanel_fnQueryInterface
 *
 * NOTES supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_ControlPanel_fnQueryInterface(IShellFolder2 *iface, REFIID riid,
        void **ppvObject)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);

    TRACE("(%p)->(%s,%p)\n", This, shdebugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
	IsEqualIID(riid, &IID_IShellFolder) || IsEqualIID(riid, &IID_IShellFolder2))
	*ppvObject = &This->IShellFolder2_iface;
    else if (IsEqualIID(riid, &IID_IPersist) ||
	       IsEqualIID(riid, &IID_IPersistFolder) || IsEqualIID(riid, &IID_IPersistFolder2))
        *ppvObject = &This->IPersistFolder2_iface;
    else if (IsEqualIID(riid, &IID_IShellExecuteHookW))
        *ppvObject = &This->IShellExecuteHookW_iface;
    else if (IsEqualIID(riid, &IID_IShellExecuteHookA))
        *ppvObject = &This->IShellExecuteHookA_iface;

    if (*ppvObject) {
	IUnknown_AddRef((IUnknown *)(*ppvObject));
	TRACE("-- Interface:(%p)->(%p)\n", ppvObject, *ppvObject);
	return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ISF_ControlPanel_fnAddRef(IShellFolder2 *iface)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI ISF_ControlPanel_fnRelease(IShellFolder2 *iface)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, refCount + 1);

    if (!refCount) {
        TRACE("-- destroying IShellFolder(%p)\n", This);
        SHFree(This->pidlRoot);
        LocalFree(This);
    }
    return refCount;
}

/**************************************************************************
*	ISF_ControlPanel_fnParseDisplayName
*/
static HRESULT WINAPI ISF_ControlPanel_fnParseDisplayName(IShellFolder2 *iface, HWND hwndOwner,
        LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten, LPITEMIDLIST *ppidl,
        DWORD *pdwAttributes)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);

    HRESULT hr = E_INVALIDARG;

    FIXME("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
	   This, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName), pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
	*pchEaten = 0;

    TRACE("(%p)->(-- ret=0x%08lx)\n", This, hr);

    return hr;
}

static LPITEMIDLIST _ILCreateCPanelApplet(LPCSTR name, LPCSTR displayName,
 LPCSTR comment, int iconIdx)
{
    PIDLCPanelStruct *p;
    LPITEMIDLIST pidl;
    PIDLDATA tmp;
    int size0 = (char*)tmp.u.cpanel.szName-(char*)&tmp.u.cpanel;
    int size = size0;
    int l;

    tmp.type = PT_CPLAPPLET;
    tmp.u.cpanel.dummy = 0;
    tmp.u.cpanel.iconIdx = iconIdx;

    l = strlen(name);
    size += l+1;

    tmp.u.cpanel.offsDispName = l+1;
    l = strlen(displayName);
    size += l+1;

    tmp.u.cpanel.offsComment = tmp.u.cpanel.offsDispName+1+l;
    l = strlen(comment);
    size += l+1;

    pidl = SHAlloc(size+4);
    if (!pidl)
        return NULL;

    pidl->mkid.cb = size+2;
    memcpy(pidl->mkid.abID, &tmp, 2+size0);

    p = &((PIDLDATA*)pidl->mkid.abID)->u.cpanel;
    memcpy(p->szName, name, strlen(name) + 1);
    memcpy(p->szName+tmp.u.cpanel.offsDispName, displayName, strlen(displayName) + 1);
    memcpy(p->szName+tmp.u.cpanel.offsComment, comment, strlen(comment) + 1);

    *(WORD*)((char*)pidl+(size+2)) = 0;

    pcheck(pidl);

    return pidl;
}

/**************************************************************************
 *  _ILGetCPanelPointer()
 * gets a pointer to the control panel struct stored in the pidl
 */
static PIDLCPanelStruct* _ILGetCPanelPointer(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (pdata && pdata->type==PT_CPLAPPLET)
        return &pdata->u.cpanel;

    return NULL;
}

 /**************************************************************************
 *		ISF_ControlPanel_fnEnumObjects
 */
static BOOL SHELL_RegisterCPanelApp(IEnumIDListImpl *list, LPCSTR path)
{
    LPITEMIDLIST pidl;
    CPlApplet* applet;
    CPanel panel;
    CPLINFO info;
    unsigned i;
    int iconIdx;

    char displayName[MAX_PATH];
    char comment[MAX_PATH];

    WCHAR wpath[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

    list_init( &panel.applets );
    applet = Control_LoadApplet(0, wpath, &panel);

    if (applet)
    {
        for(i=0; i<applet->count; ++i)
        {
            WideCharToMultiByte(CP_ACP, 0, applet->info[i].name, -1, displayName, MAX_PATH, 0, 0);
            WideCharToMultiByte(CP_ACP, 0, applet->info[i].info, -1, comment, MAX_PATH, 0, 0);

            applet->proc(0, CPL_INQUIRE, i, (LPARAM)&info);

            if (info.idIcon > 0)
                iconIdx = -info.idIcon; /* negative icon index instead of icon number */
            else
                iconIdx = 0;

            pidl = _ILCreateCPanelApplet(path, displayName, comment, iconIdx);

            if (pidl)
                AddToEnumList(list, pidl);
        }
        Control_UnloadApplet(applet);
    }
    return TRUE;
}

static int SHELL_RegisterRegistryCPanelApps(IEnumIDListImpl *list, HKEY hkey_root, LPCSTR szRepPath)
{
    char name[MAX_PATH];
    char value[MAX_PATH];
    HKEY hkey;

    int cnt = 0;

    if (RegOpenKeyA(hkey_root, szRepPath, &hkey) == ERROR_SUCCESS)
    {
        int idx = 0;

        for(;; ++idx)
        {
            DWORD nameLen = MAX_PATH;
            DWORD valueLen = MAX_PATH;

            if (RegEnumValueA(hkey, idx, name, &nameLen, NULL, NULL, (LPBYTE)value, &valueLen) != ERROR_SUCCESS)
                break;

            if (SHELL_RegisterCPanelApp(list, value))
                ++cnt;
        }
        RegCloseKey(hkey);
    }

    return cnt;
}

static int SHELL_RegisterCPanelFolders(IEnumIDListImpl *list, HKEY hkey_root, const WCHAR *reg_path)
{
    char name[MAX_PATH];
    HKEY hkey;

    int cnt = 0;

    if (RegOpenKeyW(hkey_root, reg_path, &hkey) == ERROR_SUCCESS)
    {
        int idx = 0;
        for(;; ++idx)
        {
            if (RegEnumKeyA(hkey, idx, name, MAX_PATH) != ERROR_SUCCESS)
                break;

            if (*name == '{')
            {
                LPITEMIDLIST pidl = _ILCreateGuidFromStrA(name);

                if (pidl && AddToEnumList(list, pidl))
                    ++cnt;
            }
        }

        RegCloseKey(hkey);
    }

  return cnt;
}

/**************************************************************************
 *  CreateCPanelEnumList()
 */
static BOOL CreateCPanelEnumList(IEnumIDListImpl *list, DWORD dwFlags)
{
    CHAR szPath[MAX_PATH];
    WIN32_FIND_DATAA wfd;
    HANDLE hFile;

    TRACE("(%p)->(flags=0x%08lx)\n", list, dwFlags);

    /* enumerate control panel folders */
    if (dwFlags & SHCONTF_FOLDERS)
        SHELL_RegisterCPanelFolders(list, HKEY_LOCAL_MACHINE, name_spaceW);

    /* enumerate the control panel applets */
    if (dwFlags & SHCONTF_NONFOLDERS)
    {
        LPSTR p;

        GetSystemDirectoryA(szPath, MAX_PATH);
        p = PathAddBackslashA(szPath);
        strcpy(p, "*.cpl");

        TRACE("-- (%p)-> enumerate SHCONTF_NONFOLDERS of %s\n", list, debugstr_a(szPath));
        hFile = FindFirstFileA(szPath, &wfd);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(dwFlags & SHCONTF_INCLUDEHIDDEN) && (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
                    continue;

                if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    strcpy(p, wfd.cFileName);
                    SHELL_RegisterCPanelApp(list, szPath);
                }
            } while(FindNextFileA(hFile, &wfd));
            FindClose(hFile);
        }

        SHELL_RegisterRegistryCPanelApps(list, HKEY_LOCAL_MACHINE,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls");
        SHELL_RegisterRegistryCPanelApps(list, HKEY_CURRENT_USER,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls");
    }
    return TRUE;
}

/**************************************************************************
*		ISF_ControlPanel_fnEnumObjects
*/
static HRESULT WINAPI ISF_ControlPanel_fnEnumObjects(IShellFolder2 *iface, HWND hwndOwner,
        DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);
    IEnumIDListImpl *list;

    TRACE("(%p)->(HWND=%p flags=0x%08lx pplist=%p)\n", This, hwndOwner, dwFlags, ppEnumIDList);

    if (!(list = IEnumIDList_Constructor()))
        return E_OUTOFMEMORY;
    CreateCPanelEnumList(list, dwFlags);
    *ppEnumIDList = &list->IEnumIDList_iface;

    TRACE("--(%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return S_OK;
}

/**************************************************************************
*		ISF_ControlPanel_fnBindToObject
*/
static HRESULT WINAPI ISF_ControlPanel_fnBindToObject(IShellFolder2 *iface, LPCITEMIDLIST pidl,
        LPBC pbcReserved, REFIID riid, void **ppvOut)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);

    TRACE("(%p)->(pidl=%p,%p,%s,%p)\n", This, pidl, pbcReserved, shdebugstr_guid(riid), ppvOut);

    return SHELL32_BindToChild(This->pidlRoot, &CLSID_ShellFSFolder, NULL, pidl, riid, ppvOut);
}

/**************************************************************************
*	ISF_ControlPanel_fnBindToStorage
*/
static HRESULT WINAPI ISF_ControlPanel_fnBindToStorage(IShellFolder2 *iface, LPCITEMIDLIST pidl,
        LPBC pbcReserved, REFIID riid, void **ppvOut)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);

    FIXME("(%p)->(pidl=%p,%p,%s,%p) stub\n", This, pidl, pbcReserved, shdebugstr_guid(riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
* 	ISF_ControlPanel_fnCompareIDs
*/

static HRESULT WINAPI ISF_ControlPanel_fnCompareIDs(IShellFolder2 *iface, LPARAM lParam,
        LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);

    int nReturn;

    TRACE("(%p)->(0x%08Ix,pidl1=%p,pidl2=%p)\n", This, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs(&This->IShellFolder2_iface, lParam, pidl1, pidl2);
    TRACE("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
*	ISF_ControlPanel_fnCreateViewObject
*/
static HRESULT WINAPI ISF_ControlPanel_fnCreateViewObject(IShellFolder2 *iface, HWND hwndOwner,
        REFIID riid, void **ppvOut)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);

    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(hwnd=%p,%s,%p)\n", This, hwndOwner, shdebugstr_guid(riid), ppvOut);

    if (ppvOut) {
	*ppvOut = NULL;

	if (IsEqualIID(riid, &IID_IDropTarget)) {
	    WARN("IDropTarget not implemented\n");
	    hr = E_NOTIMPL;
	} else if (IsEqualIID(riid, &IID_IShellView)) {
	    pShellView = IShellView_Constructor((IShellFolder *) iface);
	    if (pShellView) {
		hr = IShellView_QueryInterface(pShellView, riid, ppvOut);
		IShellView_Release(pShellView);
	    }
	} else {
	    FIXME("invalid/unsupported interface %s\n", shdebugstr_guid(riid));
	    hr = E_NOINTERFACE;
	}
    }
    TRACE("--(%p)->(interface=%p)\n", This, ppvOut);
    return hr;
}

static BOOL validate_name_space(const ITEMIDLIST *pidl)
{
    HKEY hkey, hitem;
    WCHAR *guidW;
    GUID *guid;
    LSTATUS r;

    if (!_ILIsPidlSimple(pidl))
        return FALSE;

    guid = _ILGetGUIDPointer(pidl);
    if (!guid)
        return FALSE;
    if (StringFromCLSID(guid, &guidW) != S_OK)
        return FALSE;

    r = RegOpenKeyW(HKEY_LOCAL_MACHINE, name_spaceW, &hkey);
    if (r == ERROR_SUCCESS)
    {
        r = RegOpenKeyW(hkey, guidW, &hitem);
        if (r == ERROR_SUCCESS)
            RegCloseKey(hitem);
        RegCloseKey(hkey);
    }
    CoTaskMemFree(guidW);
    return r == ERROR_SUCCESS;
}

/**************************************************************************
*  ISF_ControlPanel_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_ControlPanel_fnGetAttributesOf(IShellFolder2 *iface, UINT cidl,
        LPCITEMIDLIST *apidl, DWORD *rgfInOut)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);

    HRESULT hr = S_OK;

    TRACE("(%p)->(cidl=%d apidl=%p mask=%p (0x%08lx))\n",
          This, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
	*rgfInOut = ~0;

    while(cidl > 0 && *apidl) {
	pdump(*apidl);

        /* TODO: panel with GUID can contain sub-items but we don't support it yet */
        if (!(*rgfInOut & SFGAO_VALIDATE) || _ILGetCPanelPointer(*apidl)
                || validate_name_space(*apidl))
            *rgfInOut &= SFGAO_CANLINK;
        else
            *rgfInOut &= SFGAO_VALIDATE;

	apidl++;
	cidl--;
    }

    TRACE("-- result=0x%08lx\n", *rgfInOut);
    return hr;
}

/**************************************************************************
*	ISF_ControlPanel_fnGetUIObjectOf
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
static HRESULT WINAPI ISF_ControlPanel_fnGetUIObjectOf(IShellFolder2 *iface, HWND hwndOwner,
        UINT cidl, LPCITEMIDLIST *apidl, REFIID riid, UINT *prgfInOut, void **ppvOut)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);

    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
	   This, hwndOwner, cidl, apidl, shdebugstr_guid(riid), prgfInOut, ppvOut);

    if (ppvOut) {
	*ppvOut = NULL;

	if (IsEqualIID(riid, &IID_IContextMenu) &&(cidl >= 1)) {
	    return ItemMenu_Constructor((IShellFolder*)iface, This->pidlRoot, apidl, cidl, riid, ppvOut);
	} else if (IsEqualIID(riid, &IID_IDataObject) &&(cidl >= 1)) {
	    pObj = (LPUNKNOWN) IDataObject_Constructor(hwndOwner, This->pidlRoot, apidl, cidl);
	    hr = S_OK;
	} else if (IsEqualIID(riid, &IID_IExtractIconA) &&(cidl == 1)) {
	    pidl = ILCombine(This->pidlRoot, apidl[0]);
	    pObj = (LPUNKNOWN) IExtractIconA_Constructor(pidl);
	    ILFree(pidl);
	    hr = S_OK;
	} else if (IsEqualIID(riid, &IID_IExtractIconW) &&(cidl == 1)) {
	    pidl = ILCombine(This->pidlRoot, apidl[0]);
	    pObj = (LPUNKNOWN) IExtractIconW_Constructor(pidl);
	    ILFree(pidl);
	    hr = S_OK;
	} else if ((IsEqualIID(riid,&IID_IShellLinkW) || IsEqualIID(riid,&IID_IShellLinkA))
				&& (cidl == 1)) {
	    pidl = ILCombine(This->pidlRoot, apidl[0]);
	    hr = IShellLink_ConstructFromFile(NULL, riid, pidl, &pObj);
	    ILFree(pidl);
	} else {
	    hr = E_NOINTERFACE;
	}

	if (SUCCEEDED(hr) && !pObj)
	    hr = E_OUTOFMEMORY;

	*ppvOut = pObj;
    }
    TRACE("(%p)->hr=0x%08lx\n", This, hr);
    return hr;
}

/**************************************************************************
*	ISF_ControlPanel_fnGetDisplayNameOf
*/
static HRESULT WINAPI ISF_ControlPanel_fnGetDisplayNameOf(IShellFolder2 *iface, LPCITEMIDLIST pidl,
        DWORD dwFlags, LPSTRRET strRet)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);

    CHAR szPath[MAX_PATH];
    WCHAR wszPath[MAX_PATH+1]; /* +1 for potential backslash */
    PIDLCPanelStruct* pcpanel;

    *szPath = '\0';

    TRACE("(%p)->(pidl=%p,0x%08lx,%p)\n", This, pidl, dwFlags, strRet);
    pdump(pidl);

    if (!pidl || !strRet)
	return E_INVALIDARG;

    pcpanel = _ILGetCPanelPointer(pidl);

    if (pcpanel) {
	lstrcpyA(szPath, pcpanel->szName+pcpanel->offsDispName);

	if (!(dwFlags & SHGDN_FORPARSING))
	    FIXME("retrieve display name from control panel app\n");
    }
    /* take names of special folders only if it's only this folder */
    else if (_ILIsSpecialFolder(pidl)) {
	BOOL bSimplePidl = _ILIsPidlSimple(pidl);

	if (bSimplePidl) {
	    _ILSimpleGetTextW(pidl, wszPath, MAX_PATH);	/* append my own path */
	} else {
	    FIXME("special pidl\n");
	}

	if ((dwFlags & SHGDN_FORPARSING) && !bSimplePidl) { /* go deeper if needed */
	    int len = 0;

	    PathAddBackslashW(wszPath);
	    len = lstrlenW(wszPath);

            if (FAILED(SHELL32_GetDisplayNameOfChild(iface, pidl, dwFlags | SHGDN_INFOLDER, wszPath + len, MAX_PATH + 1 - len)))
		return E_OUTOFMEMORY;
	    if (!WideCharToMultiByte(CP_ACP, 0, wszPath, -1, szPath, MAX_PATH, NULL, NULL))
		wszPath[0] = '\0';
	}
    }

    strRet->uType = STRRET_CSTR;
    lstrcpynA(strRet->cStr, szPath, MAX_PATH);

    TRACE("--(%p)->(%s)\n", This, szPath);
    return S_OK;
}

/**************************************************************************
*  ISF_ControlPanel_fnSetNameOf
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
static HRESULT WINAPI ISF_ControlPanel_fnSetNameOf(IShellFolder2 *iface, HWND hwndOwner,
        LPCITEMIDLIST pidl, LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST *pPidlOut)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);
    FIXME("(%p)->(%p,pidl=%p,%s,%lu,%p)\n", This, hwndOwner, pidl, debugstr_w(lpName), dwFlags, pPidlOut);
    return E_FAIL;
}

static HRESULT WINAPI ISF_ControlPanel_fnGetDefaultSearchGUID(IShellFolder2 *iface, GUID *guid)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);
    TRACE("(%p)->(%p)\n", This, guid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_ControlPanel_fnEnumSearches(IShellFolder2 *iface,
        IEnumExtraSearch **ppenum)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_ControlPanel_fnGetDefaultColumn(IShellFolder2 *iface, DWORD reserved,
        ULONG *sort, ULONG *display)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);

    TRACE("(%p)->(%#lx %p %p)\n", This, reserved, sort, display);

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_ControlPanel_fnGetDefaultColumnState(IShellFolder2 *iface, UINT iColumn,
        DWORD *pcsFlags)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);

    TRACE("(%p)\n", This);

    if (!pcsFlags || iColumn >= CONROLPANELSHELLVIEWCOLUMNS) return E_INVALIDARG;
    *pcsFlags = ControlPanelSFHeader[iColumn].pcsFlags;
    return S_OK;
}
static HRESULT WINAPI ISF_ControlPanel_fnGetDetailsEx(IShellFolder2 *iface, LPCITEMIDLIST pidl,
        const SHCOLUMNID *pscid, VARIANT *pv)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_ControlPanel_fnGetDetailsOf(IShellFolder2 *iface, LPCITEMIDLIST pidl,
        UINT iColumn, SHELLDETAILS *psd)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);
    PIDLCPanelStruct* pcpanel;

    TRACE("(%p)->(%p %i %p)\n", This, pidl, iColumn, psd);

    if (!psd || iColumn >= CONROLPANELSHELLVIEWCOLUMNS)
	return E_INVALIDARG;

    if (!pidl)
        return SHELL32_GetColumnDetails(ControlPanelSFHeader, iColumn, psd);

    psd->str.cStr[0] = 0x00;
    psd->str.uType = STRRET_CSTR;
    switch(iColumn)
    {
    case 1:		/* comment */
        pcpanel = _ILGetCPanelPointer(pidl);

        if (pcpanel)
            lstrcpyA(psd->str.cStr, pcpanel->szName+pcpanel->offsComment);
        else
            _ILGetFileType(pidl, psd->str.cStr, MAX_PATH);
        break;

    default:
        return shellfolder_get_file_details( iface, pidl, ControlPanelSFHeader, iColumn, psd );
    }
    return S_OK;
}
static HRESULT WINAPI ISF_ControlPanel_fnMapColumnToSCID(IShellFolder2 *iface, UINT column,
        SHCOLUMNID *pscid)
{
    ICPanelImpl *This = impl_from_IShellFolder2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IShellFolder2Vtbl vt_ShellFolder2 =
{

    ISF_ControlPanel_fnQueryInterface,
    ISF_ControlPanel_fnAddRef,
    ISF_ControlPanel_fnRelease,
    ISF_ControlPanel_fnParseDisplayName,
    ISF_ControlPanel_fnEnumObjects,
    ISF_ControlPanel_fnBindToObject,
    ISF_ControlPanel_fnBindToStorage,
    ISF_ControlPanel_fnCompareIDs,
    ISF_ControlPanel_fnCreateViewObject,
    ISF_ControlPanel_fnGetAttributesOf,
    ISF_ControlPanel_fnGetUIObjectOf,
    ISF_ControlPanel_fnGetDisplayNameOf,
    ISF_ControlPanel_fnSetNameOf,

    /* ShellFolder2 */
    ISF_ControlPanel_fnGetDefaultSearchGUID,
    ISF_ControlPanel_fnEnumSearches,
    ISF_ControlPanel_fnGetDefaultColumn,
    ISF_ControlPanel_fnGetDefaultColumnState,
    ISF_ControlPanel_fnGetDetailsEx,
    ISF_ControlPanel_fnGetDetailsOf,
    ISF_ControlPanel_fnMapColumnToSCID
};

/************************************************************************
 *	ICPanel_PersistFolder2_QueryInterface
 */
static HRESULT WINAPI ICPanel_PersistFolder2_QueryInterface(IPersistFolder2 * iface, REFIID iid, LPVOID * ppvObject)
{
    ICPanelImpl *This = impl_from_IPersistFolder2(iface);

    TRACE("(%p)\n", This);

    return IShellFolder2_QueryInterface(&This->IShellFolder2_iface, iid, ppvObject);
}

/************************************************************************
 *	ICPanel_PersistFolder2_AddRef
 */
static ULONG WINAPI ICPanel_PersistFolder2_AddRef(IPersistFolder2 * iface)
{
    ICPanelImpl *This = impl_from_IPersistFolder2(iface);

    TRACE("(%p)->(count=%lu)\n", This, This->ref);

    return IShellFolder2_AddRef(&This->IShellFolder2_iface);
}

/************************************************************************
 *	ISFPersistFolder_Release
 */
static ULONG WINAPI ICPanel_PersistFolder2_Release(IPersistFolder2 * iface)
{
    ICPanelImpl *This = impl_from_IPersistFolder2(iface);

    TRACE("(%p)->(count=%lu)\n", This, This->ref);

    return IShellFolder2_Release(&This->IShellFolder2_iface);
}

/************************************************************************
 *	ICPanel_PersistFolder2_GetClassID
 */
static HRESULT WINAPI ICPanel_PersistFolder2_GetClassID(IPersistFolder2 * iface, CLSID * lpClassId)
{
    ICPanelImpl *This = impl_from_IPersistFolder2(iface);

    TRACE("(%p)\n", This);

    if (!lpClassId)
	return E_POINTER;
    *lpClassId = CLSID_ControlPanel;

    return S_OK;
}

/************************************************************************
 *	ICPanel_PersistFolder2_Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
static HRESULT WINAPI ICPanel_PersistFolder2_Initialize(IPersistFolder2 * iface, LPCITEMIDLIST pidl)
{
    ICPanelImpl *This = impl_from_IPersistFolder2(iface);
    TRACE("(%p)->(%p)\n", This, pidl);
    return E_NOTIMPL;
}

/**************************************************************************
 *	IPersistFolder2_fnGetCurFolder
 */
static HRESULT WINAPI ICPanel_PersistFolder2_GetCurFolder(IPersistFolder2 * iface, LPITEMIDLIST * pidl)
{
    ICPanelImpl *This = impl_from_IPersistFolder2(iface);

    TRACE("(%p)->(%p)\n", This, pidl);

    if (!pidl)
	return E_POINTER;
    *pidl = ILClone(This->pidlRoot);
    return S_OK;
}

static const IPersistFolder2Vtbl vt_PersistFolder2 =
{

    ICPanel_PersistFolder2_QueryInterface,
    ICPanel_PersistFolder2_AddRef,
    ICPanel_PersistFolder2_Release,
    ICPanel_PersistFolder2_GetClassID,
    ICPanel_PersistFolder2_Initialize,
    ICPanel_PersistFolder2_GetCurFolder
};

HRESULT CPanel_GetIconLocationW(LPCITEMIDLIST pidl,
               LPWSTR szIconFile, UINT cchMax, int* piIndex)
{
    PIDLCPanelStruct* pcpanel = _ILGetCPanelPointer(pidl);

    if (!pcpanel)
	return E_INVALIDARG;

    MultiByteToWideChar(CP_ACP, 0, pcpanel->szName, -1, szIconFile, cchMax);
    *piIndex = pcpanel->iconIdx!=-1? pcpanel->iconIdx: 0;

    return S_OK;
}


/**************************************************************************
* IShellExecuteHookW Implementation
*/

static HRESULT WINAPI IShellExecuteHookW_fnQueryInterface(
               IShellExecuteHookW* iface, REFIID riid, void** ppvObject)
{
    ICPanelImpl *This = impl_from_IShellExecuteHookW(iface);

    TRACE("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_QueryInterface(This->pUnkOuter, riid, ppvObject);
}

static ULONG STDMETHODCALLTYPE IShellExecuteHookW_fnAddRef(IShellExecuteHookW* iface)
{
    ICPanelImpl *This = impl_from_IShellExecuteHookW(iface);

    TRACE("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_AddRef(This->pUnkOuter);
}

static ULONG STDMETHODCALLTYPE IShellExecuteHookW_fnRelease(IShellExecuteHookW* iface)
{
    ICPanelImpl *This = impl_from_IShellExecuteHookW(iface);

    TRACE("(%p)\n", This);

    return IUnknown_Release(This->pUnkOuter);
}

static HRESULT WINAPI IShellExecuteHookW_fnExecute(IShellExecuteHookW *iface,
        LPSHELLEXECUTEINFOW psei)
{
    ICPanelImpl *This = impl_from_IShellExecuteHookW(iface);
    SHELLEXECUTEINFOW sei_tmp;
    PIDLCPanelStruct* pcpanel;
    WCHAR path[MAX_PATH];
    WCHAR params[MAX_PATH];
    BOOL ret;
    int l;

    TRACE("(%p)->execute(%p)\n", This, psei);

    if (!psei)
	return E_INVALIDARG;

    pcpanel = _ILGetCPanelPointer(ILFindLastID(psei->lpIDList));

    if (!pcpanel)
	return E_INVALIDARG;

    path[0] = '\"';
    /* Return value from MultiByteToWideChar includes terminating NUL, which
     * compensates for the starting double quote we just put in */
    l = MultiByteToWideChar(CP_ACP, 0, pcpanel->szName, -1, path+1, MAX_PATH-1);

    /* pass applet name to Control_RunDLL to distinguish between applets in one .cpl file */
    path[l++] = '"';
    path[l] = '\0';

    MultiByteToWideChar(CP_ACP, 0, pcpanel->szName+pcpanel->offsDispName, -1, params, MAX_PATH);

    sei_tmp = *psei;
    sei_tmp.lpFile = path;
    sei_tmp.lpParameters = params;
    sei_tmp.fMask &= ~SEE_MASK_INVOKEIDLIST;
    sei_tmp.lpVerb = L"cplopen";

    ret = ShellExecuteExW(&sei_tmp);
    if (ret)
	return S_OK;
    else
	return S_FALSE;
}

static const IShellExecuteHookWVtbl vt_ShellExecuteHookW =
{

    IShellExecuteHookW_fnQueryInterface,
    IShellExecuteHookW_fnAddRef,
    IShellExecuteHookW_fnRelease,

    IShellExecuteHookW_fnExecute
};


/**************************************************************************
* IShellExecuteHookA Implementation
*/

static HRESULT WINAPI IShellExecuteHookA_fnQueryInterface(IShellExecuteHookA* iface, REFIID riid, void** ppvObject)
{
    ICPanelImpl *This = impl_from_IShellExecuteHookA(iface);

    TRACE("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_QueryInterface(This->pUnkOuter, riid, ppvObject);
}

static ULONG STDMETHODCALLTYPE IShellExecuteHookA_fnAddRef(IShellExecuteHookA* iface)
{
    ICPanelImpl *This = impl_from_IShellExecuteHookA(iface);

    TRACE("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_AddRef(This->pUnkOuter);
}

static ULONG STDMETHODCALLTYPE IShellExecuteHookA_fnRelease(IShellExecuteHookA* iface)
{
    ICPanelImpl *This = impl_from_IShellExecuteHookA(iface);

    TRACE("(%p)\n", This);

    return IUnknown_Release(This->pUnkOuter);
}

static HRESULT WINAPI IShellExecuteHookA_fnExecute(IShellExecuteHookA *iface,
        LPSHELLEXECUTEINFOA psei)
{
    ICPanelImpl *This = impl_from_IShellExecuteHookA(iface);

    SHELLEXECUTEINFOA sei_tmp;
    PIDLCPanelStruct* pcpanel;
    char path[MAX_PATH];
    BOOL ret;

    TRACE("(%p)->execute(%p)\n", This, psei);

    if (!psei)
	return E_INVALIDARG;

    pcpanel = _ILGetCPanelPointer(ILFindLastID(psei->lpIDList));

    if (!pcpanel)
	return E_INVALIDARG;

    path[0] = '\"';
    memcpy(path+1, pcpanel->szName, strlen(pcpanel->szName) + 1);

    /* pass applet name to Control_RunDLL to distinguish between applets in one .cpl file */
    lstrcatA(path, "\" ");
    lstrcatA(path, pcpanel->szName+pcpanel->offsDispName);

    sei_tmp = *psei;
    sei_tmp.lpFile = path;
    sei_tmp.fMask &= ~SEE_MASK_INVOKEIDLIST;

    ret = ShellExecuteExA(&sei_tmp);
    if (ret)
	return S_OK;
    else
	return S_FALSE;
}

static const IShellExecuteHookAVtbl vt_ShellExecuteHookA =
{
    IShellExecuteHookA_fnQueryInterface,
    IShellExecuteHookA_fnAddRef,
    IShellExecuteHookA_fnRelease,
    IShellExecuteHookA_fnExecute
};
