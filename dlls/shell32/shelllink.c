/*
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
 *
 */

#include <string.h>
#include "debugtools.h"
#include "winerror.h"

#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "wine/obj_shelllink.h"
#include "wine/undocshell.h"

#include "heap.h"
#include "winnls.h"
#include "pidl.h"
#include "shell32_main.h"
#include "shlguid.h"

DEFAULT_DEBUG_CHANNEL(shell);

/* link file formats */

#include "pshpack1.h"

/* flag1: lnk elements: simple link has 0x0B */
#define	WORKDIR		0x10
#define	ARGUMENT	0x20
#define	ICON		0x40
#define UNC		0x80

/* fStartup */
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
	DWORD	fStartup;	/* 0x3c startup type */
	DWORD	wHotKey;	/* 0x40 hotkey */
	DWORD	Unknown5;	/* 0x44 */
	DWORD	Unknown6;	/* 0x48 */
	USHORT	PidlSize;	/* 0x4c */
	ITEMIDLIST Pidl;	/* 0x4e */
} LINK_HEADER, * PLINK_HEADER;

#define LINK_HEADER_SIZE (sizeof(LINK_HEADER)-sizeof(ITEMIDLIST))

#include "poppack.h"

static ICOM_VTABLE(IShellLinkA)		slvt;
static ICOM_VTABLE(IShellLinkW)		slvtw;
static ICOM_VTABLE(IPersistFile)	pfvt;
static ICOM_VTABLE(IPersistStream)	psvt;

/* IShellLink Implementation */

typedef struct
{
	ICOM_VFIELD(IShellLinkA);
	DWORD				ref;

	ICOM_VTABLE(IShellLinkW)*	lpvtblw;
	ICOM_VTABLE(IPersistFile)*	lpvtblPersistFile;
	ICOM_VTABLE(IPersistStream)*	lpvtblPersistStream;
	
	/* internal stream of the IPersistFile interface */
	IStream*			lpFileStream;
	
	/* data structures according to the informations in the lnk */
	LPSTR		sPath;
	LPITEMIDLIST	pPidl;
	WORD		wHotKey;
	SYSTEMTIME	time1;
	SYSTEMTIME	time2;
	SYSTEMTIME	time3;

} IShellLinkImpl;

#define _IShellLinkW_Offset ((int)(&(((IShellLinkImpl*)0)->lpvtblw)))
#define _ICOM_THIS_From_IShellLinkW(class, name) class* This = (class*)(((char*)name)-_IShellLinkW_Offset);

#define _IPersistFile_Offset ((int)(&(((IShellLinkImpl*)0)->lpvtblPersistFile)))
#define _ICOM_THIS_From_IPersistFile(class, name) class* This = (class*)(((char*)name)-_IPersistFile_Offset);

#define _IPersistStream_Offset ((int)(&(((IShellLinkImpl*)0)->lpvtblPersistStream)))
#define _ICOM_THIS_From_IPersistStream(class, name) class* This = (class*)(((char*)name)-_IPersistStream_Offset);
#define _IPersistStream_From_ICOM_THIS(class, name) class* StreamThis = (class*)(((char*)name)+_IPersistStream_Offset);

/**************************************************************************
 *  IPersistFile_QueryInterface
 */
static HRESULT WINAPI IPersistFile_fnQueryInterface(
	IPersistFile* iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface)

	TRACE("(%p)\n",This);

	return IShellLinkA_QueryInterface((IShellLinkA*)This, riid, ppvObj);
}

/******************************************************************************
 * IPersistFile_AddRef
 */
static ULONG WINAPI IPersistFile_fnAddRef(IPersistFile* iface)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface)

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellLinkA_AddRef((IShellLinkA*)This);
}
/******************************************************************************
 * IPersistFile_Release
 */
static ULONG WINAPI IPersistFile_fnRelease(IPersistFile* iface)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface)

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellLinkA_Release((IShellLinkA*)This);
}

static HRESULT WINAPI IPersistFile_fnGetClassID(IPersistFile* iface, CLSID *pClassID)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface)
	FIXME("(%p)\n",This);
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_fnIsDirty(IPersistFile* iface)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface)
	FIXME("(%p)\n",This);
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_fnLoad(IPersistFile* iface, LPCOLESTR pszFileName, DWORD dwMode)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface)
	_IPersistStream_From_ICOM_THIS(IPersistStream, This)

	LPSTR		sFile = HEAP_strdupWtoA ( GetProcessHeap(), 0, pszFileName);
	HRESULT		hRet = E_FAIL;

	TRACE("(%p, %s)\n",This, sFile);
	

	if (This->lpFileStream)
	  IStream_Release(This->lpFileStream);
	
	if SUCCEEDED(CreateStreamOnFile(sFile, &(This->lpFileStream)))
	{
	  if SUCCEEDED (IPersistStream_Load(StreamThis, This->lpFileStream))
	  {
	    return NOERROR;
	  }
	}
	
	return hRet;
}

static HRESULT WINAPI IPersistFile_fnSave(IPersistFile* iface, LPCOLESTR pszFileName, BOOL fRemember)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);
	FIXME("(%p)->(%s)\n",This,debugstr_w(pszFileName));
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_fnSaveCompleted(IPersistFile* iface, LPCOLESTR pszFileName)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);
	FIXME("(%p)->(%s)\n",This,debugstr_w(pszFileName));
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_fnGetCurFile(IPersistFile* iface, LPOLESTR *ppszFileName)
{
	_ICOM_THIS_From_IPersistFile(IShellLinkImpl, iface);
	FIXME("(%p)\n",This);
	return NOERROR;
}

static ICOM_VTABLE(IPersistFile) pfvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

/************************************************************************
 * IPersistStream_QueryInterface
 */
static HRESULT WINAPI IPersistStream_fnQueryInterface(
	IPersistStream* iface,
	REFIID     riid,
	VOID**     ppvoid)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

	TRACE("(%p)\n",This);

	return IShellLinkA_QueryInterface((IShellLinkA*)This, riid, ppvoid);
}

/************************************************************************
 * IPersistStream_Release
 */
static ULONG WINAPI IPersistStream_fnRelease(
	IPersistStream* iface)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

	TRACE("(%p)\n",This);

	return IShellLinkA_Release((IShellLinkA*)This);
}

/************************************************************************
 * IPersistStream_AddRef
 */
static ULONG WINAPI IPersistStream_fnAddRef(
	IPersistStream* iface)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

	TRACE("(%p)\n",This);

	return IShellLinkA_AddRef((IShellLinkA*)This);
} 

/************************************************************************
 * IPersistStream_GetClassID
 *
 */
static HRESULT WINAPI IPersistStream_fnGetClassID(
	IPersistStream* iface,
	CLSID* pClassID)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

	TRACE("(%p)\n", This);

	if (pClassID==0)
	  return E_POINTER;

/*	memcpy(pClassID, &CLSID_???, sizeof(CLSID_???)); */

	return S_OK;
}

/************************************************************************
 * IPersistStream_IsDirty (IPersistStream)
 */
static HRESULT WINAPI IPersistStream_fnIsDirty(
	IPersistStream*  iface)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

	TRACE("(%p)\n", This);

	return S_OK;
}
/************************************************************************
 * IPersistStream_Load (IPersistStream)
 */

static HRESULT WINAPI IPersistStream_fnLoad(
	IPersistStream*  iface,
	IStream*         pLoadStream)
{
	PLINK_HEADER lpLinkHeader = HeapAlloc(GetProcessHeap(), 0, LINK_HEADER_SIZE);
	ULONG	dwBytesRead;
	DWORD	ret = E_FAIL;
	char	sTemp[MAX_PATH];
	
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);

	TRACE("(%p)(%p)\n", This, pLoadStream);

	if ( ! pLoadStream)
	{
	  return STG_E_INVALIDPOINTER;
	}
	
	IStream_AddRef (pLoadStream);
	if(lpLinkHeader)
	{
	  if (SUCCEEDED(IStream_Read(pLoadStream, lpLinkHeader, LINK_HEADER_SIZE, &dwBytesRead)))
	  {
	    if ((lpLinkHeader->MagicStr == 0x0000004CL) && IsEqualIID(&lpLinkHeader->MagicGuid, &CLSID_ShellLink))
	    {
	      lpLinkHeader = HeapReAlloc(GetProcessHeap(), 0, lpLinkHeader, LINK_HEADER_SIZE+lpLinkHeader->PidlSize);
	      if (lpLinkHeader)
	      {
	        if (SUCCEEDED(IStream_Read(pLoadStream, &(lpLinkHeader->Pidl), lpLinkHeader->PidlSize, &dwBytesRead)))
	        {
	          if (pcheck (&lpLinkHeader->Pidl))
	          {	
	            This->pPidl = ILClone (&lpLinkHeader->Pidl);

	            SHGetPathFromIDListA(&lpLinkHeader->Pidl, sTemp);
	            This->sPath = HEAP_strdupA ( GetProcessHeap(), 0, sTemp);
	          }
		  This->wHotKey = lpLinkHeader->wHotKey;
		  FileTimeToSystemTime (&lpLinkHeader->Time1, &This->time1);
		  FileTimeToSystemTime (&lpLinkHeader->Time2, &This->time2);
		  FileTimeToSystemTime (&lpLinkHeader->Time3, &This->time3);
#if 1
		  GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&This->time1, NULL, sTemp, 256);
		  TRACE("-- time1: %s\n", sTemp);
		  GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&This->time2, NULL, sTemp, 256);
		  TRACE("-- time1: %s\n", sTemp);
		  GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&This->time3, NULL, sTemp, 256);
		  TRACE("-- time1: %s\n", sTemp);
		  pdump (This->pPidl);
#endif		  
		  ret = S_OK;
	        }
	      }
	    }
	    else
	    {
	      WARN("stream contains no link!\n");
	    }
	  }
	}

	IStream_Release (pLoadStream);

	pdump(This->pPidl);
	
	HeapFree(GetProcessHeap(), 0, lpLinkHeader);

	return ret;
}

/************************************************************************
 * IPersistStream_Save (IPersistStream)
 */
static HRESULT WINAPI IPersistStream_fnSave(
	IPersistStream*  iface,
	IStream*         pOutStream,
	BOOL             fClearDirty)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);
	
	TRACE("(%p) %p %x\n", This, pOutStream, fClearDirty);

	return E_NOTIMPL;
}

/************************************************************************
 * IPersistStream_GetSizeMax (IPersistStream)
 */
static HRESULT WINAPI IPersistStream_fnGetSizeMax(
	IPersistStream*  iface,
	ULARGE_INTEGER*  pcbSize)
{
	_ICOM_THIS_From_IPersistStream(IShellLinkImpl, iface);
	
	TRACE("(%p)\n", This);

	return E_NOTIMPL;
}

static ICOM_VTABLE(IPersistStream) psvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IPersistStream_fnQueryInterface,
	IPersistStream_fnAddRef,
	IPersistStream_fnRelease,
	IPersistStream_fnGetClassID,
	IPersistStream_fnIsDirty,
	IPersistStream_fnLoad,
	IPersistStream_fnSave,
	IPersistStream_fnGetSizeMax
};

/**************************************************************************
 *	  IShellLink_Constructor
 */
IShellLinkA * IShellLink_Constructor(BOOL bUnicode) 
{	IShellLinkImpl * sl;

	sl = (IShellLinkImpl *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IShellLinkImpl));
	sl->ref = 1;
	ICOM_VTBL(sl) = &slvt;
	sl->lpvtblw = &slvtw;
	sl->lpvtblPersistFile = &pfvt;
	sl->lpvtblPersistStream = &psvt;
	
	TRACE("(%p)->()\n",sl);
	shell32_ObjCount++;
	return bUnicode ? (IShellLinkA *) &(sl->lpvtblw) : (IShellLinkA *)sl;
}

/**************************************************************************
 *  IShellLinkA_QueryInterface
 */
static HRESULT WINAPI IShellLinkA_fnQueryInterface( IShellLinkA * iface, REFIID riid,  LPVOID *ppvObj)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE("(%p)->(\n\tIID:\t%s)\n",This,debugstr_guid(riid));

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown) ||
	   IsEqualIID(riid, &IID_IShellLinkA))
	{
	  *ppvObj = This;
	}   
	else if(IsEqualIID(riid, &IID_IShellLinkW))
	{
	  *ppvObj = (IShellLinkW *)&(This->lpvtblw);
	}   
	else if(IsEqualIID(riid, &IID_IPersistFile))
	{
	  *ppvObj = (IPersistFile *)&(This->lpvtblPersistFile);
	}
	else if(IsEqualIID(riid, &IID_IPersistStream))
	{
	  *ppvObj = (IPersistStream *)&(This->lpvtblPersistStream);
	}   

	if(*ppvObj)
	{
	  IUnknown_AddRef((IUnknown*)(*ppvObj));
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IShellLinkA_AddRef
 */
static ULONG WINAPI IShellLinkA_fnAddRef(IShellLinkA * iface)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}
/******************************************************************************
 *	IShellLinkA_Release
 */
static ULONG WINAPI IShellLinkA_fnRelease(IShellLinkA * iface)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount--;
	if (!--(This->ref)) 
	{ TRACE("-- destroying IShellLink(%p)\n",This);

	  if (This->sPath)
	    HeapFree(GetProcessHeap(),0,This->sPath);

	  if (This->pPidl)
	    SHFree(This->pPidl);

	  if (This->lpFileStream)
	    IStream_Release(This->lpFileStream);

	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IShellLinkA_fnGetPath(IShellLinkA * iface, LPSTR pszFile,INT cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE("(%p)->(pfile=%p len=%u find_data=%p flags=%lu)(%s)\n",This, pszFile, cchMaxPath, pfd, fFlags, debugstr_a(This->sPath));

	if (This->sPath)
	  lstrcpynA(pszFile,This->sPath, cchMaxPath);
	else
	  return E_FAIL;

	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnGetIDList(IShellLinkA * iface, LPITEMIDLIST * ppidl)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE("(%p)->(ppidl=%p)\n",This, ppidl);

	*ppidl = ILClone(This->pPidl);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnSetIDList(IShellLinkA * iface, LPCITEMIDLIST pidl)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE("(%p)->(pidl=%p)\n",This, pidl);

	if (This->pPidl)
	    SHFree(This->pPidl);
	This->pPidl = ILClone (pidl);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnGetDescription(IShellLinkA * iface, LPSTR pszName,INT cchMaxName)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(%p len=%u)\n",This, pszName, cchMaxName);
	lstrcpynA(pszName,"Description, FIXME",cchMaxName);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnSetDescription(IShellLinkA * iface, LPCSTR pszName)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(desc=%s)\n",This, pszName);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnGetWorkingDirectory(IShellLinkA * iface, LPSTR pszDir,INT cchMaxPath)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->()\n",This);
	lstrcpynA(pszDir,"c:\\", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnSetWorkingDirectory(IShellLinkA * iface, LPCSTR pszDir)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(dir=%s)\n",This, pszDir);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnGetArguments(IShellLinkA * iface, LPSTR pszArgs,INT cchMaxPath)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(%p len=%u)\n",This, pszArgs, cchMaxPath);
	lstrcpynA(pszArgs, "", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnSetArguments(IShellLinkA * iface, LPCSTR pszArgs)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(args=%s)\n",This, pszArgs);

	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnGetHotkey(IShellLinkA * iface, WORD *pwHotkey)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE("(%p)->(%p)(0x%08x)\n",This, pwHotkey, This->wHotKey);

	*pwHotkey = This->wHotKey;

	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnSetHotkey(IShellLinkA * iface, WORD wHotkey)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	TRACE("(%p)->(hotkey=%x)\n",This, wHotkey);
	
	This->wHotKey = wHotkey;

	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnGetShowCmd(IShellLinkA * iface, INT *piShowCmd)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(%p)\n",This, piShowCmd);
	*piShowCmd=0;
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnSetShowCmd(IShellLinkA * iface, INT iShowCmd)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(showcmd=%x)\n",This, iShowCmd);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnGetIconLocation(IShellLinkA * iface, LPSTR pszIconPath,INT cchIconPath,INT *piIcon)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(%p len=%u iicon=%p)\n",This, pszIconPath, cchIconPath, piIcon);
	lstrcpynA(pszIconPath,"shell32.dll",cchIconPath);
	*piIcon=1;
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnSetIconLocation(IShellLinkA * iface, LPCSTR pszIconPath,INT iIcon)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(path=%s iicon=%u)\n",This, pszIconPath, iIcon);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnSetRelativePath(IShellLinkA * iface, LPCSTR pszPathRel, DWORD dwReserved)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(path=%s %lx)\n",This, pszPathRel, dwReserved);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnResolve(IShellLinkA * iface, HWND hwnd, DWORD fFlags)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(hwnd=%x flags=%lx)\n",This, hwnd, fFlags);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkA_fnSetPath(IShellLinkA * iface, LPCSTR pszFile)
{
	ICOM_THIS(IShellLinkImpl, iface);
	
	FIXME("(%p)->(path=%s)\n",This, pszFile);
	return NOERROR;
}

/**************************************************************************
* IShellLink Implementation
*/

static ICOM_VTABLE(IShellLinkA) slvt = 
{	
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IShellLinkA_fnQueryInterface,
	IShellLinkA_fnAddRef,
	IShellLinkA_fnRelease,
	IShellLinkA_fnGetPath,
	IShellLinkA_fnGetIDList,
	IShellLinkA_fnSetIDList,
	IShellLinkA_fnGetDescription,
	IShellLinkA_fnSetDescription,
	IShellLinkA_fnGetWorkingDirectory,
	IShellLinkA_fnSetWorkingDirectory,
	IShellLinkA_fnGetArguments,
	IShellLinkA_fnSetArguments,
	IShellLinkA_fnGetHotkey,
	IShellLinkA_fnSetHotkey,
	IShellLinkA_fnGetShowCmd,
	IShellLinkA_fnSetShowCmd,
	IShellLinkA_fnGetIconLocation,
	IShellLinkA_fnSetIconLocation,
	IShellLinkA_fnSetRelativePath,
	IShellLinkA_fnResolve,
	IShellLinkA_fnSetPath
};


/**************************************************************************
 *  IShellLinkW_fnQueryInterface
 */
static HRESULT WINAPI IShellLinkW_fnQueryInterface(
  IShellLinkW * iface, REFIID riid, LPVOID *ppvObj)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	return IShellLinkA_QueryInterface((IShellLinkA*)This, riid, ppvObj);
}

/******************************************************************************
 * IShellLinkW_fnAddRef
 */
static ULONG WINAPI IShellLinkW_fnAddRef(IShellLinkW * iface)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellLinkA_AddRef((IShellLinkA*)This);
}
/******************************************************************************
 * IShellLinkW_fnRelease
 */

static ULONG WINAPI IShellLinkW_fnRelease(IShellLinkW * iface)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellLinkA_Release((IShellLinkA*)This);
}

static HRESULT WINAPI IShellLinkW_fnGetPath(IShellLinkW * iface, LPWSTR pszFile,INT cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(pfile=%p len=%u find_data=%p flags=%lu)\n",This, pszFile, cchMaxPath, pfd, fFlags);
	lstrcpynAtoW(pszFile,"c:\\foo.bar", cchMaxPath);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetIDList(IShellLinkW * iface, LPITEMIDLIST * ppidl)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(ppidl=%p)\n",This, ppidl);
	*ppidl = _ILCreateDesktop();
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetIDList(IShellLinkW * iface, LPCITEMIDLIST pidl)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(pidl=%p)\n",This, pidl);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetDescription(IShellLinkW * iface, LPWSTR pszName,INT cchMaxName)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(%p len=%u)\n",This, pszName, cchMaxName);
	lstrcpynAtoW(pszName,"Description, FIXME",cchMaxName);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetDescription(IShellLinkW * iface, LPCWSTR pszName)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(desc=%s)\n",This, debugstr_w(pszName));
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetWorkingDirectory(IShellLinkW * iface, LPWSTR pszDir,INT cchMaxPath)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->()\n",This);
	lstrcpynAtoW(pszDir,"c:\\", cchMaxPath);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetWorkingDirectory(IShellLinkW * iface, LPCWSTR pszDir)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(dir=%s)\n",This, debugstr_w(pszDir));
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetArguments(IShellLinkW * iface, LPWSTR pszArgs,INT cchMaxPath)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(%p len=%u)\n",This, pszArgs, cchMaxPath);
	lstrcpynAtoW(pszArgs, "", cchMaxPath);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetArguments(IShellLinkW * iface, LPCWSTR pszArgs)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(args=%s)\n",This, debugstr_w(pszArgs));
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetHotkey(IShellLinkW * iface, WORD *pwHotkey)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(%p)\n",This, pwHotkey);
	*pwHotkey=0x0;
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetHotkey(IShellLinkW * iface, WORD wHotkey)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(hotkey=%x)\n",This, wHotkey);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetShowCmd(IShellLinkW * iface, INT *piShowCmd)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(%p)\n",This, piShowCmd);
	*piShowCmd=0;
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetShowCmd(IShellLinkW * iface, INT iShowCmd)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(showcmd=%x)\n",This, iShowCmd);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnGetIconLocation(IShellLinkW * iface, LPWSTR pszIconPath,INT cchIconPath,INT *piIcon)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(%p len=%u iicon=%p)\n",This, pszIconPath, cchIconPath, piIcon);
	lstrcpynAtoW(pszIconPath,"shell32.dll",cchIconPath);
	*piIcon=1;
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetIconLocation(IShellLinkW * iface, LPCWSTR pszIconPath,INT iIcon)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(path=%s iicon=%u)\n",This, debugstr_w(pszIconPath), iIcon);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetRelativePath(IShellLinkW * iface, LPCWSTR pszPathRel, DWORD dwReserved)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(path=%s %lx)\n",This, debugstr_w(pszPathRel), dwReserved);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnResolve(IShellLinkW * iface, HWND hwnd, DWORD fFlags)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(hwnd=%x flags=%lx)\n",This, hwnd, fFlags);
	return NOERROR;
}

static HRESULT WINAPI IShellLinkW_fnSetPath(IShellLinkW * iface, LPCWSTR pszFile)
{
	_ICOM_THIS_From_IShellLinkW(IShellLinkImpl, iface);
	
	FIXME("(%p)->(path=%s)\n",This, debugstr_w(pszFile));
	return NOERROR;
}

/**************************************************************************
* IShellLinkW Implementation
*/

static ICOM_VTABLE(IShellLinkW) slvtw = 
{	
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IShellLinkW_fnQueryInterface,
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

