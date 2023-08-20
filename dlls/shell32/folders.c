/*
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "shlguid.h"

#include "wine/debug.h"

#include "pidl.h"
#include "shell32_main.h"
#include "shfldr.h"
#include "shresdef.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/***********************************************************************
*   IExtractIconW implementation
*/
typedef struct
{
        IExtractIconW      IExtractIconW_iface;
        IExtractIconA      IExtractIconA_iface;
        IPersistFile       IPersistFile_iface;
	LONG               ref;
	LPITEMIDLIST       pidl;
} IExtractIconWImpl;

static inline IExtractIconWImpl *impl_from_IExtractIconW(IExtractIconW *iface)
{
    return CONTAINING_RECORD(iface, IExtractIconWImpl, IExtractIconW_iface);
}

static inline IExtractIconWImpl *impl_from_IExtractIconA(IExtractIconA *iface)
{
    return CONTAINING_RECORD(iface, IExtractIconWImpl, IExtractIconA_iface);
}

static inline IExtractIconWImpl *impl_from_IPersistFile(IPersistFile *iface)
{
    return CONTAINING_RECORD(iface, IExtractIconWImpl, IPersistFile_iface);
}


/**************************************************************************
 *  IExtractIconW::QueryInterface
 */
static HRESULT WINAPI IExtractIconW_fnQueryInterface(IExtractIconW *iface, REFIID riid,
        void **ppv)
{
    IExtractIconWImpl *This = impl_from_IExtractIconW(iface);

    TRACE("(%p)->(\n\tIID:\t%s,%p)\n", This, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IExtractIconW))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IPersistFile))
        *ppv = &This->IPersistFile_iface;
    else if (IsEqualIID(riid, &IID_IExtractIconA))
        *ppv = &This->IExtractIconA_iface;

    if(*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        TRACE("-- Interface: (%p)->(%p)\n", ppv, *ppv);
        return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/**************************************************************************
*  IExtractIconW::AddRef
*/
static ULONG WINAPI IExtractIconW_fnAddRef(IExtractIconW * iface)
{
        IExtractIconWImpl *This = impl_from_IExtractIconW(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%lu)\n", This, refCount - 1);

	return refCount;
}
/**************************************************************************
*  IExtractIconW::Release
*/
static ULONG WINAPI IExtractIconW_fnRelease(IExtractIconW * iface)
{
        IExtractIconWImpl *This = impl_from_IExtractIconW(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%lu)\n", This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying IExtractIcon(%p)\n",This);
	  SHFree(This->pidl);
	  free(This);
	}
	return refCount;
}

static HRESULT getIconLocationForFolder(IExtractIconWImpl *This, UINT uFlags, LPWSTR szIconFile,
        UINT cchMax, int *piIndex, UINT *pwFlags)
{
    int icon_idx;
    WCHAR wszPath[MAX_PATH];
    WCHAR wszCLSIDValue[CHARS_IN_GUID];

    if (SHELL32_GetCustomFolderAttribute(This->pidl, L".ShellClassInfo", L"IconFile",
        wszPath, MAX_PATH))
    {
        WCHAR wszIconIndex[10];
        SHELL32_GetCustomFolderAttribute(This->pidl, L".ShellClassInfo", L"IconIndex",
            wszIconIndex, 10);
        *piIndex = wcstol(wszIconIndex, NULL, 10);
    }
    else if (SHELL32_GetCustomFolderAttribute(This->pidl, L".ShellClassInfo", L"CLSID",
        wszCLSIDValue, CHARS_IN_GUID) &&
        HCR_GetDefaultIconW(wszCLSIDValue, szIconFile, cchMax, &icon_idx))
    {
       *piIndex = icon_idx;
    }
    else if (SHELL32_GetCustomFolderAttribute(This->pidl, L".ShellClassInfo", L"CLSID2",
        wszCLSIDValue, CHARS_IN_GUID) &&
        HCR_GetDefaultIconW(wszCLSIDValue, szIconFile, cchMax, &icon_idx))
    {
       *piIndex = icon_idx;
    }
    else
    {
        if (!HCR_GetDefaultIconW(L"Folder", szIconFile, cchMax, &icon_idx))
        {
            lstrcpynW(szIconFile, swShell32Name, cchMax);
            icon_idx = -IDI_SHELL_FOLDER;
        }

        if (uFlags & GIL_OPENICON)
            *piIndex = icon_idx<0? icon_idx-1: icon_idx+1;
        else
            *piIndex = icon_idx;
    }

    return S_OK;
}

WCHAR swShell32Name[MAX_PATH];

/**************************************************************************
*  IExtractIconW::GetIconLocation
*
* mapping filetype to icon
*/
static HRESULT WINAPI IExtractIconW_fnGetIconLocation(IExtractIconW * iface, UINT uFlags,
        LPWSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags)
{
        IExtractIconWImpl *This = impl_from_IExtractIconW(iface);
	char	sTemp[MAX_PATH];
	int		icon_idx;
	GUID const * riid;
	LPITEMIDLIST	pSimplePidl = ILFindLastID(This->pidl);

	TRACE("(%p) (flags=%u, %p, %u, %p, %p)\n", This, uFlags, szIconFile,
                cchMax, piIndex, pwFlags);

	if (pwFlags)
	  *pwFlags = 0;

	if (_ILIsDesktop(pSimplePidl))
	{
	  lstrcpynW(szIconFile, swShell32Name, cchMax);
	  *piIndex = -IDI_SHELL_DESKTOP;
	}

	/* my computer and other shell extensions */
	else if ((riid = _ILGetGUIDPointer(pSimplePidl)))
	{
	  WCHAR xriid[50];

	  swprintf(xriid, ARRAY_SIZE(xriid), L"CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
	          riid->Data1, riid->Data2, riid->Data3,
	          riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
	          riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7]);

	  if (HCR_GetDefaultIconW(xriid, szIconFile, cchMax, &icon_idx))
	  {
	    *piIndex = icon_idx;
	  }
	  else
	  {
	    lstrcpynW(szIconFile, swShell32Name, cchMax);
            if(IsEqualGUID(riid, &CLSID_MyComputer))
                *piIndex = -IDI_SHELL_MY_COMPUTER;
            else if(IsEqualGUID(riid, &CLSID_MyDocuments))
                *piIndex = -IDI_SHELL_MY_DOCUMENTS;
            else if(IsEqualGUID(riid, &CLSID_NetworkPlaces))
                *piIndex = -IDI_SHELL_MY_NETWORK_PLACES;
            else if(IsEqualGUID(riid, &CLSID_UnixFolder) ||
                    IsEqualGUID(riid, &CLSID_UnixDosFolder))
                *piIndex = -IDI_SHELL_DRIVE;
            else
                *piIndex = -IDI_SHELL_FOLDER;
	  }
	}

	else if (_ILIsDrive (pSimplePidl))
	{
	  icon_idx = -1;

	  if (_ILGetDrive(pSimplePidl, sTemp, MAX_PATH))
	  {
		switch(GetDriveTypeA(sTemp))
		{
                  case DRIVE_REMOVABLE:   icon_idx = IDI_SHELL_FLOPPY;        break;
                  case DRIVE_CDROM:       icon_idx = IDI_SHELL_OPTICAL_DRIVE; break;
                  case DRIVE_REMOTE:      icon_idx = IDI_SHELL_NETDRIVE;      break;
                  case DRIVE_RAMDISK:     icon_idx = IDI_SHELL_RAMDISK;       break;
		}
	  }

	  if (icon_idx != -1)
	  {
		lstrcpynW(szIconFile, swShell32Name, cchMax);
		*piIndex = -icon_idx;
	  }
	  else
	  {
		if (HCR_GetDefaultIconW(L"Drive", szIconFile, cchMax, &icon_idx))
		{
		  *piIndex = icon_idx;
		}
		else
		{
		  lstrcpynW(szIconFile, swShell32Name, cchMax);
		  *piIndex = -IDI_SHELL_DRIVE;
		}
	  }
	}
	else if (_ILIsFolder (pSimplePidl))
	{
            getIconLocationForFolder(This, uFlags, szIconFile, cchMax, piIndex, pwFlags);
	}
	else
	{
	  BOOL found = FALSE;

	  if (_ILIsCPanelStruct(pSimplePidl))
	  {
	    if (SUCCEEDED(CPanel_GetIconLocationW(pSimplePidl, szIconFile, cchMax, piIndex)))
		found = TRUE;
	  }
	  else if (_ILGetExtension(pSimplePidl, sTemp, MAX_PATH))
	  {
	    if (HCR_MapTypeToValueA(sTemp, sTemp, MAX_PATH, TRUE)
		&& HCR_GetDefaultIconA(sTemp, sTemp, MAX_PATH, &icon_idx))
	    {
	      if (!lstrcmpA("%1", sTemp))		/* icon is in the file */
	      {
		SHGetPathFromIDListW(This->pidl, szIconFile);
		*piIndex = 0;
	      }
	      else
	      {
		MultiByteToWideChar(CP_ACP, 0, sTemp, -1, szIconFile, cchMax);
		*piIndex = icon_idx;
	      }

	      found = TRUE;
	    }
	    else if (!lstrcmpiA(sTemp, "lnkfile"))
	    {
	      /* extract icon from shell shortcut */
	      IShellFolder* dsf;
	      IShellLinkW* psl;

	      if (SUCCEEDED(SHGetDesktopFolder(&dsf)))
	      {
		HRESULT hr = IShellFolder_GetUIObjectOf(dsf, NULL, 1, (LPCITEMIDLIST*)&This->pidl, &IID_IShellLinkW, NULL, (LPVOID*)&psl);

		if (SUCCEEDED(hr))
		{
		  hr = IShellLinkW_GetIconLocation(psl, szIconFile, cchMax, piIndex);

		  if (SUCCEEDED(hr) && *szIconFile)
		    found = TRUE;

		  IShellLinkW_Release(psl);
		}

		IShellFolder_Release(dsf);
	      }
	    }
	  }

	  if (!found)					/* default icon */
	  {
	    lstrcpynW(szIconFile, swShell32Name, cchMax);
	    *piIndex = 0;
	  }
	}

	TRACE("-- %s %x\n", debugstr_w(szIconFile), *piIndex);
	return S_OK;
}

/**************************************************************************
*  IExtractIconW::Extract
*/
static HRESULT WINAPI IExtractIconW_fnExtract(IExtractIconW * iface, LPCWSTR pszFile,
        UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
        IExtractIconWImpl *This = impl_from_IExtractIconW(iface);
        int index;
        HIMAGELIST big_icons, small_icons;

        FIXME("(%p) (file=%s index=%d %p %p size=%08x) semi-stub\n", This, debugstr_w(pszFile),
                (signed)nIconIndex, phiconLarge, phiconSmall, nIconSize);

        index = SIC_GetIconIndex(pszFile, nIconIndex, 0);
        Shell_GetImageLists( &big_icons, &small_icons );
	if (phiconLarge)
	  *phiconLarge = ImageList_GetIcon(big_icons, index, ILD_TRANSPARENT);

	if (phiconSmall)
	  *phiconSmall = ImageList_GetIcon(small_icons, index, ILD_TRANSPARENT);

	return S_OK;
}

static const IExtractIconWVtbl eivt =
{
	IExtractIconW_fnQueryInterface,
	IExtractIconW_fnAddRef,
	IExtractIconW_fnRelease,
	IExtractIconW_fnGetIconLocation,
	IExtractIconW_fnExtract
};

/**************************************************************************
 *  IExtractIconA::QueryInterface
 */
static HRESULT WINAPI IExtractIconA_fnQueryInterface(IExtractIconA * iface, REFIID riid,
        void **ppv)
{
    IExtractIconWImpl *This = impl_from_IExtractIconA(iface);

    return IExtractIconW_QueryInterface(&This->IExtractIconW_iface, riid, ppv);
}

/**************************************************************************
*  IExtractIconA::AddRef
*/
static ULONG WINAPI IExtractIconA_fnAddRef(IExtractIconA * iface)
{
    IExtractIconWImpl *This = impl_from_IExtractIconA(iface);

    return IExtractIconW_AddRef(&This->IExtractIconW_iface);
}
/**************************************************************************
*  IExtractIconA::Release
*/
static ULONG WINAPI IExtractIconA_fnRelease(IExtractIconA * iface)
{
    IExtractIconWImpl *This = impl_from_IExtractIconA(iface);

    return IExtractIconW_Release(&This->IExtractIconW_iface);
}
/**************************************************************************
*  IExtractIconA::GetIconLocation
*
* mapping filetype to icon
*/
static HRESULT WINAPI IExtractIconA_fnGetIconLocation(IExtractIconA * iface, UINT uFlags,
        LPSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags)
{
        IExtractIconWImpl *This = impl_from_IExtractIconA(iface);
	HRESULT ret;
	WCHAR *lpwstrFile = malloc(cchMax * sizeof(WCHAR));

	TRACE("(%p) (flags=%u %p %u %p %p)\n", This, uFlags, szIconFile, cchMax, piIndex, pwFlags);

        ret = IExtractIconW_GetIconLocation(&This->IExtractIconW_iface, uFlags, lpwstrFile, cchMax,
                piIndex, pwFlags);
	WideCharToMultiByte(CP_ACP, 0, lpwstrFile, -1, szIconFile, cchMax, NULL, NULL);
	free(lpwstrFile);

	TRACE("-- %s %x\n", szIconFile, *piIndex);
	return ret;
}
/**************************************************************************
*  IExtractIconA::Extract
*/
static HRESULT WINAPI IExtractIconA_fnExtract(IExtractIconA * iface, LPCSTR pszFile,
        UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
        IExtractIconWImpl *This = impl_from_IExtractIconA(iface);
	HRESULT ret;
	INT len = MultiByteToWideChar(CP_ACP, 0, pszFile, -1, NULL, 0);
	WCHAR *lpwstrFile = malloc(len * sizeof(WCHAR));

	TRACE("(%p) (file=%p index=%u %p %p size=%u)\n", This, pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);

	MultiByteToWideChar(CP_ACP, 0, pszFile, -1, lpwstrFile, len);
        ret = IExtractIconW_Extract(&This->IExtractIconW_iface, lpwstrFile, nIconIndex, phiconLarge,
                phiconSmall, nIconSize);
	free(lpwstrFile);
	return ret;
}

static const IExtractIconAVtbl eiavt =
{
	IExtractIconA_fnQueryInterface,
	IExtractIconA_fnAddRef,
	IExtractIconA_fnRelease,
	IExtractIconA_fnGetIconLocation,
	IExtractIconA_fnExtract
};

/************************************************************************
 * IEIPersistFile::QueryInterface
 */
static HRESULT WINAPI IEIPersistFile_fnQueryInterface(IPersistFile *iface, REFIID iid,
        void **ppv)
{
    IExtractIconWImpl *This = impl_from_IPersistFile(iface);

    return IExtractIconW_QueryInterface(&This->IExtractIconW_iface, iid, ppv);
}

/************************************************************************
 * IEIPersistFile::AddRef
 */
static ULONG WINAPI IEIPersistFile_fnAddRef(IPersistFile *iface)
{
    IExtractIconWImpl *This = impl_from_IPersistFile(iface);

    return IExtractIconW_AddRef(&This->IExtractIconW_iface);
}

/************************************************************************
 * IEIPersistFile::Release
 */
static ULONG WINAPI IEIPersistFile_fnRelease(IPersistFile *iface)
{
    IExtractIconWImpl *This = impl_from_IPersistFile(iface);

    return IExtractIconW_Release(&This->IExtractIconW_iface);
}

/************************************************************************
 * IEIPersistFile::GetClassID
 */
static HRESULT WINAPI IEIPersistFile_fnGetClassID(IPersistFile *iface, LPCLSID lpClassId)
{
	CLSID StdFolderID = { 0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

	if (lpClassId==NULL)
	  return E_POINTER;

	*lpClassId = StdFolderID;

	return S_OK;
}

/************************************************************************
 * IEIPersistFile_Load
 */
static HRESULT WINAPI IEIPersistFile_fnLoad(IPersistFile* iface, LPCOLESTR pszFileName,
        DWORD dwMode)
{
    IExtractIconWImpl *This = impl_from_IPersistFile(iface);

    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static const IPersistFileVtbl pfvt =
{
	IEIPersistFile_fnQueryInterface,
	IEIPersistFile_fnAddRef,
	IEIPersistFile_fnRelease,
	IEIPersistFile_fnGetClassID,
	(void *) 0xdeadbeef /* IEIPersistFile_fnIsDirty */,
	IEIPersistFile_fnLoad,
	(void *) 0xdeadbeef /* IEIPersistFile_fnSave */,
	(void *) 0xdeadbeef /* IEIPersistFile_fnSaveCompleted */,
	(void *) 0xdeadbeef /* IEIPersistFile_fnGetCurFile */
};

static IExtractIconWImpl *extracticon_create(LPCITEMIDLIST pidl)
{
    IExtractIconWImpl *ei;

    TRACE("%p\n", pidl);

    ei = malloc(sizeof(*ei));
    ei->ref=1;
    ei->IExtractIconW_iface.lpVtbl = &eivt;
    ei->IExtractIconA_iface.lpVtbl = &eiavt;
    ei->IPersistFile_iface.lpVtbl = &pfvt;
    ei->pidl=ILClone(pidl);

    pdump(pidl);

    TRACE("(%p)\n", ei);
    return ei;
}

IExtractIconW *IExtractIconW_Constructor(LPCITEMIDLIST pidl)
{
    IExtractIconWImpl *ei = extracticon_create(pidl);

    return &ei->IExtractIconW_iface;
}

IExtractIconA *IExtractIconA_Constructor(LPCITEMIDLIST pidl)
{
    IExtractIconWImpl *ei = extracticon_create(pidl);

    return &ei->IExtractIconA_iface;
}
