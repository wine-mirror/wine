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
#include "winerror.h"

#include "windef.h"
#include "wingdi.h"
#include "pidl.h"
#include "shlobj.h"
#include "winreg.h"
#include "prsht.h"

#include "shell32_main.h"
#include "shellfolder.h"

#include "shresdef.h"
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define FCIDM_BASE 0x7000

typedef struct
{
    IContextMenu3 IContextMenu3_iface;
    IShellExtInit IShellExtInit_iface;
    IObjectWithSite IObjectWithSite_iface;
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

static inline ContextMenu *impl_from_IShellExtInit(IShellExtInit *iface)
{
    return CONTAINING_RECORD(iface, ContextMenu, IShellExtInit_iface);
}

static inline ContextMenu *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, ContextMenu, IObjectWithSite_iface);
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
    else if (IsEqualIID(riid, &IID_IShellExtInit))
    {
        *ppvObj = &This->IShellExtInit_iface;
    }
    else if (IsEqualIID(riid, &IID_IObjectWithSite))
    {
        *ppvObj = &This->IObjectWithSite_iface;
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
    TRACE("(%p)->(%lu)\n", This, ref);
    return ref;
}

static ULONG WINAPI ContextMenu_Release(IContextMenu3 *iface)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, ref);

    if (!ref)
    {
        if(This->parent)
            IShellFolder_Release(This->parent);

        SHFree(This->pidl);
        _ILFreeaPidl(This->apidl, This->cidl);

        free(This);
    }

    return ref;
}

static UINT max_menu_id(HMENU hmenu, UINT offset, UINT last)
{
    int i;
    UINT max_id = 0;

    for (i = GetMenuItemCount(hmenu) - 1; i >= 0; i--)
    {
        MENUITEMINFOW item;
        memset(&item, 0, sizeof(MENUITEMINFOW));
        item.cbSize = sizeof(MENUITEMINFOW);
        item.fMask =  MIIM_ID | MIIM_SUBMENU | MIIM_TYPE;
        if (!GetMenuItemInfoW(hmenu, i, TRUE, &item))
            continue;
        if (!(item.fType & MFT_SEPARATOR))
        {
            if (item.hSubMenu)
            {
                UINT submenu_max_id = max_menu_id(item.hSubMenu, offset, last);
                if (max_id < submenu_max_id)
                    max_id = submenu_max_id;
            }
            if (item.wID + offset <= last)
            {
                if (max_id <= item.wID + offset)
                    max_id = item.wID + offset + 1;
            }
        }
    }
    return max_id;
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

        Shell_MergeMenus(hmenu, GetSubMenu(hmenures, 0), indexMenu, idCmdFirst - FCIDM_BASE, idCmdLast, MM_SUBMENUSHAVEIDS);
        uIDMax = max_menu_id(GetSubMenu(hmenures, 0), idCmdFirst - FCIDM_BASE, idCmdLast);

        DestroyMenu(hmenures);

        if(This->allvalues)
        {
            MENUITEMINFOW mi;
            WCHAR str[255];
            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
            mi.dwTypeData = str;
            mi.cch = 255;
            GetMenuItemInfoW(hmenu, FCIDM_SHVIEW_EXPLORE - FCIDM_BASE + idCmdFirst, MF_BYCOMMAND, &mi);
            RemoveMenu(hmenu, FCIDM_SHVIEW_EXPLORE - FCIDM_BASE + idCmdFirst, MF_BYCOMMAND);

            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_STRING;
            mi.dwTypeData = str;
            mi.fState = MFS_ENABLED;
            mi.wID = FCIDM_SHVIEW_EXPLORE - FCIDM_BASE + idCmdFirst;
            mi.fType = MFT_STRING;
            InsertMenuItemW(hmenu, (uFlags & CMF_EXPLORE) ? 1 : 2, MF_BYPOSITION, &mi);
        }

        SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);

        if(uFlags & ~CMF_CANRENAME)
            RemoveMenu(hmenu, FCIDM_SHVIEW_RENAME - FCIDM_BASE + idCmdFirst, MF_BYCOMMAND);
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

            EnableMenuItem(hmenu, FCIDM_SHVIEW_RENAME - FCIDM_BASE + idCmdFirst, enable);
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
	ILFree(pidlFQ);
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
	psh->phpage[psh->nPages++] = hpage;

	return TRUE;
}

static BOOL format_date(FILETIME *time, WCHAR *buffer, DWORD size)
{
    FILETIME ft;
    SYSTEMTIME st;
    int ret;

    if (!FileTimeToLocalFileTime(time, &ft))
        return FALSE;

    if (!FileTimeToSystemTime(&ft, &st))
        return FALSE;

    ret = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, buffer, size);
    if (ret)
    {
        buffer[ret - 1] = ' ';
        ret = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, buffer + ret , size - ret);
    }
    return ret != 0;
}

static BOOL get_program_description(WCHAR *path, WCHAR *buffer, DWORD size)
{
    WCHAR fileDescW[41], *desc;
    DWORD versize, *lang;
    UINT dlen, llen, i;
    BOOL ret = FALSE;
    PVOID data;

    versize = GetFileVersionInfoSizeW(path, NULL);
    if (!versize) return FALSE;

    data = malloc(versize);
    if (!data) return FALSE;

    if (!GetFileVersionInfoW(path, 0, versize, data))
        goto out;

    if (!VerQueryValueW(data, L"\\VarFileInfo\\Translation", (LPVOID *)&lang, &llen))
        goto out;

    for (i = 0; i < llen / sizeof(DWORD); i++)
    {
        swprintf(fileDescW, ARRAY_SIZE(fileDescW), L"\\StringFileInfo\\%04x%04x\\FileDescription",
                 LOWORD(lang[i]), HIWORD(lang[i]));
        if (VerQueryValueW(data, fileDescW, (LPVOID *)&desc, &dlen))
        {
            if (dlen > size - 1) dlen = size - 1;
            memcpy(buffer, desc, dlen * sizeof(WCHAR));
            buffer[dlen] = 0;
            ret = TRUE;
            break;
        }
    }

out:
    free(data);
    return ret;
}

struct file_properties_info
{
    WCHAR path[MAX_PATH];
    WCHAR dir[MAX_PATH];
    WCHAR *filename;
    DWORD attrib;
};

static void init_file_properties_dlg(HWND hwndDlg, struct file_properties_info *props)
{
    WCHAR buffer[MAX_PATH], buffer2[MAX_PATH];
    WIN32_FILE_ATTRIBUTE_DATA exinfo;
    SHFILEINFOW shinfo;

    SetDlgItemTextW(hwndDlg, IDC_FPROP_PATH, props->filename);
    SetDlgItemTextW(hwndDlg, IDC_FPROP_LOCATION, props->dir);

    if (SHGetFileInfoW(props->path, 0, &shinfo, sizeof(shinfo), SHGFI_TYPENAME|SHGFI_ICON))
    {
        if (shinfo.hIcon)
        {
            SendDlgItemMessageW(hwndDlg, IDC_FPROP_ICON, STM_SETICON, (WPARAM)shinfo.hIcon, 0);
            DestroyIcon(shinfo.hIcon);
        }
        if (shinfo.szTypeName[0])
            SetDlgItemTextW(hwndDlg, IDC_FPROP_TYPE, shinfo.szTypeName);
    }

    if (!GetFileAttributesExW(props->path, GetFileExInfoStandard, &exinfo))
        return;

    if (format_date(&exinfo.ftCreationTime, buffer, ARRAY_SIZE(buffer)))
        SetDlgItemTextW(hwndDlg, IDC_FPROP_CREATED, buffer);

    if (exinfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        SendDlgItemMessageW(hwndDlg, IDC_FPROP_READONLY, BM_SETCHECK, BST_CHECKED, 0);
    if (exinfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        SendDlgItemMessageW(hwndDlg, IDC_FPROP_HIDDEN, BM_SETCHECK, BST_CHECKED, 0);
    if (exinfo.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
        SendDlgItemMessageW(hwndDlg, IDC_FPROP_ARCHIVE, BM_SETCHECK, BST_CHECKED, 0);

    if (exinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        SetDlgItemTextW(hwndDlg, IDC_FPROP_SIZE, L"(unknown)");
        /* TODO: Implement counting for directories */
        return;
    }

    /* Information about files only */
    StrFormatByteSizeW(((LONGLONG)exinfo.nFileSizeHigh << 32) | exinfo.nFileSizeLow,
                       buffer, ARRAY_SIZE(buffer));
    SetDlgItemTextW(hwndDlg, IDC_FPROP_SIZE, buffer);

    if (format_date(&exinfo.ftLastWriteTime, buffer, ARRAY_SIZE(buffer)))
        SetDlgItemTextW(hwndDlg, IDC_FPROP_MODIFIED, buffer);
    if (format_date(&exinfo.ftLastAccessTime, buffer, ARRAY_SIZE(buffer)))
        SetDlgItemTextW(hwndDlg, IDC_FPROP_ACCESSED, buffer);

    if (FindExecutableW(props->path, NULL, buffer) <= (HINSTANCE)32)
        return;

    /* Information about executables */
    if (SHGetFileInfoW(buffer, 0, &shinfo, sizeof(shinfo), SHGFI_ICON | SHGFI_SMALLICON) && shinfo.hIcon)
        SendDlgItemMessageW(hwndDlg, IDC_FPROP_PROG_ICON, STM_SETICON, (WPARAM)shinfo.hIcon, 0);

    if (get_program_description(buffer, buffer2, ARRAY_SIZE(buffer2)))
        SetDlgItemTextW(hwndDlg, IDC_FPROP_PROG_NAME, buffer2);
    else
    {
        WCHAR *p = wcsrchr(buffer, '\\');
        SetDlgItemTextW(hwndDlg, IDC_FPROP_PROG_NAME, p ? ++p : buffer);
    }
}

static INT_PTR CALLBACK file_properties_proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGEW page = (LPPROPSHEETPAGEW)lParam;
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)page->lParam);
            init_file_properties_dlg(hwndDlg, (struct file_properties_info *)page->lParam);
            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_FPROP_PROG_CHANGE)
            {
                /* TODO: Implement file association dialog */
                MessageBoxA(hwndDlg, "Not implemented yet.", "Error", MB_OK | MB_ICONEXCLAMATION);
            }
            else if (LOWORD(wParam) == IDC_FPROP_READONLY ||
                     LOWORD(wParam) == IDC_FPROP_HIDDEN ||
                     LOWORD(wParam) == IDC_FPROP_ARCHIVE)
            {
                SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            }
            else if (LOWORD(wParam) == IDC_FPROP_PATH && HIWORD(wParam) == EN_CHANGE)
            {
                SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            }
            break;

        case WM_NOTIFY:
            {
                LPPSHNOTIFY notify = (LPPSHNOTIFY)lParam;
                if (notify->hdr.code == PSN_APPLY)
                {
                    struct file_properties_info *props = (struct file_properties_info *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
                    WCHAR newname[MAX_PATH], newpath[MAX_PATH];
                    DWORD attributes;

                    attributes = GetFileAttributesW(props->path);
                    if (attributes != INVALID_FILE_ATTRIBUTES)
                    {
                        attributes &= ~(FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_ARCHIVE);

                        if (SendDlgItemMessageW(hwndDlg, IDC_FPROP_READONLY, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            attributes |= FILE_ATTRIBUTE_READONLY;
                        if (SendDlgItemMessageW(hwndDlg, IDC_FPROP_HIDDEN, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            attributes |= FILE_ATTRIBUTE_HIDDEN;
                        if (SendDlgItemMessageW(hwndDlg, IDC_FPROP_ARCHIVE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            attributes |= FILE_ATTRIBUTE_ARCHIVE;

                        if (!SetFileAttributesW(props->path, attributes))
                            ERR("failed to update file attributes of %s\n", debugstr_w(props->path));
                    }

                    /* Update filename if it was changed */
                    if (GetDlgItemTextW(hwndDlg, IDC_FPROP_PATH, newname, ARRAY_SIZE(newname)) &&
                        wcscmp(props->filename, newname) &&
                        lstrlenW(props->dir) + lstrlenW(newname) + 2 < ARRAY_SIZE(newpath))
                    {
                        lstrcpyW(newpath, props->dir);
                        lstrcatW(newpath, L"\\");
                        lstrcatW(newpath, newname);

                        if (!MoveFileW(props->path, newpath))
                            ERR("failed to move file %s to %s\n", debugstr_w(props->path), debugstr_w(newpath));
                        else
                        {
                            WCHAR *p;
                            lstrcpyW(props->path, newpath);
                            lstrcpyW(props->dir, newpath);
                            if ((p = wcsrchr(props->dir, '\\')))
                            {
                                *p = 0;
                                props->filename = p + 1;
                            }
                            else
                                props->filename = props->dir;
                            SetDlgItemTextW(hwndDlg, IDC_FPROP_LOCATION, props->dir);
                        }
                    }

                    return TRUE;
                }
            }
            break;

        default:
            break;
    }
    return FALSE;
}

static UINT CALLBACK file_properties_callback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGEW page)
{
    struct file_properties_info *props = (struct file_properties_info *)page->lParam;
    if (uMsg == PSPCB_RELEASE)
    {
        free(props);
    }
    return 1;
}

static void init_file_properties_pages(IDataObject *dataobject, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    struct file_properties_info *props;
    HPROPSHEETPAGE general_page;
    PROPSHEETPAGEW propsheet;
    FORMATETC format;
    STGMEDIUM stgm;
    HRESULT hr;
    WCHAR *p;

    props = malloc(sizeof(*props));
    if (!props) return;

    format.cfFormat = CF_HDROP;
    format.ptd      = NULL;
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex   = -1;
    format.tymed    = TYMED_HGLOBAL;

    hr = IDataObject_GetData(dataobject, &format, &stgm);
    if (FAILED(hr)) goto error;

    if (!DragQueryFileW((HDROP)stgm.hGlobal, 0, props->path, ARRAY_SIZE(props->path)))
    {
        ReleaseStgMedium(&stgm);
        goto error;
    }

    ReleaseStgMedium(&stgm);

    props->attrib = GetFileAttributesW(props->path);
    if (props->attrib == INVALID_FILE_ATTRIBUTES)
        goto error;

    lstrcpyW(props->dir, props->path);
    if ((p = wcsrchr(props->dir, '\\')))
    {
        *p = 0;
        props->filename = p + 1;
    }
    else
        props->filename = props->dir;

    memset(&propsheet, 0, sizeof(propsheet));
    propsheet.dwSize        = sizeof(propsheet);
    propsheet.dwFlags       = PSP_DEFAULT | PSP_USECALLBACK;
    propsheet.hInstance     = shell32_hInstance;
    if (props->attrib & FILE_ATTRIBUTE_DIRECTORY)
        propsheet.pszTemplate = (LPWSTR)MAKEINTRESOURCE(IDD_FOLDER_PROPERTIES);
    else
        propsheet.pszTemplate = (LPWSTR)MAKEINTRESOURCE(IDD_FILE_PROPERTIES);
    propsheet.pfnDlgProc    = file_properties_proc;
    propsheet.pfnCallback   = file_properties_callback;
    propsheet.lParam        = (LPARAM)props;

    general_page = CreatePropertySheetPageW(&propsheet);
    if (general_page)
        lpfnAddPage(general_page, lParam);
    return;

error:
    free(props);
}

#define MAX_PROP_PAGES 99

static void DoOpenProperties(ContextMenu *This, HWND hwnd)
{
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
	psh.phpage = hpages;
	psh.nStartPage = 0;

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
	    lstrcpynW(wszFiletype, L"Folder", 64);
	}
	else if (_ILIsSpecialFolder(This->apidl[0]))
	{
	    LPGUID folderGUID;
	    folderGUID = _ILGetGUIDPointer(This->apidl[0]);
	    lstrcpyW(wszFiletype, L"CLSID\\");
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
            init_file_properties_pages(lpDo, Properties_AddPropSheetCallback, (LPARAM)&psh);

	    hpsxa = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, wszFiletype, MAX_PROP_PAGES - psh.nPages, lpDo);
	    if (hpsxa != NULL)
	    {
		SHAddFromPropSheetExtArray(hpsxa, Properties_AddPropSheetCallback, (LPARAM)&psh);
		SHDestroyPropSheetExtArray(hpsxa);
	    }
	    hpsxa = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, L"*", MAX_PROP_PAGES - psh.nPages, lpDo);
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

    if (IS_INTRESOURCE(lpcmi->lpVerb) && LOWORD(lpcmi->lpVerb) > FCIDM_SHVIEWLAST)
    {
        TRACE("Invalid Verb %x\n", LOWORD(lpcmi->lpVerb));
        return E_INVALIDARG;
    }

    if (IS_INTRESOURCE(lpcmi->lpVerb))
    {
        switch(LOWORD(lpcmi->lpVerb) + FCIDM_BASE)
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
        else if (strcmp(lpcmi->lpVerb,"copy")==0)
            DoCopyOrCut(This, lpcmi->hwnd, FALSE);
        else if (strcmp(lpcmi->lpVerb,"cut")==0)
            DoCopyOrCut(This, lpcmi->hwnd, TRUE);
        else if (strcmp(lpcmi->lpVerb,"properties")==0)
            DoOpenProperties(This, lpcmi->hwnd);
        else {
            FIXME("Unhandled string verb %s\n",debugstr_a(lpcmi->lpVerb));
            return E_FAIL;
        }
    }
    return S_OK;
}

static HRESULT WINAPI ItemMenu_GetCommandString(IContextMenu3 *iface, UINT_PTR cmdid, UINT flags,
    UINT *reserved, LPSTR name, UINT maxlen)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    const WCHAR *cmdW = NULL;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%Ix, %#x, %p, %p, %u)\n", This, cmdid, flags, reserved, name, maxlen);

    switch (flags)
    {
    case GCS_HELPTEXTA:
    case GCS_HELPTEXTW:
        hr = E_NOTIMPL;
        break;

    case GCS_VERBA:
    case GCS_VERBW:
        switch (cmdid + FCIDM_BASE)
        {
        case FCIDM_SHVIEW_OPEN:
            cmdW = L"open";
            break;
        case FCIDM_SHVIEW_EXPLORE:
            cmdW = L"explore";
            break;
        case FCIDM_SHVIEW_CUT:
            cmdW = L"cut";
            break;
        case FCIDM_SHVIEW_COPY:
            cmdW = L"copy";
            break;
        case FCIDM_SHVIEW_CREATELINK:
            cmdW = L"link";
            break;
        case FCIDM_SHVIEW_DELETE:
            cmdW = L"delete";
            break;
        case FCIDM_SHVIEW_PROPERTIES:
            cmdW = L"properties";
            break;
        case FCIDM_SHVIEW_RENAME:
            cmdW = L"rename";
            break;
        }

        if (!cmdW)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (flags == GCS_VERBA)
            WideCharToMultiByte(CP_ACP, 0, cmdW, -1, name, maxlen, NULL, NULL);
        else
            lstrcpynW((WCHAR *)name, cmdW, maxlen);

        TRACE("name %s\n", flags == GCS_VERBA ? debugstr_a(name) : debugstr_w((WCHAR *)name));
        break;

    case GCS_VALIDATEA:
    case GCS_VALIDATEW:
        break;
    }

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
    FIXME("(%p)->(0x%x 0x%Ix 0x%Ix): stub\n", This, msg, wParam, lParam);
    return E_NOTIMPL;
}

static HRESULT WINAPI ContextMenu_HandleMenuMsg2(IContextMenu3 *iface, UINT msg,
    WPARAM wParam, LPARAM lParam, LRESULT *result)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    FIXME("(%p)->(0x%x 0x%Ix 0x%Ix %p): stub\n", This, msg, wParam, lParam, result);
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

static HRESULT WINAPI ShellExtInit_QueryInterface(IShellExtInit *iface, REFIID riid, void **obj)
{
    ContextMenu *This = impl_from_IShellExtInit(iface);
    return IContextMenu3_QueryInterface(&This->IContextMenu3_iface, riid, obj);
}

static ULONG WINAPI ShellExtInit_AddRef(IShellExtInit *iface)
{
    ContextMenu *This = impl_from_IShellExtInit(iface);
    return IContextMenu3_AddRef(&This->IContextMenu3_iface);
}

static ULONG WINAPI ShellExtInit_Release(IShellExtInit *iface)
{
    ContextMenu *This = impl_from_IShellExtInit(iface);
    return IContextMenu3_Release(&This->IContextMenu3_iface);
}

static HRESULT WINAPI ShellExtInit_Initialize(IShellExtInit *iface, LPCITEMIDLIST folder,
    IDataObject *dataobj, HKEY progidkey)
{
    ContextMenu *This = impl_from_IShellExtInit(iface);

    FIXME("(%p)->(%p %p %p): stub\n", This, folder, dataobj, progidkey);

    return E_NOTIMPL;
}

static const IShellExtInitVtbl ShellExtInitVtbl =
{
    ShellExtInit_QueryInterface,
    ShellExtInit_AddRef,
    ShellExtInit_Release,
    ShellExtInit_Initialize
};

static HRESULT WINAPI ObjectWithSite_QueryInterface(IObjectWithSite *iface, REFIID riid, void **obj)
{
    ContextMenu *This = impl_from_IObjectWithSite(iface);
    return IContextMenu3_QueryInterface(&This->IContextMenu3_iface, riid, obj);
}

static ULONG WINAPI ObjectWithSite_AddRef(IObjectWithSite *iface)
{
    ContextMenu *This = impl_from_IObjectWithSite(iface);
    return IContextMenu3_AddRef(&This->IContextMenu3_iface);
}

static ULONG WINAPI ObjectWithSite_Release(IObjectWithSite *iface)
{
    ContextMenu *This = impl_from_IObjectWithSite(iface);
    return IContextMenu3_Release(&This->IContextMenu3_iface);
}

static HRESULT WINAPI ObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *site)
{
    ContextMenu *This = impl_from_IObjectWithSite(iface);

    FIXME("(%p)->(%p): stub\n", This, site);

    return E_NOTIMPL;
}

static HRESULT WINAPI ObjectWithSite_GetSite(IObjectWithSite *iface, REFIID riid, void **site)
{
    ContextMenu *This = impl_from_IObjectWithSite(iface);

    FIXME("(%p)->(%s %p): stub\n", This, debugstr_guid(riid), site);

    return E_NOTIMPL;
}

static const IObjectWithSiteVtbl ObjectWithSiteVtbl =
{
    ObjectWithSite_QueryInterface,
    ObjectWithSite_AddRef,
    ObjectWithSite_Release,
    ObjectWithSite_SetSite,
    ObjectWithSite_GetSite,
};

HRESULT ItemMenu_Constructor(IShellFolder *parent, LPCITEMIDLIST pidl, const LPCITEMIDLIST *apidl, UINT cidl,
    REFIID riid, void **pObj)
{
    ContextMenu* This;
    HRESULT hr;
    UINT i;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IContextMenu3_iface.lpVtbl = &ItemContextMenuVtbl;
    This->IShellExtInit_iface.lpVtbl = &ShellExtInitVtbl;
    This->IObjectWithSite_iface.lpVtbl = &ObjectWithSiteVtbl;
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
        Shell_MergeMenus (hMenu, GetSubMenu(hMyMenu,0), indexMenu,
                          idCmdFirst - FCIDM_BASE, idCmdLast, MM_SUBMENUSHAVEIDS);
        idMax = max_menu_id(GetSubMenu(hMyMenu, 0), idCmdFirst - FCIDM_BASE, idCmdLast);
        hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, idMax-idCmdFirst);
    }
    DestroyMenu(hMyMenu);

    TRACE("(%p)->returning 0x%lx\n",This,hr);
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

static HRESULT paste_pidls(ContextMenu *This, ITEMIDLIST **pidls, UINT count)
{
    IShellFolder *psfDesktop;
    UINT i;
    HRESULT hr = S_OK;

    /* bind to the source shellfolder */
    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
        return hr;

    for (i = 0; SUCCEEDED(hr) && i < count; i++) {
        ITEMIDLIST *pidl_dir = NULL;
        ITEMIDLIST *pidl_item;
        IShellFolder *psfFrom = NULL;

        pidl_dir = ILClone(pidls[i]);
        ILRemoveLastID(pidl_dir);
        pidl_item = ILFindLastID(pidls[i]);
        hr = IShellFolder_BindToObject(psfDesktop, pidl_dir, NULL, &IID_IShellFolder, (LPVOID*)&psfFrom);

        if (psfFrom)
        {
            /* get source and destination shellfolder */
            ISFHelper *psfhlpdst = NULL, *psfhlpsrc = NULL;
            hr = IShellFolder_QueryInterface(This->parent, &IID_ISFHelper, (void**)&psfhlpdst);
            if (SUCCEEDED(hr))
                hr = IShellFolder_QueryInterface(psfFrom, &IID_ISFHelper, (void**)&psfhlpsrc);

            /* do the copy/move */
            if (psfhlpdst && psfhlpsrc)
            {
                hr = ISFHelper_CopyItems(psfhlpdst, psfFrom, 1, (LPCITEMIDLIST*)&pidl_item);
                /* FIXME handle move
                ISFHelper_DeleteItems(psfhlpsrc, 1, &pidl_item);
                */
            }
            if(psfhlpdst) ISFHelper_Release(psfhlpdst);
            if(psfhlpsrc) ISFHelper_Release(psfhlpsrc);
            IShellFolder_Release(psfFrom);
        }
        ILFree(pidl_dir);
    }

    IShellFolder_Release(psfDesktop);
    return hr;
}

static HRESULT DoPaste(ContextMenu *This)
{
	IDataObject * pda;
	HRESULT hr;

	TRACE("\n");

	hr = OleGetClipboard(&pda);
	if(SUCCEEDED(hr))
	{
	  STGMEDIUM medium;
	  FORMATETC formatetc;
	  HRESULT format_hr;

	  TRACE("pda=%p\n", pda);

	  /* Set the FORMATETC structure*/
	  InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLISTW), TYMED_HGLOBAL);

	  /* Get the pidls from IDataObject */
	  format_hr = IDataObject_GetData(pda,&formatetc,&medium);
	  if(SUCCEEDED(format_hr))
	  {
	    LPITEMIDLIST * apidl;
	    LPITEMIDLIST pidl;

	    LPIDA lpcida = GlobalLock(medium.hGlobal);
	    TRACE("cida=%p\n", lpcida);
	    if(lpcida)
	    {
	      apidl = _ILCopyCidaToaPidl(&pidl, lpcida);
	      if (apidl)
	      {
	        hr = paste_pidls(This, apidl, lpcida->cidl);
	        _ILFreeaPidl(apidl, lpcida->cidl);
	        SHFree(pidl);
	      }
	      else
	        hr = HRESULT_FROM_WIN32(GetLastError());
	      GlobalUnlock(medium.hGlobal);
	    }
	    else
	      hr = HRESULT_FROM_WIN32(GetLastError());
	    ReleaseStgMedium(&medium);
	  }

	  if(FAILED(format_hr))
	  {
	    InitFormatEtc(formatetc, CF_HDROP, TYMED_HGLOBAL);
	    format_hr = IDataObject_GetData(pda,&formatetc,&medium);
	    if(SUCCEEDED(format_hr))
	    {
	      WCHAR path[MAX_PATH];
	      UINT i, count;
	      ITEMIDLIST **pidls;

	      TRACE("CF_HDROP=%p\n", medium.hGlobal);
	      count = DragQueryFileW(medium.hGlobal, -1, NULL, 0);
	      pidls = SHAlloc(count*sizeof(ITEMIDLIST*));
	      if (pidls)
	      {
	        for (i = 0; i < count; i++)
	        {
	          DragQueryFileW(medium.hGlobal, i, path, ARRAY_SIZE(path));
	          if ((pidls[i] = ILCreateFromPathW(path)) == NULL)
	          {
	            hr = E_FAIL;
	            break;
	          }
	        }
	        if (SUCCEEDED(hr))
	          hr = paste_pidls(This, pidls, count);
	        _ILFreeaPidl(pidls, count);
	      }
	      else
	        hr = HRESULT_FROM_WIN32(GetLastError());
	      ReleaseStgMedium(&medium);
	    }
	  }

	  if (FAILED(format_hr))
	  {
	    ERR("there are no supported and retrievable clipboard formats\n");
	    hr = format_hr;
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
	return hr;
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
        else if (!strcmp(lpcmi->lpVerb, "paste"))
        {
            DoPaste(This);
        }
        else
        {
            FIXME("please report: unknown verb %s\n", debugstr_a(lpcmi->lpVerb));
        }
    }
    else
    {
        switch (LOWORD(lpcmi->lpVerb) + FCIDM_BASE)
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
    const WCHAR *cmdW = NULL;
    HRESULT hr = E_FAIL;

    TRACE("(%p)->(idcom=%Ix flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

    switch (uFlags)
    {
    case GCS_HELPTEXTA:
    case GCS_HELPTEXTW:
        hr = E_NOTIMPL;
        break;

    case GCS_VERBA:
    case GCS_VERBW:
        switch (idCommand + FCIDM_BASE)
        {
        case FCIDM_SHVIEW_INSERT:
            cmdW = L"paste";
            break;
        case FCIDM_SHVIEW_PROPERTIES:
            cmdW = L"properties";
            break;
        }

        if (!cmdW)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (uFlags == GCS_VERBA)
            WideCharToMultiByte(CP_ACP, 0, cmdW, -1, lpszName, uMaxNameLen, NULL, NULL);
        else
            lstrcpynW((WCHAR *)lpszName, cmdW, uMaxNameLen);
        TRACE("name %s\n", uFlags == GCS_VERBA ? debugstr_a(lpszName) : debugstr_w((WCHAR *)lpszName));
        hr = S_OK;
        break;

    case GCS_VALIDATEA:
    case GCS_VALIDATEW:
        /* test the existence of the menu items, the file dialog enables
           the buttons according to this */
        if (HIWORD(idCommand))
        {
            if (!strcmp((LPSTR)idCommand, CMDSTR_VIEWLISTA) ||
                !strcmp((LPSTR)idCommand, CMDSTR_VIEWDETAILSA) ||
                !strcmp((LPSTR)idCommand, CMDSTR_NEWFOLDERA))
                hr = S_OK;
            else
            {
                FIXME("unknown command string %s\n", uFlags == GCS_VALIDATEA ? debugstr_a((LPSTR)idCommand) : debugstr_w((WCHAR*)idCommand));
                hr = E_FAIL;
            }
        }
        break;
    }
    return hr;
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

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IContextMenu3_iface.lpVtbl = &BackgroundContextMenuVtbl;
    This->IShellExtInit_iface.lpVtbl = &ShellExtInitVtbl;
    This->IObjectWithSite_iface.lpVtbl = &ObjectWithSiteVtbl;
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
