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

DWORD WINAPI SHCLSIDFromStringA (LPSTR clsid, CLSID *id);
extern IShellFolder * IShellFolder_Constructor(
	IShellFolder * psf,
	LPITEMIDLIST pidl);
extern HRESULT IFSFolder_Constructor(
	IUnknown * pUnkOuter,
	REFIID riid,
	LPVOID * ppv);

/*************************************************************************
 * SHCoCreateInstance [SHELL32.102]
 * 
 * NOTES
 *     exported by ordinal
 */
LRESULT WINAPI SHCoCreateInstance(
	LPSTR aclsid,
	REFCLSID clsid,
	IUnknown * unknownouter,
	REFIID refiid,
	LPVOID *ppv)
{
	DWORD	hres;
	IID	iid;
	CLSID * myclsid = (CLSID*)clsid;
	
	if (!clsid)
	{
	  if (!aclsid) return REGDB_E_CLASSNOTREG;
	  SHCLSIDFromStringA(aclsid, &iid);
	  myclsid = &iid;
	}

	TRACE("(%p,\n\tCLSID:\t%s, unk:%p\n\tIID:\t%s,%p)\n",
		aclsid,debugstr_guid(myclsid),unknownouter,debugstr_guid(refiid),ppv);

	if IsEqualCLSID(myclsid, &CLSID_ShellFSFolder)
	{
	  hres = IFSFolder_Constructor(unknownouter, refiid, ppv);
	}
	else
	{
	  hres = CoCreateInstance(myclsid, unknownouter, CLSCTX_INPROC_SERVER, refiid, ppv);
	}
	
	if(hres!=S_OK)
	{
	  ERR("failed (0x%08lx) to create \n\tCLSID:\t%s\n\tIID:\t%s\n",
              hres, debugstr_guid(myclsid), debugstr_guid(refiid));
	  ERR("class not found in registry\n");
	}

	TRACE("-- instance: %p\n",*ppv);
	return hres;
}

/*************************************************************************
 * SHELL32_DllGetClassObject   [SHELL32.128]
 */
HRESULT WINAPI SHELL32_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv)
{	HRESULT	hres = E_OUTOFMEMORY;
	LPCLASSFACTORY lpclf;

	TRACE("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
	
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
	{
	  WARN("-- CLSID not found\n");
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
DWORD WINAPI SHCLSIDFromStringA (LPSTR clsid, CLSID *id)
{
	TRACE("(%p(%s) %p)\n", clsid, clsid, id);
	return CLSIDFromString16(clsid, id); 
}
DWORD WINAPI SHCLSIDFromStringW (LPWSTR clsid, CLSID *id)
{
	TRACE("(%p(%s) %p)\n", clsid, debugstr_w(clsid), id);
	return CLSIDFromString(clsid, id); 
}
DWORD WINAPI SHCLSIDFromStringAW (LPVOID clsid, CLSID *id)
{
	if (VERSION_OsIsUnicode())
	  return SHCLSIDFromStringW (clsid, id);
	return SHCLSIDFromStringA (clsid, id);
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
{
	TRACE("(%p)\n", lpmal);
	return CoGetMalloc(0,lpmal);
}

/*************************************************************************
 * SHGetDesktopFolder			[SHELL32.216]
 */
LPSHELLFOLDER pdesktopfolder=NULL;

DWORD WINAPI SHGetDesktopFolder(IShellFolder **psf)
{
	HRESULT	hres = S_OK;
	LPCLASSFACTORY lpclf;
	TRACE("%p->(%p)\n",psf,*psf);

	*psf=NULL;

	if (!pdesktopfolder) 
	{
	  lpclf = IClassFactory_Constructor(&CLSID_ShellDesktop);
	  if(lpclf) 
	  {
	    hres = IClassFactory_CreateInstance(lpclf,NULL,(REFIID)&IID_IShellFolder, (void*)&pdesktopfolder);
	    IClassFactory_Release(lpclf);
	  }  
	}
	
	if (pdesktopfolder) 
	{
	  /* even if we create the folder, add a ref so the application can´t destroy the folder*/
	  IShellFolder_AddRef(pdesktopfolder);
	  *psf = pdesktopfolder;
	}

	TRACE("-- %p->(%p)\n",psf, *psf);
	return hres;
}

/**************************************************************************
*  IClassFactory Implementation
*/

typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
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
	ICOM_VTBL(lpclf) = &clfvt;
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
	TRACE("(%p)->(\n\tIID:\t%s)\n",This,debugstr_guid(riid));

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
	TRACE("-- Interface: %s E_NOINTERFACE\n", debugstr_guid(riid));
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

	TRACE("%p->(%p,\n\tIID:\t%s,%p)\n",This,pUnknown,debugstr_guid(riid),ppObject);

	*ppObject = NULL;
		
	if(pUnknown)
	{
	  return(CLASS_E_NOAGGREGATION);
	}

	if (IsEqualCLSID(This->rclsid, &CLSID_ShellDesktop))
	{
	  pObj = (IUnknown *)ISF_Desktop_Constructor();
	}
	else if (IsEqualCLSID(This->rclsid, &CLSID_ShellLink))
	{
	  pObj = (IUnknown *)IShellLink_Constructor(FALSE);
	} 
	else
	{
	  ERR("unknown IID requested\n\tIID:\t%s\n",debugstr_guid(riid));
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

/**************************************************************************
 * Default ClassFactory Implementation
 *
 * SHCreateDefClassObject
 *
 * NOTES
 *  helper function for dll's without a own classfactory
 *  a generic classfactory is returned
 *  when the CreateInstance of the cf is called the callback is executed
 */
typedef HRESULT (CALLBACK * LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);

typedef struct
{
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
    CLSID			*rclsid;
    LPFNCREATEINSTANCE		lpfnCI;
    const IID *			riidInst;
    UINT *			pcRefDll; /* pointer to refcounter in external dll (ugrrr...) */
} IDefClFImpl;

static ICOM_VTABLE(IClassFactory) dclfvt;

/**************************************************************************
 *  IDefClF_fnConstructor
 */

IClassFactory * IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, UINT * pcRefDll, REFIID riidInst)
{
	IDefClFImpl* lpclf;

	lpclf = (IDefClFImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDefClFImpl));
	lpclf->ref = 1;
	ICOM_VTBL(lpclf) = &dclfvt;
	lpclf->lpfnCI = lpfnCI;
	lpclf->pcRefDll = pcRefDll;

	if (pcRefDll) 
	  (*pcRefDll)++;

	lpclf->riidInst = riidInst;

	TRACE("(%p)\n\tIID:\t%s\n",lpclf, debugstr_guid(riidInst));
	shell32_ObjCount++;
	return (LPCLASSFACTORY)lpclf;
}
/**************************************************************************
 *  IDefClF_fnQueryInterface
 */
static HRESULT WINAPI IDefClF_fnQueryInterface(
  LPCLASSFACTORY iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IDefClFImpl,iface);

	TRACE("(%p)->(\n\tIID:\t%s)\n",This,debugstr_guid(riid));

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
	TRACE("-- Interface: %s E_NOINTERFACE\n", debugstr_guid(riid));
	return E_NOINTERFACE;
}  
/******************************************************************************
 * IDefClF_fnAddRef
 */
static ULONG WINAPI IDefClF_fnAddRef(LPCLASSFACTORY iface)
{
	ICOM_THIS(IDefClFImpl,iface);
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;

	return ++(This->ref);
}
/******************************************************************************
 * IDefClF_fnRelease
 */
static ULONG WINAPI IDefClF_fnRelease(LPCLASSFACTORY iface)
{
	ICOM_THIS(IDefClFImpl,iface);
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{ 
	  if (This->pcRefDll) 
	    (*This->pcRefDll)--;

	  TRACE("-- destroying IClassFactory(%p)\n",This);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}
/******************************************************************************
 * IDefClF_fnCreateInstance
 */
static HRESULT WINAPI IDefClF_fnCreateInstance(
  LPCLASSFACTORY iface, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject)
{
	ICOM_THIS(IDefClFImpl,iface);

	TRACE("%p->(%p,\n\tIID:\t%s,%p)\n",This,pUnkOuter,debugstr_guid(riid),ppvObject);

	*ppvObject = NULL;
		
	if(pUnkOuter)
	  return(CLASS_E_NOAGGREGATION);

	if ( This->riidInst==NULL ||
	     IsEqualCLSID(riid, This->riidInst) ||
	     IsEqualCLSID(riid, &IID_IUnknown) )
	{
	  return This->lpfnCI(pUnkOuter, riid, ppvObject);
	}

	ERR("unknown IID requested\n\tIID:\t%s\n",debugstr_guid(riid));
	return E_NOINTERFACE;
}
/******************************************************************************
 * IDefClF_fnLockServer
 */
static HRESULT WINAPI IDefClF_fnLockServer(LPCLASSFACTORY iface, BOOL fLock)
{
	ICOM_THIS(IDefClFImpl,iface);
	TRACE("%p->(0x%x), not implemented\n",This, fLock);
	return E_NOTIMPL;
}

static ICOM_VTABLE(IClassFactory) dclfvt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
	LPFNCREATEINSTANCE lpfnCI,	/* create instance callback entry */
	UINT	*pcRefDll,		/* ref count of the dll */
	REFIID	riidInst)		/* optional interface to the instance */
{
	TRACE("\n\tIID:\t%s %p %p %p \n\tIIDIns:\t%s\n",
              debugstr_guid(riid), ppv, lpfnCI, pcRefDll, debugstr_guid(riidInst));

	if ( IsEqualCLSID(riid, &IID_IClassFactory) )
	{
	  IClassFactory * pcf = IDefClF_fnConstructor(lpfnCI, pcRefDll, riidInst);
	  if (pcf)
	  {
	    *ppv = pcf;
	    return NOERROR;
	  }
	  return E_OUTOFMEMORY;
	}
	return E_NOINTERFACE;
}

