/*
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
 *
 */

#include <string.h>
#include "debug.h"
#include "winerror.h"

#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "wine/obj_shelllink.h"

#include "heap.h"
#include "winnls.h"
#include "pidl.h"
#include "shell32_main.h"
#include "shlguid.h"

/* link file formats */

#pragma (1);

/* lnk elements: simple link has 0x0B */
#define	WORKDIR		0x10
#define	ARGUMENT	0x20
#define	ICON		0x40
/* startup type */
#define	NORMAL		0x01
#define	MAXIMIZED	0x03
#define	MINIMIZED	0x07

typedef struct _LINK_HEADER	
{	DWORD	MagicStr;	/* 0x00 'L','\0','\0','\0' */
	GUID	MagicGuid;	/* 0x04 is CLSID_ShellLink */
	DWORD	Flag1;		/* 0x14 describes elements following */
	DWORD	Flag2;		/* 0x18 */
	FILETIME Time1;		/* 0x1c */
	FILETIME Time2;		/* 0x24 */
	FILETIME Time3;		/* 0x2c */
	DWORD	Unknown1;	/* 0x34 */
	DWORD	Unknown2;	/* 0x38 icon number */
	DWORD	Flag3;		/* 0x3c startup type */
	DWORD	Unknown4;	/* 0x40 hotkey */
	DWORD	Unknown5;	/* 0x44 */
	DWORD	Unknown6;	/* 0x48 */
	USHORT	PidlSize;	/* 0x4c */
	ITEMIDLIST Pidl;	/* 0x4e */
} LINK_HEADER, * PLINK_HEADER;

#pragma (4);

/* IPersistFile Implementation */
typedef struct
{
	/* IUnknown fields */
	ICOM_VTABLE(IPersistFile)*	lpvtbl;
	DWORD		ref;
	LPSTR		sPath;
	LPITEMIDLIST	pPidl;

} IPersistFileImpl;

static struct ICOM_VTABLE(IPersistFile) pfvt;


/**************************************************************************
 *	  IPersistFile_Constructor
 */
IPersistFileImpl * IPersistFile_Constructor(void) 
{
	IPersistFileImpl* sl;

	sl = (IPersistFileImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IPersistFileImpl));
	sl->ref = 1;
	sl->lpvtbl = &pfvt;
	sl->sPath = NULL;
	sl->pPidl = NULL;
			
	TRACE(shell,"(%p)->()\n",sl);
	shell32_ObjCount++;
	return sl;
}

/**************************************************************************
 *  IPersistFile_QueryInterface
 */
static HRESULT WINAPI IPersistFile_fnQueryInterface(
  IPersistFile* iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IPersistFileImpl,iface);

	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",This,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IPersistFile))  /*IPersistFile*/
	{    *ppvObj = (LPPERSISTFILE)This;
	}   

	if(*ppvObj)
	{ IPersistFile_AddRef((IPersistFile*)*ppvObj);      
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IPersistFile_AddRef
 */
static ULONG WINAPI IPersistFile_fnAddRef(IPersistFile* iface)
{
	ICOM_THIS(IPersistFileImpl,iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}
/******************************************************************************
 * IPersistFile_Release
 */
static ULONG WINAPI IPersistFile_fnRelease(IPersistFile* iface)
{
	ICOM_THIS(IPersistFileImpl,iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{ TRACE(shell,"-- destroying IPersistFile(%p)\n",This);
	  if (This->sPath)
	    HeapFree(GetProcessHeap(),0,This->sPath);
	  if (This->pPidl)
	    SHFree(This->pPidl);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IPersistFile_fnGetClassID(const IPersistFile* iface, CLSID *pClassID)
{
	ICOM_CTHIS(IPersistFile,iface);
	FIXME(shell,"(%p)\n",This);
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_fnIsDirty(const IPersistFile* iface)
{
	ICOM_CTHIS(IPersistFile,iface);
	FIXME(shell,"(%p)\n",This);
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_fnLoad(IPersistFile* iface, LPCOLESTR pszFileName, DWORD dwMode)
{
	ICOM_THIS(IPersistFileImpl,iface);

	OFSTRUCT	ofs;
	LPSTR	sFile = HEAP_strdupWtoA ( GetProcessHeap(), 0, pszFileName);
	HFILE	hFile = OpenFile( sFile, &ofs, OF_READ );
	HANDLE	hMapping;
	PLINK_HEADER	pImage;
	HRESULT		hRet = E_FAIL;
	CHAR		sTemp[512];
	
	TRACE(shell,"(%p)->(%s)\n",This,sFile); 

	HeapFree(GetProcessHeap(),0,sFile);

	if ( !(hMapping = CreateFileMappingA(hFile,NULL,PAGE_READONLY|SEC_COMMIT,0,0,NULL))) 
	{ WARN(shell,"failed to create filemap.\n");
	  goto end_1;
	}

	if ( !(pImage = (PLINK_HEADER) MapViewOfFile(hMapping,FILE_MAP_READ,0,0,0))) 
	{ WARN(shell,"failed to mmap filemap.\n");
	  goto end_2;	
	}
	
	if (!( (pImage->MagicStr == 0x0000004CL) && IsEqualIID(&pImage->MagicGuid, &CLSID_ShellLink)))
	{ TRACE(shell,"file isn't a lnk\n");
	  goto end_3;
	}
	
	{ /* for debugging */
	  SYSTEMTIME time;
	  FileTimeToSystemTime (&pImage->Time1, &time);
	  GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&time, NULL,  sTemp, 256);
	  TRACE(shell, "-- time1: %s\n", sTemp);

	  FileTimeToSystemTime (&pImage->Time2, &time);
	  GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&time, NULL,  sTemp, 256);
	  TRACE(shell, "-- time2: %s\n", sTemp);

	  FileTimeToSystemTime (&pImage->Time3, &time);
	  GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&time, NULL,  sTemp, 256);
	  TRACE(shell, "-- time3: %s\n", sTemp);
	  pdump (&pImage->Pidl);
	}

	This->pPidl = ILClone (&pImage->Pidl);

	_ILGetPidlPath(&pImage->Pidl, sTemp, 512);
	This->sPath = HEAP_strdupA ( GetProcessHeap(), 0, sTemp);
		
	hRet = NOERROR;
end_3:	UnmapViewOfFile(pImage);
end_2:	CloseHandle(hMapping);
end_1:	_lclose( hFile);

	return hRet;
}

static HRESULT WINAPI IPersistFile_fnSave(IPersistFile* iface, LPCOLESTR pszFileName, BOOL fRemember)
{
	ICOM_THIS(IPersistFileImpl,iface);
	FIXME(shell,"(%p)->(%s)\n",This,debugstr_w(pszFileName));
       	return NOERROR;
}
static HRESULT WINAPI IPersistFile_fnSaveCompleted(IPersistFile* iface, LPCOLESTR pszFileName)
{
	ICOM_THIS(IPersistFileImpl,iface);
	FIXME(shell,"(%p)->(%s)\n",This,debugstr_w(pszFileName));
        return NOERROR;
}
static HRESULT WINAPI IPersistFile_fnGetCurFile(const IPersistFile* iface, LPOLESTR *ppszFileName)
{
	ICOM_CTHIS(IPersistFileImpl,iface);
	FIXME(shell,"(%p)\n",This);
	return NOERROR;
}
  
static struct ICOM_VTABLE(IPersistFile) pfvt = 
{
	IPersistFile_fnQueryInterface,
	IPersistFile_fnAddRef,
	IPersistFile_fnRelease,
	IPersistFile_fnGetClassID,
	IPersistFile_fnIsDirty,
	IPersistFile_fnLoad,
	IPersistFile_fnSave,
	IPersistFile_fnSaveCompleted,
	IPersistFile_fnGetCurFile
};


/**************************************************************************
*  IShellLink's IClassFactory implementation
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VTABLE(IClassFactory)* lpvtbl;
    DWORD                       ref;
} IClassFactoryImpl;

static ICOM_VTABLE(IClassFactory) slcfvt;

/**************************************************************************
 *  IShellLink_CF_Constructor
 */

LPCLASSFACTORY IShellLink_CF_Constructor(void)
{
	IClassFactoryImpl* lpclf;

	lpclf= (IClassFactoryImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IClassFactoryImpl));
	lpclf->ref = 1;
	lpclf->lpvtbl = &slcfvt;
	TRACE(shell,"(%p)->()\n",lpclf);
	shell32_ObjCount++;
	return (LPCLASSFACTORY)lpclf;
}
/**************************************************************************
 *  IShellLink_CF_QueryInterface
 */
static HRESULT WINAPI IShellLink_CF_QueryInterface(
  IClassFactory* iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	char	xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",This,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          	/*IUnknown*/
	{ *ppvObj = (LPUNKNOWN)This; 
	}
	else if(IsEqualIID(riid, &IID_IClassFactory))  /*IClassFactory*/
	{ *ppvObj = (LPCLASSFACTORY)This;
	}   

	if(*ppvObj)
	{ IUnknown_AddRef((IUnknown*)*ppvObj);  	
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IShellLink_CF_AddRef
 */
static ULONG WINAPI IShellLink_CF_AddRef(IClassFactory* iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}
/******************************************************************************
 * IShellLink_CF_Release
 */
static ULONG WINAPI IShellLink_CF_Release(IClassFactory* iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount--;
	if (!--(This->ref)) 
	{ TRACE(shell,"-- destroying IClassFactory(%p)\n",This);
		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
	return This->ref;
}
/******************************************************************************
 * IShellLink_CF_CreateInstance
 */
static HRESULT WINAPI IShellLink_CF_CreateInstance(
  IClassFactory* iface, LPUNKNOWN pUnknown, REFIID riid, LPVOID *ppObject)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	IUnknown *pObj = NULL;
	HRESULT hres;
	char	xriid[50];

	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"%p->(%p,\n\tIID:\t%s,%p)\n",This,pUnknown,xriid,ppObject);

	*ppObject = NULL;
		
	if(pUnknown)
	{	return(CLASS_E_NOAGGREGATION);
	}

	if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IShellLink))
	{ pObj = (IUnknown *)IShellLink_Constructor();
	} 
	else
	{ ERR(shell,"unknown IID requested\n\tIID:\t%s\n",xriid);
	  return(E_NOINTERFACE);
	}
	
	if (!pObj)
	{ return(E_OUTOFMEMORY);
	}
	 
	hres = IUnknown_QueryInterface(pObj,riid, ppObject);
	IUnknown_Release(pObj);
	TRACE(shell,"-- Object created: (%p)->%p\n",This,*ppObject);

	return hres;
}
/******************************************************************************
 * IShellLink_CF_LockServer
 */
static HRESULT WINAPI IShellLink_CF_LockServer(IClassFactory* iface, BOOL fLock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE(shell,"%p->(0x%x), not implemented\n",This, fLock);
	return E_NOTIMPL;
}
static ICOM_VTABLE(IClassFactory) slcfvt = 
{
    IShellLink_CF_QueryInterface,
    IShellLink_CF_AddRef,
  IShellLink_CF_Release,
  IShellLink_CF_CreateInstance,
  IShellLink_CF_LockServer
};

/**************************************************************************
 * IShellLink Implementation 
 */

typedef struct
{
	ICOM_VTABLE(IShellLink)*	lpvtbl;
	DWORD				ref;
	IPersistFileImpl*		lppf;

} IShellLinkImpl;

static ICOM_VTABLE(IShellLink) slvt;

/**************************************************************************
 *	  IShellLink_Constructor
 */
IShellLink * IShellLink_Constructor(void) 
{	IShellLinkImpl * sl;

	sl = (IShellLinkImpl *)HeapAlloc(GetProcessHeap(),0,sizeof(IShellLinkImpl));
	sl->ref = 1;
	sl->lpvtbl = &slvt;

	sl->lppf = IPersistFile_Constructor();

	TRACE(shell,"(%p)->()\n",sl);
	shell32_ObjCount++;
	return (IShellLink *)sl;
}

/**************************************************************************
 *  IShellLink::QueryInterface
 */
static HRESULT WINAPI IShellLink_fnQueryInterface( IShellLink * iface, REFIID riid,  LPVOID *ppvObj)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",This,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IShellLink))  /*IShellLink*/
	{    *ppvObj = (IShellLink *)This;
	}   
	else if(IsEqualIID(riid, &IID_IPersistFile))  /*IPersistFile*/
	{    *ppvObj = (IPersistFile *)This->lppf;
	}   

	if(*ppvObj)
	{ IShellLink_AddRef((IShellLink*)*ppvObj);      
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IShellLink_AddRef
 */
static ULONG WINAPI IShellLink_fnAddRef(IShellLink * iface)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}
/******************************************************************************
 *	IShellLink_Release
 */
static ULONG WINAPI IShellLink_fnRelease(IShellLink * iface)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount--;
	if (!--(This->ref)) 
	{ TRACE(shell,"-- destroying IShellLink(%p)\n",This);
	  IPersistFile_Release((IPersistFile*) This->lppf);	/* IPersistFile*/
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IShellLink_fnGetPath(IShellLink * iface, LPSTR pszFile,INT cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE(shell,"(%p)->(pfile=%p len=%u find_data=%p flags=%lu)\n",This, pszFile, cchMaxPath, pfd, fFlags);

	strncpy(pszFile,This->lppf->sPath, cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnGetIDList(IShellLink * iface, LPITEMIDLIST * ppidl)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE(shell,"(%p)->(ppidl=%p)\n",This, ppidl);

	*ppidl = ILClone(This->lppf->pPidl);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnSetIDList(IShellLink * iface, LPCITEMIDLIST pidl)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE (shell,"(%p)->(pidl=%p)\n",This, pidl);

	if (This->lppf->pPidl)
	    SHFree(This->lppf->pPidl);
	This->lppf->pPidl = ILClone (pidl);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnGetDescription(IShellLink * iface, LPSTR pszName,INT cchMaxName)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(%p len=%u)\n",This, pszName, cchMaxName);
	strncpy(pszName,"Description, FIXME",cchMaxName);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnSetDescription(IShellLink * iface, LPCSTR pszName)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(desc=%s)\n",This, pszName);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnGetWorkingDirectory(IShellLink * iface, LPSTR pszDir,INT cchMaxPath)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->()\n",This);
	strncpy(pszDir,"c:\\", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnSetWorkingDirectory(IShellLink * iface, LPCSTR pszDir)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(dir=%s)\n",This, pszDir);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnGetArguments(IShellLink * iface, LPSTR pszArgs,INT cchMaxPath)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(%p len=%u)\n",This, pszArgs, cchMaxPath);
	strncpy(pszArgs, "", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnSetArguments(IShellLink * iface, LPCSTR pszArgs)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(args=%s)\n",This, pszArgs);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnGetHotkey(IShellLink * iface, WORD *pwHotkey)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(%p)\n",This, pwHotkey);
	*pwHotkey=0x0;
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnSetHotkey(IShellLink * iface, WORD wHotkey)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(hotkey=%x)\n",This, wHotkey);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnGetShowCmd(IShellLink * iface, INT *piShowCmd)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(%p)\n",This, piShowCmd);
	*piShowCmd=0;
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnSetShowCmd(IShellLink * iface, INT iShowCmd)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(showcmd=%x)\n",This, iShowCmd);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnGetIconLocation(IShellLink * iface, LPSTR pszIconPath,INT cchIconPath,INT *piIcon)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(%p len=%u iicon=%p)\n",This, pszIconPath, cchIconPath, piIcon);
	strncpy(pszIconPath,"shell32.dll",cchIconPath);
	*piIcon=1;
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnSetIconLocation(IShellLink * iface, LPCSTR pszIconPath,INT iIcon)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(path=%s iicon=%u)\n",This, pszIconPath, iIcon);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnSetRelativePath(IShellLink * iface, LPCSTR pszPathRel, DWORD dwReserved)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(path=%s %lx)\n",This, pszPathRel, dwReserved);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnResolve(IShellLink * iface, HWND hwnd, DWORD fFlags)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(hwnd=%x flags=%lx)\n",This, hwnd, fFlags);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_fnSetPath(IShellLink * iface, LPCSTR pszFile)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME(shell,"(%p)->(path=%s)\n",This, pszFile);
	return NOERROR;
}

/**************************************************************************
* IShellLink Implementation
*/

static ICOM_VTABLE(IShellLink) slvt = 
{	IShellLink_fnQueryInterface,
	IShellLink_fnAddRef,
	IShellLink_fnRelease,
	IShellLink_fnGetPath,
	IShellLink_fnGetIDList,
	IShellLink_fnSetIDList,
	IShellLink_fnGetDescription,
	IShellLink_fnSetDescription,
	IShellLink_fnGetWorkingDirectory,
	IShellLink_fnSetWorkingDirectory,
	IShellLink_fnGetArguments,
	IShellLink_fnSetArguments,
	IShellLink_fnGetHotkey,
	IShellLink_fnSetHotkey,
	IShellLink_fnGetShowCmd,
	IShellLink_fnSetShowCmd,
	IShellLink_fnGetIconLocation,
	IShellLink_fnSetIconLocation,
	IShellLink_fnSetRelativePath,
	IShellLink_fnResolve,
	IShellLink_fnSetPath
};

/**************************************************************************
*  IShellLink's IClassFactory implementation
*/

static ICOM_VTABLE(IClassFactory) slwcfvt;

/**************************************************************************
 *  IShellLinkW_CF_Constructor
 */

LPCLASSFACTORY IShellLinkW_CF_Constructor(void)
{
	IClassFactoryImpl* lpclf;

	lpclf= (IClassFactoryImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IClassFactoryImpl));
	lpclf->ref = 1;
	lpclf->lpvtbl = &slwcfvt;
	TRACE(shell,"(%p)->()\n",lpclf);
	shell32_ObjCount++;
	return (LPCLASSFACTORY)lpclf;
}
/**************************************************************************
 *  IShellLinkW_CF_QueryInterface
 */
static HRESULT WINAPI IShellLinkW_CF_QueryInterface(
  LPCLASSFACTORY iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	char	xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",This,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          	/*IUnknown*/
	{ *ppvObj = (LPUNKNOWN)This; 
	}
	else if(IsEqualIID(riid, &IID_IClassFactory))  /*IClassFactory*/
	{ *ppvObj = (LPCLASSFACTORY)This;
	}   

	if(*ppvObj) {
	  IUnknown_AddRef((IUnknown*)*ppvObj);  	
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IShellLinkW_CF_AddRef
 */
static ULONG WINAPI IShellLinkW_CF_AddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}
/******************************************************************************
 * IShellLinkW_CF_Release
 */
static ULONG WINAPI IShellLinkW_CF_Release(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount--;
	if (!--(This->ref)) 
	{ TRACE(shell,"-- destroying IClassFactory(%p)\n",This);
		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
	return This->ref;
}
/******************************************************************************
 * IShellLinkW_CF_CreateInstance
 */
static HRESULT WINAPI IShellLinkW_CF_CreateInstance(
  LPCLASSFACTORY iface, LPUNKNOWN pUnknown, REFIID riid, LPVOID *ppObject)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	IUnknown *pObj = NULL;
	HRESULT hres;
	char	xriid[50];

	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"%p->(%p,\n\tIID:\t%s,%p)\n",This,pUnknown,xriid,ppObject);

	*ppObject = NULL;
		
	if(pUnknown)
	{ return(CLASS_E_NOAGGREGATION);
	}

	if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IShellLinkW))
	{ pObj = (IUnknown *)IShellLinkW_Constructor();
	} 
	else
	{ ERR(shell,"unknown IID requested\n\tIID:\t%s\n",xriid);
	  return(E_NOINTERFACE);
	}
	
	if (!pObj)
	{ return(E_OUTOFMEMORY);
	}
	 
	hres = pObj->lpvtbl->fnQueryInterface(pObj,riid, ppObject);
	pObj->lpvtbl->fnRelease(pObj);
	TRACE(shell,"-- Object created: (%p)->%p\n",This,*ppObject);

	return hres;
}
/******************************************************************************
 * IShellLinkW_CF_LockServer
 */

static HRESULT WINAPI IShellLinkW_CF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE(shell,"%p->(0x%x), not implemented\n",This, fLock);
	return E_NOTIMPL;
}

static ICOM_VTABLE(IClassFactory) slwcfvt = 
{
    IShellLinkW_CF_QueryInterface,
    IShellLinkW_CF_AddRef,
  IShellLinkW_CF_Release,
  IShellLinkW_CF_CreateInstance,
  IShellLinkW_CF_LockServer
};


/**************************************************************************
 * IShellLink Implementation 
 */

typedef struct 
{
	ICOM_VTABLE(IShellLinkW)*	lpvtbl;
	DWORD				ref;
	IPersistFileImpl*		lppf;

} IShellLinkWImpl;

static ICOM_VTABLE(IShellLinkW) slvtw;

/**************************************************************************
 *	  IShellLinkW_fnConstructor
 */
IShellLinkW * IShellLinkW_Constructor(void) 
{	IShellLinkWImpl* sl;

	sl = (IShellLinkWImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IShellLinkWImpl));
	sl->ref = 1;
	sl->lpvtbl = &slvtw;

	sl->lppf = IPersistFile_Constructor();

	TRACE(shell,"(%p)->()\n",sl);
	shell32_ObjCount++;
	return (IShellLinkW*)sl;
}

/**************************************************************************
 *  IShellLinkW_fnQueryInterface
 */
static HRESULT WINAPI IShellLinkW_fnQueryInterface(
  IShellLinkW * iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",This,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IShellLinkW))  /*IShellLinkW*/
	{    *ppvObj = (IShellLinkW *)This;
	}   
	else if(IsEqualIID(riid, &IID_IPersistFile))  /*IPersistFile*/
	{    *ppvObj = (IPersistFile *)This->lppf;
	}   

	if(*ppvObj)
	{ IShellLink_AddRef((IShellLinkW*)*ppvObj);      
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}

	TRACE(shell,"-- Interface: E_NOINTERFACE\n");

	return E_NOINTERFACE;
}  
/******************************************************************************
 * IShellLinkW_fnAddRef
 */
static ULONG WINAPI IShellLinkW_fnAddRef(IShellLinkW * iface)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}
/******************************************************************************
 * IShellLinkW_fnRelease
 */

static ULONG WINAPI IShellLinkW_fnRelease(IShellLinkW * iface)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount--;
	if (!--(This->ref)) 
	{ TRACE(shell,"-- destroying IShellLinkW(%p)\n",This);
	  IPersistFile_Release((IPersistFile*)This->lppf);	/* IPersistFile*/
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IShellLinkW_fnGetPath(IShellLinkW * iface, LPWSTR pszFile,INT cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(pfile=%p len=%u find_data=%p flags=%lu)\n",This, pszFile, cchMaxPath, pfd, fFlags);
	lstrcpynAtoW(pszFile,"c:\\foo.bar", cchMaxPath);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetIDList(IShellLinkW * iface, LPITEMIDLIST * ppidl)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(ppidl=%p)\n",This, ppidl);
	*ppidl = _ILCreateDesktop();
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetIDList(IShellLinkW * iface, LPCITEMIDLIST pidl)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(pidl=%p)\n",This, pidl);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetDescription(IShellLinkW * iface, LPWSTR pszName,INT cchMaxName)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(%p len=%u)\n",This, pszName, cchMaxName);
	lstrcpynAtoW(pszName,"Description, FIXME",cchMaxName);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetDescription(IShellLinkW * iface, LPCWSTR pszName)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(desc=%s)\n",This, debugstr_w(pszName));
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetWorkingDirectory(IShellLinkW * iface, LPWSTR pszDir,INT cchMaxPath)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->()\n",This);
	lstrcpynAtoW(pszDir,"c:\\", cchMaxPath);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetWorkingDirectory(IShellLinkW * iface, LPCWSTR pszDir)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(dir=%s)\n",This, debugstr_w(pszDir));
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetArguments(IShellLinkW * iface, LPWSTR pszArgs,INT cchMaxPath)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(%p len=%u)\n",This, pszArgs, cchMaxPath);
	lstrcpynAtoW(pszArgs, "", cchMaxPath);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetArguments(IShellLinkW * iface, LPCWSTR pszArgs)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(args=%s)\n",This, debugstr_w(pszArgs));
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetHotkey(IShellLinkW * iface, WORD *pwHotkey)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(%p)\n",This, pwHotkey);
	*pwHotkey=0x0;
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetHotkey(IShellLinkW * iface, WORD wHotkey)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(hotkey=%x)\n",This, wHotkey);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetShowCmd(IShellLinkW * iface, INT *piShowCmd)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(%p)\n",This, piShowCmd);
	*piShowCmd=0;
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetShowCmd(IShellLinkW * iface, INT iShowCmd)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(showcmd=%x)\n",This, iShowCmd);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetIconLocation(IShellLinkW * iface, LPWSTR pszIconPath,INT cchIconPath,INT *piIcon)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(%p len=%u iicon=%p)\n",This, pszIconPath, cchIconPath, piIcon);
	lstrcpynAtoW(pszIconPath,"shell32.dll",cchIconPath);
	*piIcon=1;
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetIconLocation(IShellLinkW * iface, LPCWSTR pszIconPath,INT iIcon)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(path=%s iicon=%u)\n",This, debugstr_w(pszIconPath), iIcon);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetRelativePath(IShellLinkW * iface, LPCWSTR pszPathRel, DWORD dwReserved)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(path=%s %lx)\n",This, debugstr_w(pszPathRel), dwReserved);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnResolve(IShellLinkW * iface, HWND hwnd, DWORD fFlags)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(hwnd=%x flags=%lx)\n",This, hwnd, fFlags);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetPath(IShellLinkW * iface, LPCWSTR pszFile)
{
	ICOM_THIS(IShellLinkWImpl, iface);
	
	FIXME(shell,"(%p)->(path=%s)\n",This, debugstr_w(pszFile));
	return NOERROR;
}

/**************************************************************************
* IShellLinkW Implementation
*/

static ICOM_VTABLE(IShellLinkW) slvtw = 
{	IShellLinkW_fnQueryInterface,
	IShellLinkW_fnAddRef,
	IShellLinkW_fnRelease,
	IShellLinkW_fnGetPath,
	IShellLinkW_fnGetIDList,
	IShellLinkW_fnSetIDList,
	IShellLinkW_fnGetDescription,
	IShellLinkW_fnSetDescription,
	IShellLinkW_fnGetWorkingDirectory,
	IShellLinkW_fnSetWorkingDirectory,
	IShellLinkW_fnGetArguments,
	IShellLinkW_fnSetArguments,
	IShellLinkW_fnGetHotkey,
	IShellLinkW_fnSetHotkey,
	IShellLinkW_fnGetShowCmd,
	IShellLinkW_fnSetShowCmd,
	IShellLinkW_fnGetIconLocation,
	IShellLinkW_fnSetIconLocation,
	IShellLinkW_fnSetRelativePath,
	IShellLinkW_fnResolve,
	IShellLinkW_fnSetPath
};

