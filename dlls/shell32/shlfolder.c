/*
 *	Shell Folder stuff
 *
 *	Copyright 1997			Marcus Meissner
 *	Copyright 1998, 1999, 2002	Juergen Schmied
 *
 *	IShellFolder2 and related interfaces
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

#include "pidl.h"
#include "shell32_main.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "shfldr.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/* SHCreateLinks flags */
#define SHCLF_PREFIXNAME        0x01
#define SHCLF_CREATEONDESKTOP   0x02

/***************************************************************************
 *  SHELL32_GetCustomFolderAttribute (internal function)
 *
 * Gets a value from the folder's desktop.ini file, if one exists.
 *
 * PARAMETERS
 *  pidl          [I] Folder containing the desktop.ini file.
 *  pwszHeading   [I] Heading in .ini file.
 *  pwszAttribute [I] Attribute in .ini file.
 *  pwszValue     [O] Buffer to store value into.
 *  cchValue      [I] Size in characters including NULL of buffer pointed to
 *                    by pwszValue.
 *
 *  RETURNS
 *    TRUE if returned non-NULL value.
 *    FALSE otherwise.
 */
static inline BOOL SHELL32_GetCustomFolderAttributeFromPath(
    LPWSTR pwszFolderPath, LPCWSTR pwszHeading, LPCWSTR pwszAttribute,
    LPWSTR pwszValue, DWORD cchValue)
{
    PathAddBackslashW(pwszFolderPath);
    PathAppendW(pwszFolderPath, L"desktop.ini");
    return GetPrivateProfileStringW(pwszHeading, pwszAttribute, L"", pwszValue, cchValue, pwszFolderPath);
}

BOOL SHELL32_GetCustomFolderAttribute(
    LPCITEMIDLIST pidl, LPCWSTR pwszHeading, LPCWSTR pwszAttribute,
    LPWSTR pwszValue, DWORD cchValue)
{
    DWORD dwAttrib = FILE_ATTRIBUTE_SYSTEM;
    WCHAR wszFolderPath[MAX_PATH];

    /* Hack around not having system attribute on non-Windows file systems */
    if (0)
        dwAttrib = _ILGetFileAttributes(pidl, NULL, 0);

    if (dwAttrib & FILE_ATTRIBUTE_SYSTEM)
    {
        if (!SHGetPathFromIDListW(pidl, wszFolderPath))
            return FALSE;

        return SHELL32_GetCustomFolderAttributeFromPath(wszFolderPath, pwszHeading, 
                                                pwszAttribute, pwszValue, cchValue);
    }
    return FALSE;
}

/***************************************************************************
 *  GetNextElement (internal function)
 *
 * Gets a part of a string till the first backslash.
 *
 * PARAMETERS
 *  pszNext [IN] string to get the element from
 *  pszOut  [IN] pointer to buffer which receives string
 *  dwOut   [IN] length of pszOut
 *
 *  RETURNS
 *    LPSTR pointer to first, not yet parsed char
 */

LPCWSTR GetNextElementW (LPCWSTR pszNext, LPWSTR pszOut, DWORD dwOut)
{
    LPCWSTR pszTail = pszNext;
    DWORD dwCopy;

    TRACE ("(%s %p 0x%08lx)\n", debugstr_w(pszNext), pszOut, dwOut);

    *pszOut = 0;

    if (!pszNext || !*pszNext)
	return NULL;

    while (*pszTail && (*pszTail != (WCHAR) '\\'))
	pszTail++;

    dwCopy = pszTail - pszNext + 1;
    lstrcpynW (pszOut, pszNext, (dwOut < dwCopy) ? dwOut : dwCopy);

    if (*pszTail)
	pszTail++;
    else
	pszTail = NULL;

    TRACE ("--(%s %s 0x%08lx %p)\n", debugstr_w (pszNext), debugstr_w (pszOut), dwOut, pszTail);
    return pszTail;
}

HRESULT SHELL32_ParseNextElement (IShellFolder2 * psf, HWND hwndOwner, LPBC pbc,
				  LPITEMIDLIST * pidlInOut, LPOLESTR szNext, DWORD * pEaten, DWORD * pdwAttributes)
{
    LPITEMIDLIST pidlOut = NULL, pidlTemp = NULL;
    IShellFolder *psfChild;
    HRESULT hr;

    TRACE ("(%p, %p, %p, %s)\n", psf, pbc, pidlInOut ? *pidlInOut : NULL, debugstr_w (szNext));

    /* get the shellfolder for the child pidl and let it analyse further */
    hr = IShellFolder2_BindToObject (psf, *pidlInOut, pbc, &IID_IShellFolder, (LPVOID *) & psfChild);

    if (SUCCEEDED(hr)) {
	hr = IShellFolder_ParseDisplayName (psfChild, hwndOwner, pbc, szNext, pEaten, &pidlOut, pdwAttributes);
	IShellFolder_Release (psfChild);

	if (SUCCEEDED(hr)) {
	    pidlTemp = ILCombine (*pidlInOut, pidlOut);

	    if (!pidlTemp)
		hr = E_OUTOFMEMORY;
	}

	if (pidlOut)
	    ILFree (pidlOut);
    }

    ILFree (*pidlInOut);
    *pidlInOut = pidlTemp;

    TRACE ("-- pidl=%p ret=0x%08lx\n", pidlInOut ? *pidlInOut : NULL, hr);
    return hr;
}

/***********************************************************************
 *	SHELL32_CoCreateInitSF
 *
 * Creates a shell folder and initializes it with a pidl and a root folder
 * via IPersistFolder3 or IPersistFolder.
 *
 * NOTES
 *   pathRoot can be NULL for Folders being a drive.
 *   In this case the absolute path is built from pidlChild (eg. C:)
 */
static HRESULT SHELL32_CoCreateInitSF (LPCITEMIDLIST pidlRoot, LPCWSTR pathRoot,
                LPCITEMIDLIST pidlChild, REFCLSID clsid, LPVOID * ppvOut)
{
    HRESULT hr;

    TRACE ("(%p %s %p %s %p)\n", pidlRoot, debugstr_w(pathRoot), pidlChild, debugstr_guid(clsid), ppvOut);

    hr = SHCoCreateInstance(NULL, clsid, NULL, &IID_IShellFolder, ppvOut);
    if (SUCCEEDED (hr))
    {
	LPITEMIDLIST pidlAbsolute = ILCombine (pidlRoot, pidlChild);
	IPersistFolder *pPF;
	IPersistFolder3 *ppf;

        if (_ILIsFolder(pidlChild) &&
            SUCCEEDED (IUnknown_QueryInterface ((IUnknown *) * ppvOut, &IID_IPersistFolder3, (LPVOID *) & ppf))) 
        {
	    PERSIST_FOLDER_TARGET_INFO ppfti;

	    ZeroMemory (&ppfti, sizeof (ppfti));

	    /* fill the PERSIST_FOLDER_TARGET_INFO */
	    ppfti.dwAttributes = -1;
	    ppfti.csidl = -1;

	    /* build path */
	    if (pathRoot) {
		lstrcpynW (ppfti.szTargetParsingName, pathRoot, MAX_PATH - 1);
		PathAddBackslashW(ppfti.szTargetParsingName); /* FIXME: why have drives a backslash here ? */
	    }

	    if (pidlChild) {
                int len = lstrlenW(ppfti.szTargetParsingName);

		if (!_ILSimpleGetTextW(pidlChild, ppfti.szTargetParsingName + len, MAX_PATH - len))
			hr = E_INVALIDARG;
	    }

	    IPersistFolder3_InitializeEx (ppf, NULL, pidlAbsolute, &ppfti);
	    IPersistFolder3_Release (ppf);
	}
	else if (SUCCEEDED ((hr = IUnknown_QueryInterface ((IUnknown *) * ppvOut, &IID_IPersistFolder, (LPVOID *) & pPF)))) {
	    IPersistFolder_Initialize (pPF, pidlAbsolute);
	    IPersistFolder_Release (pPF);
	}
	ILFree (pidlAbsolute);
    }
    TRACE ("-- (%p) ret=0x%08lx\n", *ppvOut, hr);
    return hr;
}

/***********************************************************************
 *	SHELL32_BindToChild [Internal]
 *
 * Common code for IShellFolder_BindToObject.
 * 
 * PARAMS
 *  pidlRoot     [I] The parent shell folder's absolute pidl.
 *  pathRoot     [I] Absolute dos path of the parent shell folder.
 *  pidlComplete [I] PIDL of the child. Relative to pidlRoot.
 *  riid         [I] GUID of the interface, which ppvOut shall be bound to.
 *  ppvOut       [O] A reference to the child's interface (riid).
 *
 * NOTES
 *  pidlComplete has to contain at least one non empty SHITEMID.
 *  This function makes special assumptions on the shell namespace, which
 *  means you probably can't use it for your IShellFolder implementation.
 */
HRESULT SHELL32_BindToChild (LPCITEMIDLIST pidlRoot, const CLSID *clsidChild,
                             LPCWSTR pathRoot, LPCITEMIDLIST pidlComplete, REFIID riid, LPVOID * ppvOut)
{
    GUID const *clsid;
    IShellFolder *pSF;
    HRESULT hr;
    LPITEMIDLIST pidlChild;

    TRACE("(%p %s %p %s %p)\n", pidlRoot, debugstr_w(pathRoot), pidlComplete, debugstr_guid(riid), ppvOut);

    if (!pidlRoot || !ppvOut || _ILIsEmpty(pidlComplete))
        return E_INVALIDARG;

    *ppvOut = NULL;

    pidlChild = ILCloneFirst (pidlComplete);

    if ((clsid = _ILGetGUIDPointer (pidlChild))) {
        /* virtual folder */
        hr = SHELL32_CoCreateInitSF (pidlRoot, pathRoot, pidlChild, clsid, (LPVOID *)&pSF);
    } else if (_ILIsValue(pidlChild)) {
        /* Don't bind to files */
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    } else {
        /* file system folder */
        CLSID clsidFolder = *clsidChild;
        WCHAR wszCLSIDValue[CHARS_IN_GUID], wszFolderPath[MAX_PATH], *pwszPathTail = wszFolderPath;

        /* see if folder CLSID should be overridden by desktop.ini file */
        if (pathRoot) {
            lstrcpynW(wszFolderPath, pathRoot, MAX_PATH);
            pwszPathTail = PathAddBackslashW(wszFolderPath);
        }

        _ILSimpleGetTextW(pidlChild,pwszPathTail,MAX_PATH - (int)(pwszPathTail - wszFolderPath));

        if (SHELL32_GetCustomFolderAttributeFromPath (wszFolderPath,
            L".ShellClassInfo", L"CLSID", wszCLSIDValue, CHARS_IN_GUID))
            CLSIDFromString (wszCLSIDValue, &clsidFolder);

        hr = SHELL32_CoCreateInitSF (pidlRoot, pathRoot, pidlChild,
                                     &clsidFolder, (LPVOID *)&pSF);
    }
    ILFree (pidlChild);

    if (SUCCEEDED (hr)) {
        if (_ILIsPidlSimple (pidlComplete)) {
            /* no sub folders */
            hr = IShellFolder_QueryInterface (pSF, riid, ppvOut);
        } else {
            /* go deeper */
            hr = IShellFolder_BindToObject (pSF, ILGetNext (pidlComplete), NULL, riid, ppvOut);
        }
        IShellFolder_Release (pSF);
    }

    TRACE ("-- returning (%p) 0x%08lx\n", *ppvOut, hr);

    return hr;
}

/***********************************************************************
 *	SHELL32_GetDisplayNameOfChild
 *
 * Retrieves the display name of a child object of a shellfolder.
 *
 * For a pidl eg. [subpidl1][subpidl2][subpidl3]:
 * - it binds to the child shellfolder [subpidl1]
 * - asks it for the displayname of [subpidl2][subpidl3]
 *
 * Is possible the pidl is a simple pidl. In this case it asks the
 * subfolder for the displayname of an empty pidl. The subfolder
 * returns the own displayname eg. "::{guid}". This is used for
 * virtual folders with the registry key WantsFORPARSING set.
 */
HRESULT SHELL32_GetDisplayNameOfChild (IShellFolder2 * psf,
				       LPCITEMIDLIST pidl, DWORD dwFlags, LPWSTR szOut, DWORD dwOutLen)
{
    LPITEMIDLIST pidlFirst;
    HRESULT hr;

    TRACE ("(%p)->(pidl=%p 0x%08lx %p 0x%08lx)\n", psf, pidl, dwFlags, szOut, dwOutLen);
    pdump (pidl);

    pidlFirst = ILCloneFirst (pidl);
    if (pidlFirst) {
	IShellFolder2 *psfChild;

	hr = IShellFolder2_BindToObject (psf, pidlFirst, NULL, &IID_IShellFolder, (LPVOID *) & psfChild);
	if (SUCCEEDED (hr)) {
	    STRRET strTemp;
	    LPITEMIDLIST pidlNext = ILGetNext (pidl);

	    hr = IShellFolder2_GetDisplayNameOf (psfChild, pidlNext, dwFlags, &strTemp);
	    if (SUCCEEDED (hr))
		hr = StrRetToBufW (&strTemp, pidlNext, szOut, dwOutLen);
	    IShellFolder2_Release (psfChild);
	}
	ILFree (pidlFirst);
    } else
	hr = E_OUTOFMEMORY;

    TRACE ("-- ret=0x%08lx %s\n", hr, debugstr_w(szOut));

    return hr;
}

/***********************************************************************
 *  SHELL32_GetItemAttributes
 *
 * NOTES
 * Observed values:
 *  folder:	0xE0000177	FILESYSTEM | HASSUBFOLDER | FOLDER
 *  file:	0x40000177	FILESYSTEM
 *  drive:	0xf0000144	FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  mycomputer:	0xb0000154	HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  (seems to be default for shell extensions if no registry entry exists)
 *
 * win2k:
 *  folder:    0xF0400177      FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR | CANMONIKER
 *  file:      0x40400177      FILESYSTEM | CANMONIKER
 *  drive      0xF0400154      FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR | CANMONIKER | CANRENAME (LABEL)
 *
 * According to the MSDN documentation this function should not set flags. It claims only to reset flags when necessary.
 * However it turns out the native shell32.dll _sets_ flags in several cases - so do we.
 */
HRESULT SHELL32_GetItemAttributes (IShellFolder2 *psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes)
{
    DWORD dwAttributes;
    BOOL has_guid;
    static const DWORD dwSupportedAttr=
                          SFGAO_CANCOPY |           /*0x00000001 */
                          SFGAO_CANMOVE |           /*0x00000002 */
                          SFGAO_CANLINK |           /*0x00000004 */
                          SFGAO_CANRENAME |         /*0x00000010 */
                          SFGAO_CANDELETE |         /*0x00000020 */
                          SFGAO_HASPROPSHEET |      /*0x00000040 */
                          SFGAO_DROPTARGET |        /*0x00000100 */
                          SFGAO_LINK |              /*0x00010000 */
                          SFGAO_READONLY |          /*0x00040000 */
                          SFGAO_HIDDEN |            /*0x00080000 */
                          SFGAO_FILESYSANCESTOR |   /*0x10000000 */
                          SFGAO_FOLDER |            /*0x20000000 */
                          SFGAO_FILESYSTEM |        /*0x40000000 */
                          SFGAO_HASSUBFOLDER;       /*0x80000000 */
    
    TRACE ("0x%08lx\n", *pdwAttributes);

    if (*pdwAttributes & ~dwSupportedAttr)
    {
        WARN ("attributes 0x%08lx not implemented\n", (*pdwAttributes & ~dwSupportedAttr));
        *pdwAttributes &= dwSupportedAttr;
    }

    has_guid = _ILGetGUIDPointer(pidl) != NULL;

    dwAttributes = *pdwAttributes;

    if (_ILIsDrive (pidl)) {
        *pdwAttributes &= SFGAO_HASSUBFOLDER|SFGAO_FILESYSTEM|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|
	    SFGAO_DROPTARGET|SFGAO_HASPROPSHEET|SFGAO_CANLINK;
    } else if (has_guid && HCR_GetFolderAttributes(pidl, &dwAttributes)) {
	*pdwAttributes = dwAttributes;
    } else if (_ILGetDataPointer (pidl)) {
	DWORD file_attr = _ILGetFileAttributes (pidl, NULL, 0);

        if (!file_attr) {
	    WCHAR path[MAX_PATH];
	    STRRET strret;

	    /* File attributes are not present in the internal PIDL structure, so get them from the file system. */

            HRESULT hr = IShellFolder2_GetDisplayNameOf(psf, pidl, SHGDN_FORPARSING, &strret);
	    if (SUCCEEDED(hr)) {
		hr = StrRetToBufW(&strret, pidl, path, MAX_PATH);

		/* call GetFileAttributes() only for file system paths, not for parsing names like "::{...}" */
		if (SUCCEEDED(hr) && path[0]!=':')
		    file_attr = GetFileAttributesW(path);
	    }
	}

        /* Set common attributes */
        *pdwAttributes |= SFGAO_FILESYSTEM | SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANDELETE |
                          SFGAO_CANRENAME | SFGAO_CANLINK | SFGAO_CANMOVE | SFGAO_CANCOPY;

	if (file_attr & FILE_ATTRIBUTE_DIRECTORY)
	    *pdwAttributes |=  (SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR | SFGAO_STORAGE);
	else
        {
	    *pdwAttributes &= ~(SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR | SFGAO_STORAGE);
	    *pdwAttributes |= SFGAO_STREAM;
        }

	if (file_attr & FILE_ATTRIBUTE_HIDDEN)
	    *pdwAttributes |=  SFGAO_HIDDEN;
	else
	    *pdwAttributes &= ~SFGAO_HIDDEN;

	if (file_attr & FILE_ATTRIBUTE_READONLY)
	    *pdwAttributes |=  SFGAO_READONLY;
	else
	    *pdwAttributes &= ~SFGAO_READONLY;

	if (SFGAO_LINK & *pdwAttributes) {
	    char ext[MAX_PATH];

	    if (!_ILGetExtension(pidl, ext, MAX_PATH) || lstrcmpiA(ext, "lnk"))
		*pdwAttributes &= ~SFGAO_LINK;
	}
    } else {
	*pdwAttributes &= SFGAO_HASSUBFOLDER|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|SFGAO_DROPTARGET|SFGAO_HASPROPSHEET|SFGAO_CANRENAME|SFGAO_CANLINK;
    }
    TRACE ("-- 0x%08lx\n", *pdwAttributes);
    return S_OK;
}

/***********************************************************************
 *  SHELL32_CompareIDs
 */
HRESULT SHELL32_CompareIDs(IShellFolder2 *sf, LPARAM lParam, LPCITEMIDLIST pidl1,
        LPCITEMIDLIST pidl2)
{
    int type1, type2;
    char szTemp1[MAX_PATH];
    char szTemp2[MAX_PATH];
    HRESULT nReturn;
    LPITEMIDLIST firstpidl, nextpidl1, nextpidl2;
    IShellFolder *psf;

    /* test for empty pidls */
    BOOL isEmpty1 = _ILIsDesktop (pidl1);
    BOOL isEmpty2 = _ILIsDesktop (pidl2);

    if (isEmpty1 && isEmpty2)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, 0 );
    if (isEmpty1)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, (WORD)-1 );
    if (isEmpty2)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 );

    /* test for different types. Sort order is the PT_* constant */
    type1 = _ILGetDataPointer (pidl1)->type;
    type2 = _ILGetDataPointer (pidl2)->type;
    if (type1 < type2)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, (WORD)-1 );
    else if (type1 > type2)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 );

    /* test for name of pidl */
    _ILSimpleGetText (pidl1, szTemp1, MAX_PATH);
    _ILSimpleGetText (pidl2, szTemp2, MAX_PATH);
    nReturn = lstrcmpiA (szTemp1, szTemp2);
    if (nReturn < 0)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, (WORD)-1 );
    else if (nReturn > 0)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 );

    /* test of complex pidls */
    firstpidl = ILCloneFirst (pidl1);
    nextpidl1 = ILGetNext (pidl1);
    nextpidl2 = ILGetNext (pidl2);

    /* optimizing: test special cases and bind not deeper */
    /* the deeper shellfolder would do the same */
    isEmpty1 = _ILIsDesktop (nextpidl1);
    isEmpty2 = _ILIsDesktop (nextpidl2);

    if (isEmpty1 && isEmpty2) {
        nReturn = MAKE_HRESULT( SEVERITY_SUCCESS, 0, 0 );
    } else if (isEmpty1) {
        nReturn = MAKE_HRESULT( SEVERITY_SUCCESS, 0, (WORD)-1 );
    } else if (isEmpty2) {
        nReturn = MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 );
    /* optimizing end */
    } else if (SUCCEEDED(IShellFolder2_BindToObject(sf, firstpidl, NULL, &IID_IShellFolder, (void **)&psf))) {
	nReturn = IShellFolder_CompareIDs (psf, lParam, nextpidl1, nextpidl2);
	IShellFolder_Release (psf);
    }
    ILFree (firstpidl);
    return nReturn;
}

HRESULT SHELL32_GetColumnDetails(const shvheader *data, int column, SHELLDETAILS *details)
{
    details->fmt = data[column].fmt;
    details->cxChar = data[column].cxChar;

    if (SHELL_OsIsUnicode())
    {
        details->str.pOleStr = CoTaskMemAlloc(MAX_PATH * sizeof(WCHAR));
        if (!details->str.pOleStr) return E_OUTOFMEMORY;

        details->str.uType = STRRET_WSTR;
        LoadStringW(shell32_hInstance, data[column].colnameid, details->str.pOleStr, MAX_PATH);
    }
    else
    {
        details->str.uType = STRRET_CSTR;
        LoadStringA(shell32_hInstance, data[column].colnameid, details->str.cStr, MAX_PATH);
    }

    return S_OK;
}

HRESULT shellfolder_map_column_to_scid(const shvheader *header, UINT column, SHCOLUMNID *scid)
{
    if (header[column].fmtid == NULL)
    {
        FIXME("missing property id for column %u.\n", column);
        memset(scid, 0, sizeof(*scid));
        return E_NOTIMPL;
    }

    scid->fmtid = *header[column].fmtid;
    scid->pid = header[column].pid;

    return S_OK;
}

HRESULT shellfolder_get_file_details(IShellFolder2 *iface, LPCITEMIDLIST pidl, const shvheader *header,
                                     int column, SHELLDETAILS *psd)
{
    psd->str.uType = STRRET_CSTR;
    switch (header[column].pid)
    {
    case PID_STG_NAME:
        return IShellFolder2_GetDisplayNameOf( iface, pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str );
    case PID_STG_SIZE:
        _ILGetFileSize( pidl, psd->str.cStr, MAX_PATH );
        break;
    case PID_STG_STORAGETYPE:
        _ILGetFileType( pidl, psd->str.cStr, MAX_PATH );
        break;
    case PID_STG_WRITETIME:
        _ILGetFileDate( pidl, psd->str.cStr, MAX_PATH );
        break;
    case PID_STG_ATTRIBUTES:
        _ILGetFileAttributes( pidl, psd->str.cStr, MAX_PATH );
        break;
    }
    return S_OK;
}

/***********************************************************************
 *  SHCreateLinks
 *
 *   Undocumented.
 */
HRESULT WINAPI SHCreateLinks( HWND hWnd, LPCSTR lpszDir, LPDATAOBJECT lpDataObject,
                              UINT uFlags, LPITEMIDLIST *lppidlLinks)
{
    FIXME("%p %s %p %08x %p\n",hWnd,lpszDir,lpDataObject,uFlags,lppidlLinks);
    return E_NOTIMPL;
}

/***********************************************************************
 *  SHOpenFolderAndSelectItems
 *
 *   Added in XP.
 */
HRESULT WINAPI SHOpenFolderAndSelectItems(PCIDLIST_ABSOLUTE pidlFolder, UINT cidl,
                                          PCUITEMID_CHILD_ARRAY apidl, DWORD flags)
{
    static const unsigned int magic = 0xe32ee32e;
    unsigned int i, uint_flags, size, child_count = 0;
    const ITEMIDLIST *pidl_parent, *pidl_child;
    VARIANT var_parent, var_empty;
    ITEMIDLIST *pidl_tmp = NULL;
    SHELLEXECUTEINFOW sei = {0};
    COPYDATASTRUCT cds = {0};
    IDispatch *dispatch;
    int timeout = 1000;
    unsigned char *ptr;
    IShellWindows *sw;
    BOOL ret = FALSE;
    HRESULT hr;
    LONG hwnd;

    TRACE("%p %u %p 0x%lx\n", pidlFolder, cidl, apidl, flags);

    if (!pidlFolder)
        return E_INVALIDARG;

    if (flags & OFASI_OPENDESKTOP)
        FIXME("Ignoring unsupported OFASI_OPENDESKTOP flag.\n");

    if (flags & OFASI_EDIT && cidl > 1)
        flags &= ~OFASI_EDIT;

    hr = CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER, &IID_IShellWindows,
                          (void **)&sw);
    if (FAILED(hr))
        return hr;

    if (!cidl)
    {
        pidl_tmp = ILClone(pidlFolder);
        ILRemoveLastID(pidl_tmp);
        pidl_parent = pidl_tmp;

        pidl_child = ILFindLastID(pidlFolder);
        apidl = &pidl_child;
        cidl = 1;
    }
    else
    {
        pidl_parent = pidlFolder;
    }

    /* Find the existing explorer window for the parent path. Create a new one if not present. */
    VariantInit(&var_empty);
    VariantInit(&var_parent);
    size = ILGetSize(pidl_parent);
    V_VT(&var_parent) = VT_ARRAY | VT_UI1;
    V_ARRAY(&var_parent) = SafeArrayCreateVector(VT_UI1, 0, size);
    memcpy(V_ARRAY(&var_parent)->pvData, pidl_parent, size);
    hr = IShellWindows_FindWindowSW(sw, &var_parent, &var_empty, SWC_EXPLORER, &hwnd, 0, &dispatch);
    if (hr != S_OK)
    {
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_IDLIST | SEE_MASK_NOASYNC | SEE_MASK_WAITFORINPUTIDLE;
        sei.lpVerb = L"explore";
        sei.lpIDList = (void *)pidl_parent;
        sei.nShow = SW_NORMAL;
        if (!ShellExecuteExW(&sei))
        {
            WARN("Failed to create a explorer window.\n");
            goto done;
        }

        while (timeout > 0)
        {
            hr = IShellWindows_FindWindowSW(sw, &var_parent, &var_empty, SWC_EXPLORER, &hwnd, 0,
                                            &dispatch);
            if (hr == S_OK)
                break;

            timeout -= 100;
            Sleep(100);
        }

        if (hr != S_OK)
        {
            WARN("Failed to find the explorer window.\n");
            goto done;
        }
    }

    /* Send WM_COPYDATA to tell explorer.exe to open windows */
    size = sizeof(cidl) + sizeof(uint_flags);
    for (i = 0; i < cidl; ++i)
        size += ILGetSize(apidl[i]);

    cds.dwData = magic;
    cds.cbData = size;
    cds.lpData = malloc(size);
    if (!cds.lpData)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    /* Add the count of child ITEMIDLIST, set its value at the end */
    ptr = (unsigned char *)cds.lpData + sizeof(cidl);

    /* Add flags. Have to use unsigned int because DWORD may have a different size */
    uint_flags = flags;
    memcpy(ptr, &uint_flags, sizeof(uint_flags));
    ptr += sizeof(uint_flags);

    /* Add child ITEMIDLIST */
    for (i = 0; i < cidl; ++i)
    {
        if (apidl != &pidl_child)
            pidl_child = ILFindChild(pidl_parent, apidl[i]);

        if (pidl_child)
        {
            size = ILGetSize(pidl_child);
            memcpy(ptr, pidl_child, size);
            ptr += size;
            ++child_count;
        }
    }

    /* Set the count of child ITEMIDLIST */
    memcpy(cds.lpData, &child_count, sizeof(child_count));

    SetForegroundWindow(GetAncestor((HWND)(LONG_PTR)hwnd, GA_ROOT));
    ret = SendMessageW((HWND)(LONG_PTR)hwnd, WM_COPYDATA, 0, (LPARAM)&cds);
    hr = ret ? S_OK : E_FAIL;

done:
    free(cds.lpData);
    VariantClear(&var_parent);
    if (pidl_tmp)
        ILFree(pidl_tmp);
    IShellWindows_Release(sw);
    return hr;
}

/***********************************************************************
 *  SHGetSetFolderCustomSettings (SHELL32.709)
 *
 *   Only Unicode above Server 2003, writes/reads from a Desktop.ini
 */
HRESULT WINAPI SHGetSetFolderCustomSettings( LPSHFOLDERCUSTOMSETTINGS fcs, PCWSTR path, DWORD flag )
{
    WCHAR bufferW[MAX_PATH];
    HRESULT hr;

    hr = E_FAIL;
    bufferW[0] = 0;

    if (!fcs || !path)
        return hr;

    if (flag & FCS_FORCEWRITE)
    {
        if (fcs->dwMask & FCSM_ICONFILE)
        {
            lstrcpyW(bufferW, path);
            PathAddBackslashW(bufferW);
            lstrcatW(bufferW, L"desktop.ini");

            if (WritePrivateProfileStringW(L".ShellClassInfo", L"IconResource", fcs->pszIconFile, bufferW))
            {
                TRACE("Wrote an iconresource entry %s into %s\n", debugstr_w(fcs->pszIconFile), debugstr_w(bufferW));
                hr = S_OK;
            }
            else
                ERR("Failed to write (to) Desktop.ini file\n");
        }
    }
    else
        FIXME("%p %s 0x%lx: stub\n", fcs, debugstr_w(path), flag);

    return hr;
}

/***********************************************************************
 * SHLimitInputEdit (SHELL32.747)
 */
HRESULT WINAPI SHLimitInputEdit( HWND textbox, IShellFolder *folder )
{
    FIXME("%p %p: stub\n", textbox, folder);
    return E_NOTIMPL;
}
