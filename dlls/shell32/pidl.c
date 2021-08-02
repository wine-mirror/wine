/*
 *    pidl Handling
 *
 *    Copyright 1998    Juergen Schmied
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
 *
 * NOTES
 *  a pidl == NULL means desktop and is legal
 *
 */

#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "objbase.h"
#include "shlguid.h"
#include "winerror.h"
#include "winnls.h"
#include "undocshell.h"
#include "shell32_main.h"
#include "shlwapi.h"

#include "pidl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(pidl);
WINE_DECLARE_DEBUG_CHANNEL(shell);

/* from comctl32.dll */
extern LPVOID WINAPI Alloc(INT);
extern BOOL WINAPI Free(LPVOID);

static LPSTR _ILGetSTextPointer(LPCITEMIDLIST pidl);
static LPWSTR _ILGetTextPointerW(LPCITEMIDLIST pidl);

/*************************************************************************
 * ILGetDisplayNameExA
 *
 * Retrieves the display name of an ItemIDList
 *
 * PARAMS
 *  psf        [I]   Shell Folder to start with, if NULL the desktop is used
 *  pidl       [I]   ItemIDList relative to the psf to get the display name for
 *  path       [O]   Filled in with the display name, assumed to be at least MAX_PATH long
 *  type       [I]   Type of display name to retrieve
 *                    0 = SHGDN_FORPARSING | SHGDN_FORADDRESSBAR uses always the desktop as root
 *                    1 = SHGDN_NORMAL relative to the root folder
 *                    2 = SHGDN_INFOLDER relative to the root folder, only the last name
 *
 * RETURNS
 *  True if the display name could be retrieved successfully, False otherwise
 */
static BOOL ILGetDisplayNameExA(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, LPSTR path, DWORD type)
{
    BOOL ret = FALSE;
    WCHAR wPath[MAX_PATH];

    TRACE("%p %p %p %d\n", psf, pidl, path, type);

    if (!pidl || !path)
        return FALSE;

    ret = ILGetDisplayNameExW(psf, pidl, wPath, type);
    WideCharToMultiByte(CP_ACP, 0, wPath, -1, path, MAX_PATH, NULL, NULL);
    TRACE("%p %p %s\n", psf, pidl, debugstr_a(path));

    return ret;
}

BOOL ILGetDisplayNameExW(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, LPWSTR path, DWORD type)
{
    LPSHELLFOLDER psfParent, lsf = psf;
    HRESULT ret = NO_ERROR;
    LPCITEMIDLIST pidllast;
    STRRET strret;
    DWORD flag;

    TRACE("%p %p %p %x\n", psf, pidl, path, type);

    if (!pidl || !path)
        return FALSE;

    if (!lsf)
    {
        ret = SHGetDesktopFolder(&lsf);
        if (FAILED(ret))
            return FALSE;
    }

    switch (type)
    {
    case ILGDN_FORPARSING:
        flag = SHGDN_FORPARSING | SHGDN_FORADDRESSBAR;
        break;
    case ILGDN_NORMAL:
        flag = SHGDN_NORMAL;
        break;
    case ILGDN_INFOLDER:
        flag = SHGDN_INFOLDER;
        break;
    default:
        FIXME("Unknown type parameter = %x\n", type);
        flag = SHGDN_FORPARSING | SHGDN_FORADDRESSBAR;
        break;
    }

    if (!*(const WORD*)pidl || type == ILGDN_FORPARSING)
    {
        ret = IShellFolder_GetDisplayNameOf(lsf, pidl, flag, &strret);
        if (SUCCEEDED(ret))
        {
            if(!StrRetToStrNW(path, MAX_PATH, &strret, pidl))
                ret = E_FAIL;
        }
    }
    else
    {
        ret = SHBindToParent(pidl, &IID_IShellFolder, (LPVOID*)&psfParent, &pidllast);
        if (SUCCEEDED(ret))
        {
            ret = IShellFolder_GetDisplayNameOf(psfParent, pidllast, flag, &strret);
            if (SUCCEEDED(ret))
            {
                if(!StrRetToStrNW(path, MAX_PATH, &strret, pidllast))
                    ret = E_FAIL;
            }
            IShellFolder_Release(psfParent);
        }
    }

    TRACE("%p %p %s\n", psf, pidl, debugstr_w(path));

    if (!psf)
        IShellFolder_Release(lsf);
    return SUCCEEDED(ret);
}

/*************************************************************************
 * ILGetDisplayNameEx        [SHELL32.186]
 */
BOOL WINAPI ILGetDisplayNameEx(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, LPVOID path, DWORD type)
{
    TRACE_(shell)("%p %p %p %d\n", psf, pidl, path, type);

    if (SHELL_OsIsUnicode())
        return ILGetDisplayNameExW(psf, pidl, path, type);
    return ILGetDisplayNameExA(psf, pidl, path, type);
}

/*************************************************************************
 * ILGetDisplayName            [SHELL32.15]
 */
BOOL WINAPI ILGetDisplayName(LPCITEMIDLIST pidl, LPVOID path)
{
    TRACE_(shell)("%p %p\n", pidl, path);

    if (SHELL_OsIsUnicode())
        return ILGetDisplayNameExW(NULL, pidl, path, ILGDN_FORPARSING);
    return ILGetDisplayNameExA(NULL, pidl, path, ILGDN_FORPARSING);
}

/*************************************************************************
 * ILFindLastID [SHELL32.16]
 *
 * NOTES
 *   observed: pidl=Desktop return=pidl
 */
LPITEMIDLIST WINAPI ILFindLastID(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST   pidlLast = pidl;

    TRACE("(pidl=%p)\n",pidl);

    if (!pidl)
        return NULL;

    while (pidl->mkid.cb)
    {
        pidlLast = pidl;
        pidl = ILGetNext(pidl);
    }
    return (LPITEMIDLIST)pidlLast;
}

/*************************************************************************
 * ILRemoveLastID [SHELL32.17]
 *
 * NOTES
 *   when pidl=Desktop return=FALSE
 */
BOOL WINAPI ILRemoveLastID(LPITEMIDLIST pidl)
{
    TRACE_(shell)("pidl=%p\n",pidl);

    if (_ILIsEmpty(pidl))
        return FALSE;
    ILFindLastID(pidl)->mkid.cb = 0;
    return TRUE;
}

/*************************************************************************
 * ILClone [SHELL32.18]
 *
 * NOTES
 *    duplicate an idlist
 */
LPITEMIDLIST WINAPI ILClone (LPCITEMIDLIST pidl)
{
    DWORD    len;
    LPITEMIDLIST  newpidl;

    if (!pidl)
        return NULL;

    len = ILGetSize(pidl);
    newpidl = SHAlloc(len);
    if (newpidl)
        memcpy(newpidl,pidl,len);

    TRACE("pidl=%p newpidl=%p\n",pidl, newpidl);
    pdump(pidl);

    return newpidl;
}

/*************************************************************************
 * ILCloneFirst [SHELL32.19]
 *
 * NOTES
 *  duplicates the first idlist of a complex pidl
 */
LPITEMIDLIST WINAPI ILCloneFirst(LPCITEMIDLIST pidl)
{
    DWORD len;
    LPITEMIDLIST pidlNew = NULL;

    TRACE("pidl=%p\n", pidl);
    pdump(pidl);

    if (pidl)
    {
        len = pidl->mkid.cb;
        pidlNew = SHAlloc(len+2);
        if (pidlNew)
        {
            memcpy(pidlNew,pidl,len+2);        /* 2 -> mind a desktop pidl */

            if (len)
                ILGetNext(pidlNew)->mkid.cb = 0x00;
        }
    }
    TRACE("-- newpidl=%p\n",pidlNew);

    return pidlNew;
}

/*************************************************************************
 * ILLoadFromStream (SHELL32.26)
 *
 * NOTES
 *   the first two bytes are the len, the pidl is following then
 */
HRESULT WINAPI ILLoadFromStream (IStream * pStream, LPITEMIDLIST * ppPidl)
{
    WORD        wLen = 0;
    DWORD       dwBytesRead;
    HRESULT     ret = E_FAIL;


    TRACE_(shell)("%p %p\n", pStream ,  ppPidl);

    SHFree(*ppPidl);
    *ppPidl = NULL;

    IStream_AddRef (pStream);

    if (SUCCEEDED(IStream_Read(pStream, &wLen, 2, &dwBytesRead)))
    {
        TRACE("PIDL length is %d\n", wLen);
        if (wLen != 0)
        {
            *ppPidl = SHAlloc (wLen);
            if (SUCCEEDED(IStream_Read(pStream, *ppPidl , wLen, &dwBytesRead)))
            {
                TRACE("Stream read OK\n");
                ret = S_OK;
            }
            else
            {
                WARN("reading pidl failed\n");
                SHFree(*ppPidl);
                *ppPidl = NULL;
            }
        }
        else
        {
            *ppPidl = NULL;
            ret = S_OK;
        }
    }

    /* we are not yet fully compatible */
    if (*ppPidl && !pcheck(*ppPidl))
    {
        WARN("Check failed\n");
        SHFree(*ppPidl);
        *ppPidl = NULL;
    }

    IStream_Release (pStream);
    TRACE("done\n");
    return ret;
}

/*************************************************************************
 * ILSaveToStream (SHELL32.27)
 *
 * NOTES
 *   the first two bytes are the len, the pidl is following then
 */
HRESULT WINAPI ILSaveToStream (IStream * pStream, LPCITEMIDLIST pPidl)
{
    WORD        wLen = 0;
    HRESULT        ret = E_FAIL;

    TRACE_(shell)("%p %p\n", pStream, pPidl);

    IStream_AddRef (pStream);

    wLen = ILGetSize(pPidl);

    if (SUCCEEDED(IStream_Write(pStream, &wLen, 2, NULL)))
    {
        if (SUCCEEDED(IStream_Write(pStream, pPidl, wLen, NULL)))
            ret = S_OK;
    }
    IStream_Release (pStream);

    return ret;
}

/*************************************************************************
 * SHILCreateFromPath        [SHELL32.28]
 *
 * Create an ItemIDList from a path
 *
 * PARAMS
 *  path       [I]
 *  ppidl      [O]
 *  attributes [I/O] requested attributes on call and actual attributes when
 *                   the function returns
 *
 * RETURNS
 *  NO_ERROR if successful, or an OLE errer code otherwise
 *
 * NOTES
 *  Wrapper for IShellFolder_ParseDisplayName().
 */
static HRESULT SHILCreateFromPathA(LPCSTR path, LPITEMIDLIST * ppidl, DWORD * attributes)
{
    WCHAR lpszDisplayName[MAX_PATH];

    TRACE_(shell)("%s %p 0x%08x\n", path, ppidl, attributes ? *attributes : 0);

    if (!MultiByteToWideChar(CP_ACP, 0, path, -1, lpszDisplayName, MAX_PATH))
        lpszDisplayName[MAX_PATH-1] = 0;

    return SHILCreateFromPathW(lpszDisplayName, ppidl, attributes);
}

HRESULT SHILCreateFromPathW(LPCWSTR path, LPITEMIDLIST * ppidl, DWORD * attributes)
{
    LPSHELLFOLDER sf;
    DWORD pchEaten;
    HRESULT ret = E_FAIL;

    TRACE_(shell)("%s %p 0x%08x\n", debugstr_w(path), ppidl, attributes ? *attributes : 0);

    if (SUCCEEDED (SHGetDesktopFolder(&sf)))
    {
        ret = IShellFolder_ParseDisplayName(sf, 0, NULL, (LPWSTR)path, &pchEaten, ppidl, attributes);
        IShellFolder_Release(sf);
    }
    return ret;
}

HRESULT WINAPI SHILCreateFromPathAW (LPCVOID path, LPITEMIDLIST * ppidl, DWORD * attributes)
{
    if ( SHELL_OsIsUnicode())
        return SHILCreateFromPathW (path, ppidl, attributes);
    return SHILCreateFromPathA (path, ppidl, attributes);
}

/*************************************************************************
 * SHCloneSpecialIDList      [SHELL32.89]
 *
 * Create an ItemIDList to one of the special folders.

 * PARAMS
 *  hwndOwner    [in]
 *  nFolder      [in]    CSIDL_xxxxx
 *  fCreate      [in]    Create folder if it does not exist
 *
 * RETURNS
 *  Success: The newly created pidl
 *  Failure: NULL, if inputs are invalid.
 *
 * NOTES
 *  exported by ordinal.
 *  Caller is responsible for deallocating the returned ItemIDList with the
 *  shells IMalloc interface, aka ILFree.
 */
LPITEMIDLIST WINAPI SHCloneSpecialIDList(HWND hwndOwner, DWORD nFolder, BOOL fCreate)
{
    LPITEMIDLIST ppidl;
    TRACE_(shell)("(hwnd=%p,csidl=0x%x,%s).\n", hwndOwner, nFolder, fCreate ? "T" : "F");

    if (fCreate)
        nFolder |= CSIDL_FLAG_CREATE;

    SHGetSpecialFolderLocation(hwndOwner, nFolder, &ppidl);
    return ppidl;
}

/*************************************************************************
 * ILGlobalClone             [SHELL32.20]
 *
 * Clones an ItemIDList using Alloc.
 *
 * PARAMS
 *  pidl       [I]   ItemIDList to clone
 *
 * RETURNS
 *  Newly allocated ItemIDList.
 *
 * NOTES
 *  exported by ordinal.
 */
LPITEMIDLIST WINAPI ILGlobalClone(LPCITEMIDLIST pidl)
{
    DWORD    len;
    LPITEMIDLIST  newpidl;

    if (!pidl)
        return NULL;

    len = ILGetSize(pidl);
    newpidl = Alloc(len);
    if (newpidl)
        memcpy(newpidl,pidl,len);

    TRACE("pidl=%p newpidl=%p\n",pidl, newpidl);
    pdump(pidl);

    return newpidl;
}

/*************************************************************************
 * ILIsEqual [SHELL32.21]
 *
 */
BOOL WINAPI ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    char    szData1[MAX_PATH];
    char    szData2[MAX_PATH];

    LPCITEMIDLIST pidltemp1 = pidl1;
    LPCITEMIDLIST pidltemp2 = pidl2;

    TRACE("pidl1=%p pidl2=%p\n",pidl1, pidl2);

    /*
     * Explorer reads from registry directly (StreamMRU),
     * so we can only check here
     */
    if (!pcheck(pidl1) || !pcheck (pidl2))
        return FALSE;

    pdump (pidl1);
    pdump (pidl2);

    if (!pidl1 || !pidl2)
        return FALSE;

    while (pidltemp1->mkid.cb && pidltemp2->mkid.cb)
    {
        _ILSimpleGetText(pidltemp1, szData1, MAX_PATH);
        _ILSimpleGetText(pidltemp2, szData2, MAX_PATH);

        if (lstrcmpiA( szData1, szData2 ))
            return FALSE;

        pidltemp1 = ILGetNext(pidltemp1);
        pidltemp2 = ILGetNext(pidltemp2);
    }

    if (!pidltemp1->mkid.cb && !pidltemp2->mkid.cb)
        return TRUE;

    return FALSE;
}

/*************************************************************************
 * ILIsParent                [SHELL32.23]
 *
 * Verifies that pidlParent is indeed the (immediate) parent of pidlChild.
 *
 * PARAMS
 *  pidlParent [I]
 *  pidlChild  [I]
 *  bImmediate [I]   only return true if the parent is the direct parent
 *                   of the child
 *
 * RETURNS
 *  True if the parent ItemIDlist is a complete part of the child ItemIdList,
 *  False otherwise.
 *
 * NOTES
 *  parent = a/b, child = a/b/c -> true, c is in folder a/b
 *  child = a/b/c/d -> false if bImmediate is true, d is not in folder a/b
 *  child = a/b/c/d -> true if bImmediate is false, d is in a subfolder of a/b
 *  child = a/b -> false if bImmediate is true
 *  child = a/b -> true if bImmediate is false
 */
BOOL WINAPI ILIsParent(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild, BOOL bImmediate)
{
    char    szData1[MAX_PATH];
    char    szData2[MAX_PATH];
    LPCITEMIDLIST pParent = pidlParent;
    LPCITEMIDLIST pChild = pidlChild;

    TRACE("%p %p %x\n", pidlParent, pidlChild, bImmediate);

    if (!pParent || !pChild)
        return FALSE;

    while (pParent->mkid.cb && pChild->mkid.cb)
    {
        _ILSimpleGetText(pParent, szData1, MAX_PATH);
        _ILSimpleGetText(pChild, szData2, MAX_PATH);

        if (lstrcmpiA( szData1, szData2 ))
            return FALSE;

        pParent = ILGetNext(pParent);
        pChild = ILGetNext(pChild);
    }

    /* child has shorter name than parent */
    if (pParent->mkid.cb)
        return FALSE;

    /* not immediate descent */
    if ((!pChild->mkid.cb || ILGetNext(pChild)->mkid.cb) && bImmediate)
        return FALSE;

    return TRUE;
}

/*************************************************************************
 * ILFindChild               [SHELL32.24]
 *
 *  Compares elements from pidl1 and pidl2.
 *
 * PARAMS
 *  pidl1      [I]
 *  pidl2      [I]
 *
 * RETURNS
 *  pidl1 is desktop      pidl2
 *  pidl1 shorter pidl2   pointer to first different element of pidl2
 *                        if there was at least one equal element
 *  pidl2 shorter pidl1   0
 *  pidl2 equal pidl1     pointer to last 0x00-element of pidl2
 *
 * NOTES
 *  exported by ordinal.
 */
LPITEMIDLIST WINAPI ILFindChild(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    char    szData1[MAX_PATH];
    char    szData2[MAX_PATH];

    LPCITEMIDLIST pidltemp1 = pidl1;
    LPCITEMIDLIST pidltemp2 = pidl2;
    LPCITEMIDLIST ret=NULL;

    TRACE("pidl1=%p pidl2=%p\n",pidl1, pidl2);

    /* explorer reads from registry directly (StreamMRU),
       so we can only check here */
    if ((!pcheck (pidl1)) || (!pcheck (pidl2)))
        return FALSE;

    pdump (pidl1);
    pdump (pidl2);

    if (_ILIsDesktop(pidl1))
    {
        ret = pidl2;
    }
    else
    {
        while (pidltemp1->mkid.cb && pidltemp2->mkid.cb)
        {
            _ILSimpleGetText(pidltemp1, szData1, MAX_PATH);
            _ILSimpleGetText(pidltemp2, szData2, MAX_PATH);

            if (lstrcmpiA(szData1,szData2))
                break;

            pidltemp1 = ILGetNext(pidltemp1);
            pidltemp2 = ILGetNext(pidltemp2);
            ret = pidltemp2;
        }

        if (pidltemp1->mkid.cb)
            ret = NULL; /* elements of pidl1 left*/
    }
    TRACE_(shell)("--- %p\n", ret);
    return (LPITEMIDLIST)ret; /* pidl 1 is shorter */
}

/*************************************************************************
 * ILCombine                 [SHELL32.25]
 *
 * Concatenates two complex ItemIDLists.
 *
 * PARAMS
 *  pidl1      [I]   first complex ItemIDLists
 *  pidl2      [I]   complex ItemIDLists to append
 *
 * RETURNS
 *  if both pidl's == NULL      NULL
 *  if pidl1 == NULL            cloned pidl2
 *  if pidl2 == NULL            cloned pidl1
 *  otherwise new pidl with pidl2 appended to pidl1
 *
 * NOTES
 *  exported by ordinal.
 *  Does not destroy the passed in ItemIDLists!
 */
LPITEMIDLIST WINAPI ILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    DWORD    len1,len2;
    LPITEMIDLIST  pidlNew;

    TRACE("pidl=%p pidl=%p\n",pidl1,pidl2);

    if (!pidl1 && !pidl2) return NULL;

    pdump (pidl1);
    pdump (pidl2);

    if (!pidl1)
    {
        pidlNew = ILClone(pidl2);
        return pidlNew;
    }

    if (!pidl2)
    {
        pidlNew = ILClone(pidl1);
        return pidlNew;
    }

    len1  = ILGetSize(pidl1)-2;
    len2  = ILGetSize(pidl2);
    pidlNew  = SHAlloc(len1+len2);

    if (pidlNew)
    {
        memcpy(pidlNew,pidl1,len1);
        memcpy(((BYTE *)pidlNew)+len1,pidl2,len2);
    }

    /*  TRACE(pidl,"--new pidl=%p\n",pidlNew);*/
    return pidlNew;
}

/*************************************************************************
 *  SHGetRealIDL [SHELL32.98]
 *
 * NOTES
 */
HRESULT WINAPI SHGetRealIDL(LPSHELLFOLDER lpsf, LPCITEMIDLIST pidlSimple, LPITEMIDLIST *pidlReal)
{
    IDataObject* pDataObj;
    HRESULT hr;

    hr = IShellFolder_GetUIObjectOf(lpsf, 0, 1, &pidlSimple,
                             &IID_IDataObject, 0, (LPVOID*)&pDataObj);
    if (SUCCEEDED(hr))
    {
        STGMEDIUM medium;
        FORMATETC fmt;

        fmt.cfFormat = RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
        fmt.ptd = NULL;
        fmt.dwAspect = DVASPECT_CONTENT;
        fmt.lindex = -1;
        fmt.tymed = TYMED_HGLOBAL;

        hr = IDataObject_GetData(pDataObj, &fmt, &medium);

        IDataObject_Release(pDataObj);

        if (SUCCEEDED(hr))
        {
            /*assert(pida->cidl==1);*/
            LPIDA pida = GlobalLock(medium.u.hGlobal);

            LPCITEMIDLIST pidl_folder = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[0]);
            LPCITEMIDLIST pidl_child = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[1]);

            *pidlReal = ILCombine(pidl_folder, pidl_child);

            if (!*pidlReal)
                hr = E_OUTOFMEMORY;

            GlobalUnlock(medium.u.hGlobal);
            GlobalFree(medium.u.hGlobal);
        }
    }

    return hr;
}

/*************************************************************************
 *  SHLogILFromFSIL [SHELL32.95]
 *
 * NOTES
 *  pild = CSIDL_DESKTOP    ret = 0
 *  pild = CSIDL_DRIVES     ret = 0
 */
LPITEMIDLIST WINAPI SHLogILFromFSIL(LPITEMIDLIST pidl)
{
    FIXME("(pidl=%p)\n",pidl);

    pdump(pidl);

    return 0;
}

/*************************************************************************
 * ILGetSize                 [SHELL32.152]
 *
 * Gets the byte size of an ItemIDList including zero terminator
 *
 * PARAMS
 *  pidl       [I]   ItemIDList
 *
 * RETURNS
 *  size of pidl in bytes
 *
 * NOTES
 *  exported by ordinal
 */
UINT WINAPI ILGetSize(LPCITEMIDLIST pidl)
{
    LPCSHITEMID si = &(pidl->mkid);
    UINT len=0;

    if (pidl)
    {
        while (si->cb)
        {
            len += si->cb;
            si  = (LPCSHITEMID)(((const BYTE*)si)+si->cb);
        }
        len += 2;
    }
    TRACE("pidl=%p size=%u\n",pidl, len);
    return len;
}

/*************************************************************************
 * ILGetNext                 [SHELL32.153]
 *
 * Gets the next ItemID of an ItemIDList
 *
 * PARAMS
 *  pidl       [I]   ItemIDList
 *
 * RETURNS
 *  null -> null
 *  desktop -> null
 *  simple pidl -> pointer to 0x0000 element
 *
 * NOTES
 *  exported by ordinal.
 */
LPITEMIDLIST WINAPI ILGetNext(LPCITEMIDLIST pidl)
{
    WORD len;

    TRACE("%p\n", pidl);

    if (pidl)
    {
        len =  pidl->mkid.cb;
        if (len)
        {
            pidl = (LPCITEMIDLIST) (((const BYTE*)pidl)+len);
            TRACE("-- %p\n", pidl);
            return (LPITEMIDLIST)pidl;
        }
    }
    return NULL;
}

/*************************************************************************
 * ILAppendID                [SHELL32.154]
 *
 * Adds the single ItemID item to the ItemIDList indicated by pidl.
 * If bEnd is FALSE, inserts the item in the front of the list,
 * otherwise it adds the item to the end. (???)
 *
 * PARAMS
 *  pidl       [I]   ItemIDList to extend
 *  item       [I]   ItemID to prepend/append
 *  bEnd       [I]   Indicates if the item should be appended
 *
 * NOTES
 *  Destroys the passed in idlist! (???)
 */
LPITEMIDLIST WINAPI ILAppendID(LPITEMIDLIST pidl, LPCSHITEMID item, BOOL bEnd)
{
    LPITEMIDLIST idlRet;
    LPCITEMIDLIST itemid = (LPCITEMIDLIST)item;

    WARN("(pidl=%p,pidl=%p,%08u)semi-stub\n",pidl,item,bEnd);

    pdump (pidl);
    pdump (itemid);

    if (_ILIsDesktop(pidl))
    {
        idlRet = ILClone(itemid);
        SHFree (pidl);
        return idlRet;
    }

    if (bEnd)
        idlRet = ILCombine(pidl, itemid);
    else
        idlRet = ILCombine(itemid, pidl);

    SHFree(pidl);
    return idlRet;
}

/*************************************************************************
 * ILFree                    [SHELL32.155]
 *
 * Frees memory (if not NULL) allocated by SHMalloc allocator
 *
 * PARAMS
 *  pidl         [I]
 *
 * RETURNS
 *  Nothing
 *
 * NOTES
 *  exported by ordinal
 */
void WINAPI ILFree(LPITEMIDLIST pidl)
{
    TRACE("(pidl=%p)\n",pidl);
    SHFree(pidl);
}

/*************************************************************************
 * ILGlobalFree              [SHELL32.156]
 *
 * Frees memory (if not NULL) allocated by Alloc allocator
 *
 * PARAMS
 *  pidl         [I]
 *
 * RETURNS
 *  Nothing
 *
 * NOTES
 *  exported by ordinal.
 */
void WINAPI ILGlobalFree( LPITEMIDLIST pidl)
{
    TRACE("%p\n", pidl);

    Free(pidl);
}

/*************************************************************************
 * ILCreateFromPathA         [SHELL32.189]
 *
 * Creates a complex ItemIDList from a path and returns it.
 *
 * PARAMS
 *  path         [I]
 *
 * RETURNS
 *  the newly created complex ItemIDList or NULL if failed
 *
 * NOTES
 *  exported by ordinal.
 */
LPITEMIDLIST WINAPI ILCreateFromPathA (LPCSTR path)
{
    LPITEMIDLIST pidlnew = NULL;

    TRACE_(shell)("%s\n", debugstr_a(path));

    if (SUCCEEDED(SHILCreateFromPathA(path, &pidlnew, NULL)))
        return pidlnew;
    return NULL;
}

/*************************************************************************
 * ILCreateFromPathW         [SHELL32.190]
 *
 * See ILCreateFromPathA.
 */
LPITEMIDLIST WINAPI ILCreateFromPathW (LPCWSTR path)
{
    LPITEMIDLIST pidlnew = NULL;

    TRACE_(shell)("%s\n", debugstr_w(path));

    if (SUCCEEDED(SHILCreateFromPathW(path, &pidlnew, NULL)))
        return pidlnew;
    return NULL;
}

/*************************************************************************
 * ILCreateFromPath          [SHELL32.157]
 */
LPITEMIDLIST WINAPI ILCreateFromPathAW (LPCVOID path)
{
    if ( SHELL_OsIsUnicode())
        return ILCreateFromPathW (path);
    return ILCreateFromPathA (path);
}

/*************************************************************************
 * _ILParsePathW             [internal]
 *
 * Creates an ItemIDList from a path and returns it.
 *
 * PARAMS
 *  path         [I]   path to parse and convert into an ItemIDList
 *  lpFindFile   [I]   pointer to buffer to initialize the FileSystem
 *                     Bind Data object with
 *  bBindCtx     [I]   indicates to create a BindContext and assign a
 *                     FileSystem Bind Data object
 *  ppidl        [O]   the newly create ItemIDList
 *  prgfInOut    [I/O] requested attributes on input and actual
 *                     attributes on return
 *
 * RETURNS
 *  NO_ERROR on success or an OLE error code
 *
 * NOTES
 *  If either lpFindFile is non-NULL or bBindCtx is TRUE, this function
 *  creates a BindContext object and assigns a FileSystem Bind Data object
 *  to it, passing the BindContext to IShellFolder_ParseDisplayName. Each
 *  IShellFolder uses that FileSystem Bind Data object of the BindContext
 *  to pass data about the current path element to the next object. This
 *  is used to avoid having to verify the current path element on disk, so
 *  that creating an ItemIDList from a nonexistent path still can work.
 */
static HRESULT _ILParsePathW(LPCWSTR path, LPWIN32_FIND_DATAW lpFindFile,
                             BOOL bBindCtx, LPITEMIDLIST *ppidl, LPDWORD prgfInOut)
{
    LPSHELLFOLDER pSF = NULL;
    LPBC pBC = NULL;
    HRESULT ret;

    TRACE("%s %p %d (%p)->%p (%p)->0x%x\n", debugstr_w(path), lpFindFile, bBindCtx,
                                             ppidl, ppidl ? *ppidl : NULL,
                                             prgfInOut, prgfInOut ? *prgfInOut : 0);

    ret = SHGetDesktopFolder(&pSF);
    if (FAILED(ret))
        return ret;

    if (lpFindFile || bBindCtx)
        ret = IFileSystemBindData_Constructor(lpFindFile, &pBC);

    if (SUCCEEDED(ret))
    {
        ret = IShellFolder_ParseDisplayName(pSF, 0, pBC, (LPOLESTR)path, NULL, ppidl, prgfInOut);
    }

    if (pBC)
    {
        IBindCtx_Release(pBC);
        pBC = NULL;
    }

    IShellFolder_Release(pSF);

    if (FAILED(ret) && ppidl)
        *ppidl = NULL;

    TRACE("%s %p 0x%x\n", debugstr_w(path), ppidl ? *ppidl : NULL, prgfInOut ? *prgfInOut : 0);

    return ret;
}

/*************************************************************************
 * SHSimpleIDListFromPath    [SHELL32.162]
 *
 * Creates a simple ItemIDList from a path and returns it. This function
 * does not fail on nonexistent paths.
 *
 * PARAMS
 *  path         [I]   path to parse and convert into an ItemIDList
 *
 * RETURNS
 *  the newly created simple ItemIDList
 *
 * NOTES
 *  Simple in the name does not mean a relative ItemIDList but rather a
 *  fully qualified list, where only the file name is filled in and the
 *  directory flag for those ItemID elements this is known about, eg.
 *  it is not the last element in the ItemIDList or the actual directory
 *  exists on disk.
 *  exported by ordinal.
 */
LPITEMIDLIST SHSimpleIDListFromPathA(LPCSTR lpszPath)
{
    LPITEMIDLIST pidl = NULL;
    LPWSTR wPath = NULL;
    int len;

    TRACE("%s\n", debugstr_a(lpszPath));

    if (lpszPath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszPath, -1, NULL, 0);
        wPath = heap_alloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszPath, -1, wPath, len);
    }

    _ILParsePathW(wPath, NULL, TRUE, &pidl, NULL);

    heap_free(wPath);
    TRACE("%s %p\n", debugstr_a(lpszPath), pidl);
    return pidl;
}

LPITEMIDLIST SHSimpleIDListFromPathW(LPCWSTR lpszPath)
{
    LPITEMIDLIST pidl = NULL;

    TRACE("%s\n", debugstr_w(lpszPath));

    _ILParsePathW(lpszPath, NULL, TRUE, &pidl, NULL);
    TRACE("%s %p\n", debugstr_w(lpszPath), pidl);
    return pidl;
}

LPITEMIDLIST WINAPI SHSimpleIDListFromPathAW(LPCVOID lpszPath)
{
    if ( SHELL_OsIsUnicode())
        return SHSimpleIDListFromPathW (lpszPath);
    return SHSimpleIDListFromPathA (lpszPath);
}

/*************************************************************************
 * SHGetDataFromIDListA [SHELL32.247]
 *
 * NOTES
 *  the pidl can be a simple one. since we can't get the path out of the pidl
 *  we have to take all data from the pidl
 */
HRESULT WINAPI SHGetDataFromIDListA(LPSHELLFOLDER psf, LPCITEMIDLIST pidl,
                                    int nFormat, LPVOID dest, int len)
{
    LPSTR filename, shortname;
    WIN32_FIND_DATAA * pfd;

    TRACE_(shell)("%p, %p, %d, %p, %d.\n", psf, pidl, nFormat, dest, len);

    pdump(pidl);
    if (!psf || !dest)
        return E_INVALIDARG;

    switch (nFormat)
    {
    case SHGDFIL_FINDDATA:
        pfd = dest;

        if (_ILIsDrive(pidl) || _ILIsSpecialFolder(pidl))
            return E_INVALIDARG;

        if (len < sizeof(WIN32_FIND_DATAA))
            return E_INVALIDARG;

        ZeroMemory(pfd, sizeof (WIN32_FIND_DATAA));
        _ILGetFileDateTime( pidl, &(pfd->ftLastWriteTime));
        pfd->dwFileAttributes = _ILGetFileAttributes(pidl, NULL, 0);
        pfd->nFileSizeLow = _ILGetFileSize ( pidl, NULL, 0);

        filename = _ILGetTextPointer(pidl);
        shortname = _ILGetSTextPointer(pidl);

        if (filename)
            lstrcpynA(pfd->cFileName, filename, sizeof(pfd->cFileName));
        else
            pfd->cFileName[0] = '\0';

        if (shortname)
            lstrcpynA(pfd->cAlternateFileName, shortname, sizeof(pfd->cAlternateFileName));
        else
            pfd->cAlternateFileName[0] = '\0';
        return S_OK;

    case SHGDFIL_NETRESOURCE:
    case SHGDFIL_DESCRIPTIONID:
        FIXME_(shell)("SHGDFIL %i stub\n", nFormat);
        break;

    default:
        ERR_(shell)("Unknown SHGDFIL %i, please report\n", nFormat);
    }

    return E_INVALIDARG;
}

/*************************************************************************
 * SHGetDataFromIDListW [SHELL32.248]
 *
 */
HRESULT WINAPI SHGetDataFromIDListW(LPSHELLFOLDER psf, LPCITEMIDLIST pidl,
                                    int nFormat, LPVOID dest, int len)
{
    LPSTR filename, shortname;
    WIN32_FIND_DATAW * pfd = dest;

    TRACE_(shell)("%p, %p, %d, %p, %d.\n", psf, pidl, nFormat, dest, len);

    pdump(pidl);

    if (!psf || !dest)
        return E_INVALIDARG;

    switch (nFormat)
    {
    case SHGDFIL_FINDDATA:
        pfd = dest;

        if (_ILIsDrive(pidl))
            return E_INVALIDARG;

        if (len < sizeof(WIN32_FIND_DATAW))
            return E_INVALIDARG;

        ZeroMemory(pfd, sizeof (WIN32_FIND_DATAW));
        _ILGetFileDateTime( pidl, &(pfd->ftLastWriteTime));
        pfd->dwFileAttributes = _ILGetFileAttributes(pidl, NULL, 0);
        pfd->nFileSizeLow = _ILGetFileSize ( pidl, NULL, 0);

        filename = _ILGetTextPointer(pidl);
        shortname = _ILGetSTextPointer(pidl);

        if (!filename)
            pfd->cFileName[0] = '\0';
        else if (!MultiByteToWideChar(CP_ACP, 0, filename, -1, pfd->cFileName, MAX_PATH))
            pfd->cFileName[MAX_PATH-1] = 0;

        if (!shortname)
            pfd->cAlternateFileName[0] = '\0';
        else if (!MultiByteToWideChar(CP_ACP, 0, shortname, -1, pfd->cAlternateFileName, 14))
            pfd->cAlternateFileName[13] = 0;
        return S_OK;

    case SHGDFIL_NETRESOURCE:
    case SHGDFIL_DESCRIPTIONID:
        FIXME_(shell)("SHGDFIL %i stub\n", nFormat);
        break;

    default:
        ERR_(shell)("Unknown SHGDFIL %i, please report\n", nFormat);
    }

    return E_INVALIDARG;
}

/*************************************************************************
 * SHGetPathFromIDListA        [SHELL32.@][NT 4.0: SHELL32.220]
 *
 * PARAMETERS
 *  pidl,   [IN] pidl
 *  pszPath [OUT] path
 *
 * RETURNS
 *  path from a passed PIDL.
 *
 * NOTES
 *    NULL returns FALSE
 *    desktop pidl gives path to desktop directory back
 *    special pidls returning FALSE
 */
BOOL WINAPI SHGetPathFromIDListA(LPCITEMIDLIST pidl, LPSTR pszPath)
{
    WCHAR wszPath[MAX_PATH];
    BOOL bSuccess;

    bSuccess = SHGetPathFromIDListW(pidl, wszPath);
    WideCharToMultiByte(CP_ACP, 0, wszPath, -1, pszPath, MAX_PATH, NULL, NULL);

    return bSuccess;
}

/*************************************************************************
 * SHGetPathFromIDListW             [SHELL32.@]
 *
 * See SHGetPathFromIDListA.
 */
BOOL WINAPI SHGetPathFromIDListW(LPCITEMIDLIST pidl, LPWSTR pszPath)
{
    return SHGetPathFromIDListEx(pidl, pszPath, MAX_PATH, 0);
}

/*************************************************************************
 * SHGetPathFromIDListEx             [SHELL32.@]
 */
BOOL WINAPI SHGetPathFromIDListEx(LPCITEMIDLIST pidl, WCHAR *path, DWORD path_size, GPFIDL_FLAGS flags)
{
    HRESULT hr;
    LPCITEMIDLIST pidlLast;
    LPSHELLFOLDER psfFolder;
    DWORD dwAttributes;
    STRRET strret;

    TRACE_(shell)("(pidl=%p,%p,%u,%x)\n", pidl, path, path_size, flags);
    pdump(pidl);

    if (flags != GPFIDL_DEFAULT)
        FIXME("Unsupported flags %x\n", flags);

    *path = '\0';
    if (!pidl)
        return FALSE;

    hr = SHBindToParent(pidl, &IID_IShellFolder, (VOID**)&psfFolder, &pidlLast);
    if (FAILED(hr)) return FALSE;

    dwAttributes = SFGAO_FILESYSTEM;
    hr = IShellFolder_GetAttributesOf(psfFolder, 1, &pidlLast, &dwAttributes);
    if (FAILED(hr) || !(dwAttributes & SFGAO_FILESYSTEM)) {
        IShellFolder_Release(psfFolder);
        return FALSE;
    }
                
    hr = IShellFolder_GetDisplayNameOf(psfFolder, pidlLast, SHGDN_FORPARSING, &strret);
    IShellFolder_Release(psfFolder);
    if (FAILED(hr)) return FALSE;

    hr = StrRetToBufW(&strret, pidlLast, path, path_size);

    TRACE_(shell)("-- %s, 0x%08x\n",debugstr_w(path), hr);
    return SUCCEEDED(hr);
}

/*************************************************************************
 *    SHBindToParent        [shell version 5.0]
 */
HRESULT WINAPI SHBindToParent(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv, LPCITEMIDLIST *ppidlLast)
{
    IShellFolder    * psfDesktop;
    HRESULT         hr=E_FAIL;

    TRACE_(shell)("pidl=%p\n", pidl);
    pdump(pidl);
    
    if (!pidl || !ppv)
        return E_INVALIDARG;
    
    *ppv = NULL;
    if (ppidlLast)
        *ppidlLast = NULL;

    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
        return hr;

    if (_ILIsPidlSimple(pidl))
    {
        /* we are on desktop level */
        hr = IShellFolder_QueryInterface(psfDesktop, riid, ppv);
    }
    else
    {
        LPITEMIDLIST pidlParent = ILClone(pidl);
        ILRemoveLastID(pidlParent);
        hr = IShellFolder_BindToObject(psfDesktop, pidlParent, NULL, riid, ppv);
        SHFree (pidlParent);
    }

    IShellFolder_Release(psfDesktop);

    if (SUCCEEDED(hr) && ppidlLast)
        *ppidlLast = ILFindLastID(pidl);

    TRACE_(shell)("-- psf=%p pidl=%p ret=0x%08x\n", *ppv, (ppidlLast)?*ppidlLast:NULL, hr);
    return hr;
}

HRESULT WINAPI SHBindToObject(IShellFolder *psf, LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void **ppv)
{
    IShellFolder *psfDesktop = NULL;
    HRESULT hr;

    TRACE_(shell)("%p,%p,%p,%s,%p\n", psf, pidl, pbc, debugstr_guid(riid), ppv);
    pdump(pidl);

    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    if (!psf)
    {
        hr = SHGetDesktopFolder(&psfDesktop);
        if (FAILED(hr))
            return hr;
        psf = psfDesktop;
    }

    if (_ILIsPidlSimple(pidl))
        /* we are on desktop level */
        hr = IShellFolder_QueryInterface(psf, riid, ppv);
    else
        hr = IShellFolder_BindToObject(psf, pidl, pbc, riid, ppv);

    if (psfDesktop)
        IShellFolder_Release(psfDesktop);

    TRACE_(shell)("-- ppv=%p ret=0x%08x\n", *ppv, hr);
    return hr;
}


/*************************************************************************
 * SHParseDisplayName             [SHELL32.@]
 */
HRESULT WINAPI SHParseDisplayName(LPCWSTR name, IBindCtx *bindctx, LPITEMIDLIST *pidlist,
                                  SFGAOF attr_in, SFGAOF *attr_out)
{
    IShellFolder *desktop;
    HRESULT hr;

    TRACE("%s %p %p %d %p\n", debugstr_w(name), bindctx, pidlist, attr_in, attr_out);

    *pidlist = NULL;

    if (!name) return E_INVALIDARG;

    hr = SHGetDesktopFolder(&desktop);
    if (hr != S_OK) return hr;

    hr = IShellFolder_ParseDisplayName(desktop, NULL, bindctx, (LPWSTR)name, NULL, pidlist, &attr_in);
    if (attr_out) *attr_out = attr_in;

    IShellFolder_Release(desktop);

    return hr;
}

/*************************************************************************
 * SHGetNameFromIDList             [SHELL32.@]
 */
HRESULT WINAPI SHGetNameFromIDList(PCIDLIST_ABSOLUTE pidl, SIGDN sigdnName, PWSTR *ppszName)
{
    IShellFolder *psfparent;
    LPCITEMIDLIST child_pidl;
    STRRET disp_name;
    HRESULT ret;

    TRACE("%p 0x%08x %p\n", pidl, sigdnName, ppszName);

    *ppszName = NULL;
    ret = SHBindToParent(pidl, &IID_IShellFolder, (void**)&psfparent, &child_pidl);
    if(SUCCEEDED(ret))
    {
        switch(sigdnName)
        {
                                                /* sigdnName & 0xffff */
        case SIGDN_NORMALDISPLAY:               /* SHGDN_NORMAL */
        case SIGDN_PARENTRELATIVEPARSING:       /* SHGDN_INFOLDER | SHGDN_FORPARSING */
        case SIGDN_PARENTRELATIVEEDITING:       /* SHGDN_INFOLDER | SHGDN_FOREDITING */
        case SIGDN_DESKTOPABSOLUTEPARSING:      /* SHGDN_FORPARSING */
        case SIGDN_DESKTOPABSOLUTEEDITING:      /* SHGDN_FOREDITING | SHGDN_FORADDRESSBAR*/
        case SIGDN_PARENTRELATIVEFORADDRESSBAR: /* SIGDN_INFOLDER | SHGDN_FORADDRESSBAR */
        case SIGDN_PARENTRELATIVE:              /* SIGDN_INFOLDER */

            disp_name.uType = STRRET_WSTR;
            ret = IShellFolder_GetDisplayNameOf(psfparent, child_pidl,
                                                sigdnName & 0xffff,
                                                &disp_name);
            if(SUCCEEDED(ret))
                ret = StrRetToStrW(&disp_name, pidl, ppszName);

            break;

        case SIGDN_FILESYSPATH:
            *ppszName = CoTaskMemAlloc(sizeof(WCHAR)*MAX_PATH);
            if(SHGetPathFromIDListW(pidl, *ppszName))
            {
                TRACE("Got string %s\n", debugstr_w(*ppszName));
                ret = S_OK;
            }
            else
            {
                CoTaskMemFree(*ppszName);
                ret = E_INVALIDARG;
            }
            break;

        case SIGDN_URL:
        default:
            FIXME("Unsupported SIGDN %x\n", sigdnName);
            ret = E_FAIL;
        }

        IShellFolder_Release(psfparent);
    }
    return ret;
}

/*************************************************************************
 * SHGetIDListFromObject             [SHELL32.@]
 */
HRESULT WINAPI SHGetIDListFromObject(IUnknown *punk, PIDLIST_ABSOLUTE *ppidl)
{
    IPersistIDList *ppersidl;
    IPersistFolder2 *ppf2;
    IDataObject *pdo;
    IFolderView *pfv;
    HRESULT ret;

    if(!punk)
        return E_NOINTERFACE;

    *ppidl = NULL;

    /* Try IPersistIDList */
    ret = IUnknown_QueryInterface(punk, &IID_IPersistIDList, (void**)&ppersidl);
    if(SUCCEEDED(ret))
    {
        TRACE("IPersistIDList (%p)\n", ppersidl);
        ret = IPersistIDList_GetIDList(ppersidl, ppidl);
        IPersistIDList_Release(ppersidl);
        if(SUCCEEDED(ret))
            return ret;
    }

    /* Try IPersistFolder2 */
    ret = IUnknown_QueryInterface(punk, &IID_IPersistFolder2, (void**)&ppf2);
    if(SUCCEEDED(ret))
    {
        TRACE("IPersistFolder2 (%p)\n", ppf2);
        ret = IPersistFolder2_GetCurFolder(ppf2, ppidl);
        IPersistFolder2_Release(ppf2);
        if(SUCCEEDED(ret))
            return ret;
    }

    /* Try IDataObject */
    ret = IUnknown_QueryInterface(punk, &IID_IDataObject, (void**)&pdo);
    if(SUCCEEDED(ret))
    {
        IShellItem *psi;
        TRACE("IDataObject (%p)\n", pdo);
        ret = SHGetItemFromDataObject(pdo, DOGIF_ONLY_IF_ONE,
                                      &IID_IShellItem, (void**)&psi);
        if(SUCCEEDED(ret))
        {
            ret = SHGetIDListFromObject((IUnknown*)psi, ppidl);
            IShellItem_Release(psi);
        }
        IDataObject_Release(pdo);

        if(SUCCEEDED(ret))
            return ret;
    }

    /* Try IFolderView */
    ret = IUnknown_QueryInterface(punk, &IID_IFolderView, (void**)&pfv);
    if(SUCCEEDED(ret))
    {
        IShellFolder *psf;
        TRACE("IFolderView (%p)\n", pfv);
        ret = IFolderView_GetFolder(pfv, &IID_IShellFolder, (void**)&psf);
        if(SUCCEEDED(ret))
        {
            /* We might be able to get IPersistFolder2 from a shellfolder. */
            ret = SHGetIDListFromObject((IUnknown*)psf, ppidl);
            IShellFolder_Release(psf);
        }
        IFolderView_Release(pfv);
        return ret;
    }

    return ret;
}

/**************************************************************************
 *
 *        internal functions
 *
 *    ### 1. section creating pidls ###
 *
 *************************************************************************
 */

/* Basic PIDL constructor.  Allocates size + 5 bytes, where:
 * - two bytes are SHITEMID.cb
 * - one byte is PIDLDATA.type
 * - two bytes are the NULL PIDL terminator
 * Sets type of the returned PIDL to type.
 */
static LPITEMIDLIST _ILAlloc(PIDLTYPE type, unsigned int size)
{
    LPITEMIDLIST pidlOut = NULL;

    pidlOut = SHAlloc(size + 5);
    if(pidlOut)
    {
        LPPIDLDATA pData;
        LPITEMIDLIST pidlNext;

        ZeroMemory(pidlOut, size + 5);
        pidlOut->mkid.cb = size + 3;

        pData = _ILGetDataPointer(pidlOut);
        if (pData)
            pData->type = type;

        pidlNext = ILGetNext(pidlOut);
        if (pidlNext)
            pidlNext->mkid.cb = 0x00;
        TRACE("-- (pidl=%p, size=%u)\n", pidlOut, size);
    }

    return pidlOut;
}

LPITEMIDLIST _ILCreateDesktop(void)
{
    LPITEMIDLIST ret;

    TRACE("()\n");
    ret = SHAlloc(2);
    if (ret)
        ret->mkid.cb = 0;
    return ret;
}

LPITEMIDLIST _ILCreateMyComputer(void)
{
    TRACE("()\n");
    return _ILCreateGuid(PT_GUID, &CLSID_MyComputer);
}

LPITEMIDLIST _ILCreateMyDocuments(void)
{
    TRACE("()\n");
    return _ILCreateGuid(PT_GUID, &CLSID_MyDocuments);
}

LPITEMIDLIST _ILCreateIExplore(void)
{
    TRACE("()\n");
    return _ILCreateGuid(PT_GUID, &CLSID_Internet);
}

LPITEMIDLIST _ILCreateControlPanel(void)
{
    LPITEMIDLIST parent = _ILCreateGuid(PT_GUID, &CLSID_MyComputer), ret = NULL;

    TRACE("()\n");
    if (parent)
    {
        LPITEMIDLIST cpl = _ILCreateGuid(PT_SHELLEXT, &CLSID_ControlPanel);

        if (cpl)
        {
            ret = ILCombine(parent, cpl);
            SHFree(cpl);
        }
        SHFree(parent);
    }
    return ret;
}

LPITEMIDLIST _ILCreatePrinters(void)
{
    LPITEMIDLIST parent = _ILCreateGuid(PT_GUID, &CLSID_MyComputer), ret = NULL;

    TRACE("()\n");
    if (parent)
    {
        LPITEMIDLIST printers = _ILCreateGuid(PT_YAGUID, &CLSID_Printers);

        if (printers)
        {
            ret = ILCombine(parent, printers);
            SHFree(printers);
        }
        SHFree(parent);
    }
    return ret;
}

LPITEMIDLIST _ILCreateNetwork(void)
{
    TRACE("()\n");
    return _ILCreateGuid(PT_GUID, &CLSID_NetworkPlaces);
}

LPITEMIDLIST _ILCreateBitBucket(void)
{
    TRACE("()\n");
    return _ILCreateGuid(PT_GUID, &CLSID_RecycleBin);
}

LPITEMIDLIST _ILCreateNetHood(void)
{
    TRACE("()\n");
    return _ILCreateGuid(PT_GUID, &CLSID_NetworkPlaces);
}

LPITEMIDLIST _ILCreateGuid(PIDLTYPE type, REFIID guid)
{
    LPITEMIDLIST pidlOut;

    if (type == PT_SHELLEXT || type == PT_GUID || type == PT_YAGUID)
    {
        pidlOut = _ILAlloc(type, sizeof(GUIDStruct));
        if (pidlOut)
        {
            LPPIDLDATA pData = _ILGetDataPointer(pidlOut);

            pData->u.guid.guid = *guid;
            TRACE("-- create GUID-pidl %s\n",
                  debugstr_guid(&(pData->u.guid.guid)));
        }
    }
    else
    {
        WARN("%d: invalid type for GUID\n", type);
        pidlOut = NULL;
    }
    return pidlOut;
}

LPITEMIDLIST _ILCreateGuidFromStrA(LPCSTR szGUID)
{
    IID iid;

    if (FAILED(SHCLSIDFromStringA(szGUID, &iid)))
    {
        ERR("%s is not a GUID\n", szGUID);
        return NULL;
    }
    return _ILCreateGuid(PT_GUID, &iid);
}

LPITEMIDLIST _ILCreateGuidFromStrW(LPCWSTR szGUID)
{
    IID iid;

    if (FAILED(SHCLSIDFromStringW(szGUID, &iid)))
    {
        ERR("%s is not a GUID\n", debugstr_w(szGUID));
        return NULL;
    }
    return _ILCreateGuid(PT_GUID, &iid);
}

LPITEMIDLIST _ILCreateFromFindDataW( const WIN32_FIND_DATAW *wfd )
{
    char    buff[MAX_PATH + 14 +1]; /* see WIN32_FIND_DATA */
    DWORD   len, len1, wlen, alen;
    LPITEMIDLIST pidl;
    PIDLTYPE type;

    if (!wfd)
        return NULL;

    TRACE("(%s, %s)\n",debugstr_w(wfd->cAlternateFileName), debugstr_w(wfd->cFileName));

    /* prepare buffer with both names */
    len = WideCharToMultiByte(CP_ACP,0,wfd->cFileName,-1,buff,MAX_PATH,NULL,NULL);
    len1 = WideCharToMultiByte(CP_ACP,0,wfd->cAlternateFileName,-1, buff+len, sizeof(buff)-len, NULL, NULL);
    alen = len + len1;

    type = (wfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? PT_FOLDER : PT_VALUE;

    wlen = lstrlenW(wfd->cFileName) + 1;
    pidl = _ILAlloc(type, FIELD_OFFSET(FileStruct, szNames[alen + (alen & 1)]) +
                    FIELD_OFFSET(FileStructW, wszName[wlen]) + sizeof(WORD));
    if (pidl)
    {
        LPPIDLDATA pData = _ILGetDataPointer(pidl);
        FileStruct *fs = &pData->u.file;
        FileStructW *fsw;
        WORD *pOffsetW;

        FileTimeToDosDateTime( &wfd->ftLastWriteTime, &fs->uFileDate, &fs->uFileTime);
        fs->dwFileSize = wfd->nFileSizeLow;
        fs->uFileAttribs = wfd->dwFileAttributes;
        memcpy(fs->szNames, buff, alen);

        fsw = (FileStructW*)(pData->u.file.szNames + alen + (alen & 0x1));
        fsw->cbLen = FIELD_OFFSET(FileStructW, wszName[wlen]) + sizeof(WORD);
        FileTimeToDosDateTime( &wfd->ftCreationTime, &fsw->uCreationDate, &fsw->uCreationTime);
        FileTimeToDosDateTime( &wfd->ftLastAccessTime, &fsw->uLastAccessDate, &fsw->uLastAccessTime);
        memcpy(fsw->wszName, wfd->cFileName, wlen * sizeof(WCHAR));

        pOffsetW = (WORD*)((LPBYTE)pidl + pidl->mkid.cb - sizeof(WORD));
        *pOffsetW = (LPBYTE)fsw - (LPBYTE)pidl;
        TRACE("-- Set Value: %s\n",debugstr_w(fsw->wszName));
    }
    return pidl;

}

HRESULT _ILCreateFromPathW(LPCWSTR szPath, LPITEMIDLIST* ppidl)
{
    HANDLE hFile;
    WIN32_FIND_DATAW stffile;

    if (!ppidl)
        return E_INVALIDARG;

    hFile = FindFirstFileW(szPath, &stffile);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    FindClose(hFile);

    *ppidl = _ILCreateFromFindDataW(&stffile);

    return *ppidl ? S_OK : E_OUTOFMEMORY;
}

LPITEMIDLIST _ILCreateDrive(LPCWSTR lpszNew)
{
    LPITEMIDLIST pidlOut;

    TRACE("(%s)\n",debugstr_w(lpszNew));

    pidlOut = _ILAlloc(PT_DRIVE, sizeof(DriveStruct));
    if (pidlOut)
    {
        LPSTR pszDest;

        pszDest = _ILGetTextPointer(pidlOut);
        if (pszDest)
        {
            strcpy(pszDest, "x:\\");
            pszDest[0]=toupperW(lpszNew[0]);
            TRACE("-- create Drive: %s\n", debugstr_a(pszDest));
        }
    }
    return pidlOut;
}

LPITEMIDLIST _ILCreateEntireNetwork(void)
{
    LPITEMIDLIST pidlOut;

    TRACE("\n");

    pidlOut = _ILAlloc(PT_NETWORK, FIELD_OFFSET(PIDLDATA, u.network.szNames[sizeof("Entire Network")]));
    if (pidlOut)
    {
        LPPIDLDATA pData = _ILGetDataPointer(pidlOut);

        pData->u.network.dummy = 0;
        strcpy(pData->u.network.szNames, "Entire Network");
    }
    return pidlOut;
}

/**************************************************************************
 *  _ILGetDrive()
 *
 *  Gets the text for the drive eg. 'c:\'
 *
 * RETURNS
 *  strlen (lpszText)
 */
DWORD _ILGetDrive(LPCITEMIDLIST pidl,LPSTR pOut, UINT uSize)
{
    TRACE("(%p,%p,%u)\n",pidl,pOut,uSize);

    if(_ILIsMyComputer(pidl))
        pidl = ILGetNext(pidl);

    if (pidl && _ILIsDrive(pidl))
        return _ILSimpleGetText(pidl, pOut, uSize);

    return 0;
}

/**************************************************************************
 *
 *    ### 2. section testing pidls ###
 *
 **************************************************************************
 *  _ILIsUnicode()
 *  _ILIsDesktop()
 *  _ILIsMyComputer()
 *  _ILIsSpecialFolder()
 *  _ILIsDrive()
 *  _ILIsFolder()
 *  _ILIsValue()
 *  _ILIsPidlSimple()
 */
BOOL _ILIsUnicode(LPCITEMIDLIST pidl)
{
    LPPIDLDATA lpPData = _ILGetDataPointer(pidl);

    TRACE("(%p)\n",pidl);

    return (pidl && lpPData && PT_VALUEW == lpPData->type);
}

BOOL _ILIsDesktop(LPCITEMIDLIST pidl)
{
    TRACE("(%p)\n",pidl);

    return !pidl || !pidl->mkid.cb;
}

BOOL _ILIsMyComputer(LPCITEMIDLIST pidl)
{
    REFIID iid = _ILGetGUIDPointer(pidl);

    TRACE("(%p)\n",pidl);

    if (iid)
        return IsEqualIID(iid, &CLSID_MyComputer);
    return FALSE;
}

BOOL _ILIsSpecialFolder (LPCITEMIDLIST pidl)
{
    LPPIDLDATA lpPData = _ILGetDataPointer(pidl);

    TRACE("(%p)\n",pidl);

    return (pidl && ( (lpPData && (PT_GUID== lpPData->type || PT_SHELLEXT== lpPData->type || PT_YAGUID == lpPData->type)) ||
              (pidl && pidl->mkid.cb == 0x00)
            ));
}

BOOL _ILIsDrive(LPCITEMIDLIST pidl)
{
    LPPIDLDATA lpPData = _ILGetDataPointer(pidl);

    TRACE("(%p)\n",pidl);

    return (pidl && lpPData && (PT_DRIVE == lpPData->type ||
                    PT_DRIVE1 == lpPData->type ||
                    PT_DRIVE2 == lpPData->type ||
                    PT_DRIVE3 == lpPData->type));
}

BOOL _ILIsFolder(LPCITEMIDLIST pidl)
{
    LPPIDLDATA lpPData = _ILGetDataPointer(pidl);

    TRACE("(%p)\n",pidl);

    return (pidl && lpPData && (PT_FOLDER == lpPData->type || PT_FOLDER1 == lpPData->type));
}

BOOL _ILIsValue(LPCITEMIDLIST pidl)
{
    LPPIDLDATA lpPData = _ILGetDataPointer(pidl);

    TRACE("(%p)\n",pidl);

    return (pidl && lpPData && PT_VALUE == lpPData->type);
}

BOOL _ILIsCPanelStruct(LPCITEMIDLIST pidl)
{
    LPPIDLDATA lpPData = _ILGetDataPointer(pidl);

    TRACE("(%p)\n",pidl);

    return (pidl && lpPData && (lpPData->type == 0));
}

/**************************************************************************
 *    _ILIsPidlSimple
 */
BOOL _ILIsPidlSimple(LPCITEMIDLIST pidl)
{
    BOOL ret = TRUE;

    if(! _ILIsDesktop(pidl))    /* pidl=NULL or mkid.cb=0 */
    {
        WORD len = pidl->mkid.cb;
        LPCITEMIDLIST pidlnext = (LPCITEMIDLIST) (((const BYTE*)pidl) + len );

        if (pidlnext->mkid.cb)
            ret = FALSE;
    }

    TRACE("%s\n", ret ? "Yes" : "No");
    return ret;
}

/**************************************************************************
 *
 *    ### 3. section getting values from pidls ###
 */

 /**************************************************************************
 *  _ILSimpleGetText
 *
 * gets the text for the first item in the pidl (eg. simple pidl)
 *
 * returns the length of the string
 */
DWORD _ILSimpleGetText (LPCITEMIDLIST pidl, LPSTR szOut, UINT uOutSize)
{
    DWORD        dwReturn=0;
    LPSTR        szSrc;
    LPWSTR       szSrcW;
    GUID const * riid;
    char szTemp[MAX_PATH];

    TRACE("(%p %p %x)\n",pidl,szOut,uOutSize);

    if (!pidl)
        return 0;

    if (szOut)
        *szOut = 0;

    if (_ILIsDesktop(pidl))
    {
        /* desktop */
        if (HCR_GetClassNameA(&CLSID_ShellDesktop, szTemp, MAX_PATH))
        {
            if (szOut)
                lstrcpynA(szOut, szTemp, uOutSize);

            dwReturn = strlen (szTemp);
        }
    }
    else if (( szSrc = _ILGetTextPointer(pidl) ))
    {
        /* filesystem */
        if (szOut)
            lstrcpynA(szOut, szSrc, uOutSize);

        dwReturn = strlen(szSrc);
    }
    else if (( szSrcW = _ILGetTextPointerW(pidl) ))
    {
        /* unicode filesystem */
        WideCharToMultiByte(CP_ACP,0,szSrcW, -1, szTemp, MAX_PATH, NULL, NULL);

        if (szOut)
            lstrcpynA(szOut, szTemp, uOutSize);

        dwReturn = strlen (szTemp);
    }
    else if (( riid = _ILGetGUIDPointer(pidl) ))
    {
        /* special folder */
        if ( HCR_GetClassNameA(riid, szTemp, MAX_PATH) )
        {
            if (szOut)
                lstrcpynA(szOut, szTemp, uOutSize);

            dwReturn = strlen (szTemp);
        }
    }
    else
    {
        ERR("-- no text\n");
    }

    TRACE("-- (%p=%s 0x%08x)\n",szOut,debugstr_a(szOut),dwReturn);
    return dwReturn;
}

 /**************************************************************************
 *  _ILSimpleGetTextW
 *
 * gets the text for the first item in the pidl (eg. simple pidl)
 *
 * returns the length of the string
 */
DWORD _ILSimpleGetTextW (LPCITEMIDLIST pidl, LPWSTR szOut, UINT uOutSize)
{
    DWORD   dwReturn;
    FileStructW *pFileStructW = _ILGetFileStructW(pidl);

    TRACE("(%p %p %x)\n",pidl,szOut,uOutSize);

    if (pFileStructW) {
        lstrcpynW(szOut, pFileStructW->wszName, uOutSize);
        dwReturn = lstrlenW(pFileStructW->wszName);
    } else {
        GUID const * riid;
        WCHAR szTemp[MAX_PATH];
        LPSTR szSrc;
        LPWSTR szSrcW;
        dwReturn=0;

        if (!pidl)
            return 0;

        if (szOut)
            *szOut = 0;

        if (_ILIsDesktop(pidl))
        {
            /* desktop */
            if (HCR_GetClassNameW(&CLSID_ShellDesktop, szTemp, MAX_PATH))
            {
                if (szOut)
                    lstrcpynW(szOut, szTemp, uOutSize);

                dwReturn = lstrlenW (szTemp);
            }
        }
        else if (( szSrcW = _ILGetTextPointerW(pidl) ))
        {
            /* unicode filesystem */
            if (szOut)
                lstrcpynW(szOut, szSrcW, uOutSize);

            dwReturn = lstrlenW(szSrcW);
        }
        else if (( szSrc = _ILGetTextPointer(pidl) ))
        {
            /* filesystem */
            MultiByteToWideChar(CP_ACP, 0, szSrc, -1, szTemp, MAX_PATH);

            if (szOut)
                lstrcpynW(szOut, szTemp, uOutSize);

            dwReturn = lstrlenW (szTemp);
        }
        else if (( riid = _ILGetGUIDPointer(pidl) ))
        {
            /* special folder */
            if ( HCR_GetClassNameW(riid, szTemp, MAX_PATH) )
            {
                if (szOut)
                    lstrcpynW(szOut, szTemp, uOutSize);

                dwReturn = lstrlenW (szTemp);
            }
        }
        else
        {
            ERR("-- no text\n");
        }
    }

    TRACE("-- (%p=%s 0x%08x)\n",szOut,debugstr_w(szOut),dwReturn);
    return dwReturn;
}

/**************************************************************************
 *
 *    ### 4. getting pointers to parts of pidls ###
 *
 **************************************************************************
 *  _ILGetDataPointer()
 */
LPPIDLDATA _ILGetDataPointer(LPCITEMIDLIST pidl)
{
    if(!_ILIsEmpty(pidl))
        return (LPPIDLDATA)pidl->mkid.abID;
    return NULL;
}

/**************************************************************************
 *  _ILGetTextPointerW()
 * gets a pointer to the unicode long filename string stored in the pidl
 */
static LPWSTR _ILGetTextPointerW(LPCITEMIDLIST pidl)
{
    /* TRACE(pidl,"(pidl%p)\n", pidl);*/

    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (!pdata)
        return NULL;

    switch (pdata->type)
    {
    case PT_GUID:
    case PT_SHELLEXT:
    case PT_YAGUID:
        return NULL;

    case PT_DRIVE:
    case PT_DRIVE1:
    case PT_DRIVE2:
    case PT_DRIVE3:
        /*return (LPSTR)&(pdata->u.drive.szDriveName);*/
        return NULL;

    case PT_FOLDER:
    case PT_FOLDER1:
    case PT_VALUE:
    case PT_IESPECIAL1:
    case PT_IESPECIAL2:
        /*return (LPSTR)&(pdata->u.file.szNames);*/
        return NULL;

    case PT_WORKGRP:
    case PT_COMP:
    case PT_NETWORK:
    case PT_NETPROVIDER:
    case PT_SHARE:
        /*return (LPSTR)&(pdata->u.network.szNames);*/
        return NULL;

    case PT_VALUEW:
        return (LPWSTR)pdata->u.file.szNames;
    }
    return NULL;
}


/**************************************************************************
 *  _ILGetTextPointer()
 * gets a pointer to the long filename string stored in the pidl
 */
LPSTR _ILGetTextPointer(LPCITEMIDLIST pidl)
{
    /* TRACE(pidl,"(pidl%p)\n", pidl);*/

    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (!pdata)
        return NULL;

    switch (pdata->type)
    {
    case PT_GUID:
    case PT_SHELLEXT:
    case PT_YAGUID:
        return NULL;

    case PT_DRIVE:
    case PT_DRIVE1:
    case PT_DRIVE2:
    case PT_DRIVE3:
        return pdata->u.drive.szDriveName;

    case PT_FOLDER:
    case PT_FOLDER1:
    case PT_VALUE:
    case PT_IESPECIAL1:
    case PT_IESPECIAL2:
        return pdata->u.file.szNames;

    case PT_WORKGRP:
    case PT_COMP:
    case PT_NETWORK:
    case PT_NETPROVIDER:
    case PT_SHARE:
        return pdata->u.network.szNames;
    }
    return NULL;
}

/**************************************************************************
 *  _ILGetSTextPointer()
 * gets a pointer to the short filename string stored in the pidl
 */
static LPSTR _ILGetSTextPointer(LPCITEMIDLIST pidl)
{
    /* TRACE(pidl,"(pidl%p)\n", pidl); */

    LPPIDLDATA pdata =_ILGetDataPointer(pidl);

    if (!pdata)
        return NULL;

    switch (pdata->type)
    {
    case PT_FOLDER:
    case PT_VALUE:
    case PT_IESPECIAL1:
    case PT_IESPECIAL2:
        return pdata->u.file.szNames + strlen (pdata->u.file.szNames) + 1;

    case PT_WORKGRP:
        return pdata->u.network.szNames + strlen (pdata->u.network.szNames) + 1;
    }
    return NULL;
}

/**************************************************************************
 * _ILGetGUIDPointer()
 *
 * returns reference to guid stored in some pidls
 */
IID* _ILGetGUIDPointer(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata =_ILGetDataPointer(pidl);

    TRACE("%p\n", pidl);

    if (!pdata)
        return NULL;

    TRACE("pdata->type 0x%04x\n", pdata->type);
    switch (pdata->type)
    {
    case PT_SHELLEXT:
    case PT_GUID:
    case PT_YAGUID:
        return &(pdata->u.guid.guid);

    default:
        TRACE("Unknown pidl type 0x%04x\n", pdata->type);
        break;
    }
    return NULL;
}

/******************************************************************************
 * _ILGetFileStructW [Internal]
 *
 * Get pointer the a SHITEMID's FileStructW field if present
 *
 * PARAMS
 *  pidl [I] The SHITEMID
 *
 * RETURNS
 *  Success: Pointer to pidl's FileStructW field.
 *  Failure: NULL
 */
FileStructW* _ILGetFileStructW(LPCITEMIDLIST pidl) {
    FileStructW *pFileStructW;
    WORD cbOffset;
    
    if (!(_ILIsValue(pidl) || _ILIsFolder(pidl)))
        return NULL;

    cbOffset = *(const WORD *)((const BYTE *)pidl + pidl->mkid.cb - sizeof(WORD));
    pFileStructW = (FileStructW*)((LPBYTE)pidl + cbOffset);

    /* Currently I don't see a fool prove way to figure out if a pidl is for sure of WinXP
     * style with a FileStructW member. If we switch all our shellfolder-implementations to
     * the new format, this won't be a problem. For now, we do as many sanity checks as possible. */
    if ((cbOffset & 0x1) || /* FileStructW member is word aligned in the pidl */
        /* FileStructW is positioned after FileStruct */
        cbOffset < sizeof(pidl->mkid.cb) + sizeof(PIDLTYPE) + sizeof(FileStruct) ||
        /* There has to be enough space at cbOffset in the pidl to hold FileStructW and cbOffset */
        cbOffset > pidl->mkid.cb - sizeof(cbOffset) - sizeof(FileStructW) ||
        pidl->mkid.cb != cbOffset + pFileStructW->cbLen)
    {
        WARN("Invalid pidl format (cbOffset = %d)!\n", cbOffset);
        return NULL;
    }

    return pFileStructW;
}

/*************************************************************************
 * _ILGetFileDateTime
 *
 * Given the ItemIdList, get the FileTime
 *
 * PARAMS
 *      pidl        [I] The ItemIDList
 *      pFt         [I] the resulted FILETIME of the file
 *
 * RETURNS
 *     True if Successful
 *
 * NOTES
 *
 */
BOOL _ILGetFileDateTime(LPCITEMIDLIST pidl, FILETIME *pFt)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (!pdata)
        return FALSE;

    switch (pdata->type)
    {
    case PT_FOLDER:
    case PT_VALUE:
        DosDateTimeToFileTime(pdata->u.file.uFileDate, pdata->u.file.uFileTime, pFt);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

BOOL _ILGetFileDate (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{
    FILETIME ft,lft;
    SYSTEMTIME time;
    BOOL ret;

    if (_ILGetFileDateTime( pidl, &ft ))
    {
        FileTimeToLocalFileTime(&ft, &lft);
        FileTimeToSystemTime (&lft, &time);

        ret = GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&time, NULL,  pOut, uOutSize);
        if (ret) 
        {
            /* Append space + time without seconds */
            pOut[ret-1] = ' ';
            GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &time, NULL, &pOut[ret], uOutSize - ret);
        }
    }
    else
    {
        pOut[0] = '\0';
        ret = FALSE;
    }
    return ret;
}

/*************************************************************************
 * _ILGetFileSize
 *
 * Given the ItemIdList, get the FileSize
 *
 * PARAMS
 *    pidl     [I] The ItemIDList
 *    pOut     [I] The buffer to save the result
 *    uOutsize [I] The size of the buffer
 *
 * RETURNS
 *    The FileSize
 *
 * NOTES
 *    pOut can be null when no string is needed
 *
 */
DWORD _ILGetFileSize (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);
    DWORD dwSize;

    if (!pdata)
        return 0;

    switch (pdata->type)
    {
    case PT_VALUE:
        dwSize = pdata->u.file.dwFileSize;
        if (pOut)
            StrFormatKBSizeA(dwSize, pOut, uOutSize);
        return dwSize;
    }
    if (pOut)
        *pOut = 0x00;
    return 0;
}

BOOL _ILGetExtension (LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{
    char szTemp[MAX_PATH];
    const char * pPoint;
    LPCITEMIDLIST  pidlTemp=pidl;

    TRACE("pidl=%p\n",pidl);

    if (!pidl)
        return FALSE;

    pidlTemp = ILFindLastID(pidl);

    if (!_ILIsValue(pidlTemp))
        return FALSE;
    if (!_ILSimpleGetText(pidlTemp, szTemp, MAX_PATH))
        return FALSE;

    pPoint = PathFindExtensionA(szTemp);

    if (!*pPoint)
        return FALSE;

    pPoint++;
    lstrcpynA(pOut, pPoint, uOutSize);
    TRACE("%s\n",pOut);

    return TRUE;
}

/*************************************************************************
 * _ILGetFileType
 *
 * Given the ItemIdList, get the file type description
 *
 * PARAMS
 *      pidl        [I] The ItemIDList (simple)
 *      pOut        [I] The buffer to save the result
 *      uOutsize    [I] The size of the buffer
 *
 * RETURNS
 *    nothing
 *
 * NOTES
 *    This function copies as much as possible into the buffer.
 */
void _ILGetFileType(LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{
    if(_ILIsValue(pidl))
    {
        char sTemp[64];

        /* "name" or "name." or any unhandled => "File" */
        lstrcpynA (pOut, "File", uOutSize);

        /* If there's space for more, try to get a better description. */
        if (uOutSize > 6 && _ILGetExtension (pidl, sTemp, 64))
        {
            if (!( HCR_MapTypeToValueA(sTemp, sTemp, 64, TRUE)
                && HCR_MapTypeToValueA(sTemp, pOut, uOutSize, FALSE ))
                && sTemp[0])
            {
                lstrcpynA (pOut, sTemp, uOutSize - 6);
                strcat (pOut, " file");
            }
        }
    }
    else
        lstrcpynA(pOut, "Folder", uOutSize);
}

/*************************************************************************
 * _ILGetFileAttributes
 *
 * Given the ItemIdList, get the Attrib string format
 *
 * PARAMS
 *      pidl        [I] The ItemIDList
 *      pOut        [I] The buffer to save the result
 *      uOutsize    [I] The size of the Buffer
 *
 * RETURNS
 *     Attributes
 *
 * FIXME
 *  return value 0 in case of error is a valid return value
 *
 */
DWORD _ILGetFileAttributes(LPCITEMIDLIST pidl, LPSTR pOut, UINT uOutSize)
{
    LPPIDLDATA pData = _ILGetDataPointer(pidl);
    WORD wAttrib = 0;
    int i;

    if (!pData)
        return 0;

    switch(pData->type)
    {
    case PT_FOLDER:
    case PT_VALUE:
        wAttrib = pData->u.file.uFileAttribs;
        break;
    }

    if(uOutSize >= 6)
    {
        i=0;
        if(wAttrib & FILE_ATTRIBUTE_READONLY)
            pOut[i++] = 'R';
        if(wAttrib & FILE_ATTRIBUTE_HIDDEN)
            pOut[i++] = 'H';
        if(wAttrib & FILE_ATTRIBUTE_SYSTEM)
            pOut[i++] = 'S';
        if(wAttrib & FILE_ATTRIBUTE_ARCHIVE)
            pOut[i++] = 'A';
        if(wAttrib & FILE_ATTRIBUTE_COMPRESSED)
            pOut[i++] = 'C';
        pOut[i] = 0x00;
    }
    return wAttrib;
}

/*************************************************************************
 * ILFreeaPidl
 *
 * frees an aPidl struct
 */
void _ILFreeaPidl(LPITEMIDLIST * apidl, UINT cidl)
{
    UINT   i;

    if (apidl)
    {
        for (i = 0; i < cidl; i++)
            SHFree(apidl[i]);
        SHFree(apidl);
    }
}

/*************************************************************************
 * ILCopyaPidl
 *
 * copies an aPidl struct
 */
LPITEMIDLIST* _ILCopyaPidl(const LPCITEMIDLIST * apidlsrc, UINT cidl)
{
    UINT i;
    LPITEMIDLIST *apidldest;

    if (!apidlsrc)
        return NULL;

    apidldest = SHAlloc(cidl * sizeof(LPITEMIDLIST));

    for (i = 0; i < cidl; i++)
        apidldest[i] = ILClone(apidlsrc[i]);

    return apidldest;
}

/*************************************************************************
 * _ILCopyCidaToaPidl
 *
 * creates aPidl from CIDA
 */
LPITEMIDLIST* _ILCopyCidaToaPidl(LPITEMIDLIST* pidl, const CIDA * cida)
{
    UINT i;
    LPITEMIDLIST *dst;

    dst = SHAlloc(cida->cidl * sizeof(LPITEMIDLIST));
    if (!dst)
        return NULL;

    if (pidl)
        *pidl = ILClone((LPCITEMIDLIST)(&((const BYTE*)cida)[cida->aoffset[0]]));

    for (i = 0; i < cida->cidl; i++)
        dst[i] = ILClone((LPCITEMIDLIST)(&((const BYTE*)cida)[cida->aoffset[i + 1]]));

    return dst;
}
