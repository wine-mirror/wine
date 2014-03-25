/*
 *	IContextMenu for items in the shellview
 *
 * Copyright 1998-2000 Juergen Schmied <juergen.schmied@debitel.net>,
 *                                     <juergen.schmied@metronet.de>
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

#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "winerror.h"
#include "wine/debug.h"

#include "windef.h"
#include "wingdi.h"
#include "pidl.h"
#include "undocshell.h"
#include "shlobj.h"
#include "winreg.h"
#include "prsht.h"

#include "shell32_main.h"
#include "shellfolder.h"

#include "shresdef.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct
{
    IContextMenu3 IContextMenu3_iface;
    LONG ref;

    IShellFolder* parent;

    /* item menu data */
    LPITEMIDLIST  pidl;  /* root pidl */
    LPITEMIDLIST *apidl; /* array of child pidls */
    UINT cidl;
    BOOL allvalues;

    /* background menu data */
    BOOL desktop;
} ContextMenu;

static inline ContextMenu *impl_from_IContextMenu3(IContextMenu3 *iface)
{
    return CONTAINING_RECORD(iface, ContextMenu, IContextMenu3_iface);
}

static HRESULT WINAPI ContextMenu_QueryInterface(IContextMenu3 *iface, REFIID riid, LPVOID *ppvObj)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown)      ||
        IsEqualIID(riid, &IID_IContextMenu)  ||
        IsEqualIID(riid, &IID_IContextMenu2) ||
        IsEqualIID(riid, &IID_IContextMenu3))
    {
        *ppvObj = &This->IContextMenu3_iface;
    }
    else if (IsEqualIID(riid, &IID_IShellExtInit))  /*IShellExtInit*/
    {
        FIXME("-- LPSHELLEXTINIT pointer requested\n");
    }

    if(*ppvObj)
    {
        IContextMenu3_AddRef(iface);
        return S_OK;
    }

    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ContextMenu_AddRef(IContextMenu3 *iface)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%u)\n", This, ref);
    return ref;
}

static ULONG WINAPI ContextMenu_Release(IContextMenu3 *iface)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n", This, ref);

    if (!ref)
    {
        if(This->parent)
            IShellFolder_Release(This->parent);

        SHFree(This->pidl);
        _ILFreeaPidl(This->apidl, This->cidl);

        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI ItemMenu_QueryContextMenu(
	IContextMenu3 *iface,
	HMENU hmenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    INT uIDMax;

    TRACE("(%p)->(%p %d 0x%x 0x%x 0x%x )\n", This, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if(!(CMF_DEFAULTONLY & uFlags) && This->cidl > 0)
    {
        HMENU hmenures = LoadMenuW(shell32_hInstance, MAKEINTRESOURCEW(MENU_SHV_FILE));

        if(uFlags & CMF_EXPLORE)
            RemoveMenu(hmenures, FCIDM_SHVIEW_OPEN, MF_BYCOMMAND);

        uIDMax = Shell_MergeMenus(hmenu, GetSubMenu(hmenures, 0), indexMenu, idCmdFirst, idCmdLast, MM_SUBMENUSHAVEIDS);

        DestroyMenu(hmenures);

        if(This->allvalues)
        {
            MENUITEMINFOW mi;
            WCHAR str[255];
            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
            mi.dwTypeData = str;
            mi.cch = 255;
            GetMenuItemInfoW(hmenu, FCIDM_SHVIEW_EXPLORE, MF_BYCOMMAND, &mi);
            RemoveMenu(hmenu, FCIDM_SHVIEW_EXPLORE + idCmdFirst, MF_BYCOMMAND);

            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_STRING;
            mi.dwTypeData = str;
            mi.fState = MFS_ENABLED;
            mi.wID = FCIDM_SHVIEW_EXPLORE;
            mi.fType = MFT_STRING;
            InsertMenuItemW(hmenu, (uFlags & CMF_EXPLORE) ? 1 : 2, MF_BYPOSITION, &mi);
        }

        SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);

        if(uFlags & ~CMF_CANRENAME)
            RemoveMenu(hmenu, FCIDM_SHVIEW_RENAME, MF_BYCOMMAND);
        else
        {
            UINT enable = MF_BYCOMMAND;

            /* can't rename more than one item at a time*/
            if (!This->apidl || This->cidl > 1)
                enable |= MFS_DISABLED;
            else
            {
                DWORD attr = SFGAO_CANRENAME;

                IShellFolder_GetAttributesOf(This->parent, 1, (LPCITEMIDLIST*)This->apidl, &attr);
                enable |= (attr & SFGAO_CANRENAME) ? MFS_ENABLED : MFS_DISABLED;
            }

            EnableMenuItem(hmenu, FCIDM_SHVIEW_RENAME, enable);
        }

        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, uIDMax-idCmdFirst);
    }
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
}

/**************************************************************************
* DoOpenExplore
*
*  for folders only
*/

static void DoOpenExplore(ContextMenu *This, HWND hwnd, LPCSTR verb)
{
        UINT i;
        BOOL bFolderFound = FALSE;
	LPITEMIDLIST	pidlFQ;
	SHELLEXECUTEINFOA	sei;

	/* Find the first item in the list that is not a value. These commands
	    should never be invoked if there isn't at least one folder item in the list.*/

	for(i = 0; i<This->cidl; i++)
	{
	  if(!_ILIsValue(This->apidl[i]))
	  {
	    bFolderFound = TRUE;
	    break;
	  }
	}

	if (!bFolderFound) return;

	pidlFQ = ILCombine(This->pidl, This->apidl[i]);

	ZeroMemory(&sei, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
	sei.lpIDList = pidlFQ;
	sei.lpClass = "Folder";
	sei.hwnd = hwnd;
	sei.nShow = SW_SHOWNORMAL;
	sei.lpVerb = verb;
	ShellExecuteExA(&sei);
	SHFree(pidlFQ);
}

/**************************************************************************
 * DoDelete
 *
 * deletes the currently selected items
 */
static void DoDelete(ContextMenu *This)
{
    ISFHelper *helper;

    IShellFolder_QueryInterface(This->parent, &IID_ISFHelper, (void**)&helper);
    if (helper)
    {
        ISFHelper_DeleteItems(helper, This->cidl, (LPCITEMIDLIST*)This->apidl);
        ISFHelper_Release(helper);
    }
}

/**************************************************************************
 * DoCopyOrCut
 *
 * copies the currently selected items into the clipboard
 */
static void DoCopyOrCut(ContextMenu *This, HWND hwnd, BOOL cut)
{
    IDataObject *dataobject;

    TRACE("(%p)->(wnd=%p, cut=%d)\n", This, hwnd, cut);

    if (SUCCEEDED(IShellFolder_GetUIObjectOf(This->parent, hwnd, This->cidl, (LPCITEMIDLIST*)This->apidl, &IID_IDataObject, 0, (void**)&dataobject)))
    {
        OleSetClipboard(dataobject);
        IDataObject_Release(dataobject);
    }
}

/**************************************************************************
 * Properties_AddPropSheetCallback
 *
 * Used by DoOpenProperties through SHCreatePropSheetExtArrayEx to add
 * propertysheet pages from shell extensions.
 */
static BOOL CALLBACK Properties_AddPropSheetCallback(HPROPSHEETPAGE hpage, LPARAM lparam)
{
	LPPROPSHEETHEADERW psh = (LPPROPSHEETHEADERW) lparam;
	psh->u3.phpage[psh->nPages++] = hpage;

	return TRUE;
}

#define MAX_PROP_PAGES 99

static void DoOpenProperties(ContextMenu *This, HWND hwnd)
{
	static const WCHAR wszFolder[] = {'F','o','l','d','e','r', 0};
	static const WCHAR wszFiletypeAll[] = {'*',0};
	LPSHELLFOLDER lpDesktopSF;
	LPSHELLFOLDER lpSF;
	LPDATAOBJECT lpDo;
	WCHAR wszFiletype[MAX_PATH];
	WCHAR wszFilename[MAX_PATH];
	PROPSHEETHEADERW psh;
	HPROPSHEETPAGE hpages[MAX_PROP_PAGES];
	HPSXA hpsxa;
	UINT ret;

	TRACE("(%p)->(wnd=%p)\n", This, hwnd);

	ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
	psh.dwSize = sizeof (PROPSHEETHEADERW);
	psh.hwndParent = hwnd;
	psh.dwFlags = PSH_PROPTITLE;
	psh.nPages = 0;
	psh.u3.phpage = hpages;
	psh.u2.nStartPage = 0;

	_ILSimpleGetTextW(This->apidl[0], (LPVOID)wszFilename, MAX_PATH);
	psh.pszCaption = (LPCWSTR)wszFilename;

	/* Find out where to look for the shell extensions */
	if (_ILIsValue(This->apidl[0]))
	{
	    char sTemp[64];
	    sTemp[0] = 0;
	    if (_ILGetExtension(This->apidl[0], sTemp, 64))
	    {
		HCR_MapTypeToValueA(sTemp, sTemp, 64, TRUE);
		MultiByteToWideChar(CP_ACP, 0, sTemp, -1, wszFiletype, MAX_PATH);
	    }
	    else
	    {
		wszFiletype[0] = 0;
	    }
	}
	else if (_ILIsFolder(This->apidl[0]))
	{
	    lstrcpynW(wszFiletype, wszFolder, 64);
	}
	else if (_ILIsSpecialFolder(This->apidl[0]))
	{
	    LPGUID folderGUID;
	    static const WCHAR wszclsid[] = {'C','L','S','I','D','\\', 0};
	    folderGUID = _ILGetGUIDPointer(This->apidl[0]);
	    lstrcpyW(wszFiletype, wszclsid);
	    StringFromGUID2(folderGUID, &wszFiletype[6], MAX_PATH - 6);
	}
	else
	{
	    FIXME("Requested properties for unknown type.\n");
	    return;
	}

	/* Get a suitable DataObject for accessing the files */
	SHGetDesktopFolder(&lpDesktopSF);
	if (_ILIsPidlSimple(This->pidl))
	{
	    ret = IShellFolder_GetUIObjectOf(lpDesktopSF, hwnd, This->cidl, (LPCITEMIDLIST*)This->apidl,
					     &IID_IDataObject, NULL, (LPVOID *)&lpDo);
	    IShellFolder_Release(lpDesktopSF);
	}
	else
	{
	    IShellFolder_BindToObject(lpDesktopSF, This->pidl, NULL, &IID_IShellFolder, (LPVOID*) &lpSF);
	    ret = IShellFolder_GetUIObjectOf(lpSF, hwnd, This->cidl, (LPCITEMIDLIST*)This->apidl,
					     &IID_IDataObject, NULL, (LPVOID *)&lpDo);
	    IShellFolder_Release(lpSF);
	    IShellFolder_Release(lpDesktopSF);
	}

	if (SUCCEEDED(ret))
	{
	    hpsxa = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, wszFiletype, MAX_PROP_PAGES - psh.nPages, lpDo);
	    if (hpsxa != NULL)
	    {
		SHAddFromPropSheetExtArray(hpsxa, Properties_AddPropSheetCallback, (LPARAM)&psh);
		SHDestroyPropSheetExtArray(hpsxa);
	    }
	    hpsxa = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, wszFiletypeAll, MAX_PROP_PAGES - psh.nPages, lpDo);
	    if (hpsxa != NULL)
	    {
		SHAddFromPropSheetExtArray(hpsxa, Properties_AddPropSheetCallback, (LPARAM)&psh);
		SHDestroyPropSheetExtArray(hpsxa);
	    }
	    IDataObject_Release(lpDo);
	}

	if (psh.nPages)
	    PropertySheetW(&psh);
	else
	    FIXME("No property pages found.\n");
}

static HRESULT WINAPI ItemMenu_InvokeCommand(
	IContextMenu3 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);

    if (lpcmi->cbSize != sizeof(CMINVOKECOMMANDINFO))
        FIXME("Is an EX structure\n");

    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);

    if( HIWORD(lpcmi->lpVerb)==0 && LOWORD(lpcmi->lpVerb) > FCIDM_SHVIEWLAST)
    {
        TRACE("Invalid Verb %x\n",LOWORD(lpcmi->lpVerb));
        return E_INVALIDARG;
    }

    if (HIWORD(lpcmi->lpVerb) == 0)
    {
        switch(LOWORD(lpcmi->lpVerb))
        {
        case FCIDM_SHVIEW_EXPLORE:
            TRACE("Verb FCIDM_SHVIEW_EXPLORE\n");
            DoOpenExplore(This, lpcmi->hwnd, "explore");
            break;
        case FCIDM_SHVIEW_OPEN:
            TRACE("Verb FCIDM_SHVIEW_OPEN\n");
            DoOpenExplore(This, lpcmi->hwnd, "open");
            break;
        case FCIDM_SHVIEW_RENAME:
        {
            IShellBrowser *browser;

            /* get the active IShellView */
            browser = (IShellBrowser*)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER, 0, 0);
            if (browser)
            {
                IShellView *view;

                if(SUCCEEDED(IShellBrowser_QueryActiveShellView(browser, &view)))
                {
                    TRACE("(shellview=%p)\n", view);
                    IShellView_SelectItem(view, This->apidl[0],
                         SVSI_DESELECTOTHERS|SVSI_EDIT|SVSI_ENSUREVISIBLE|SVSI_FOCUSED|SVSI_SELECT);
                    IShellView_Release(view);
                }
            }
            break;
        }
        case FCIDM_SHVIEW_DELETE:
            TRACE("Verb FCIDM_SHVIEW_DELETE\n");
            DoDelete(This);
            break;
        case FCIDM_SHVIEW_COPY:
            TRACE("Verb FCIDM_SHVIEW_COPY\n");
            DoCopyOrCut(This, lpcmi->hwnd, FALSE);
            break;
        case FCIDM_SHVIEW_CUT:
            TRACE("Verb FCIDM_SHVIEW_CUT\n");
            DoCopyOrCut(This, lpcmi->hwnd, TRUE);
            break;
        case FCIDM_SHVIEW_PROPERTIES:
            TRACE("Verb FCIDM_SHVIEW_PROPERTIES\n");
            DoOpenProperties(This, lpcmi->hwnd);
            break;
        default:
            FIXME("Unhandled Verb %xl\n",LOWORD(lpcmi->lpVerb));
            return E_INVALIDARG;
        }
    }
    else
    {
        TRACE("Verb is %s\n",debugstr_a(lpcmi->lpVerb));
        if (strcmp(lpcmi->lpVerb,"delete")==0)
            DoDelete(This);
        else if (strcmp(lpcmi->lpVerb,"properties")==0)
            DoOpenProperties(This, lpcmi->hwnd);
        else {
            FIXME("Unhandled string verb %s\n",debugstr_a(lpcmi->lpVerb));
            return E_FAIL;
        }
    }
    return S_OK;
}

static HRESULT WINAPI ItemMenu_GetCommandString(
	IContextMenu3 *iface,
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
	ContextMenu *This = impl_from_IContextMenu3(iface);
	HRESULT hr = E_INVALIDARG;

	TRACE("(%p)->(%lx flags=%x %p name=%p len=%x)\n", This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	switch(uFlags)
	{
	  case GCS_HELPTEXTA:
	  case GCS_HELPTEXTW:
	    hr = E_NOTIMPL;
	    break;

	  case GCS_VERBA:
	    switch(idCommand)
	    {
            case FCIDM_SHVIEW_OPEN:
                strcpy(lpszName, "open");
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_EXPLORE:
                strcpy(lpszName, "explore");
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_CUT:
                strcpy(lpszName, "cut");
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_COPY:
                strcpy(lpszName, "copy");
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_CREATELINK:
                strcpy(lpszName, "link");
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_DELETE:
                strcpy(lpszName, "delete");
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_PROPERTIES:
                strcpy(lpszName, "properties");
                hr = S_OK;
                break;
	    case FCIDM_SHVIEW_RENAME:
	        strcpy(lpszName, "rename");
	        hr = S_OK;
	        break;
	    }
	    break;

	     /* NT 4.0 with IE 3.0x or no IE will always call This with GCS_VERBW. In This
	     case, you need to do the lstrcpyW to the pointer passed.*/
	  case GCS_VERBW:
	    switch(idCommand)
	    {
            case FCIDM_SHVIEW_OPEN:
                MultiByteToWideChar(CP_ACP, 0, "open", -1, (LPWSTR)lpszName, uMaxNameLen);
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_EXPLORE:
                MultiByteToWideChar(CP_ACP, 0, "explore", -1, (LPWSTR)lpszName, uMaxNameLen);
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_CUT:
                MultiByteToWideChar(CP_ACP, 0, "cut", -1, (LPWSTR)lpszName, uMaxNameLen);
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_COPY:
                MultiByteToWideChar(CP_ACP, 0, "copy", -1, (LPWSTR)lpszName, uMaxNameLen);
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_CREATELINK:
                MultiByteToWideChar(CP_ACP, 0, "link", -1, (LPWSTR)lpszName, uMaxNameLen);
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_DELETE:
                MultiByteToWideChar(CP_ACP, 0, "delete", -1, (LPWSTR)lpszName, uMaxNameLen);
                hr = S_OK;
                break;
            case FCIDM_SHVIEW_PROPERTIES:
                MultiByteToWideChar(CP_ACP, 0, "properties", -1, (LPWSTR)lpszName, uMaxNameLen);
                hr = S_OK;
                break;
	    case FCIDM_SHVIEW_RENAME:
                MultiByteToWideChar( CP_ACP, 0, "rename", -1, (LPWSTR)lpszName, uMaxNameLen );
	        hr = S_OK;
	        break;
	    }
	    break;

	  case GCS_VALIDATEA:
	  case GCS_VALIDATEW:
	    hr = S_OK;
	    break;
	}
	TRACE("-- (%p)->(name=%s)\n", This, lpszName);
	return hr;
}

/**************************************************************************
* NOTES
*  should be only in IContextMenu2 and IContextMenu3
*  is nevertheless called from word95
*/
static HRESULT WINAPI ContextMenu_HandleMenuMsg(IContextMenu3 *iface, UINT msg,
    WPARAM wParam, LPARAM lParam)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    FIXME("(%p)->(0x%x 0x%lx 0x%lx): stub\n", This, msg, wParam, lParam);
    return E_NOTIMPL;
}

static HRESULT WINAPI ContextMenu_HandleMenuMsg2(IContextMenu3 *iface, UINT msg,
    WPARAM wParam, LPARAM lParam, LRESULT *result)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    FIXME("(%p)->(0x%x 0x%lx 0x%lx %p): stub\n", This, msg, wParam, lParam, result);
    return E_NOTIMPL;
}

static const IContextMenu3Vtbl ItemContextMenuVtbl =
{
    ContextMenu_QueryInterface,
    ContextMenu_AddRef,
    ContextMenu_Release,
    ItemMenu_QueryContextMenu,
    ItemMenu_InvokeCommand,
    ItemMenu_GetCommandString,
    ContextMenu_HandleMenuMsg,
    ContextMenu_HandleMenuMsg2
};

HRESULT ItemMenu_Constructor(IShellFolder *parent, LPCITEMIDLIST pidl, const LPCITEMIDLIST *apidl, UINT cidl,
    REFIID riid, void **pObj)
{
    ContextMenu* This;
    HRESULT hr;
    UINT i;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IContextMenu3_iface.lpVtbl = &ItemContextMenuVtbl;
    This->ref = 1;
    This->parent = parent;
    if (parent) IShellFolder_AddRef(parent);

    This->pidl = ILClone(pidl);
    This->apidl = _ILCopyaPidl(apidl, cidl);
    This->cidl = cidl;
    This->allvalues = TRUE;

    This->desktop = FALSE;

    for (i = 0; i < cidl; i++)
       This->allvalues &= (_ILIsValue(apidl[i]) ? 1 : 0);

    hr = IContextMenu3_QueryInterface(&This->IContextMenu3_iface, riid, pObj);
    IContextMenu3_Release(&This->IContextMenu3_iface);

    return hr;
}

/* Background menu implementation */
static HRESULT WINAPI BackgroundMenu_QueryContextMenu(
	IContextMenu3 *iface,
	HMENU hMenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    HMENU hMyMenu;
    UINT idMax;
    HRESULT hr;

    TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",
          This, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    hMyMenu = LoadMenuA(shell32_hInstance, "MENU_002");
    if (uFlags & CMF_DEFAULTONLY)
    {
        HMENU ourMenu = GetSubMenu(hMyMenu,0);
        UINT oldDef = GetMenuDefaultItem(hMenu,TRUE,GMDI_USEDISABLED);
        UINT newDef = GetMenuDefaultItem(ourMenu,TRUE,GMDI_USEDISABLED);
        if (newDef != oldDef)
            SetMenuDefaultItem(hMenu,newDef,TRUE);
        if (newDef!=0xFFFFFFFF)
            hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, newDef+1);
        else
            hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    }
    else
    {
        idMax = Shell_MergeMenus (hMenu, GetSubMenu(hMyMenu,0), indexMenu,
                                  idCmdFirst, idCmdLast, MM_SUBMENUSHAVEIDS);
        hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, idMax-idCmdFirst);
    }
    DestroyMenu(hMyMenu);

    TRACE("(%p)->returning 0x%x\n",This,hr);
    return hr;
}

static void DoNewFolder(ContextMenu *This, IShellView *view)
{
    ISFHelper *helper;

    IShellFolder_QueryInterface(This->parent, &IID_ISFHelper, (void**)&helper);
    if (helper)
    {
        WCHAR nameW[MAX_PATH];
        LPITEMIDLIST pidl;

        ISFHelper_GetUniqueName(helper, nameW, MAX_PATH);
        ISFHelper_AddFolder(helper, 0, nameW, &pidl);

        if (view)
        {
	    /* if we are in a shellview do labeledit */
	    IShellView_SelectItem(view,
                    pidl,(SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE
                    |SVSI_FOCUSED|SVSI_SELECT));
        }

        SHFree(pidl);
        ISFHelper_Release(helper);
    }
}

static BOOL DoPaste(ContextMenu *This)
{
	BOOL bSuccess = FALSE;
	IDataObject * pda;

	TRACE("\n");

	if(SUCCEEDED(OleGetClipboard(&pda)))
	{
	  STGMEDIUM medium;
	  FORMATETC formatetc;

	  TRACE("pda=%p\n", pda);

	  /* Set the FORMATETC structure*/
	  InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLISTW), TYMED_HGLOBAL);

	  /* Get the pidls from IDataObject */
	  if(SUCCEEDED(IDataObject_GetData(pda,&formatetc,&medium)))
          {
	    LPITEMIDLIST * apidl;
	    LPITEMIDLIST pidl;
	    IShellFolder *psfFrom = NULL, *psfDesktop;

	    LPIDA lpcida = GlobalLock(medium.u.hGlobal);
	    TRACE("cida=%p\n", lpcida);

	    apidl = _ILCopyCidaToaPidl(&pidl, lpcida);

	    /* bind to the source shellfolder */
	    SHGetDesktopFolder(&psfDesktop);
	    if(psfDesktop)
	    {
	      IShellFolder_BindToObject(psfDesktop, pidl, NULL, &IID_IShellFolder, (LPVOID*)&psfFrom);
	      IShellFolder_Release(psfDesktop);
	    }

	    if (psfFrom)
	    {
	      /* get source and destination shellfolder */
	      ISFHelper *psfhlpdst, *psfhlpsrc;
	      IShellFolder_QueryInterface(This->parent, &IID_ISFHelper, (void**)&psfhlpdst);
	      IShellFolder_QueryInterface(psfFrom, &IID_ISFHelper, (void**)&psfhlpsrc);

	      /* do the copy/move */
	      if (psfhlpdst && psfhlpsrc)
	      {
	        ISFHelper_CopyItems(psfhlpdst, psfFrom, lpcida->cidl, (LPCITEMIDLIST*)apidl);
		/* FIXME handle move
		ISFHelper_DeleteItems(psfhlpsrc, lpcida->cidl, apidl);
		*/
	      }
	      if(psfhlpdst) ISFHelper_Release(psfhlpdst);
	      if(psfhlpsrc) ISFHelper_Release(psfhlpsrc);
	      IShellFolder_Release(psfFrom);
	    }

	    _ILFreeaPidl(apidl, lpcida->cidl);
	    SHFree(pidl);

	    /* release the medium*/
	    ReleaseStgMedium(&medium);
	  }
	  IDataObject_Release(pda);
	}
#if 0
	HGLOBAL  hMem;

	OpenClipboard(NULL);
	hMem = GetClipboardData(CF_HDROP);

	if(hMem)
	{
          char * pDropFiles = GlobalLock(hMem);
	  if(pDropFiles)
	  {
	    int len, offset = sizeof(DROPFILESTRUCT);

	    while( pDropFiles[offset] != 0)
	    {
	      len = strlen(pDropFiles + offset);
	      TRACE("%s\n", pDropFiles + offset);
	      offset += len+1;
	    }
	  }
	  GlobalUnlock(hMem);
	}
	CloseClipboard();
#endif
	return bSuccess;
}

static HRESULT WINAPI BackgroundMenu_InvokeCommand(
	IContextMenu3 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    IShellBrowser *browser;
    IShellView *view = NULL;
    HWND hWnd = NULL;

    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n", This, lpcmi, lpcmi->lpVerb, lpcmi->hwnd);

    /* get the active IShellView */
    if ((browser = (IShellBrowser*)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER, 0, 0)))
    {
        if (SUCCEEDED(IShellBrowser_QueryActiveShellView(browser, &view)))
	    IShellView_GetWindow(view, &hWnd);
    }

    if(HIWORD(lpcmi->lpVerb))
    {
        TRACE("%s\n", debugstr_a(lpcmi->lpVerb));

        if (!strcmp(lpcmi->lpVerb, CMDSTR_NEWFOLDERA))
        {
            DoNewFolder(This, view);
        }
        else if (!strcmp(lpcmi->lpVerb, CMDSTR_VIEWLISTA))
        {
            if (hWnd) SendMessageA(hWnd, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_LISTVIEW, 0), 0);
        }
        else if (!strcmp(lpcmi->lpVerb, CMDSTR_VIEWDETAILSA))
        {
	    if (hWnd) SendMessageA(hWnd, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_REPORTVIEW, 0), 0);
        }
        else
        {
            FIXME("please report: unknown verb %s\n", debugstr_a(lpcmi->lpVerb));
        }
    }
    else
    {
        switch (LOWORD(lpcmi->lpVerb))
        {
	    case FCIDM_SHVIEW_REFRESH:
	        if (view) IShellView_Refresh(view);
                break;

            case FCIDM_SHVIEW_NEWFOLDER:
                DoNewFolder(This, view);
                break;

            case FCIDM_SHVIEW_INSERT:
                DoPaste(This);
                break;

            case FCIDM_SHVIEW_PROPERTIES:
                if (This->desktop) {
		    ShellExecuteA(lpcmi->hwnd, "open", "rundll32.exe shell32.dll,Control_RunDLL desk.cpl", NULL, NULL, SW_SHOWNORMAL);
		} else {
		    FIXME("launch item properties dialog\n");
		}
		break;

            default:
                /* if it's an id just pass it to the parent shv */
                if (hWnd) SendMessageA(hWnd, WM_COMMAND, MAKEWPARAM(LOWORD(lpcmi->lpVerb), 0), 0);
                break;
         }
    }

    if (view)
        IShellView_Release(view);

    return S_OK;
}

static HRESULT WINAPI BackgroundMenu_GetCommandString(
	IContextMenu3 *iface,
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
        ContextMenu *This = impl_from_IContextMenu3(iface);

	TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	/* test the existence of the menu items, the file dialog enables
	   the buttons according to this */
	if (uFlags == GCS_VALIDATEA)
	{
	  if(HIWORD(idCommand))
	  {
	    if (!strcmp((LPSTR)idCommand, CMDSTR_VIEWLISTA) ||
	        !strcmp((LPSTR)idCommand, CMDSTR_VIEWDETAILSA) ||
	        !strcmp((LPSTR)idCommand, CMDSTR_NEWFOLDERA))
	    {
	      return S_OK;
	    }
	  }
	}

	FIXME("unknown command string\n");
	return E_FAIL;
}

static const IContextMenu3Vtbl BackgroundContextMenuVtbl =
{
    ContextMenu_QueryInterface,
    ContextMenu_AddRef,
    ContextMenu_Release,
    BackgroundMenu_QueryContextMenu,
    BackgroundMenu_InvokeCommand,
    BackgroundMenu_GetCommandString,
    ContextMenu_HandleMenuMsg,
    ContextMenu_HandleMenuMsg2
};

HRESULT BackgroundMenu_Constructor(IShellFolder *parent, BOOL desktop, REFIID riid, void **pObj)
{
    ContextMenu *This;
    HRESULT hr;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IContextMenu3_iface.lpVtbl = &BackgroundContextMenuVtbl;
    This->ref = 1;
    This->parent = parent;

    This->pidl = NULL;
    This->apidl = NULL;
    This->cidl = 0;
    This->allvalues = FALSE;

    This->desktop = desktop;
    if (parent) IShellFolder_AddRef(parent);

    hr = IContextMenu3_QueryInterface(&This->IContextMenu3_iface, riid, pObj);
    IContextMenu3_Release(&This->IContextMenu3_iface);

    return hr;
}
