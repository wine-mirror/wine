/*
 *	handling of SHELL32.DLL OLE-Objects
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied  <juergen.schmied@metronet.de>
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "shellapi.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "shlguid.h"
#include "shldisp.h"
#include "gdiplus.h"
#include "shimgdata.h"
#include "winreg.h"
#include "winerror.h"

#include "shell32_main.h"

#include "wine/debug.h"
#include "shlwapi.h"
#include "debughlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern INT WINAPI SHStringFromGUIDW(REFGUID guid, LPWSTR lpszDest, INT cchMax);  /* shlwapi.24 */
static HRESULT WINAPI ShellImageDataFactory_Constructor(IUnknown *outer, REFIID riid, void **obj);

/**************************************************************************
 * Default ClassFactory types
 */
typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);
static IClassFactory * IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst);

/* this table contains all CLSIDs of shell32 objects */
static const struct {
	REFIID			clsid;
	LPFNCREATEINSTANCE	lpfnCI;
} InterfaceTable[] = {

	{&CLSID_ApplicationAssociationRegistration, ApplicationAssociationRegistration_Constructor},
	{&CLSID_ApplicationDestinations, ApplicationDestinations_Constructor},
	{&CLSID_ApplicationDocumentLists, ApplicationDocumentLists_Constructor},
	{&CLSID_AutoComplete,   IAutoComplete_Constructor},
	{&CLSID_ControlPanel,	IControlPanel_Constructor},
	{&CLSID_DragDropHelper, IDropTargetHelper_Constructor},
	{&CLSID_FolderShortcut, FolderShortcut_Constructor},
	{&CLSID_MyComputer,	ISF_MyComputer_Constructor},
	{&CLSID_MyDocuments,    MyDocuments_Constructor},
	{&CLSID_NetworkPlaces,  ISF_NetworkPlaces_Constructor},
	{&CLSID_NewMenu,        new_menu_create},
	{&CLSID_Printers,       Printers_Constructor},
	{&CLSID_QueryAssociations, QueryAssociations_Constructor},
	{&CLSID_RecycleBin,     RecycleBin_Constructor},
	{&CLSID_ShellDesktop,	ISF_Desktop_Constructor},
	{&CLSID_ShellFSFolder,	IFSFolder_Constructor},
	{&CLSID_ShellItem,	IShellItem_Constructor},
	{&CLSID_ShellLink,	IShellLink_Constructor},
	{&CLSID_UnixDosFolder,  UnixDosFolder_Constructor},
	{&CLSID_UnixFolder,     UnixFolder_Constructor},
	{&CLSID_ExplorerBrowser,ExplorerBrowser_Constructor},
	{&CLSID_KnownFolderManager, KnownFolderManager_Constructor},
	{&CLSID_Shell,          IShellDispatch_Constructor},
	{&CLSID_DestinationList, CustomDestinationList_Constructor},
	{&CLSID_ShellImageDataFactory, ShellImageDataFactory_Constructor},
	{&CLSID_FileOperation, IFileOperation_Constructor},
	{&CLSID_ActiveDesktop, ActiveDesktop_Constructor},
	{&CLSID_EnumerableObjectCollection, EnumerableObjectCollection_Constructor},
	{NULL, NULL}
};

/*************************************************************************
 * SHCoCreateInstance [SHELL32.102]
 *
 * Equivalent to CoCreateInstance. Under Windows 9x this function could sometimes
 * use the shell32 built-in "mini-COM" without the need to load ole32.dll - see
 * SHLoadOLE for details.
 *
 * Under wine if a "LoadWithoutCOM" value is present or the object resides in
 * shell32.dll the function will load the object manually without the help of ole32
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     CoCreateInstance, SHLoadOLE
 */
HRESULT WINAPI SHCoCreateInstance(
	LPCWSTR aclsid,
	const CLSID *clsid,
	LPUNKNOWN pUnkOuter,
	REFIID refiid,
	LPVOID *ppv)
{
	DWORD	hres;
	IID	iid;
	const	CLSID * myclsid = clsid;
	WCHAR	sKeyName[MAX_PATH];
	WCHAR	sClassID[60];
	WCHAR	sDllPath[MAX_PATH];
	HKEY	hKey = 0;
	DWORD	dwSize;
	IClassFactory * pcf = NULL;

	if(!ppv) return E_POINTER;
	*ppv=NULL;

	/* if the clsid is a string, convert it */
	if (!clsid)
	{
	  if (!aclsid) return REGDB_E_CLASSNOTREG;
	  SHCLSIDFromStringW(aclsid, &iid);
	  myclsid = &iid;
	}

	TRACE("(%p,%s,unk:%p,%s,%p)\n",
		aclsid,shdebugstr_guid(myclsid),pUnkOuter,shdebugstr_guid(refiid),ppv);

        if (SUCCEEDED(DllGetClassObject(myclsid, &IID_IClassFactory,(LPVOID*)&pcf)))
        {
            hres = IClassFactory_CreateInstance(pcf, pUnkOuter, refiid, ppv);
            IClassFactory_Release(pcf);
            goto end;
        }

	/* we look up the dll path in the registry */
        SHStringFromGUIDW(myclsid, sClassID, ARRAY_SIZE(sClassID));
        swprintf( sKeyName, ARRAY_SIZE(sKeyName), L"CLSID\\%s\\InprocServer32", sClassID );

	if (RegOpenKeyExW(HKEY_CLASSES_ROOT, sKeyName, 0, KEY_READ, &hKey))
            return E_ACCESSDENIED;

        /* if a special registry key is set, we load a shell extension without help of OLE32 */
        if (!SHQueryValueExW(hKey, L"LoadWithoutCOM", 0, 0, 0, 0))
        {
	    /* load an external dll without ole32 */
	    HANDLE hLibrary;
	    typedef HRESULT (CALLBACK *DllGetClassObjectFunc)(REFCLSID clsid, REFIID iid, LPVOID *ppv);
	    DllGetClassObjectFunc DllGetClassObject;

            dwSize = sizeof(sDllPath);
            SHQueryValueExW(hKey, NULL, 0,0, sDllPath, &dwSize );

	    if ((hLibrary = LoadLibraryExW(sDllPath, 0, LOAD_WITH_ALTERED_SEARCH_PATH)) == 0) {
	        ERR("couldn't load InprocServer32 dll %s\n", debugstr_w(sDllPath));
		hres = E_ACCESSDENIED;
	        goto end;
	    } else if (!(DllGetClassObject = (DllGetClassObjectFunc)GetProcAddress(hLibrary, "DllGetClassObject"))) {
	        ERR("couldn't find function DllGetClassObject in %s\n", debugstr_w(sDllPath));
	        FreeLibrary( hLibrary );
		hres = E_ACCESSDENIED;
	        goto end;
            } else if (FAILED(hres = DllGetClassObject(myclsid, &IID_IClassFactory, (LPVOID*)&pcf))) {
		    TRACE("GetClassObject failed 0x%08lx\n", hres);
		    goto end;
	    }

            hres = IClassFactory_CreateInstance(pcf, pUnkOuter, refiid, ppv);
            IClassFactory_Release(pcf);
	} else {

	    /* load an external dll in the usual way */
	    hres = CoCreateInstance(myclsid, pUnkOuter, CLSCTX_INPROC_SERVER, refiid, ppv);
	}

end:
        if (hKey) RegCloseKey(hKey);
	if(hres!=S_OK)
	{
	  ERR("failed (0x%08lx) to create CLSID:%s IID:%s\n",
              hres, shdebugstr_guid(myclsid), shdebugstr_guid(refiid));
	  ERR("class not found in registry\n");
	}

	TRACE("-- instance: %p\n",*ppv);
	return hres;
}

/*************************************************************************
 * DllGetClassObject     [SHELL32.@]
 * SHDllGetClassObject   [SHELL32.128]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
	IClassFactory * pcf = NULL;
	HRESULT	hres;
	int i;

	TRACE("CLSID:%s,IID:%s\n",shdebugstr_guid(rclsid),shdebugstr_guid(iid));

	if (!ppv) return E_INVALIDARG;
	*ppv = NULL;

	/* search our internal interface table */
	for(i=0;InterfaceTable[i].clsid;i++) {
	    if(IsEqualIID(InterfaceTable[i].clsid, rclsid)) {
	        TRACE("index[%u]\n", i);
	        pcf = IDefClF_fnConstructor(InterfaceTable[i].lpfnCI, NULL, NULL);
	        break;
	    }
	}

        if (!pcf) {
	    FIXME("failed for CLSID=%s\n", shdebugstr_guid(rclsid));
	    return CLASS_E_CLASSNOTAVAILABLE;
	}

	hres = IClassFactory_QueryInterface(pcf, iid, ppv);
	IClassFactory_Release(pcf);

	TRACE("-- pointer to class factory: %p\n",*ppv);
	return hres;
}

/*************************************************************************
 * SHCLSIDFromString				[SHELL32.147]
 *
 * Under Windows 9x this was an ANSI version of CLSIDFromString. It also allowed
 * to avoid dependency on ole32.dll (see SHLoadOLE for details).
 *
 * Under Windows NT/2000/XP this is equivalent to CLSIDFromString
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     CLSIDFromString, SHLoadOLE
 */
DWORD WINAPI SHCLSIDFromStringA (LPCSTR clsid, CLSID *id)
{
    WCHAR buffer[40];
    TRACE("(%p(%s) %p)\n", clsid, clsid, id);
    if (!MultiByteToWideChar( CP_ACP, 0, clsid, -1, buffer, ARRAY_SIZE(buffer) ))
        return CO_E_CLASSSTRING;
    return CLSIDFromString( buffer, id );
}
DWORD WINAPI SHCLSIDFromStringW (LPCWSTR clsid, CLSID *id)
{
	TRACE("(%p(%s) %p)\n", clsid, debugstr_w(clsid), id);
	return CLSIDFromString(clsid, id);
}
DWORD WINAPI SHCLSIDFromStringAW (LPCVOID clsid, CLSID *id)
{
	if (SHELL_OsIsUnicode())
	  return SHCLSIDFromStringW (clsid, id);
	return SHCLSIDFromStringA (clsid, id);
}

/*************************************************************************
 *			 SHGetMalloc			[SHELL32.@]
 *
 * Equivalent to CoGetMalloc(MEMCTX_TASK, ...). Under Windows 9x this function
 * could use the shell32 built-in "mini-COM" without the need to load ole32.dll -
 * see SHLoadOLE for details. 
 *
 * PARAMS
 *  lpmal [O] Destination for IMalloc interface.
 *
 * RETURNS
 *  Success: S_OK. lpmal contains the shells IMalloc interface.
 *  Failure. An HRESULT error code.
 *
 * SEE ALSO
 *  CoGetMalloc, SHLoadOLE
 */
HRESULT WINAPI SHGetMalloc(LPMALLOC *lpmal)
{
	TRACE("(%p)\n", lpmal);
	return CoGetMalloc(MEMCTX_TASK, lpmal);
}

/*************************************************************************
 * SHAlloc					[SHELL32.196]
 *
 * Equivalent to CoTaskMemAlloc. Under Windows 9x this function could use
 * the shell32 built-in "mini-COM" without the need to load ole32.dll -
 * see SHLoadOLE for details. 
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     CoTaskMemAlloc, SHLoadOLE
 */
LPVOID WINAPI SHAlloc(DWORD len)
{
	LPVOID ret;

	ret = CoTaskMemAlloc(len);
	TRACE("%lu bytes at %p\n",len, ret);
	return ret;
}

/*************************************************************************
 * SHFree					[SHELL32.195]
 *
 * Equivalent to CoTaskMemFree. Under Windows 9x this function could use
 * the shell32 built-in "mini-COM" without the need to load ole32.dll -
 * see SHLoadOLE for details. 
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     CoTaskMemFree, SHLoadOLE
 */
void WINAPI SHFree(LPVOID pv)
{
	TRACE("%p\n",pv);
	CoTaskMemFree(pv);
}

/*************************************************************************
 * SHGetDesktopFolder			[SHELL32.@]
 */
HRESULT WINAPI SHGetDesktopFolder(IShellFolder **psf)
{
	HRESULT	hres;

	TRACE("(%p)\n", psf);

	if(!psf) return E_INVALIDARG;

	*psf = NULL;
	hres = ISF_Desktop_Constructor(NULL, &IID_IShellFolder, (LPVOID*)psf);

	TRACE("-- %p->(%p) 0x%08lx\n", psf, *psf, hres);
	return hres;
}
/**************************************************************************
 * Default ClassFactory Implementation
 *
 * SHCreateDefClassObject
 *
 * NOTES
 *  Helper function for dlls without their own classfactory.
 *  A generic classfactory is returned.
 *  When the CreateInstance of the cf is called the callback is executed.
 */

typedef struct
{
    IClassFactory               IClassFactory_iface;
    LONG                        ref;
    CLSID			*rclsid;
    LPFNCREATEINSTANCE		lpfnCI;
    const IID *			riidInst;
    LONG *			pcRefDll; /* pointer to refcounter in external dll (ugrrr...) */
} IDefClFImpl;

static inline IDefClFImpl *impl_from_IClassFactory(IClassFactory *iface)
{
	return CONTAINING_RECORD(iface, IDefClFImpl, IClassFactory_iface);
}

static const IClassFactoryVtbl dclfvt;

/**************************************************************************
 *  IDefClF_fnConstructor
 */

static IClassFactory * IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst)
{
	IDefClFImpl* lpclf;

	lpclf = malloc(sizeof(*lpclf));
	lpclf->ref = 1;
	lpclf->IClassFactory_iface.lpVtbl = &dclfvt;
	lpclf->lpfnCI = lpfnCI;
	lpclf->pcRefDll = pcRefDll;

	if (pcRefDll) InterlockedIncrement(pcRefDll);
	lpclf->riidInst = riidInst;

	TRACE("(%p)%s\n",lpclf, shdebugstr_guid(riidInst));
	return &lpclf->IClassFactory_iface;
}
/**************************************************************************
 *  IDefClF_fnQueryInterface
 */
static HRESULT WINAPI IDefClF_fnQueryInterface(
  LPCLASSFACTORY iface, REFIID riid, LPVOID *ppvObj)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);

	TRACE("(%p)->(%s)\n",This,shdebugstr_guid(riid));

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory)) {
	  *ppvObj = This;
	  InterlockedIncrement(&This->ref);
	  return S_OK;
	}

	TRACE("-- E_NOINTERFACE\n");
	return E_NOINTERFACE;
}
/******************************************************************************
 * IDefClF_fnAddRef
 */
static ULONG WINAPI IDefClF_fnAddRef(LPCLASSFACTORY iface)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%lu)\n", This, refCount - 1);

	return refCount;
}
/******************************************************************************
 * IDefClF_fnRelease
 */
static ULONG WINAPI IDefClF_fnRelease(LPCLASSFACTORY iface)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%lu)\n", This, refCount + 1);

	if (!refCount)
	{
	  if (This->pcRefDll) InterlockedDecrement(This->pcRefDll);

	  TRACE("-- destroying IClassFactory(%p)\n",This);
	  free(This);
	}

	return refCount;
}
/******************************************************************************
 * IDefClF_fnCreateInstance
 */
static HRESULT WINAPI IDefClF_fnCreateInstance(
  LPCLASSFACTORY iface, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);

	TRACE("%p->(%p,%s,%p)\n",This,pUnkOuter,shdebugstr_guid(riid),ppvObject);

	*ppvObject = NULL;

	if ( This->riidInst==NULL ||
	     IsEqualCLSID(riid, This->riidInst) ||
	     IsEqualCLSID(riid, &IID_IUnknown) )
	{
	  return This->lpfnCI(pUnkOuter, riid, ppvObject);
	}

	ERR("unknown IID requested %s\n",shdebugstr_guid(riid));
	return E_NOINTERFACE;
}
/******************************************************************************
 * IDefClF_fnLockServer
 */
static HRESULT WINAPI IDefClF_fnLockServer(LPCLASSFACTORY iface, BOOL fLock)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);
	TRACE("%p->(0x%x), not implemented\n",This, fLock);
	return E_NOTIMPL;
}

static const IClassFactoryVtbl dclfvt =
{
    IDefClF_fnQueryInterface,
    IDefClF_fnAddRef,
  IDefClF_fnRelease,
  IDefClF_fnCreateInstance,
  IDefClF_fnLockServer
};

/******************************************************************************
 * SHCreateDefClassObject			[SHELL32.70]
 */
HRESULT WINAPI SHCreateDefClassObject(
	REFIID	riid,
	LPVOID*	ppv,
	LPFNCREATEINSTANCE lpfnCI,	/* [in] create instance callback entry */
	LPDWORD	pcRefDll,		/* [in/out] ref count of the dll */
	REFIID	riidInst)		/* [in] optional interface to the instance */
{
	IClassFactory * pcf;

	TRACE("%s %p %p %p %s\n",
              shdebugstr_guid(riid), ppv, lpfnCI, pcRefDll, shdebugstr_guid(riidInst));

	if (! IsEqualCLSID(riid, &IID_IClassFactory) ) return E_NOINTERFACE;
	if (! (pcf = IDefClF_fnConstructor(lpfnCI, (PLONG)pcRefDll, riidInst))) return E_OUTOFMEMORY;
	*ppv = pcf;
	return S_OK;
}

/*************************************************************************
 *  DragAcceptFiles		[SHELL32.@]
 */
void WINAPI DragAcceptFiles(HWND hWnd, BOOL b)
{
	LONG exstyle;

	if( !IsWindow(hWnd) ) return;
	exstyle = GetWindowLongA(hWnd,GWL_EXSTYLE);
	if (b)
	  exstyle |= WS_EX_ACCEPTFILES;
	else
	  exstyle &= ~WS_EX_ACCEPTFILES;
	SetWindowLongA(hWnd,GWL_EXSTYLE,exstyle);
}

/*************************************************************************
 * DragFinish		[SHELL32.@]
 */
void WINAPI DragFinish(HDROP h)
{
	TRACE("\n");
	GlobalFree(h);
}

/*************************************************************************
 * DragQueryPoint		[SHELL32.@]
 */
BOOL WINAPI DragQueryPoint(HDROP hDrop, POINT *p)
{
        DROPFILES *lpDropFileStruct;
	BOOL bRet;

	TRACE("\n");

	lpDropFileStruct = GlobalLock(hDrop);

        *p = lpDropFileStruct->pt;
	bRet = !lpDropFileStruct->fNC;

	GlobalUnlock(hDrop);
	return bRet;
}

/*************************************************************************
 *  DragQueryFileA		[SHELL32.@]
 *  DragQueryFile 		[SHELL32.@]
 */
UINT WINAPI DragQueryFileA(HDROP hDrop, UINT lFile, LPSTR lpszFile, UINT lLength)
{
    LPWSTR filenameW = NULL;
    LPSTR filename = NULL;
    UINT i;

    TRACE("(%p, %x, %p, %u)\n", hDrop, lFile, lpszFile, lLength);

    i = DragQueryFileW(hDrop, lFile, NULL, 0);
    if (!i || lFile == 0xFFFFFFFF) goto end;
    filenameW = malloc((i + 1) * sizeof(WCHAR));
    if (!filenameW) goto error;
    if (!DragQueryFileW(hDrop, lFile, filenameW, i + 1)) goto error;

    i = WideCharToMultiByte(CP_ACP, 0, filenameW, -1, NULL, 0, NULL, NULL);
    if (!lpszFile || !lLength)
    {
        /* minus a trailing null */
        i--;
        goto end;
    }
    filename = malloc(i);
    if (!filename) goto error;
    i = WideCharToMultiByte(CP_ACP, 0, filenameW, -1, filename, i, NULL, NULL);

    lstrcpynA(lpszFile, filename, lLength);
    i = min(i, lLength) - 1;
end:
    free(filenameW);
    free(filename);
    return i;
error:
    i = 0;
    goto end;
}

/*************************************************************************
 *  DragQueryFileW		[SHELL32.@]
 */
UINT WINAPI DragQueryFileW(
	HDROP hDrop,
	UINT lFile,
	LPWSTR lpszwFile,
	UINT lLength)
{
	LPWSTR buffer = NULL;
	LPCWSTR filename;
	UINT i = 0;
	const DROPFILES *lpDropFileStruct = GlobalLock(hDrop);

	TRACE("(%p, %x, %p, %u)\n", hDrop,lFile,lpszwFile,lLength);

	if(!lpDropFileStruct) goto end;

        if(lpDropFileStruct->fWide)
        {
            LPCWSTR p = (LPCWSTR) ((LPCSTR)lpDropFileStruct + lpDropFileStruct->pFiles);
            while (i++ < lFile)
            {
                while (*p++); /* skip filename */
                if (!*p)
                {
                    i = (lFile == 0xFFFFFFFF) ? i : 0;
                    goto end;
                }
            }
            filename = p;
        }
        else
        {
            LPCSTR p = (LPCSTR)lpDropFileStruct + lpDropFileStruct->pFiles;
            while (i++ < lFile)
            {
                while (*p++); /* skip filename */
                if (!*p)
                {
                    i = (lFile == 0xFFFFFFFF) ? i : 0;
                    goto end;
                }
            }
            i = MultiByteToWideChar(CP_ACP, 0, p, -1, NULL, 0);
            buffer = malloc(i * sizeof(WCHAR));
            if (!buffer)
            {
                i = 0;
                goto end;
            }
            MultiByteToWideChar(CP_ACP, 0, p, -1, buffer, i);
            filename = buffer;
        }

	i = lstrlenW(filename);
	if (!lpszwFile || !lLength) goto end;   /* needed buffer size */
	lstrcpynW(lpszwFile, filename, lLength);
	i = min(i, lLength - 1);
end:
	GlobalUnlock(hDrop);
	free(buffer);
	return i;
}

/*************************************************************************
 *  SHPropStgCreate             [SHELL32.685]
 */
HRESULT WINAPI SHPropStgCreate(IPropertySetStorage *psstg, REFFMTID fmtid,
        const CLSID *pclsid, DWORD grfFlags, DWORD grfMode,
        DWORD dwDisposition, IPropertyStorage **ppstg, UINT *puCodePage)
{
    PROPSPEC prop;
    PROPVARIANT ret;
    HRESULT hres;

    TRACE("%p %s %s %lx %lx %lx %p %p\n", psstg, debugstr_guid(fmtid), debugstr_guid(pclsid),
            grfFlags, grfMode, dwDisposition, ppstg, puCodePage);

    hres = IPropertySetStorage_Open(psstg, fmtid, grfMode, ppstg);

    switch(dwDisposition) {
    case CREATE_ALWAYS:
        if(SUCCEEDED(hres)) {
            IPropertyStorage_Release(*ppstg);
            hres = IPropertySetStorage_Delete(psstg, fmtid);
            if(FAILED(hres))
                return hres;
            hres = E_FAIL;
        }

    case OPEN_ALWAYS:
    case CREATE_NEW:
        if(FAILED(hres))
            hres = IPropertySetStorage_Create(psstg, fmtid, pclsid,
                    grfFlags, grfMode, ppstg);

    case OPEN_EXISTING:
        if(FAILED(hres))
            return hres;

        if(puCodePage) {
            prop.ulKind = PRSPEC_PROPID;
            prop.propid = PID_CODEPAGE;
            hres = IPropertyStorage_ReadMultiple(*ppstg, 1, &prop, &ret);
            if(FAILED(hres) || ret.vt!=VT_I2)
                *puCodePage = 0;
            else
                *puCodePage = ret.iVal;
        }
    }

    return S_OK;
}

/*************************************************************************
 *  SHPropStgReadMultiple       [SHELL32.688]
 */
HRESULT WINAPI SHPropStgReadMultiple(IPropertyStorage *pps, UINT uCodePage,
        ULONG cpspec, const PROPSPEC *rgpspec, PROPVARIANT *rgvar)
{
    STATPROPSETSTG stat;
    HRESULT hres;

    FIXME("%p %u %lu %p %p\n", pps, uCodePage, cpspec, rgpspec, rgvar);

    memset(rgvar, 0, cpspec*sizeof(PROPVARIANT));
    hres = IPropertyStorage_ReadMultiple(pps, cpspec, rgpspec, rgvar);
    if(FAILED(hres))
        return hres;

    if(!uCodePage) {
        PROPSPEC prop;
        PROPVARIANT ret;

        prop.ulKind = PRSPEC_PROPID;
        prop.propid = PID_CODEPAGE;
        hres = IPropertyStorage_ReadMultiple(pps, 1, &prop, &ret);
        if(FAILED(hres) || ret.vt!=VT_I2)
            return S_OK;

        uCodePage = ret.iVal;
    }

    hres = IPropertyStorage_Stat(pps, &stat);
    if(FAILED(hres))
        return S_OK;

    /* TODO: do something with codepage and stat */
    return S_OK;
}

/*************************************************************************
 *  SHPropStgWriteMultiple      [SHELL32.689]
 */
HRESULT WINAPI SHPropStgWriteMultiple(IPropertyStorage *pps, UINT *uCodePage,
        ULONG cpspec, const PROPSPEC *rgpspec, PROPVARIANT *rgvar, PROPID propidNameFirst)
{
    STATPROPSETSTG stat;
    UINT codepage;
    HRESULT hres;

    FIXME("%p %p %lu %p %p %ld\n", pps, uCodePage, cpspec, rgpspec, rgvar, propidNameFirst);

    hres = IPropertyStorage_Stat(pps, &stat);
    if(FAILED(hres))
        return hres;

    if(uCodePage && *uCodePage)
        codepage = *uCodePage;
    else {
        PROPSPEC prop;
        PROPVARIANT ret;

        prop.ulKind = PRSPEC_PROPID;
        prop.propid = PID_CODEPAGE;
        hres = IPropertyStorage_ReadMultiple(pps, 1, &prop, &ret);
        if(FAILED(hres))
            return hres;
        if(ret.vt!=VT_I2 || !ret.iVal)
            return E_FAIL;

        codepage = ret.iVal;
        if(uCodePage)
            *uCodePage = codepage;
    }

    /* TODO: do something with codepage and stat */

    hres = IPropertyStorage_WriteMultiple(pps, cpspec, rgpspec, rgvar, propidNameFirst);
    return hres;
}

/*************************************************************************
 *  SHCreateQueryCancelAutoPlayMoniker [SHELL32.@]
 */
HRESULT WINAPI SHCreateQueryCancelAutoPlayMoniker(IMoniker **moniker)
{
    TRACE("%p\n", moniker);

    if (!moniker) return E_INVALIDARG;
    return CreateClassMoniker(&CLSID_QueryCancelAutoPlay, moniker);
}

static HRESULT gpstatus_to_hresult(GpStatus status)
{
    switch (status)
    {
    case Ok:
        return S_OK;
    case InvalidParameter:
        return E_INVALIDARG;
    case OutOfMemory:
        return E_OUTOFMEMORY;
    case NotImplemented:
        return E_NOTIMPL;
    default:
        return E_FAIL;
    }
}

/* IShellImageData */
typedef struct
{
    IShellImageData IShellImageData_iface;
    LONG ref;

    WCHAR *path;
    GpImage *image;
} ShellImageData;

static inline ShellImageData *impl_from_IShellImageData(IShellImageData *iface)
{
    return CONTAINING_RECORD(iface, ShellImageData, IShellImageData_iface);
}

static HRESULT WINAPI ShellImageData_QueryInterface(IShellImageData *iface, REFIID riid, void **obj)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    TRACE("%p, %s, %p\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IShellImageData) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &This->IShellImageData_iface;
        IShellImageData_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ShellImageData_AddRef(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p, %lu\n", This, ref);

    return ref;
}

static ULONG WINAPI ShellImageData_Release(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, %lu\n", This, ref);

    if (!ref)
    {
        GdipDisposeImage(This->image);
        free(This->path);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ShellImageData_Decode(IShellImageData *iface, DWORD flags, ULONG cx_desired, ULONG cy_desired)
{
    ShellImageData *This = impl_from_IShellImageData(iface);
    GpImage *image;
    HRESULT hr;

    TRACE("%p, %#lx, %lu, %lu\n", This, flags, cx_desired, cy_desired);

    if (This->image)
        return S_FALSE;

    if (flags & SHIMGDEC_LOADFULL)
        FIXME("LOADFULL flag ignored\n");

    hr = gpstatus_to_hresult(GdipLoadImageFromFile(This->path, &image));
    if (FAILED(hr))
        return hr;

    if (flags & SHIMGDEC_THUMBNAIL)
    {
        hr = gpstatus_to_hresult(GdipGetImageThumbnail(image, cx_desired, cy_desired, &This->image, NULL, NULL));
        GdipDisposeImage(image);
    }
    else
        This->image = image;

    return hr;
}

static HRESULT WINAPI ShellImageData_Draw(IShellImageData *iface, HDC hdc, RECT *dest, RECT *src)
{
    ShellImageData *This = impl_from_IShellImageData(iface);
    GpGraphics *graphics;
    HRESULT hr;

    TRACE("%p, %p, %s, %s\n", This, hdc, wine_dbgstr_rect(dest), wine_dbgstr_rect(src));

    if (!This->image)
        return E_FAIL;

    if (!dest)
        return E_INVALIDARG;

    if (!src)
        return S_OK;

    hr = gpstatus_to_hresult(GdipCreateFromHDC(hdc, &graphics));
    if (FAILED(hr))
        return hr;

    hr = gpstatus_to_hresult(GdipDrawImageRectRectI(graphics, This->image, dest->left, dest->top, dest->right - dest->left,
        dest->bottom - dest->top, src->left, src->top, src->right - src->left, src->bottom - src->top,
        UnitPixel, NULL, NULL, NULL));
    GdipDeleteGraphics(graphics);

    return hr;
}

static HRESULT WINAPI ShellImageData_NextFrame(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_NextPage(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_PrevPage(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_IsTransparent(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_IsAnimated(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_IsVector(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_IsMultipage(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_IsEditable(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_IsPrintable(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_IsDecoded(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_GetCurrentPage(IShellImageData *iface, ULONG *page)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_GetPageCount(IShellImageData *iface, ULONG *count)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %p: stub\n", This, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageDate_SelectPage(IShellImageData *iface, ULONG page)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %lu: stub\n", This, page);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_GetSize(IShellImageData *iface, SIZE *size)
{
    ShellImageData *This = impl_from_IShellImageData(iface);
    REAL cx, cy;
    HRESULT hr;

    TRACE("%p, %p\n", This, size);

    if (!This->image)
        return E_FAIL;

    hr = gpstatus_to_hresult(GdipGetImageDimension(This->image, &cx, &cy));
    if (SUCCEEDED(hr))
    {
        size->cx = cx;
        size->cy = cy;
    }

    return hr;
}

static HRESULT WINAPI ShellImageData_GetRawDataFormat(IShellImageData *iface, GUID *format)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %p: stub\n", This, format);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_GetPixelFormat(IShellImageData *iface, PixelFormat *format)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %p: stub\n", This, format);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_GetDelay(IShellImageData *iface, DWORD *delay)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %p: stub\n", This, delay);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_GetProperties(IShellImageData *iface, DWORD mode, IPropertySetStorage **props)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %#lx, %p: stub\n", This, mode, props);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_Rotate(IShellImageData *iface, DWORD angle)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %lu: stub\n", This, angle);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_Scale(IShellImageData *iface, ULONG cx, ULONG cy, InterpolationMode mode)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %lu, %lu, %#x: stub\n", This, cx, cy, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_DiscardEdit(IShellImageData *iface)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageDate_SetEncoderParams(IShellImageData *iface, IPropertyBag *params)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %p: stub\n", This, params);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_DisplayName(IShellImageData *iface, LPWSTR name, UINT count)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %p, %u: stub\n", This, name, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_GetResolution(IShellImageData *iface, ULONG *res_x, ULONG *res_y)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %p, %p: stub\n", This, res_x, res_y);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_GetEncoderParams(IShellImageData *iface, GUID *format, EncoderParameters **params)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %p, %p: stub\n", This, format, params);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_RegisterAbort(IShellImageData *iface, IShellImageDataAbort *abort,
    IShellImageDataAbort **prev)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %p, %p: stub\n", This, abort, prev);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_CloneFrame(IShellImageData *iface, Image **frame)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %p: stub\n", This, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageData_ReplaceFrame(IShellImageData *iface, Image *frame)
{
    ShellImageData *This = impl_from_IShellImageData(iface);

    FIXME("%p, %p: stub\n", This, frame);

    return E_NOTIMPL;
}

static const IShellImageDataVtbl ShellImageDataVtbl =
{
    ShellImageData_QueryInterface,
    ShellImageData_AddRef,
    ShellImageData_Release,
    ShellImageData_Decode,
    ShellImageData_Draw,
    ShellImageData_NextFrame,
    ShellImageData_NextPage,
    ShellImageData_PrevPage,
    ShellImageData_IsTransparent,
    ShellImageData_IsAnimated,
    ShellImageData_IsVector,
    ShellImageData_IsMultipage,
    ShellImageData_IsEditable,
    ShellImageData_IsPrintable,
    ShellImageData_IsDecoded,
    ShellImageData_GetCurrentPage,
    ShellImageData_GetPageCount,
    ShellImageDate_SelectPage,
    ShellImageData_GetSize,
    ShellImageData_GetRawDataFormat,
    ShellImageData_GetPixelFormat,
    ShellImageData_GetDelay,
    ShellImageData_GetProperties,
    ShellImageData_Rotate,
    ShellImageData_Scale,
    ShellImageData_DiscardEdit,
    ShellImageDate_SetEncoderParams,
    ShellImageData_DisplayName,
    ShellImageData_GetResolution,
    ShellImageData_GetEncoderParams,
    ShellImageData_RegisterAbort,
    ShellImageData_CloneFrame,
    ShellImageData_ReplaceFrame,
};

static HRESULT create_shellimagedata_from_path(const WCHAR *path, IShellImageData **data)
{
    ShellImageData *This;

    This = malloc(sizeof(*This));

    This->IShellImageData_iface.lpVtbl = &ShellImageDataVtbl;
    This->ref = 1;

    This->path = wcsdup(path);
    This->image = NULL;

    *data = &This->IShellImageData_iface;
    return S_OK;
}

/* IShellImageDataFactory */
static HRESULT WINAPI ShellImageDataFactory_QueryInterface(IShellImageDataFactory *iface, REFIID riid, void **obj)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(&IID_IShellImageDataFactory, riid) || IsEqualIID(&IID_IUnknown, riid))
    {
        *obj = iface;
    }
    else
    {
        FIXME("not implemented for %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
}

static ULONG WINAPI ShellImageDataFactory_AddRef(IShellImageDataFactory *iface)
{
    TRACE("(%p)\n", iface);

    return 2;
}

static ULONG WINAPI ShellImageDataFactory_Release(IShellImageDataFactory *iface)
{
    TRACE("(%p)\n", iface);

    return 1;
}

static HRESULT WINAPI ShellImageDataFactory_CreateIShellImageData(IShellImageDataFactory *iface, IShellImageData **data)
{
    FIXME("%p, %p: stub\n", iface, data);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageDataFactory_CreateImageFromFile(IShellImageDataFactory *iface, const WCHAR *path,
    IShellImageData **data)
{
    TRACE("%p, %s, %p\n", iface, debugstr_w(path), data);

    return create_shellimagedata_from_path(path, data);
}

static HRESULT WINAPI ShellImageDataFactory_CreateImageFromStream(IShellImageDataFactory *iface, IStream *stream,
    IShellImageData **data)
{
    FIXME("%p, %p, %p: stub\n", iface, stream, data);

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellImageDataFactory_GetDataFormatFromPath(IShellImageDataFactory *iface, const WCHAR *path,
    GUID *format)
{
    FIXME("%p, %s, %p: stub\n", iface, debugstr_w(path), format);

    return E_NOTIMPL;
}

static const IShellImageDataFactoryVtbl ShellImageDataFactoryVtbl =
{
    ShellImageDataFactory_QueryInterface,
    ShellImageDataFactory_AddRef,
    ShellImageDataFactory_Release,
    ShellImageDataFactory_CreateIShellImageData,
    ShellImageDataFactory_CreateImageFromFile,
    ShellImageDataFactory_CreateImageFromStream,
    ShellImageDataFactory_GetDataFormatFromPath,
};

static IShellImageDataFactory ShellImageDataFactory = { &ShellImageDataFactoryVtbl };

HRESULT WINAPI ShellImageDataFactory_Constructor(IUnknown *outer, REFIID riid, void **obj)
{
    TRACE("%p %s %p\n", outer, debugstr_guid(riid), obj);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return IShellImageDataFactory_QueryInterface(&ShellImageDataFactory, riid, obj);
}
