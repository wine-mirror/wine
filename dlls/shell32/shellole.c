/*
 *	handling of SHELL32.DLL OLE-Objects
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied  <juergen.schmied@metronet.de>
 *
 */

#include <stdlib.h>
#include <string.h>

#include "wine/obj_base.h"
#include "wine/obj_shelllink.h"
#include "wine/obj_shellfolder.h"
#include "wine/obj_shellbrowser.h"
#include "wine/obj_contextmenu.h"
#include "wine/obj_shellextinit.h"
#include "wine/obj_extracticon.h"

#include "shlguid.h"
#include "winversion.h"
#include "winreg.h"
#include "winerror.h"
#include "debugtools.h"

#include "shell32_main.h"

DEFAULT_DEBUG_CHANNEL(shell)

/*************************************************************************
 *
 */
typedef DWORD (* WINAPI GetClassPtr)(REFCLSID,REFIID,LPVOID);

static GetClassPtr SH_find_moduleproc(LPSTR dllname,HMODULE *xhmod,LPSTR name)
{	HMODULE hmod;
	FARPROC dllunload,nameproc;

	TRACE("dll=%s, hmodule=%p, name=%s\n",dllname, xhmod, name);

	if (xhmod)
	{ *xhmod = 0;
	}

	if (!strcasecmp(PathFindFilenameA(dllname),"shell32.dll"))
	{ return (GetClassPtr)SHELL32_DllGetClassObject;
	}

	hmod = LoadLibraryExA(dllname,0,LOAD_WITH_ALTERED_SEARCH_PATH);

	if (!hmod)
	{ return NULL;
	}

	dllunload = GetProcAddress(hmod,"DllCanUnloadNow");

	if (!dllunload)
	{ if (xhmod)
	  { *xhmod = hmod;
	  }
	}

	nameproc = GetProcAddress(hmod,name);

	if (!nameproc)
	{ FreeLibrary(hmod);
	  return NULL;
	}

	/* register unloadable dll with unloadproc ... */
	return (GetClassPtr)nameproc;
}
/*************************************************************************
 *
 */
static DWORD SH_get_instance(REFCLSID clsid,LPSTR dllname,LPVOID unknownouter,REFIID refiid,LPVOID *ppv) 
{	GetClassPtr     dllgetclassob;
	DWORD		hres;
	LPCLASSFACTORY	classfac;

	char	xclsid[50],xrefiid[50];
	WINE_StringFromCLSID((LPCLSID)clsid,xclsid);
	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);

	TRACE("\n\tCLSID:%s,%s,%p,\n\tIID:%s,%p\n",xclsid, dllname,unknownouter,xrefiid,ppv);
	
	dllgetclassob = SH_find_moduleproc(dllname,NULL,"DllGetClassObject");

	if (!dllgetclassob)
	{ return 0x80070000|GetLastError();
	}

	hres = (*dllgetclassob)(clsid,(REFIID)&IID_IClassFactory,&classfac);

	if ((hres<0) || (hres>=0x80000000))
	    return hres;

	if (!classfac)
	{ FIXME("no classfactory, but hres is 0x%ld!\n",hres);
	  return E_FAIL;
	}

	IClassFactory_CreateInstance(classfac,unknownouter,refiid,ppv);
	IClassFactory_Release(classfac);
	return S_OK;
}

/*************************************************************************
 * SHCoCreateInstance [SHELL32.102]
 * 
 * NOTES
 *     exported by ordinal
 */
LRESULT WINAPI SHCoCreateInstance(
	LPSTR aclsid,
	CLSID *clsid,
	LPUNKNOWN unknownouter,
	REFIID refiid,
	LPVOID *ppv)
{
	char	buffer[256],xclsid[48],xiid[48],path[260],tmodel[100];
	HKEY	inprockey;
	DWORD	pathlen,type,tmodellen;
	DWORD	hres;
	
	WINE_StringFromCLSID(refiid,xiid);

	if (clsid)
	{ WINE_StringFromCLSID(clsid,xclsid);
	}
	else
	{ if (!aclsid)
	  {	return REGDB_E_CLASSNOTREG;
	  }
	  strcpy(xclsid,aclsid);
	}

	TRACE("(%p,\n\tSID:\t%s,%p,\n\tIID:\t%s,%p)\n",aclsid,xclsid,unknownouter,xiid,ppv);

	sprintf(buffer,"CLSID\\%s\\InProcServer32",xclsid);

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT,buffer,0,0x02000000,&inprockey))
	{ return SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,ppv);
	}
	pathlen=sizeof(path);

	if (RegQueryValueA(inprockey,NULL,path,&pathlen))
	{ RegCloseKey(inprockey);
	  return SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,ppv);
	}

	TRACE("Server dll is %s\n",path);
	tmodellen=sizeof(tmodel);
	type=REG_SZ;

	if (RegQueryValueExA(inprockey,"ThreadingModel",NULL,&type,tmodel,&tmodellen))
	{ RegCloseKey(inprockey);
	  return SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,ppv);
	}

	TRACE("Threading model is %s\n",tmodel);

	hres=SH_get_instance(clsid,path,unknownouter,refiid,ppv);
	if (hres<0 || (hres>0x80000000))
	{ hres=SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,ppv);
	}
	RegCloseKey(inprockey);

	TRACE("-- interface is %p\n", *ppv);
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
 */
HRESULT WINAPI SHELL32_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv)
{	HRESULT	hres = E_OUTOFMEMORY;
	LPCLASSFACTORY lpclf;

	char xclsid[50],xiid[50];
	WINE_StringFromCLSID((LPCLSID)rclsid,xclsid);
	WINE_StringFromCLSID((LPCLSID)iid,xiid);
	TRACE("\n\tCLSID:\t%s,\n\tIID:\t%s\n",xclsid,xiid);
	
	*ppv = NULL;
	if(IsEqualCLSID(rclsid, &CLSID_ShellDesktop)|| 
	   IsEqualCLSID(rclsid, &CLSID_ShellLink))
	{
	  lpclf = IClassFactory_Constructor( rclsid );

	  if(lpclf) 
	  {
	    hres = IClassFactory_QueryInterface(lpclf,iid, ppv);
	    IClassFactory_Release(lpclf);
	  }
	}
	else
	{ WARN("-- CLSID not found\n");
	  hres = CLASS_E_CLASSNOTAVAILABLE;
	}
	TRACE("-- pointer to class factory: %p\n",*ppv);
	return hres;
}

/*************************************************************************
 * SHCLSIDFromString				[SHELL32.147]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHCLSIDFromString (LPSTR clsid, CLSID *id)
{
	TRACE("(%p(%s) %p)\n", clsid, clsid, id);
	return CLSIDFromString16(clsid, id); 
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
DWORD WINAPI SHGetMalloc(LPMALLOC *lpmal) 
{	TRACE("(%p)\n", lpmal);
	return CoGetMalloc(0,lpmal);
}

/**************************************************************************
*  IClassFactory Implementation
*/

typedef struct
{
    /* IUnknown fields */
    ICOM_VTABLE(IClassFactory)* lpvtbl;
    DWORD                       ref;
    CLSID			*rclsid;
} IClassFactoryImpl;

static ICOM_VTABLE(IClassFactory) clfvt;

/**************************************************************************
 *  IClassFactory_Constructor
 */

LPCLASSFACTORY IClassFactory_Constructor(REFCLSID rclsid)
{
	IClassFactoryImpl* lpclf;

	lpclf= (IClassFactoryImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IClassFactoryImpl));
	lpclf->ref = 1;
	lpclf->lpvtbl = &clfvt;
	lpclf->rclsid = (CLSID*)rclsid;

	TRACE("(%p)->()\n",lpclf);
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
	TRACE("(%p)->(\n\tIID:\t%s)\n",This,xriid);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IClassFactory))  /*IClassFactory*/
	{ *ppvObj = (IClassFactory*)This;
	}   

	if(*ppvObj)
	{ IUnknown_AddRef((LPUNKNOWN)*ppvObj);  	
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: %s E_NOINTERFACE\n", xriid);
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IClassFactory_AddRef
 */
static ULONG WINAPI IClassFactory_fnAddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}
/******************************************************************************
 * IClassFactory_Release
 */
static ULONG WINAPI IClassFactory_fnRelease(LPCLASSFACTORY iface)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount--;
	if (!--(This->ref)) 
	{ TRACE("-- destroying IClassFactory(%p)\n",This);
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
	TRACE("%p->(%p,\n\tIID:\t%s,%p)\n",This,pUnknown,xriid,ppObject);

	*ppObject = NULL;
		
	if(pUnknown)
	{
	  return(CLASS_E_NOAGGREGATION);
	}

	if (IsEqualCLSID(This->rclsid, &CLSID_ShellDesktop))
	{
	  pObj = (IUnknown *)IShellFolder_Constructor(NULL,NULL);
	}
	else if (IsEqualCLSID(This->rclsid, &CLSID_ShellLink))
	{
	  pObj = (IUnknown *)IShellLink_Constructor(FALSE);
	} 
	else
	{
	  ERR("unknown IID requested\n\tIID:\t%s\n",xriid);
	  return(E_NOINTERFACE);
	}
	
	if (!pObj)
	{
	  return(E_OUTOFMEMORY);
	}
	 
	hres = IUnknown_QueryInterface(pObj,riid, ppObject);
	IUnknown_Release(pObj);

	TRACE("-- Object created: (%p)->%p\n",This,*ppObject);

	return hres;
}
/******************************************************************************
 * IClassFactory_LockServer
 */
static HRESULT WINAPI IClassFactory_fnLockServer(LPCLASSFACTORY iface, BOOL fLock)
{
	ICOM_THIS(IClassFactoryImpl,iface);
	TRACE("%p->(0x%x), not implemented\n",This, fLock);
	return E_NOTIMPL;
}

static ICOM_VTABLE(IClassFactory) clfvt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IClassFactory_fnQueryInterface,
    IClassFactory_fnAddRef,
  IClassFactory_fnRelease,
  IClassFactory_fnCreateInstance,
  IClassFactory_fnLockServer
};

