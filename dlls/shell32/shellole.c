/*
 *	Shell Folder stuff (...and all the OLE-Objects of SHELL32.DLL)
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied  <juergen.schmied@metronet.de>
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

#include "shell32_main.h"

/*************************************************************************
 *
 */
typedef DWORD (* WINAPI GetClassPtr)(REFCLSID,REFIID,LPVOID);

static GetClassPtr SH_find_moduleproc(LPSTR dllname,HMODULE32 *xhmod,LPSTR name)
{	HMODULE32 hmod;
	FARPROC32 dllunload,nameproc;

	TRACE(shell,"dll=%s, hmodule=%p, name=%s\n",dllname, xhmod, name);
	if (xhmod)
	{ *xhmod = 0;
	}
	if (!strcasecmp(PathFindFilename(dllname),"shell32.dll"))
	{ return (GetClassPtr)SHELL32_DllGetClassObject;
	}

	hmod = LoadLibraryEx32A(dllname,0,LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!hmod)
	{ return NULL;
	}
	dllunload = GetProcAddress32(hmod,"DllCanUnloadNow");
	if (!dllunload)
	{ if (xhmod)
	  { *xhmod = hmod;
	  }
	}
	nameproc = GetProcAddress32(hmod,name);
	if (!nameproc)
	{ FreeLibrary32(hmod);
	  return NULL;
	}
	/* register unloadable dll with unloadproc ... */
	return (GetClassPtr)nameproc;
}
/*************************************************************************
 *
 */
static DWORD SH_get_instance(REFCLSID clsid,LPSTR dllname,LPVOID unknownouter,REFIID refiid,LPVOID inst) 
{	GetClassPtr     dllgetclassob;
	DWORD		hres;
	LPCLASSFACTORY	classfac;

	char	xclsid[50],xrefiid[50];
	WINE_StringFromCLSID((LPCLSID)clsid,xclsid);
	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
	TRACE(shell,"\n\tCLSID:%s,%s,%p,\n\tIID:%s,%p\n",xclsid, dllname,unknownouter,xrefiid,inst);
	
	dllgetclassob = SH_find_moduleproc(dllname,NULL,"DllGetClassObject");
	if (!dllgetclassob)
	{ return 0x80070000|GetLastError();
	}

	hres = (*dllgetclassob)(clsid,(REFIID)&IID_IClassFactory,&classfac);
	if (hres<0 || (hres>=0x80000000))
	{ return hres;
	}
	if (!classfac)
	{ FIXME(shell,"no classfactory, but hres is 0x%ld!\n",hres);
	  return E_FAIL;
	}
	classfac->lpvtbl->fnCreateInstance(classfac,unknownouter,refiid,inst);
	classfac->lpvtbl->fnRelease(classfac);
	return 0;
}

/*************************************************************************
 * SHCoCreateInstance [SHELL32.102]
 * 
 * NOTES
 *     exported by ordinal
 */
LRESULT WINAPI SHCoCreateInstance(LPSTR aclsid,CLSID *clsid,LPUNKNOWN unknownouter,REFIID refiid,LPVOID inst)
{	char	buffer[256],xclsid[48],xiid[48],path[260],tmodel[100];
	HKEY	inprockey;
	DWORD	pathlen,type,tmodellen;
	DWORD	hres;
	
	WINE_StringFromCLSID(refiid,xiid);

	if (clsid)
	{ WINE_StringFromCLSID(clsid,xclsid);
	}
	else
	{ if (!aclsid)
	  {	return 0x80040154;
	  }
	  strcpy(xclsid,aclsid);
	}
	TRACE(shell,"(%p,\n\tSID:\t%s,%p,\n\tIID:\t%s,%p)\n",aclsid,xclsid,unknownouter,xiid,inst);

	sprintf(buffer,"CLSID\\%s\\InProcServer32",xclsid);

	if (RegOpenKeyEx32A(HKEY_CLASSES_ROOT,buffer,0,0x02000000,&inprockey))
	{ return SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	}
	pathlen=sizeof(path);

	if (RegQueryValue32A(inprockey,NULL,path,&pathlen))
	{ RegCloseKey(inprockey);
	  return SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	}

	TRACE(shell, "Server dll is %s\n",path);
	tmodellen=sizeof(tmodel);
	type=REG_SZ;
	if (RegQueryValueEx32A(inprockey,"ThreadingModel",NULL,&type,tmodel,&tmodellen))
	{ RegCloseKey(inprockey);
	  return SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	}

	TRACE(shell, "Threading model is %s\n",tmodel);

	hres=SH_get_instance(clsid,path,unknownouter,refiid,inst);
	if (hres<0)
	{ hres=SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	}
	RegCloseKey(inprockey);
	return hres;
}


/*************************************************************************
 * SHELL32_DllGetClassObject   [SHELL32.128]
 *
 * [Standart OLE/COM Interface Method]
 * This Function retrives the pointer to a specified interface (iid) of
 * a given class (rclsid).
 * With this pointer it's possible to call the IClassFactory_CreateInstance
 * method to get a instance of the requested Class.
 * This function does NOT instantiate the Class!!!
 * 
 * RETURNS
 *   HRESULT
 *
 */
DWORD WINAPI SHELL32_DllGetClassObject(REFCLSID rclsid,REFIID iid,LPVOID *ppv)
{	HRESULT	hres = E_OUTOFMEMORY;
	LPCLASSFACTORY lpclf;

	char xclsid[50],xiid[50];
	WINE_StringFromCLSID((LPCLSID)rclsid,xclsid);
	WINE_StringFromCLSID((LPCLSID)iid,xiid);
	TRACE(shell,"\n\tCLSID:\t%s,\n\tIID:\t%s\n",xclsid,xiid);
	
	*ppv = NULL;
	if(IsEqualCLSID(rclsid, &CLSID_ShellDesktop)|| 
	   IsEqualCLSID(rclsid, &CLSID_ShellLink))
	{ if(IsEqualCLSID(rclsid, &CLSID_ShellDesktop))      /*debug*/
	  { TRACE(shell,"-- requested CLSID_ShellDesktop\n");
	  }
	  if(IsEqualCLSID(rclsid, &CLSID_ShellLink))         /*debug*/
	  { TRACE(shell,"-- requested CLSID_ShellLink\n");
	  }
	  lpclf = IClassFactory_Constructor();
	  if(lpclf)
	  { hres = lpclf->lpvtbl->fnQueryInterface(lpclf,iid, ppv);
		lpclf->lpvtbl->fnRelease(lpclf);
	  }
	}
	else
	{ WARN(shell, "-- CLSID not found\n");
	  hres = CLASS_E_CLASSNOTAVAILABLE;
	}
	TRACE(shell,"-- return pointer to interface: %p\n",*ppv);
	return hres;
}

/*************************************************************************
 *			 SHGetMalloc			[SHELL32.220]
 * returns the interface to shell malloc.
 *
 * [SDK header win95/shlobj.h:
 * equivalent to:  #define SHGetMalloc(ppmem)   CoGetMalloc(MEMCTX_TASK, ppmem)
 * ]
 * What we are currently doing is not very wrong, since we always use the same
 * heap (ProcessHeap).
 */
DWORD WINAPI SHGetMalloc(LPMALLOC32 *lpmal) 
{	TRACE(shell,"(%p)\n", lpmal);
	return CoGetMalloc32(0,lpmal);
}

/**************************************************************************
*  IClassFactory Implementation
*/
static HRESULT WINAPI IClassFactory_QueryInterface(LPCLASSFACTORY,REFIID,LPVOID*);
static ULONG WINAPI IClassFactory_AddRef(LPCLASSFACTORY);
static ULONG WINAPI IClassFactory_Release(LPCLASSFACTORY);
static HRESULT WINAPI IClassFactory_CreateInstance();
static HRESULT WINAPI IClassFactory_LockServer();
/**************************************************************************
 *  IClassFactory_VTable
 */
static IClassFactory_VTable clfvt = 
{	IClassFactory_QueryInterface,
	IClassFactory_AddRef,
	IClassFactory_Release,
	IClassFactory_CreateInstance,
	IClassFactory_LockServer
};

/**************************************************************************
 *  IClassFactory_Constructor
 */

LPCLASSFACTORY IClassFactory_Constructor()
{	LPCLASSFACTORY	lpclf;

	lpclf= (LPCLASSFACTORY)HeapAlloc(GetProcessHeap(),0,sizeof(IClassFactory));
	lpclf->ref = 1;
	lpclf->lpvtbl = &clfvt;
  TRACE(shell,"(%p)->()\n",lpclf);
	return lpclf;
}
/**************************************************************************
 *  IClassFactory::QueryInterface
 */
static HRESULT WINAPI IClassFactory_QueryInterface(
  LPCLASSFACTORY this, REFIID riid, LPVOID *ppvObj)
{	char	xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",this,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = this; 
	}
	else if(IsEqualIID(riid, &IID_IClassFactory))  /*IClassFactory*/
	{ *ppvObj = (IClassFactory*)this;
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
 * IClassFactory_AddRef
 */
static ULONG WINAPI IClassFactory_AddRef(LPCLASSFACTORY this)
{ TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	return ++(this->ref);
}
/******************************************************************************
 * IClassFactory_Release
 */
static ULONG WINAPI IClassFactory_Release(LPCLASSFACTORY this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	if (!--(this->ref)) 
	{ TRACE(shell,"-- destroying IClassFactory(%p)\n",this);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}
/******************************************************************************
 * IClassFactory_CreateInstance
 */
static HRESULT WINAPI IClassFactory_CreateInstance(
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

	if (IsEqualIID(riid, &IID_IShellFolder))
	{ pObj = (IUnknown *)IShellFolder_Constructor(NULL,NULL);
	} 
	else if (IsEqualIID(riid, &IID_IShellView))
	{ pObj = (IUnknown *)IShellView_Constructor();
 	} 
	else if (IsEqualIID(riid, &IID_IShellLink))
	{ pObj = (IUnknown *)IShellLink_Constructor();
	} 
	else if (IsEqualIID(riid, &IID_IExtractIcon))
	{ pObj = (IUnknown *)IExtractIcon_Constructor(NULL);
	} 
	else if (IsEqualIID(riid, &IID_IContextMenu))
	{ pObj = (IUnknown *)IContextMenu_Constructor(NULL, NULL, 0);
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
 * IClassFactory_LockServer
 */
static HRESULT WINAPI IClassFactory_LockServer(LPCLASSFACTORY this, BOOL32 fLock)
{	TRACE(shell,"%p->(0x%x), not implemented\n",this, fLock);
	return E_NOTIMPL;
}
