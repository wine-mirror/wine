/*
 *	OLE2 COM objects
 *
 *	Copyright 1998 Eric Kohl
 */


#include <string.h>
#include "winbase.h"
#include "winerror.h"
#include "oleidl.h"
#include "debug.h"


/**************************************************************************
 *  IOleAdviseHolder Implementation
 */
typedef struct
{
	ICOM_VTABLE(IOleAdviseHolder)* lpvtbl;
	DWORD ref;
} IOleAdviseHolderImpl;

static HRESULT WINAPI IOleAdviseHolder_fnQueryInterface(LPOLEADVISEHOLDER,REFIID,LPVOID*);
static ULONG WINAPI   IOleAdviseHolder_fnAddRef(LPOLEADVISEHOLDER);
static ULONG WINAPI   IOleAdviseHolder_fnRelease(LPOLEADVISEHOLDER);
static HRESULT WINAPI IOleAdviseHolder_fnAdvise(LPOLEADVISEHOLDER, IAdviseSink*, DWORD*);
static HRESULT WINAPI IOleAdviseHolder_fnUnadvise (LPOLEADVISEHOLDER, DWORD);
static HRESULT WINAPI IOleAdviseHolder_fnEnumAdvise (LPOLEADVISEHOLDER, IEnumSTATDATA **);
static HRESULT WINAPI IOleAdviseHolder_fnSendOnRename (LPOLEADVISEHOLDER, IMoniker *);
static HRESULT WINAPI IOleAdviseHolder_fnSendOnSave (LPOLEADVISEHOLDER);
static HRESULT WINAPI IOleAdviseHolder_fnSendOnClose (LPOLEADVISEHOLDER);


/**************************************************************************
 *  IOleAdviseHolder_VTable
 */
static struct ICOM_VTABLE(IOleAdviseHolder) oahvt =
{
    IOleAdviseHolder_fnQueryInterface,
    IOleAdviseHolder_fnAddRef,
    IOleAdviseHolder_fnRelease,
    IOleAdviseHolder_fnAdvise,
    IOleAdviseHolder_fnUnadvise,
    IOleAdviseHolder_fnEnumAdvise,
    IOleAdviseHolder_fnSendOnRename,
    IOleAdviseHolder_fnSendOnSave,
    IOleAdviseHolder_fnSendOnClose
};

/**************************************************************************
 *  IOleAdviseHolder_Constructor
 */

LPOLEADVISEHOLDER IOleAdviseHolder_Constructor()
{
    IOleAdviseHolderImpl* lpoah;

    lpoah= (IOleAdviseHolderImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IOleAdviseHolderImpl));
    lpoah->ref = 1;
    lpoah->lpvtbl = &oahvt;
    FIXME (ole, "(%p)->()\n", lpoah);

    return (LPOLEADVISEHOLDER)lpoah;
}

/**************************************************************************
 *  IOleAdviseHolder_QueryInterface
 */
static HRESULT WINAPI
IOleAdviseHolder_fnQueryInterface (LPOLEADVISEHOLDER iface, REFIID riid, LPVOID *ppvObj)
{
		ICOM_THIS(IOleAdviseHolderImpl, iface); 
    char xriid[50];
    WINE_StringFromCLSID((LPCLSID)riid,xriid);
    FIXME (ole, "(%p)->(\n\tIID:\t%s)\n", This, xriid);

    *ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown)) {
	/* IUnknown */
	*ppvObj = This; 
    }
    else if(IsEqualIID(riid, &IID_IOleAdviseHolder)) {
	/* IOleAdviseHolder */
	*ppvObj = (IOleAdviseHolder*) This;
    }

    if(*ppvObj) {
	(*(LPOLEADVISEHOLDER*)ppvObj)->lpvtbl->fnAddRef(iface);  	
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
IOleAdviseHolder_fnAddRef (LPOLEADVISEHOLDER iface)
{
		ICOM_THIS(IOleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)->(count=%lu)\n", This, This->ref);
    return ++(This->ref);
}

/******************************************************************************
 * IOleAdviseHolder_Release
 */
static ULONG WINAPI
IOleAdviseHolder_fnRelease (LPOLEADVISEHOLDER iface)
{
		ICOM_THIS(IOleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)->(count=%lu)\n", This, This->ref);
    if (!--(This->ref)) {
	FIXME (ole, "-- destroying IOleAdviseHolder(%p)\n", This);
	HeapFree(GetProcessHeap(),0,This);
	return 0;
    }
    return This->ref;
}

/******************************************************************************
 * IOleAdviseHolder_Advise
 */
static HRESULT WINAPI
IOleAdviseHolder_fnAdvise (LPOLEADVISEHOLDER iface,
			 IAdviseSink *pAdvise, DWORD *pdwConnection)
{
		ICOM_THIS(IOleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)->(%p %p)\n", This, pAdvise, pdwConnection);

    *pdwConnection = 0;

   return S_OK;
}

/******************************************************************************
 * IOleAdviseHolder_Unadvise
 */
static HRESULT WINAPI
IOleAdviseHolder_fnUnadvise (LPOLEADVISEHOLDER iface, DWORD dwConnection)
{
		ICOM_THIS(IOleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)->(%lu)\n", This, dwConnection);

    return S_OK;
}

/******************************************************************************
 * IOleAdviseHolder_EnumAdvise
 */
static HRESULT WINAPI
IOleAdviseHolder_fnEnumAdvise (LPOLEADVISEHOLDER iface, IEnumSTATDATA **ppenumAdvise)
{
		ICOM_THIS(IOleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)->(%p)\n", This, ppenumAdvise);

    *ppenumAdvise = NULL;

    return S_OK;
}

/******************************************************************************
 * IOleAdviseHolder_SendOnRename
 */
static HRESULT WINAPI
IOleAdviseHolder_fnSendOnRename (LPOLEADVISEHOLDER iface, IMoniker *pmk)
{
		ICOM_THIS(IOleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)->(%p)\n", This, pmk);


    return S_OK;
}

/******************************************************************************
 * IOleAdviseHolder_SendOnSave
 */
static HRESULT WINAPI
IOleAdviseHolder_fnSendOnSave (LPOLEADVISEHOLDER iface)
{
		ICOM_THIS(IOleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)\n", This);


    return S_OK;
}

/******************************************************************************
 * IOleAdviseHolder_SendOnClose
 */
static HRESULT WINAPI
IOleAdviseHolder_fnSendOnClose (LPOLEADVISEHOLDER iface)
{
		ICOM_THIS(IOleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)\n", This);


    return S_OK;
}

/***********************************************************************
 * API functions
 */

/***********************************************************************
 * CreateOleAdviseHolder [OLE32.59]
 */
HRESULT WINAPI CreateOleAdviseHolder (LPOLEADVISEHOLDER *ppOAHolder)
{
    FIXME(ole,"(%p): stub!\n", ppOAHolder);

    *ppOAHolder = IOleAdviseHolder_Constructor ();
    if (*ppOAHolder)
	return S_OK;

    *ppOAHolder = 0;
    return E_OUTOFMEMORY;
}

