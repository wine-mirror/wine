/*
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
 *
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ole.h"
#include "ole2.h"
#include "debug.h"
#include "compobj.h"
#include "interfaces.h"
#include "shlobj.h"
#include "shell.h"
#include "winerror.h"
#include "winnls.h"
#include "winproc.h"
#include "commctrl.h"
#include "pidl.h"

#include "shell32_main.h"

/* IPersistFile Implementation */
static HRESULT WINAPI IPersistFile_QueryInterface(LPPERSISTFILE this, REFIID riid, LPVOID * ppvObj);
static ULONG WINAPI IPersistFile_AddRef(LPPERSISTFILE this);
static ULONG WINAPI IPersistFile_Release(LPPERSISTFILE this);

static HRESULT WINAPI IPersistFile_GetClassID (LPPERSISTFILE this, CLSID *pClassID);
static HRESULT WINAPI IPersistFile_IsDirty (LPPERSISTFILE this);
static HRESULT WINAPI IPersistFile_Load (LPPERSISTFILE this, LPCOLESTR32 pszFileName, DWORD dwMode);
static HRESULT WINAPI IPersistFile_Save (LPPERSISTFILE this, LPCOLESTR32 pszFileName, BOOL32 fRemember);
static HRESULT WINAPI IPersistFile_SaveCompleted (LPPERSISTFILE this, LPCOLESTR32 pszFileName);
static HRESULT WINAPI IPersistFile_GetCurFile (LPPERSISTFILE this, LPOLESTR32 *ppszFileName);

static struct IPersistFile_VTable pfvt = 
{	IPersistFile_QueryInterface,
	IPersistFile_AddRef,
	IPersistFile_Release,
	IPersistFile_GetClassID,
	IPersistFile_IsDirty,
	IPersistFile_Load,
	IPersistFile_Save,
	IPersistFile_SaveCompleted,
	IPersistFile_GetCurFile
};
/**************************************************************************
 *	  IPersistFile_Constructor
 */
LPPERSISTFILE IPersistFile_Constructor(void) 
{	LPPERSISTFILE sl;

	sl = (LPPERSISTFILE)HeapAlloc(GetProcessHeap(),0,sizeof(IPersistFile));
	sl->ref = 1;
	sl->lpvtbl = &pfvt;

	TRACE(shell,"(%p)->()\n",sl);
	return sl;
}

/**************************************************************************
 *  IPersistFile_QueryInterface
 */
static HRESULT WINAPI IPersistFile_QueryInterface(
  LPPERSISTFILE this, REFIID riid, LPVOID *ppvObj)
{	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",this,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = this; 
	}
	else if(IsEqualIID(riid, &IID_IPersistFile))  /*IPersistFile*/
	{    *ppvObj = (LPPERSISTFILE)this;
	}   

	if(*ppvObj)
	{ (*(LPPERSISTFILE*)ppvObj)->lpvtbl->fnAddRef(this);      
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IPersistFile_AddRef
 */
static ULONG WINAPI IPersistFile_AddRef(LPPERSISTFILE this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	return ++(this->ref);
}
/******************************************************************************
 * IPersistFile_Release
 */
static ULONG WINAPI IPersistFile_Release(LPPERSISTFILE this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	if (!--(this->ref)) 
	{ TRACE(shell,"-- destroying IPersistFile(%p)\n",this);
	  HeapFree(GetProcessHeap(),0,this);
	  return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IPersistFile_GetClassID (LPPERSISTFILE this, CLSID *pClassID)
{	FIXME(shell,"(%p)\n",this);
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_IsDirty (LPPERSISTFILE this)
{	FIXME(shell,"(%p)\n",this);
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_Load (LPPERSISTFILE this, LPCOLESTR32 pszFileName, DWORD dwMode)
{	FIXME(shell,"(%p)\n",this);
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_Save (LPPERSISTFILE this, LPCOLESTR32 pszFileName, BOOL32 fRemember)
{	FIXME(shell,"(%p)\n",this);
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_SaveCompleted (LPPERSISTFILE this, LPCOLESTR32 pszFileName)
{	FIXME(shell,"(%p)\n",this);
	return NOERROR;
}
static HRESULT WINAPI IPersistFile_GetCurFile (LPPERSISTFILE this, LPOLESTR32 *ppszFileName)
{	FIXME(shell,"(%p)\n",this);
	return NOERROR;
}


/**************************************************************************
*  IClassFactory Implementation
*/
static HRESULT WINAPI IShellLink_CF_QueryInterface(LPCLASSFACTORY,REFIID,LPVOID*);
static ULONG WINAPI IShellLink_CF_AddRef(LPCLASSFACTORY);
static ULONG WINAPI IShellLink_CF_Release(LPCLASSFACTORY);
static HRESULT WINAPI IShellLink_CF_CreateInstance(LPCLASSFACTORY, LPUNKNOWN, REFIID, LPVOID *);
static HRESULT WINAPI IShellLink_CF_LockServer(LPCLASSFACTORY, BOOL32);
/**************************************************************************
 *  IShellLink_CF_VTable
 */
static IClassFactory_VTable slcfvt = 
{	IShellLink_CF_QueryInterface,
	IShellLink_CF_AddRef,
	IShellLink_CF_Release,
	IShellLink_CF_CreateInstance,
	IShellLink_CF_LockServer
};

/**************************************************************************
 *  IShellLink_CF_Constructor
 */

LPCLASSFACTORY IShellLink_CF_Constructor(void)
{	LPCLASSFACTORY	lpclf;

	lpclf= (LPCLASSFACTORY)HeapAlloc(GetProcessHeap(),0,sizeof(IClassFactory));
	lpclf->ref = 1;
	lpclf->lpvtbl = &slcfvt;
	TRACE(shell,"(%p)->()\n",lpclf);
	return lpclf;
}
/**************************************************************************
 *  IShellLink_CF_QueryInterface
 */
static HRESULT WINAPI IShellLink_CF_QueryInterface(
  LPCLASSFACTORY this, REFIID riid, LPVOID *ppvObj)
{	char	xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",this,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          	/*IUnknown*/
	{ *ppvObj = (LPUNKNOWN)this; 
	}
	else if(IsEqualIID(riid, &IID_IClassFactory))  /*IClassFactory*/
	{ *ppvObj = (LPCLASSFACTORY)this;
	}   

	if(*ppvObj)
	{ (*(LPCLASSFACTORY*)ppvObj)->lpvtbl->fnAddRef(this);  	
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IShellLink_CF_AddRef
 */
static ULONG WINAPI IShellLink_CF_AddRef(LPCLASSFACTORY this)
{ TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	return ++(this->ref);
}
/******************************************************************************
 * IShellLink_CF_Release
 */
static ULONG WINAPI IShellLink_CF_Release(LPCLASSFACTORY this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	if (!--(this->ref)) 
	{ TRACE(shell,"-- destroying IClassFactory(%p)\n",this);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}
/******************************************************************************
 * IShellLink_CF_CreateInstance
 */
static HRESULT WINAPI IShellLink_CF_CreateInstance(
  LPCLASSFACTORY this, LPUNKNOWN pUnknown, REFIID riid, LPVOID *ppObject)
{	IUnknown *pObj = NULL;
	HRESULT hres;
	char	xriid[50];

	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"%p->(%p,\n\tIID:\t%s,%p)\n",this,pUnknown,xriid,ppObject);

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
	 
	hres = pObj->lpvtbl->fnQueryInterface(pObj,riid, ppObject);
	pObj->lpvtbl->fnRelease(pObj);
	TRACE(shell,"-- Object created: (%p)->%p\n",this,*ppObject);

	return hres;
}
/******************************************************************************
 * IShellLink_CF_LockServer
 */
static HRESULT WINAPI IShellLink_CF_LockServer(LPCLASSFACTORY this, BOOL32 fLock)
{	TRACE(shell,"%p->(0x%x), not implemented\n",this, fLock);
	return E_NOTIMPL;
}

/* IShellLink Implementation */
static HRESULT WINAPI IShellLink_QueryInterface(LPSHELLLINK,REFIID,LPVOID*);
static ULONG WINAPI IShellLink_AddRef(LPSHELLLINK);
static ULONG WINAPI IShellLink_Release(LPSHELLLINK);
static HRESULT WINAPI IShellLink_GetPath(LPSHELLLINK, LPSTR,INT32, WIN32_FIND_DATA32A *, DWORD);
static HRESULT WINAPI IShellLink_GetIDList(LPSHELLLINK, LPITEMIDLIST *);
static HRESULT WINAPI IShellLink_SetIDList(LPSHELLLINK, LPCITEMIDLIST);
static HRESULT WINAPI IShellLink_GetDescription(LPSHELLLINK, LPSTR,INT32);
static HRESULT WINAPI IShellLink_SetDescription(LPSHELLLINK, LPCSTR);
static HRESULT WINAPI IShellLink_GetWorkingDirectory(LPSHELLLINK, LPSTR,INT32);
static HRESULT WINAPI IShellLink_SetWorkingDirectory(LPSHELLLINK, LPCSTR);
static HRESULT WINAPI IShellLink_GetArguments(LPSHELLLINK, LPSTR,INT32);
static HRESULT WINAPI IShellLink_SetArguments(LPSHELLLINK, LPCSTR);
static HRESULT WINAPI IShellLink_GetHotkey(LPSHELLLINK, WORD *);
static HRESULT WINAPI IShellLink_SetHotkey(LPSHELLLINK, WORD);
static HRESULT WINAPI IShellLink_GetShowCmd(LPSHELLLINK, INT32 *);
static HRESULT WINAPI IShellLink_SetShowCmd(LPSHELLLINK, INT32);
static HRESULT WINAPI IShellLink_GetIconLocation(LPSHELLLINK, LPSTR,INT32,INT32 *);
static HRESULT WINAPI IShellLink_SetIconLocation(LPSHELLLINK, LPCSTR,INT32);
static HRESULT WINAPI IShellLink_SetRelativePath(LPSHELLLINK, LPCSTR, DWORD);
static HRESULT WINAPI IShellLink_Resolve(LPSHELLLINK, HWND32, DWORD);
static HRESULT WINAPI IShellLink_SetPath(LPSHELLLINK, LPCSTR);

/**************************************************************************
* IShellLink Implementation
*/

static struct IShellLink_VTable slvt = 
{	IShellLink_QueryInterface,
	IShellLink_AddRef,
	IShellLink_Release,
	IShellLink_GetPath,
	IShellLink_GetIDList,
	IShellLink_SetIDList,
	IShellLink_GetDescription,
	IShellLink_SetDescription,
	IShellLink_GetWorkingDirectory,
	IShellLink_SetWorkingDirectory,
	IShellLink_GetArguments,
	IShellLink_SetArguments,
	IShellLink_GetHotkey,
	IShellLink_SetHotkey,
	IShellLink_GetShowCmd,
	IShellLink_SetShowCmd,
	IShellLink_GetIconLocation,
	IShellLink_SetIconLocation,
	IShellLink_SetRelativePath,
	IShellLink_Resolve,
	IShellLink_SetPath
};

/**************************************************************************
 *	  IShellLink_Constructor
 */
LPSHELLLINK IShellLink_Constructor(void) 
{	LPSHELLLINK sl;

	sl = (LPSHELLLINK)HeapAlloc(GetProcessHeap(),0,sizeof(IShellLink));
	sl->ref = 1;
	sl->lpvtbl = &slvt;

	sl->lppf = IPersistFile_Constructor();

	TRACE(shell,"(%p)->()\n",sl);
	return sl;
}

/**************************************************************************
 *  IShellLink::QueryInterface
 */
static HRESULT WINAPI IShellLink_QueryInterface(
  LPSHELLLINK this, REFIID riid, LPVOID *ppvObj)
{	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",this,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = this; 
	}
	else if(IsEqualIID(riid, &IID_IShellLink))  /*IShellLink*/
	{    *ppvObj = (LPSHELLLINK)this;
	}   
	else if(IsEqualIID(riid, &IID_IPersistFile))  /*IPersistFile*/
	{    *ppvObj = (LPPERSISTFILE)this->lppf;
	}   

	if(*ppvObj)
	{ (*(LPSHELLLINK*)ppvObj)->lpvtbl->fnAddRef(this);      
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IShellLink_AddRef
 */
static ULONG WINAPI IShellLink_AddRef(LPSHELLLINK this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	return ++(this->ref);
}
/******************************************************************************
 *	IShellLink_Release
 */
static ULONG WINAPI IShellLink_Release(LPSHELLLINK this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	if (!--(this->ref)) 
	{ TRACE(shell,"-- destroying IShellLink(%p)\n",this);
	  this->lppf->lpvtbl->fnRelease(this->lppf);	/* IPersistFile*/
	  HeapFree(GetProcessHeap(),0,this);
	  return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IShellLink_GetPath(LPSHELLLINK this, LPSTR pszFile,INT32 cchMaxPath, WIN32_FIND_DATA32A *pfd, DWORD fFlags)
{	FIXME(shell,"(%p)->(pfile=%p len=%u find_data=%p flags=%lu)\n",this, pszFile, cchMaxPath, pfd, fFlags);
	strncpy(pszFile,"c:\\foo.bar", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetIDList(LPSHELLLINK this, LPITEMIDLIST * ppidl)
{	FIXME(shell,"(%p)->(ppidl=%p)\n",this, ppidl);
	*ppidl = _ILCreateDesktop();
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetIDList(LPSHELLLINK this, LPCITEMIDLIST pidl)
{	FIXME(shell,"(%p)->(pidl=%p)\n",this, pidl);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetDescription(LPSHELLLINK this, LPSTR pszName,INT32 cchMaxName)
{	FIXME(shell,"(%p)->(%p len=%u)\n",this, pszName, cchMaxName);
	strncpy(pszName,"Description, FIXME",cchMaxName);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetDescription(LPSHELLLINK this, LPCSTR pszName)
{	FIXME(shell,"(%p)->(desc=%s)\n",this, pszName);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetWorkingDirectory(LPSHELLLINK this, LPSTR pszDir,INT32 cchMaxPath)
{	FIXME(shell,"(%p)->()\n",this);
	strncpy(pszDir,"c:\\", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetWorkingDirectory(LPSHELLLINK this, LPCSTR pszDir)
{	FIXME(shell,"(%p)->(dir=%s)\n",this, pszDir);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetArguments(LPSHELLLINK this, LPSTR pszArgs,INT32 cchMaxPath)
{	FIXME(shell,"(%p)->(%p len=%u)\n",this, pszArgs, cchMaxPath);
	strncpy(pszArgs, "", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetArguments(LPSHELLLINK this, LPCSTR pszArgs)
{	FIXME(shell,"(%p)->(args=%s)\n",this, pszArgs);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetHotkey(LPSHELLLINK this, WORD *pwHotkey)
{	FIXME(shell,"(%p)->(%p)\n",this, pwHotkey);
	*pwHotkey=0x0;
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetHotkey(LPSHELLLINK this, WORD wHotkey)
{	FIXME(shell,"(%p)->(hotkey=%x)\n",this, wHotkey);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetShowCmd(LPSHELLLINK this, INT32 *piShowCmd)
{	FIXME(shell,"(%p)->(%p)\n",this, piShowCmd);
	*piShowCmd=0;
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetShowCmd(LPSHELLLINK this, INT32 iShowCmd)
{	FIXME(shell,"(%p)->(showcmd=%x)\n",this, iShowCmd);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_GetIconLocation(LPSHELLLINK this, LPSTR pszIconPath,INT32 cchIconPath,INT32 *piIcon)
{	FIXME(shell,"(%p)->(%p len=%u iicon=%p)\n",this, pszIconPath, cchIconPath, piIcon);
	strncpy(pszIconPath,"shell32.dll",cchIconPath);
	*piIcon=1;
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetIconLocation(LPSHELLLINK this, LPCSTR pszIconPath,INT32 iIcon)
{	FIXME(shell,"(%p)->(path=%s iicon=%u)\n",this, pszIconPath, iIcon);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetRelativePath(LPSHELLLINK this, LPCSTR pszPathRel, DWORD dwReserved)
{	FIXME(shell,"(%p)->(path=%s %lx)\n",this, pszPathRel, dwReserved);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_Resolve(LPSHELLLINK this, HWND32 hwnd, DWORD fFlags)
{	FIXME(shell,"(%p)->(hwnd=%x flags=%lx)\n",this, hwnd, fFlags);
	return NOERROR;
}
static HRESULT WINAPI IShellLink_SetPath(LPSHELLLINK this, LPCSTR pszFile)
{	FIXME(shell,"(%p)->(path=%s)\n",this, pszFile);
	return NOERROR;
}

/**************************************************************************
*  IClassFactory for IShellLinkW 
*/
static HRESULT WINAPI IShellLinkW_CF_QueryInterface(LPCLASSFACTORY,REFIID,LPVOID*);
static ULONG WINAPI IShellLinkW_CF_AddRef(LPCLASSFACTORY);
static ULONG WINAPI IShellLinkW_CF_Release(LPCLASSFACTORY);
static HRESULT WINAPI IShellLinkW_CF_CreateInstance(LPCLASSFACTORY, LPUNKNOWN, REFIID, LPVOID *);
static HRESULT WINAPI IShellLinkW_CF_LockServer(LPCLASSFACTORY, BOOL32);
/**************************************************************************
 *  IShellLinkW_CF_VTable
 */
static IClassFactory_VTable slwcfvt = 
{	IShellLinkW_CF_QueryInterface,
	IShellLinkW_CF_AddRef,
	IShellLinkW_CF_Release,
	IShellLinkW_CF_CreateInstance,
	IShellLinkW_CF_LockServer
};

/**************************************************************************
 *  IShellLinkW_CF_Constructor
 */

LPCLASSFACTORY IShellLinkW_CF_Constructor(void)
{	LPCLASSFACTORY	lpclf;

	lpclf= (LPCLASSFACTORY)HeapAlloc(GetProcessHeap(),0,sizeof(IClassFactory));
	lpclf->ref = 1;
	lpclf->lpvtbl = &slwcfvt;
	TRACE(shell,"(%p)->()\n",lpclf);
	return lpclf;
}
/**************************************************************************
 *  IShellLinkW_CF_QueryInterface
 */
static HRESULT WINAPI IShellLinkW_CF_QueryInterface(
  LPCLASSFACTORY this, REFIID riid, LPVOID *ppvObj)
{	char	xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",this,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          	/*IUnknown*/
	{ *ppvObj = (LPUNKNOWN)this; 
	}
	else if(IsEqualIID(riid, &IID_IClassFactory))  /*IClassFactory*/
	{ *ppvObj = (LPCLASSFACTORY)this;
	}   

	if(*ppvObj)
	{ (*(LPCLASSFACTORY*)ppvObj)->lpvtbl->fnAddRef(this);  	
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IShellLinkW_CF_AddRef
 */
static ULONG WINAPI IShellLinkW_CF_AddRef(LPCLASSFACTORY this)
{ TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	return ++(this->ref);
}
/******************************************************************************
 * IShellLinkW_CF_Release
 */
static ULONG WINAPI IShellLinkW_CF_Release(LPCLASSFACTORY this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	if (!--(this->ref)) 
	{ TRACE(shell,"-- destroying IClassFactory(%p)\n",this);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}
/******************************************************************************
 * IShellLinkW_CF_CreateInstance
 */
static HRESULT WINAPI IShellLinkW_CF_CreateInstance(
  LPCLASSFACTORY this, LPUNKNOWN pUnknown, REFIID riid, LPVOID *ppObject)
{	IUnknown *pObj = NULL;
	HRESULT hres;
	char	xriid[50];

	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"%p->(%p,\n\tIID:\t%s,%p)\n",this,pUnknown,xriid,ppObject);

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
	TRACE(shell,"-- Object created: (%p)->%p\n",this,*ppObject);

	return hres;
}
/******************************************************************************
 * IShellLinkW_CF_LockServer
 */

static HRESULT WINAPI IShellLinkW_CF_LockServer(LPCLASSFACTORY this, BOOL32 fLock)
{	TRACE(shell,"%p->(0x%x), not implemented\n",this, fLock);
	return E_NOTIMPL;
}

/* IShellLinkW Implementation */
static HRESULT WINAPI IShellLinkW_QueryInterface(LPSHELLLINKW,REFIID,LPVOID*);
static ULONG WINAPI IShellLinkW_AddRef(LPSHELLLINKW);
static ULONG WINAPI IShellLinkW_Release(LPSHELLLINKW);
static HRESULT WINAPI IShellLinkW_GetPath(LPSHELLLINKW, LPWSTR,INT32, WIN32_FIND_DATA32A *, DWORD);
static HRESULT WINAPI IShellLinkW_GetIDList(LPSHELLLINKW, LPITEMIDLIST *);
static HRESULT WINAPI IShellLinkW_SetIDList(LPSHELLLINKW, LPCITEMIDLIST);
static HRESULT WINAPI IShellLinkW_GetDescription(LPSHELLLINKW, LPWSTR,INT32);
static HRESULT WINAPI IShellLinkW_SetDescription(LPSHELLLINKW, LPCWSTR);
static HRESULT WINAPI IShellLinkW_GetWorkingDirectory(LPSHELLLINKW, LPWSTR,INT32);
static HRESULT WINAPI IShellLinkW_SetWorkingDirectory(LPSHELLLINKW, LPCWSTR);
static HRESULT WINAPI IShellLinkW_GetArguments(LPSHELLLINKW, LPWSTR,INT32);
static HRESULT WINAPI IShellLinkW_SetArguments(LPSHELLLINKW, LPCWSTR);
static HRESULT WINAPI IShellLinkW_GetHotkey(LPSHELLLINKW, WORD *);
static HRESULT WINAPI IShellLinkW_SetHotkey(LPSHELLLINKW, WORD);
static HRESULT WINAPI IShellLinkW_GetShowCmd(LPSHELLLINKW, INT32 *);
static HRESULT WINAPI IShellLinkW_SetShowCmd(LPSHELLLINKW, INT32);
static HRESULT WINAPI IShellLinkW_GetIconLocation(LPSHELLLINKW, LPWSTR,INT32,INT32 *);
static HRESULT WINAPI IShellLinkW_SetIconLocation(LPSHELLLINKW, LPCWSTR,INT32);
static HRESULT WINAPI IShellLinkW_SetRelativePath(LPSHELLLINKW, LPCWSTR, DWORD);
static HRESULT WINAPI IShellLinkW_Resolve(LPSHELLLINKW, HWND32, DWORD);
static HRESULT WINAPI IShellLinkW_SetPath(LPSHELLLINKW, LPCWSTR);

/**************************************************************************
* IShellLinkW Implementation
*/

static struct IShellLinkW_VTable slvtw = 
{	IShellLinkW_QueryInterface,
	IShellLinkW_AddRef,
	IShellLinkW_Release,
	IShellLinkW_GetPath,
	IShellLinkW_GetIDList,
	IShellLinkW_SetIDList,
	IShellLinkW_GetDescription,
	IShellLinkW_SetDescription,
	IShellLinkW_GetWorkingDirectory,
	IShellLinkW_SetWorkingDirectory,
	IShellLinkW_GetArguments,
	IShellLinkW_SetArguments,
	IShellLinkW_GetHotkey,
	IShellLinkW_SetHotkey,
	IShellLinkW_GetShowCmd,
	IShellLinkW_SetShowCmd,
	IShellLinkW_GetIconLocation,
	IShellLinkW_SetIconLocation,
	IShellLinkW_SetRelativePath,
	IShellLinkW_Resolve,
	IShellLinkW_SetPath
};

/**************************************************************************
 *	  IShellLinkW_Constructor
 */
LPSHELLLINKW IShellLinkW_Constructor(void) 
{	LPSHELLLINKW sl;

	sl = (LPSHELLLINKW)HeapAlloc(GetProcessHeap(),0,sizeof(IShellLinkW));
	sl->ref = 1;
	sl->lpvtbl = &slvtw;

	sl->lppf = IPersistFile_Constructor();

	TRACE(shell,"(%p)->()\n",sl);
	return sl;
}

/**************************************************************************
 *  IShellLinkW::QueryInterface
 */
static HRESULT WINAPI IShellLinkW_QueryInterface(
  LPSHELLLINKW this, REFIID riid, LPVOID *ppvObj)
{  char    xriid[50];
   WINE_StringFromCLSID((LPCLSID)riid,xriid);
   TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",this,xriid);

  *ppvObj = NULL;

  if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
  { *ppvObj = this; 
  }
  else if(IsEqualIID(riid, &IID_IShellLinkW))  /*IShellLinkW*/
  {    *ppvObj = (LPSHELLLINKW)this;
  }   
  else if(IsEqualIID(riid, &IID_IPersistFile))  /*IPersistFile*/
  {    *ppvObj = (LPPERSISTFILE)this->lppf;
  }   

  if(*ppvObj)
  { (*(LPSHELLLINKW*)ppvObj)->lpvtbl->fnAddRef(this);      
    TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
    return S_OK;
  }
    TRACE(shell,"-- Interface: E_NOINTERFACE\n");
  return E_NOINTERFACE;
}  
/******************************************************************************
 * IShellLinkW_AddRef
 */
static ULONG WINAPI IShellLinkW_AddRef(LPSHELLLINKW this)
{ TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
    return ++(this->ref);
}
/******************************************************************************
 * IClassFactory_Release
 */
static ULONG WINAPI IShellLinkW_Release(LPSHELLLINKW this)
{ TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
  if (!--(this->ref)) 
  { TRACE(shell,"-- destroying IShellLinkW(%p)\n",this);
    this->lppf->lpvtbl->fnRelease(this->lppf);	/* IPersistFile*/
    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  return this->ref;
}

static HRESULT WINAPI IShellLinkW_GetPath(LPSHELLLINKW this, LPWSTR pszFile,INT32 cchMaxPath, WIN32_FIND_DATA32A *pfd, DWORD fFlags)
{	FIXME(shell,"(%p)->(pfile=%p len=%u find_data=%p flags=%lu)\n",this, pszFile, cchMaxPath, pfd, fFlags);
	lstrcpynAtoW(pszFile,"c:\\foo.bar", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_GetIDList(LPSHELLLINKW this, LPITEMIDLIST * ppidl)
{	FIXME(shell,"(%p)->(ppidl=%p)\n",this, ppidl);
	*ppidl = _ILCreateDesktop();
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_SetIDList(LPSHELLLINKW this, LPCITEMIDLIST pidl)
{	FIXME(shell,"(%p)->(pidl=%p)\n",this, pidl);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_GetDescription(LPSHELLLINKW this, LPWSTR pszName,INT32 cchMaxName)
{	FIXME(shell,"(%p)->(%p len=%u)\n",this, pszName, cchMaxName);
	lstrcpynAtoW(pszName,"Description, FIXME",cchMaxName);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_SetDescription(LPSHELLLINKW this, LPCWSTR pszName)
{	FIXME(shell,"(%p)->(desc=%s)\n",this, debugstr_w(pszName));
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_GetWorkingDirectory(LPSHELLLINKW this, LPWSTR pszDir,INT32 cchMaxPath)
{	FIXME(shell,"(%p)->()\n",this);
	lstrcpynAtoW(pszDir,"c:\\", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_SetWorkingDirectory(LPSHELLLINKW this, LPCWSTR pszDir)
{	FIXME(shell,"(%p)->(dir=%s)\n",this, debugstr_w(pszDir));
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_GetArguments(LPSHELLLINKW this, LPWSTR pszArgs,INT32 cchMaxPath)
{	FIXME(shell,"(%p)->(%p len=%u)\n",this, pszArgs, cchMaxPath);
	lstrcpynAtoW(pszArgs, "", cchMaxPath);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_SetArguments(LPSHELLLINKW this, LPCWSTR pszArgs)
{	FIXME(shell,"(%p)->(args=%s)\n",this, debugstr_w(pszArgs));
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_GetHotkey(LPSHELLLINKW this, WORD *pwHotkey)
{	FIXME(shell,"(%p)->(%p)\n",this, pwHotkey);
	*pwHotkey=0x0;
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_SetHotkey(LPSHELLLINKW this, WORD wHotkey)
{	FIXME(shell,"(%p)->(hotkey=%x)\n",this, wHotkey);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_GetShowCmd(LPSHELLLINKW this, INT32 *piShowCmd)
{	FIXME(shell,"(%p)->(%p)\n",this, piShowCmd);
	*piShowCmd=0;
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_SetShowCmd(LPSHELLLINKW this, INT32 iShowCmd)
{	FIXME(shell,"(%p)->(showcmd=%x)\n",this, iShowCmd);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_GetIconLocation(LPSHELLLINKW this, LPWSTR pszIconPath,INT32 cchIconPath,INT32 *piIcon)
{	FIXME(shell,"(%p)->(%p len=%u iicon=%p)\n",this, pszIconPath, cchIconPath, piIcon);
	lstrcpynAtoW(pszIconPath,"shell32.dll",cchIconPath);
	*piIcon=1;
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_SetIconLocation(LPSHELLLINKW this, LPCWSTR pszIconPath,INT32 iIcon)
{	FIXME(shell,"(%p)->(path=%s iicon=%u)\n",this, debugstr_w(pszIconPath), iIcon);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_SetRelativePath(LPSHELLLINKW this, LPCWSTR pszPathRel, DWORD dwReserved)
{	FIXME(shell,"(%p)->(path=%s %lx)\n",this, debugstr_w(pszPathRel), dwReserved);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_Resolve(LPSHELLLINKW this, HWND32 hwnd, DWORD fFlags)
{	FIXME(shell,"(%p)->(hwnd=%x flags=%lx)\n",this, hwnd, fFlags);
	return NOERROR;
}
static HRESULT WINAPI IShellLinkW_SetPath(LPSHELLLINKW this, LPCWSTR pszFile)
{	FIXME(shell,"(%p)->(path=%s)\n",this, debugstr_w(pszFile));
	return NOERROR;
}

