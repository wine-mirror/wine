/*
 *	handling of SHELL32.DLL OLE-Objects
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied  <juergen.schmied@metronet.de>
 *
 */

#include <stdlib.h>
#include <string.h>
#include "winreg.h"
#include "winerror.h"
#include "objbase.h"
#include "winversion.h"

#include "shlguid.h"
#include "shlobj.h"
#include "shell32_main.h"

#include "debug.h"

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
	if (!strcasecmp(PathFindFilename32A(dllname),"shell32.dll"))
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
	if ((hres<0) || (hres>=0x80000000))
	    return hres;
	if (!classfac)
	{ FIXME(shell,"no classfactory, but hres is 0x%ld!\n",hres);
	  return E_FAIL;
	}
	IClassFactory_CreateInstance(classfac,unknownouter,refiid,inst);
	IClassFactory_Release(classfac);
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
	if (hres<0 || (hres>0x80000000))
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

	  if (IsEqualCLSID(rclsid, &CLSID_ShellLink))
	  { if (VERSION_OsIsUnicode ())
	      lpclf = IShellLinkW_CF_Constructor();
	    else  
	      lpclf = IShellLink_CF_Constructor();
	  }
	  else
	  { lpclf = IClassFactory_Constructor();
	  }

	  if(lpclf) {
	    hres = IClassFactory_QueryInterface(lpclf,iid, ppv);
	    IClassFactory_Release(lpclf);
	  }
	}
	else
	{ WARN(shell, "-- CLSID not found\n");
	  hres = CLASS_E_CLASSNOTAVAILABLE;
	}
	TRACE(shell,"-- pointer to class factory: %p\n",*ppv);
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

typedef struct
{
    /* IUnknown fields */
    ICOM_VTABLE(IClassFactory)* lpvtbl;
    DWORD                       ref;
} IClassFactoryImpl;

static ICOM_VTABLE(IClassFactory) clfvt;

/**************************************************************************
 *  IClassFactory_Constructor
 */

LPCLASSFACTORY IClassFactory_Constructor(void)
{
	IClassFactoryImpl* lpclf;

	lpclf= (IClassFactoryImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IClassFactoryImpl));
	lpclf->ref = 1;
	lpclf->lpvtbl = &clfvt;

	TRACE(shell,"(%p)->()\n",lpclf);
	shell32_ObjCount++;
	return (LPCLASSFACTORY)lpclf;
}
/**************************************************************************
 *  IClassFactory_QueryInterface
 */
static HRESULT WINAPI IClassFactory_fnQueryInterface(
  LPCLASSFACTORY iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	char	xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",This,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IClassFactory))  /*IClassFactory*/
	{ *ppvObj = (IClassFactory*)This;
	}   

	if(*ppvObj)
	{ IUnknown_AddRef((LPUNKNOWN)*ppvObj);  	
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IClassFactory_AddRef
 */
static ULONG WINAPI IClassFactory_fnAddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}
/******************************************************************************
 * IClassFactory_Release
 */
static ULONG WINAPI IClassFactory_fnRelease(LPCLASSFACTORY iface)
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
 * IClassFactory_CreateInstance
 */
static HRESULT WINAPI IClassFactory_fnCreateInstance(
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
	{	return(CLASS_E_NOAGGREGATION);
	}

	if (IsEqualIID(riid, &IID_IShellFolder))
	{ pObj = (IUnknown *)IShellFolder_Constructor(NULL,NULL);
	} 
	else if (IsEqualIID(riid, &IID_IShellView))
	{ pObj = (IUnknown *)IShellView_Constructor(NULL,NULL);
 	} 
	else if (IsEqualIID(riid, &IID_IExtractIcon))
	{ pObj = (IUnknown *)IExtractIcon_Constructor(NULL);
	} 
	else if (IsEqualIID(riid, &IID_IContextMenu))
	{ pObj = (IUnknown *)IContextMenu_Constructor(NULL, NULL, 0);
 	} 
	else if (IsEqualIID(riid, &IID_IDataObject))
	{ pObj = (IUnknown *)IDataObject_Constructor(0,NULL,NULL,0);
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
 * IClassFactory_LockServer
 */
static HRESULT WINAPI IClassFactory_fnLockServer(LPCLASSFACTORY iface, BOOL32 fLock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE(shell,"%p->(0x%x), not implemented\n",This, fLock);
	return E_NOTIMPL;
}

static ICOM_VTABLE(IClassFactory) clfvt = 
{
    IClassFactory_fnQueryInterface,
    IClassFactory_fnAddRef,
  IClassFactory_fnRelease,
  IClassFactory_fnCreateInstance,
  IClassFactory_fnLockServer
};
