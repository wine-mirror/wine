/*
 *	OLE2 COM objects
 *
 *	Copyright 1998 Eric Kohl
 */


#include "ole.h"
#include "ole2.h"
#include "winerror.h"
#include "interfaces.h"
#include "oleobj.h"
#include "debug.h"


#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)


/**************************************************************************
 *  IOleAdviseHolder Implementation
 */
static HRESULT WINAPI IOleAdviseHolder_QueryInterface(LPOLEADVISEHOLDER,REFIID,LPVOID*);
static ULONG WINAPI   IOleAdviseHolder_AddRef(LPOLEADVISEHOLDER);
static ULONG WINAPI   IOleAdviseHolder_Release(LPOLEADVISEHOLDER);
static HRESULT WINAPI IOleAdviseHolder_Advise(LPOLEADVISEHOLDER, IAdviseSink*, DWORD*);
static HRESULT WINAPI IOleAdviseHolder_Unadvise (LPOLEADVISEHOLDER, DWORD);
static HRESULT WINAPI IOleAdviseHolder_EnumAdvise (LPOLEADVISEHOLDER, IEnumSTATDATA **);
static HRESULT WINAPI IOleAdviseHolder_SendOnRename (LPOLEADVISEHOLDER, IMoniker *);
static HRESULT WINAPI IOleAdviseHolder_SendOnSave (LPOLEADVISEHOLDER this);
static HRESULT WINAPI IOleAdviseHolder_SendOnClose (LPOLEADVISEHOLDER this);


/**************************************************************************
 *  IOleAdviseHolder_VTable
 */
static IOleAdviseHolder_VTable oahvt =
{
    IOleAdviseHolder_QueryInterface,
    IOleAdviseHolder_AddRef,
    IOleAdviseHolder_Release,
    IOleAdviseHolder_Advise,
    IOleAdviseHolder_Unadvise,
    IOleAdviseHolder_EnumAdvise,
    IOleAdviseHolder_SendOnRename,
    IOleAdviseHolder_SendOnSave,
    IOleAdviseHolder_SendOnClose
};

/**************************************************************************
 *  IOleAdviseHolder_Constructor
 */

LPOLEADVISEHOLDER IOleAdviseHolder_Constructor()
{
    LPOLEADVISEHOLDER lpoah;

    lpoah= (LPOLEADVISEHOLDER)HeapAlloc(GetProcessHeap(),0,sizeof(IOleAdviseHolder));
    lpoah->ref = 1;
    lpoah->lpvtbl = &oahvt;
    FIXME (ole, "(%p)->()\n", lpoah);

    return lpoah;
}

/**************************************************************************
 *  IOleAdviseHolder_QueryInterface
 */
static HRESULT WINAPI
IOleAdviseHolder_QueryInterface (LPOLEADVISEHOLDER this, REFIID riid, LPVOID *ppvObj)
{
    char xriid[50];
    WINE_StringFromCLSID((LPCLSID)riid,xriid);
    FIXME (ole, "(%p)->(\n\tIID:\t%s)\n", this, xriid);

    *ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown)) {
	/* IUnknown */
	*ppvObj = this; 
    }
    else if(IsEqualIID(riid, &IID_IOleAdviseHolder)) {
	/* IOleAdviseHolder */
	*ppvObj = (IOleAdviseHolder*) this;
    }

    if(*ppvObj) {
	(*(LPOLEADVISEHOLDER*)ppvObj)->lpvtbl->fnAddRef(this);  	
	FIXME (ole, "-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
	return S_OK;
    }

    FIXME (ole, "-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/******************************************************************************
 * IOleAdviseHolder_AddRef
 */
static ULONG WINAPI
IOleAdviseHolder_AddRef (LPOLEADVISEHOLDER this)
{
    FIXME (ole, "(%p)->(count=%lu)\n", this, this->ref);
    return ++(this->ref);
}

/******************************************************************************
 * IOleAdviseHolder_Release
 */
static ULONG WINAPI
IOleAdviseHolder_Release (LPOLEADVISEHOLDER this)
{
    FIXME (ole, "(%p)->(count=%lu)\n", this, this->ref);
    if (!--(this->ref)) {
	FIXME (ole, "-- destroying IOleAdviseHolder(%p)\n", this);
	HeapFree(GetProcessHeap(),0,this);
	return 0;
    }
    return this->ref;
}

/******************************************************************************
 * IOleAdviseHolder_Advise
 */
static HRESULT WINAPI
IOleAdviseHolder_Advise (LPOLEADVISEHOLDER this,
			 IAdviseSink *pAdvise, DWORD *pdwConnection)
{
    FIXME (ole, "(%p)->(%p %p)\n", this, pAdvise, pdwConnection);

    *pdwConnection = 0;

   return S_OK;
}

/******************************************************************************
 * IOleAdviseHolder_Unadvise
 */
static HRESULT WINAPI
IOleAdviseHolder_Unadvise (LPOLEADVISEHOLDER this, DWORD dwConnection)
{
    FIXME (ole, "(%p)->(%lu)\n", this, dwConnection);

    return S_OK;
}

/******************************************************************************
 * IOleAdviseHolder_EnumAdvise
 */
static HRESULT WINAPI
IOleAdviseHolder_EnumAdvise (LPOLEADVISEHOLDER this, IEnumSTATDATA **ppenumAdvise)
{
    FIXME (ole, "(%p)->(%p)\n", this, ppenumAdvise);

    *ppenumAdvise = NULL;

    return S_OK;
}

/******************************************************************************
 * IOleAdviseHolder_SendOnRename
 */
static HRESULT WINAPI
IOleAdviseHolder_SendOnRename (LPOLEADVISEHOLDER this, IMoniker *pmk)
{
    FIXME (ole, "(%p)->(%p)\n", this, pmk);


    return S_OK;
}

/******************************************************************************
 * IOleAdviseHolder_SendOnSave
 */
static HRESULT WINAPI
IOleAdviseHolder_SendOnSave (LPOLEADVISEHOLDER this)
{
    FIXME (ole, "(%p)\n", this);


    return S_OK;
}

/******************************************************************************
 * IOleAdviseHolder_SendOnClose
 */
static HRESULT WINAPI
IOleAdviseHolder_SendOnClose (LPOLEADVISEHOLDER this)
{
    FIXME (ole, "(%p)\n", this);


    return S_OK;
}

/***********************************************************************
 * API functions
 */

/***********************************************************************
 * CreateOleAdviseHolder [OLE32.59]
 */
HRESULT WINAPI CreateOleAdviseHolder32 (LPOLEADVISEHOLDER *ppOAHolder)
{
    FIXME(ole,"(%p): stub!\n", ppOAHolder);

    *ppOAHolder = IOleAdviseHolder_Constructor ();
    if (*ppOAHolder)
	return S_OK;

    *ppOAHolder = 0;
    return E_OUTOFMEMORY;
}

