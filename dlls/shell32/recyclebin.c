/*
 * Trash virtual folder support. The trashing engine is implemented in trash.c
 *
 * Copyright (C) 2006 Mikolaj Zalewski
 * Copyright 2011 Jay Yang
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

#include "config.h"

#define COBJMACROS
#define NONAMELESSUNION

#include <stdarg.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "ntquery.h"
#include "shlobj.h"
#include "shresdef.h"
#include "shellfolder.h"
#include "shellapi.h"
#include "knownfolders.h"
#include "wine/debug.h"

#include "shell32_main.h"
#include "xdg.h"
#include "pidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(recyclebin);

typedef struct
{
    int column_name_id;
    const GUID *fmtId;
    DWORD pid;
    int pcsFlags;
    int fmt;
    int cxChars;
} columninfo;

static const columninfo RecycleBinColumns[] =
{
    {IDS_SHV_COLUMN1,        &FMTID_Storage,   PID_STG_NAME,       SHCOLSTATE_TYPE_STR|SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  30},
    {IDS_SHV_COLUMN_DELFROM, &FMTID_Displaced, PID_DISPLACED_FROM, SHCOLSTATE_TYPE_STR|SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  30},
    {IDS_SHV_COLUMN_DELDATE, &FMTID_Displaced, PID_DISPLACED_DATE, SHCOLSTATE_TYPE_DATE|SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT,  20},
    {IDS_SHV_COLUMN2,        &FMTID_Storage,   PID_STG_SIZE,       SHCOLSTATE_TYPE_INT|SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 20},
    {IDS_SHV_COLUMN3,        &FMTID_Storage,   PID_STG_STORAGETYPE,SHCOLSTATE_TYPE_INT|SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  20},
    {IDS_SHV_COLUMN4,        &FMTID_Storage,   PID_STG_WRITETIME,  SHCOLSTATE_TYPE_DATE|SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT,  20},
/*    {"creation time",  &FMTID_Storage,   PID_STG_CREATETIME, SHCOLSTATE_TYPE_DATE,                        LVCFMT_LEFT,  20}, */
/*    {"attribs",        &FMTID_Storage,   PID_STG_ATTRIBUTES, SHCOLSTATE_TYPE_STR,                         LVCFMT_LEFT,  20},       */
};

#define COLUMN_NAME    0
#define COLUMN_DELFROM 1
#define COLUMN_DATEDEL 2
#define COLUMN_SIZE    3
#define COLUMN_TYPE    4
#define COLUMN_MTIME   5

#define COLUMNS_COUNT  6

static HRESULT FormatDateTime(LPWSTR buffer, int size, FILETIME ft)
{
    FILETIME lft;
    SYSTEMTIME time;
    int ret;

    FileTimeToLocalFileTime(&ft, &lft);
    FileTimeToSystemTime(&lft, &time);

    ret = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time, NULL, buffer, size);
    if (ret>0 && ret<size)
    {
        /* Append space + time without seconds */
        buffer[ret-1] = ' ';
        GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &time, NULL, &buffer[ret], size - ret);
    }

    return (ret!=0 ? E_FAIL : S_OK);
}

typedef struct tagRecycleBinMenu
{
    IContextMenu2 IContextMenu2_iface;
    LONG refCount;

    UINT cidl;
    LPITEMIDLIST *apidl;
    IShellFolder2 *folder;
} RecycleBinMenu;

static const IContextMenu2Vtbl recycleBinMenuVtbl;

static RecycleBinMenu *impl_from_IContextMenu2(IContextMenu2 *iface)
{
    return CONTAINING_RECORD(iface, RecycleBinMenu, IContextMenu2_iface);
}

static IContextMenu2* RecycleBinMenu_Constructor(UINT cidl, LPCITEMIDLIST *apidl, IShellFolder2 *folder)
{
    RecycleBinMenu *This = SHAlloc(sizeof(RecycleBinMenu));
    TRACE("(%u,%p)\n",cidl,apidl);
    This->IContextMenu2_iface.lpVtbl = &recycleBinMenuVtbl;
    This->cidl = cidl;
    This->apidl = _ILCopyaPidl(apidl,cidl);
    IShellFolder2_AddRef(folder);
    This->folder = folder;
    This->refCount = 1;
    return &This->IContextMenu2_iface;
}

static HRESULT WINAPI RecycleBinMenu_QueryInterface(IContextMenu2 *iface,
                                                    REFIID riid,
                                                    void **ppvObject)
{
    RecycleBinMenu *This = impl_from_IContextMenu2(iface);
    TRACE("(%p, %s, %p) - stub\n", This, debugstr_guid(riid), ppvObject);
    return E_NOTIMPL;
}

static ULONG WINAPI RecycleBinMenu_AddRef(IContextMenu2 *iface)
{
    RecycleBinMenu *This = impl_from_IContextMenu2(iface);
    TRACE("(%p)\n", This);
    return InterlockedIncrement(&This->refCount);

}

static ULONG WINAPI RecycleBinMenu_Release(IContextMenu2 *iface)
{
    RecycleBinMenu *This = impl_from_IContextMenu2(iface);
    UINT result;
    TRACE("(%p)\n", This);
    result = InterlockedDecrement(&This->refCount);
    if (result == 0)
    {
        TRACE("Destroying object\n");
        _ILFreeaPidl(This->apidl,This->cidl);
        IShellFolder2_Release(This->folder);
        SHFree(This);
    }
    return result;
}

static HRESULT WINAPI RecycleBinMenu_QueryContextMenu(IContextMenu2 *iface,
                                                      HMENU hmenu,
                                                      UINT indexMenu,
                                                      UINT idCmdFirst,
                                                      UINT idCmdLast,
                                                      UINT uFlags)
{
    HMENU menures = LoadMenuW(shell32_hInstance,MAKEINTRESOURCEW(MENU_RECYCLEBIN));
    if(uFlags & CMF_DEFAULTONLY)
        return E_NOTIMPL;
    else{
        UINT idMax = Shell_MergeMenus(hmenu,GetSubMenu(menures,0),indexMenu,idCmdFirst,idCmdLast,MM_SUBMENUSHAVEIDS);
        TRACE("Added %d id(s)\n",idMax-idCmdFirst);
        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, idMax-idCmdFirst+1);
    }
}

static void DoErase(RecycleBinMenu *This)
{
    ISFHelper *helper;
    IShellFolder2_QueryInterface(This->folder,&IID_ISFHelper,(void**)&helper);
    if(helper)
        ISFHelper_DeleteItems(helper,This->cidl,(LPCITEMIDLIST*)This->apidl);
}

static void DoRestore(RecycleBinMenu *This)
{

    /*TODO add prompts*/
    UINT i;
    for(i=0;i<This->cidl;i++)
    {
        WIN32_FIND_DATAW data;
        TRASH_UnpackItemID(&((This->apidl[i])->mkid),&data);
        if(PathFileExistsW(data.cFileName))
        {
            PIDLIST_ABSOLUTE dest_pidl = ILCreateFromPathW(data.cFileName);
            WCHAR message[100];
            WCHAR caption[50];
            if(_ILIsFolder(ILFindLastID(dest_pidl)))
                LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_OVERWRITEFOLDER, message, ARRAY_SIZE(message));
            else
                LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_OVERWRITEFILE, message, ARRAY_SIZE(message));
            LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_OVERWRITE_CAPTION, caption, ARRAY_SIZE(caption));

            if(ShellMessageBoxW(shell32_hInstance,GetActiveWindow(),message,
                                caption,MB_YESNO|MB_ICONEXCLAMATION,
                                data.cFileName)!=IDYES)
                continue;
        }
        if(SUCCEEDED(TRASH_RestoreItem(This->apidl[i])))
        {
            IPersistFolder2 *persist;
            LPITEMIDLIST root_pidl;
            PIDLIST_ABSOLUTE dest_pidl = ILCreateFromPathW(data.cFileName);
            BOOL is_folder = _ILIsFolder(ILFindLastID(dest_pidl));
            IShellFolder2_QueryInterface(This->folder,&IID_IPersistFolder2,
                                         (void**)&persist);
            IPersistFolder2_GetCurFolder(persist,&root_pidl);
            SHChangeNotify(is_folder ? SHCNE_RMDIR : SHCNE_DELETE,
                           SHCNF_IDLIST,ILCombine(root_pidl,This->apidl[i]),0);
            SHChangeNotify(is_folder ? SHCNE_MKDIR : SHCNE_CREATE,
                           SHCNF_IDLIST,dest_pidl,0);
            ILFree(dest_pidl);
            ILFree(root_pidl);
        }
    }
}

static HRESULT WINAPI RecycleBinMenu_InvokeCommand(IContextMenu2 *iface,
                                                   LPCMINVOKECOMMANDINFO pici)
{
    RecycleBinMenu *This = impl_from_IContextMenu2(iface);
    LPCSTR verb = pici->lpVerb;
    if(IS_INTRESOURCE(verb))
    {
        switch(LOWORD(verb))
        {
        case IDM_RECYCLEBIN_ERASE:
            DoErase(This);
            break;
        case IDM_RECYCLEBIN_RESTORE:
            DoRestore(This);
            break;
        default:
            return E_NOTIMPL;
        }
    }
    return S_OK;
}

static HRESULT WINAPI RecycleBinMenu_GetCommandString(IContextMenu2 *iface,
                                                      UINT_PTR idCmd,
                                                      UINT uType,
                                                      UINT *pwReserved,
                                                      LPSTR pszName,
                                                      UINT cchMax)
{
    TRACE("(%p, %lu, %u, %p, %s, %u) - stub\n",iface,idCmd,uType,pwReserved,debugstr_a(pszName),cchMax);
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBinMenu_HandleMenuMsg(IContextMenu2 *iface,
                                                   UINT uMsg, WPARAM wParam,
                                                   LPARAM lParam)
{
    TRACE("(%p, %u, 0x%lx, 0x%lx) - stub\n",iface,uMsg,wParam,lParam);
    return E_NOTIMPL;
}


static const IContextMenu2Vtbl recycleBinMenuVtbl =
{
    RecycleBinMenu_QueryInterface,
    RecycleBinMenu_AddRef,
    RecycleBinMenu_Release,
    RecycleBinMenu_QueryContextMenu,
    RecycleBinMenu_InvokeCommand,
    RecycleBinMenu_GetCommandString,
    RecycleBinMenu_HandleMenuMsg,
};

/*
 * Recycle Bin folder
 */

typedef struct tagRecycleBin
{
    IShellFolder2 IShellFolder2_iface;
    IPersistFolder2 IPersistFolder2_iface;
    ISFHelper ISFHelper_iface;
    LONG refCount;

    LPITEMIDLIST pidl;
} RecycleBin;

static const IShellFolder2Vtbl recycleBinVtbl;
static const IPersistFolder2Vtbl recycleBinPersistVtbl;
static const ISFHelperVtbl sfhelperVtbl;

static inline RecycleBin *impl_from_IShellFolder2(IShellFolder2 *iface)
{
    return CONTAINING_RECORD(iface, RecycleBin, IShellFolder2_iface);
}

static RecycleBin *impl_from_IPersistFolder2(IPersistFolder2 *iface)
{
    return CONTAINING_RECORD(iface, RecycleBin, IPersistFolder2_iface);
}

static RecycleBin *impl_from_ISFHelper(ISFHelper *iface)
{
    return CONTAINING_RECORD(iface, RecycleBin, ISFHelper_iface);
}

static void RecycleBin_Destructor(RecycleBin *This);

HRESULT WINAPI RecycleBin_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppOutput)
{
    RecycleBin *obj;
    HRESULT ret;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    obj = SHAlloc(sizeof(RecycleBin));
    if (obj == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(obj, sizeof(RecycleBin));
    obj->IShellFolder2_iface.lpVtbl = &recycleBinVtbl;
    obj->IPersistFolder2_iface.lpVtbl = &recycleBinPersistVtbl;
    obj->ISFHelper_iface.lpVtbl = &sfhelperVtbl;
    if (FAILED(ret = IPersistFolder2_QueryInterface(&obj->IPersistFolder2_iface, riid, ppOutput)))
    {
        RecycleBin_Destructor(obj);
        return ret;
    }
/*    InterlockedIncrement(&objCount);*/
    return S_OK;
}

static void RecycleBin_Destructor(RecycleBin *This)
{
/*    InterlockedDecrement(&objCount);*/
    SHFree(This->pidl);
    SHFree(This);
}

static HRESULT WINAPI RecycleBin_QueryInterface(IShellFolder2 *iface, REFIID riid, void **ppvObject)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);
    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IShellFolder)
            || IsEqualGUID(riid, &IID_IShellFolder2))
        *ppvObject = &This->IShellFolder2_iface;

    if (IsEqualGUID(riid, &IID_IPersist) || IsEqualGUID(riid, &IID_IPersistFolder)
            || IsEqualGUID(riid, &IID_IPersistFolder2))
        *ppvObject = &This->IPersistFolder2_iface;
    if (IsEqualGUID(riid, &IID_ISFHelper))
        *ppvObject = &This->ISFHelper_iface;

    if (*ppvObject != NULL)
    {
        IUnknown_AddRef((IUnknown *)*ppvObject);
        return S_OK;
    }
    WARN("no interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI RecycleBin_AddRef(IShellFolder2 *iface)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);
    TRACE("(%p)\n", This);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI RecycleBin_Release(IShellFolder2 *iface)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);
    LONG result;

    TRACE("(%p)\n", This);
    result = InterlockedDecrement(&This->refCount);
    if (result == 0)
    {
        TRACE("Destroy object\n");
        RecycleBin_Destructor(This);
    }
    return result;
}

static HRESULT WINAPI RecycleBin_ParseDisplayName(IShellFolder2 *This, HWND hwnd, LPBC pbc,
            LPOLESTR pszDisplayName, ULONG *pchEaten, LPITEMIDLIST *ppidl,
            ULONG *pdwAttributes)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_EnumObjects(IShellFolder2 *iface, HWND hwnd, SHCONTF grfFlags, IEnumIDList **ppenumIDList)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);
    IEnumIDListImpl *list;
    LPITEMIDLIST *pidls;
    HRESULT ret = E_OUTOFMEMORY;
    int pidls_count = 0;
    int i=0;

    TRACE("(%p, %p, %x, %p)\n", This, hwnd, grfFlags, ppenumIDList);

    *ppenumIDList = NULL;
    list = IEnumIDList_Constructor();
    if (!list)
        return E_OUTOFMEMORY;

    if (grfFlags & SHCONTF_NONFOLDERS)
    {
        if (FAILED(ret = TRASH_EnumItems(NULL, &pidls, &pidls_count)))
            goto failed;
        for (i=0; i<pidls_count; i++)
            if (!AddToEnumList(list, pidls[i]))
                goto failed;
    }

    *ppenumIDList = &list->IEnumIDList_iface;
    return S_OK;

failed:
    IEnumIDList_Release(&list->IEnumIDList_iface);
    for (; i<pidls_count; i++)
        ILFree(pidls[i]);
    SHFree(pidls);
    return ret;
}

static HRESULT WINAPI RecycleBin_BindToObject(IShellFolder2 *This, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    FIXME("(%p, %p, %p, %s, %p) - stub\n", This, pidl, pbc, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_BindToStorage(IShellFolder2 *This, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    FIXME("(%p, %p, %p, %s, %p) - stub\n", This, pidl, pbc, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_CompareIDs(IShellFolder2 *iface, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);
    int ret;

    /* TODO */
    TRACE("(%p, %p, %p, %p)\n", This, (void *)lParam, pidl1, pidl2);
    if (pidl1->mkid.cb != pidl2->mkid.cb)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, pidl1->mkid.cb - pidl2->mkid.cb);
    /* Looks too complicated, but in optimized memcpy we might get
     * a 32bit wide difference and would truncate it to 16 bit, so
     * erroneously returning equality. */
    ret = memcmp(pidl1->mkid.abID, pidl2->mkid.abID, pidl1->mkid.cb);
    if (ret < 0) ret = -1;
    if (ret > 0) ret =  1;
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (unsigned short)ret);
}

static HRESULT WINAPI RecycleBin_CreateViewObject(IShellFolder2 *iface, HWND hwndOwner, REFIID riid, void **ppv)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);
    HRESULT ret;
    TRACE("(%p, %p, %s, %p)\n", This, hwndOwner, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (IsEqualGUID(riid, &IID_IShellView))
    {
        IShellView *tmp;
        CSFV sfv;

        ZeroMemory(&sfv, sizeof(sfv));
        sfv.cbSize = sizeof(sfv);
        sfv.pshf = (IShellFolder *)&This->IShellFolder2_iface;

        TRACE("Calling SHCreateShellFolderViewEx\n");
        ret = SHCreateShellFolderViewEx(&sfv, &tmp);
        TRACE("Result: %08x, output: %p\n", (unsigned int)ret, tmp);
        *ppv = tmp;
        return ret;
    }

    return E_NOINTERFACE;
}

static HRESULT WINAPI RecycleBin_GetAttributesOf(IShellFolder2 *This, UINT cidl, LPCITEMIDLIST *apidl,
                                   SFGAOF *rgfInOut)
{
    TRACE("(%p, %d, {%p, ...}, {%x})\n", This, cidl, apidl[0], *rgfInOut);
    *rgfInOut &= SFGAO_CANMOVE|SFGAO_CANDELETE|SFGAO_HASPROPSHEET|SFGAO_FILESYSTEM;
    return S_OK;
}

static HRESULT WINAPI RecycleBin_GetUIObjectOf(IShellFolder2 *iface, HWND hwndOwner, UINT cidl, LPCITEMIDLIST *apidl,
                      REFIID riid, UINT *rgfReserved, void **ppv)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);
    *ppv = NULL;
    if(IsEqualGUID(riid, &IID_IContextMenu) || IsEqualGUID(riid, &IID_IContextMenu2))
    {
        TRACE("(%p, %p, %d, {%p, ...}, %s, %p, %p)\n", This, hwndOwner, cidl, apidl[0], debugstr_guid(riid), rgfReserved, ppv);
        *ppv = RecycleBinMenu_Constructor(cidl,apidl,&(This->IShellFolder2_iface));
        return S_OK;
    }
    FIXME("(%p, %p, %d, {%p, ...}, %s, %p, %p): stub!\n", iface, hwndOwner, cidl, apidl[0], debugstr_guid(riid), rgfReserved, ppv);

    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_GetDisplayNameOf(IShellFolder2 *This, LPCITEMIDLIST pidl, SHGDNF uFlags, STRRET *pName)
{
    WIN32_FIND_DATAW data;

    TRACE("(%p, %p, %x, %p)\n", This, pidl, uFlags, pName);
    TRASH_UnpackItemID(&pidl->mkid, &data);
    pName->uType = STRRET_WSTR;
    return SHStrDupW(PathFindFileNameW(data.cFileName), &pName->u.pOleStr);
}

static HRESULT WINAPI RecycleBin_SetNameOf(IShellFolder2 *This, HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName,
            SHGDNF uFlags, LPITEMIDLIST *ppidlOut)
{
    TRACE("\n");
    return E_FAIL; /* not supported */
}

static HRESULT WINAPI RecycleBin_GetClassID(IPersistFolder2 *This, CLSID *pClassID)
{
    TRACE("(%p, %p)\n", This, pClassID);
    if (This == NULL || pClassID == NULL)
        return E_INVALIDARG;
    *pClassID = CLSID_RecycleBin;
    return S_OK;
}

static HRESULT WINAPI RecycleBin_Initialize(IPersistFolder2 *iface, LPCITEMIDLIST pidl)
{
    RecycleBin *This = impl_from_IPersistFolder2(iface);
    TRACE("(%p, %p)\n", This, pidl);

    This->pidl = ILClone(pidl);
    if (This->pidl == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI RecycleBin_GetCurFolder(IPersistFolder2 *iface, LPITEMIDLIST *ppidl)
{
    RecycleBin *This = impl_from_IPersistFolder2(iface);
    TRACE("\n");
    *ppidl = ILClone(This->pidl);
    return S_OK;
}

static HRESULT WINAPI RecycleBin_GetDefaultSearchGUID(IShellFolder2 *iface, GUID *guid)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);
    TRACE("(%p)->(%p)\n", This, guid);
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_EnumSearches(IShellFolder2 *iface, IEnumExtraSearch **ppEnum)
{
    FIXME("stub\n");
    *ppEnum = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_GetDefaultColumn(IShellFolder2 *iface, DWORD reserved, ULONG *sort, ULONG *display)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);

    TRACE("(%p)->(%#x, %p, %p)\n", This, reserved, sort, display);

    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_GetDefaultColumnState(IShellFolder2 *iface, UINT iColumn, SHCOLSTATEF *pcsFlags)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);
    TRACE("(%p, %d, %p)\n", This, iColumn, pcsFlags);
    if (iColumn >= COLUMNS_COUNT)
        return E_INVALIDARG;
    *pcsFlags = RecycleBinColumns[iColumn].pcsFlags;
    return S_OK;
}

static HRESULT WINAPI RecycleBin_GetDetailsEx(IShellFolder2 *iface, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_GetDetailsOf(IShellFolder2 *iface, LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);
    WIN32_FIND_DATAW data;
    WCHAR buffer[MAX_PATH];

    TRACE("(%p, %p, %d, %p)\n", This, pidl, iColumn, pDetails);
    if (iColumn >= COLUMNS_COUNT)
        return E_FAIL;
    pDetails->fmt = RecycleBinColumns[iColumn].fmt;
    pDetails->cxChar = RecycleBinColumns[iColumn].cxChars;
    if (pidl == NULL)
    {
        pDetails->str.uType = STRRET_WSTR;
        LoadStringW(shell32_hInstance, RecycleBinColumns[iColumn].column_name_id, buffer, MAX_PATH);
        return SHStrDupW(buffer, &pDetails->str.u.pOleStr);
    }

    if (iColumn == COLUMN_NAME)
        return RecycleBin_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL, &pDetails->str);

    TRASH_UnpackItemID(&pidl->mkid, &data);
    switch (iColumn)
    {
        case COLUMN_DATEDEL:
            FormatDateTime(buffer, MAX_PATH, data.ftLastAccessTime);
            break;
        case COLUMN_DELFROM:
            lstrcpyW(buffer, data.cFileName);
            PathRemoveFileSpecW(buffer);
            break;
        case COLUMN_SIZE:
            StrFormatKBSizeW(((LONGLONG)data.nFileSizeHigh<<32)|data.nFileSizeLow, buffer, MAX_PATH);
            break;
        case COLUMN_MTIME:
            FormatDateTime(buffer, MAX_PATH, data.ftLastWriteTime);
            break;
        case COLUMN_TYPE:
            /* TODO */
            buffer[0] = 0;
            break;
        default:
            return E_FAIL;
    }
    
    pDetails->str.uType = STRRET_WSTR;
    return SHStrDupW(buffer, &pDetails->str.u.pOleStr);
}

static HRESULT WINAPI RecycleBin_MapColumnToSCID(IShellFolder2 *iface, UINT iColumn, SHCOLUMNID *pscid)
{
    RecycleBin *This = impl_from_IShellFolder2(iface);
    TRACE("(%p, %d, %p)\n", This, iColumn, pscid);
    if (iColumn>=COLUMNS_COUNT)
        return E_INVALIDARG;
    pscid->fmtid = *RecycleBinColumns[iColumn].fmtId;
    pscid->pid = RecycleBinColumns[iColumn].pid;
    return S_OK;
}

static const IShellFolder2Vtbl recycleBinVtbl = 
{
    /* IUnknown */
    RecycleBin_QueryInterface,
    RecycleBin_AddRef,
    RecycleBin_Release,

    /* IShellFolder */
    RecycleBin_ParseDisplayName,
    RecycleBin_EnumObjects,
    RecycleBin_BindToObject,
    RecycleBin_BindToStorage,
    RecycleBin_CompareIDs,
    RecycleBin_CreateViewObject,
    RecycleBin_GetAttributesOf,
    RecycleBin_GetUIObjectOf,
    RecycleBin_GetDisplayNameOf,
    RecycleBin_SetNameOf,

    /* IShellFolder2 */
    RecycleBin_GetDefaultSearchGUID,
    RecycleBin_EnumSearches,
    RecycleBin_GetDefaultColumn,
    RecycleBin_GetDefaultColumnState,
    RecycleBin_GetDetailsEx,
    RecycleBin_GetDetailsOf,
    RecycleBin_MapColumnToSCID
};

static HRESULT WINAPI RecycleBin_IPersistFolder2_QueryInterface(IPersistFolder2 *iface, REFIID riid,
        void **ppvObject)
{
    RecycleBin *This = impl_from_IPersistFolder2(iface);

    return IShellFolder2_QueryInterface(&This->IShellFolder2_iface, riid, ppvObject);
}

static ULONG WINAPI RecycleBin_IPersistFolder2_AddRef(IPersistFolder2 *iface)
{
    RecycleBin *This = impl_from_IPersistFolder2(iface);

    return IShellFolder2_AddRef(&This->IShellFolder2_iface);
}

static ULONG WINAPI RecycleBin_IPersistFolder2_Release(IPersistFolder2 *iface)
{
    RecycleBin *This = impl_from_IPersistFolder2(iface);

    return IShellFolder2_Release(&This->IShellFolder2_iface);
}

static const IPersistFolder2Vtbl recycleBinPersistVtbl =
{
    /* IUnknown */
    RecycleBin_IPersistFolder2_QueryInterface,
    RecycleBin_IPersistFolder2_AddRef,
    RecycleBin_IPersistFolder2_Release,

    /* IPersist */
    RecycleBin_GetClassID,
    /* IPersistFolder */
    RecycleBin_Initialize,
    /* IPersistFolder2 */
    RecycleBin_GetCurFolder
};

static HRESULT WINAPI RecycleBin_ISFHelper_QueryInterface(ISFHelper *iface, REFIID riid,
        void **ppvObject)
{
    RecycleBin *This = impl_from_ISFHelper(iface);

    return IShellFolder2_QueryInterface(&This->IShellFolder2_iface, riid, ppvObject);
}

static ULONG WINAPI RecycleBin_ISFHelper_AddRef(ISFHelper *iface)
{
    RecycleBin *This = impl_from_ISFHelper(iface);

    return IShellFolder2_AddRef(&This->IShellFolder2_iface);
}

static ULONG WINAPI RecycleBin_ISFHelper_Release(ISFHelper *iface)
{
    RecycleBin *This = impl_from_ISFHelper(iface);

    return IShellFolder2_Release(&This->IShellFolder2_iface);
}

static HRESULT WINAPI RecycleBin_GetUniqueName(ISFHelper *iface,LPWSTR lpName,
                                               UINT  uLen)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_AddFolder(ISFHelper * iface, HWND hwnd,
                                           LPCWSTR pwszName,
                                           LPITEMIDLIST * ppidlOut)
{
    /*Adding folders doesn't make sense in the recycle bin*/
    return E_NOTIMPL;
}

static HRESULT erase_items(HWND parent,const LPCITEMIDLIST * apidl, UINT cidl, BOOL confirm)
{
    UINT i=0;
    HRESULT ret = S_OK;
    LPITEMIDLIST recyclebin;

    if(confirm)
    {
        WCHAR arg[MAX_PATH];
        WCHAR message[100];
        WCHAR caption[50];
        switch(cidl)
        {
        case 0:
            return S_OK;
        case 1:
            {
                WIN32_FIND_DATAW data;
                TRASH_UnpackItemID(&((*apidl)->mkid),&data);
                lstrcpynW(arg,data.cFileName,MAX_PATH);
                LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_ERASEITEM, message, ARRAY_SIZE(message));
                break;
            }
        default:
            {
                static const WCHAR format[]={'%','u','\0'};
                LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_ERASEMULTIPLE, message, ARRAY_SIZE(message));
                sprintfW(arg,format,cidl);
                break;
            }

        }
        LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_ERASE_CAPTION, caption, ARRAY_SIZE(caption));
        if(ShellMessageBoxW(shell32_hInstance,parent,message,caption,
                            MB_YESNO|MB_ICONEXCLAMATION,arg)!=IDYES)
            return ret;

    }
    SHGetFolderLocation(parent,CSIDL_BITBUCKET,0,0,&recyclebin);
    for (; i<cidl; i++)
    {
        if(SUCCEEDED(TRASH_EraseItem(apidl[i])))
            SHChangeNotify(SHCNE_DELETE,SHCNF_IDLIST,
                           ILCombine(recyclebin,apidl[i]),0);
    }
    ILFree(recyclebin);
    return S_OK;
}

static HRESULT WINAPI RecycleBin_DeleteItems(ISFHelper * iface, UINT cidl,
                                             LPCITEMIDLIST * apidl)
{
    TRACE("(%p,%u,%p)\n",iface,cidl,apidl);
    return erase_items(GetActiveWindow(),apidl,cidl,TRUE);
}

static HRESULT WINAPI RecycleBin_CopyItems(ISFHelper * iface,
                                           IShellFolder * pSFFrom,
                                           UINT cidl, LPCITEMIDLIST * apidl)
{
    return E_NOTIMPL;
}

static const ISFHelperVtbl sfhelperVtbl =
{
    RecycleBin_ISFHelper_QueryInterface,
    RecycleBin_ISFHelper_AddRef,
    RecycleBin_ISFHelper_Release,
    RecycleBin_GetUniqueName,
    RecycleBin_AddFolder,
    RecycleBin_DeleteItems,
    RecycleBin_CopyItems
};

HRESULT WINAPI SHQueryRecycleBinA(LPCSTR pszRootPath, LPSHQUERYRBINFO pSHQueryRBInfo)
{
    WCHAR wszRootPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, pszRootPath, -1, wszRootPath, MAX_PATH);
    return SHQueryRecycleBinW(wszRootPath, pSHQueryRBInfo);
}

HRESULT WINAPI SHQueryRecycleBinW(LPCWSTR pszRootPath, LPSHQUERYRBINFO pSHQueryRBInfo)
{
    LPITEMIDLIST *apidl;
    INT cidl;
    INT i=0;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_w(pszRootPath), pSHQueryRBInfo);

    hr = TRASH_EnumItems(pszRootPath, &apidl, &cidl);
    if (FAILED(hr))
        return hr;
    pSHQueryRBInfo->i64NumItems = cidl;
    pSHQueryRBInfo->i64Size = 0;
    for (; i<cidl; i++)
    {
        WIN32_FIND_DATAW data;
        TRASH_UnpackItemID(&((apidl[i])->mkid),&data);
        pSHQueryRBInfo->i64Size += ((DWORDLONG)data.nFileSizeHigh << 32) + data.nFileSizeLow;
        ILFree(apidl[i]);
    }
    SHFree(apidl);
    return S_OK;
}

HRESULT WINAPI SHEmptyRecycleBinA(HWND hwnd, LPCSTR pszRootPath, DWORD dwFlags)
{
    WCHAR wszRootPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, pszRootPath, -1, wszRootPath, MAX_PATH);
    return SHEmptyRecycleBinW(hwnd, wszRootPath, dwFlags);
}

#define SHERB_NOCONFIRMATION 1
#define SHERB_NOPROGRESSUI   2
#define SHERB_NOSOUND        4

HRESULT WINAPI SHEmptyRecycleBinW(HWND hwnd, LPCWSTR pszRootPath, DWORD dwFlags)
{
    LPITEMIDLIST *apidl;
    INT cidl;
    INT i=0;
    HRESULT ret;

    TRACE("(%p, %s, 0x%08x)\n", hwnd, debugstr_w(pszRootPath) , dwFlags);

    ret = TRASH_EnumItems(pszRootPath, &apidl, &cidl);
    if (FAILED(ret))
        return ret;

    ret = erase_items(hwnd,(const LPCITEMIDLIST*)apidl,cidl,!(dwFlags & SHERB_NOCONFIRMATION));
    for (;i<cidl;i++)
        ILFree(apidl[i]);
    SHFree(apidl);
    return ret;
}

/*************************************************************************
 * SHUpdateRecycleBinIcon                                [SHELL32.@]
 *
 * Undocumented
 */
HRESULT WINAPI SHUpdateRecycleBinIcon(void)
{
    FIXME("stub\n");
    return S_OK;
}
