/*
 *	OLE2 COM objects
 *
 *	Copyright 1998 Eric Kohl
 *      Copyright 1999 Francis Beaudet
 */


#include <string.h>
#include "winbase.h"
#include "winerror.h"
#include "oleidl.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(ole)


/**************************************************************************
 *  OleAdviseHolderImpl Implementation
 */
typedef struct OleAdviseHolderImpl
{
  ICOM_VTABLE(IOleAdviseHolder)* lpvtbl;

  DWORD ref;

  DWORD         maxSinks;
  IAdviseSink** arrayOfSinks;

} OleAdviseHolderImpl;

static LPOLEADVISEHOLDER OleAdviseHolderImpl_Constructor();
static void              OleAdviseHolderImpl_Destructor(OleAdviseHolderImpl* ptrToDestroy);
static HRESULT WINAPI    OleAdviseHolderImpl_QueryInterface(LPOLEADVISEHOLDER,REFIID,LPVOID*);
static ULONG WINAPI      OleAdviseHolderImpl_AddRef(LPOLEADVISEHOLDER);
static ULONG WINAPI      OleAdviseHolderImpl_Release(LPOLEADVISEHOLDER);
static HRESULT WINAPI    OleAdviseHolderImpl_Advise(LPOLEADVISEHOLDER, IAdviseSink*, DWORD*);
static HRESULT WINAPI    OleAdviseHolderImpl_Unadvise (LPOLEADVISEHOLDER, DWORD);
static HRESULT WINAPI    OleAdviseHolderImpl_EnumAdvise (LPOLEADVISEHOLDER, IEnumSTATDATA **);
static HRESULT WINAPI    OleAdviseHolderImpl_SendOnRename (LPOLEADVISEHOLDER, IMoniker *);
static HRESULT WINAPI    OleAdviseHolderImpl_SendOnSave (LPOLEADVISEHOLDER);
static HRESULT WINAPI    OleAdviseHolderImpl_SendOnClose (LPOLEADVISEHOLDER);


/**************************************************************************
 *  OleAdviseHolderImpl_VTable
 */
static struct ICOM_VTABLE(IOleAdviseHolder) oahvt =
{
    OleAdviseHolderImpl_QueryInterface,
    OleAdviseHolderImpl_AddRef,
    OleAdviseHolderImpl_Release,
    OleAdviseHolderImpl_Advise,
    OleAdviseHolderImpl_Unadvise,
    OleAdviseHolderImpl_EnumAdvise,
    OleAdviseHolderImpl_SendOnRename,
    OleAdviseHolderImpl_SendOnSave,
    OleAdviseHolderImpl_SendOnClose
};

/**************************************************************************
 *  OleAdviseHolderImpl_Constructor
 */

static LPOLEADVISEHOLDER OleAdviseHolderImpl_Constructor()
{
  OleAdviseHolderImpl* lpoah;
  DWORD                index;

  lpoah= (OleAdviseHolderImpl*)HeapAlloc(GetProcessHeap(),
					 0,
					 sizeof(OleAdviseHolderImpl));
  
  lpoah->lpvtbl = &oahvt;
  lpoah->ref = 1;
  lpoah->maxSinks = 10;
  lpoah->arrayOfSinks = HeapAlloc(GetProcessHeap(),
				  0,
				  lpoah->maxSinks * sizeof(IAdviseSink*));

  for (index = 0; index < lpoah->maxSinks; index++)
    lpoah->arrayOfSinks[index]=0;

  return (LPOLEADVISEHOLDER)lpoah;
}

/**************************************************************************
 *  OleAdviseHolderImpl_Destructor
 */
static void OleAdviseHolderImpl_Destructor(
  OleAdviseHolderImpl* ptrToDestroy)
{
  DWORD index;

  for (index = 0; index < ptrToDestroy->maxSinks; index++)
  {
    if (ptrToDestroy->arrayOfSinks[index]!=0)
    {
      IAdviseSink_Release(ptrToDestroy->arrayOfSinks[index]);
      ptrToDestroy->arrayOfSinks[index] = NULL;
    }
  }
  
  HeapFree(GetProcessHeap(),
	   0,
	   ptrToDestroy->arrayOfSinks);
  

  HeapFree(GetProcessHeap(),
	   0,
	   ptrToDestroy);
}

/**************************************************************************
 *  OleAdviseHolderImpl_QueryInterface
 */
static HRESULT WINAPI OleAdviseHolderImpl_QueryInterface(
  LPOLEADVISEHOLDER iface,
  REFIID            riid, 
  LPVOID*           ppvObj)
{
  ICOM_THIS(OleAdviseHolderImpl, iface); 

  /*
   * Sanity check
   */
  if (ppvObj==NULL)
    return E_POINTER;

  *ppvObj = NULL;

  if (IsEqualIID(riid, &IID_IUnknown)) 
  {
    /* IUnknown */
    *ppvObj = This; 
  }
  else if(IsEqualIID(riid, &IID_IOleAdviseHolder)) 
  {
    /* IOleAdviseHolder */
    *ppvObj = (IOleAdviseHolder*) This;
  }

  if(*ppvObj == NULL)
    return E_NOINTERFACE;
  
  /*
   * A successful QI always increments the reference count.
   */
  IUnknown_AddRef((IUnknown*)*ppvObj);

  return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_AddRef
 */
static ULONG WINAPI OleAdviseHolderImpl_AddRef(
  LPOLEADVISEHOLDER iface)
{
  ICOM_THIS(OleAdviseHolderImpl, iface); 
  
  return ++(This->ref);
}

/******************************************************************************
 * OleAdviseHolderImpl_Release
 */
static ULONG WINAPI OleAdviseHolderImpl_Release(
  LPOLEADVISEHOLDER iface)
{
  ICOM_THIS(OleAdviseHolderImpl, iface); 

  This->ref--;

  if (This->ref == 0)
  {
    OleAdviseHolderImpl_Destructor(This);

    return 0;
  }

  return This->ref;
}

/******************************************************************************
 * OleAdviseHolderImpl_Advise
 */
static HRESULT WINAPI OleAdviseHolderImpl_Advise(
  LPOLEADVISEHOLDER iface,
  IAdviseSink*      pAdvise,
  DWORD*            pdwConnection)
{
  DWORD index;
  
  ICOM_THIS(OleAdviseHolderImpl, iface); 

  TRACE(ole, "(%p, %p, %p)\n", This, pAdvise, pdwConnection);

  /*
   * Sanity check
   */
  if (pdwConnection==NULL)
    return E_POINTER;
  
  *pdwConnection = 0;

  /*
   * Find a free spot in the array.
   */
  for (index = 0; index < This->maxSinks; index++)
  {
    if (This->arrayOfSinks[index]==NULL)
      break;
  }

  /*
   * If the array is full, we need to grow it.
   */
  if (index == This->maxSinks)
  {
    DWORD i;

    This->maxSinks+=10;

    This->arrayOfSinks = HeapReAlloc(GetProcessHeap(), 
				     0,
				     This->arrayOfSinks,
				     This->maxSinks*sizeof(IAdviseSink*));

    for (i=index;i < This->maxSinks; i++)
      This->arrayOfSinks[i]=0;      
  }

  /*
   * Store the new sink
   */
  This->arrayOfSinks[index] = pAdvise;

  if (This->arrayOfSinks[index]!=NULL)
    IAdviseSink_AddRef(This->arrayOfSinks[index]);

  /*
   * Return the index as the cookie.
   * Since 0 is not a valid cookie, we will increment by
   * 1 the index in the table.
   */
  *pdwConnection = index+1;

  return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_Unadvise
 */
static HRESULT WINAPI OleAdviseHolderImpl_Unadvise(
  LPOLEADVISEHOLDER iface, 
  DWORD             dwConnection)
{
  ICOM_THIS(OleAdviseHolderImpl, iface); 

  TRACE(ole, "(%p, %lu)\n", This, dwConnection);

  /*
   * So we don't return 0 as a cookie, the index was 
   * incremented by 1 in OleAdviseHolderImpl_Advise
   * we have to compensate.
   */
  dwConnection--;
  
  /*
   * Check for invalid cookies.
   */
  if ( (dwConnection < 0) || 
       (dwConnection >= This->maxSinks) )
    return OLE_E_NOCONNECTION;

  if (This->arrayOfSinks[dwConnection] == NULL)
    return OLE_E_NOCONNECTION;

  /*
   * Release the sink and mark the spot in the list as free.
   */
  IAdviseSink_Release(This->arrayOfSinks[dwConnection]);
  This->arrayOfSinks[dwConnection] = NULL;

  return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_EnumAdvise
 */
static HRESULT WINAPI
OleAdviseHolderImpl_EnumAdvise (LPOLEADVISEHOLDER iface, IEnumSTATDATA **ppenumAdvise)
{
		ICOM_THIS(OleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)->(%p)\n", This, ppenumAdvise);

    *ppenumAdvise = NULL;

    return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_SendOnRename
 */
static HRESULT WINAPI
OleAdviseHolderImpl_SendOnRename (LPOLEADVISEHOLDER iface, IMoniker *pmk)
{
		ICOM_THIS(OleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)->(%p)\n", This, pmk);


    return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_SendOnSave
 */
static HRESULT WINAPI
OleAdviseHolderImpl_SendOnSave (LPOLEADVISEHOLDER iface)
{
		ICOM_THIS(OleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)\n", This);


    return S_OK;
}

/******************************************************************************
 * OleAdviseHolderImpl_SendOnClose
 */
static HRESULT WINAPI
OleAdviseHolderImpl_SendOnClose (LPOLEADVISEHOLDER iface)
{
  ICOM_THIS(OleAdviseHolderImpl, iface); 
    FIXME (ole, "(%p)\n", This);


    return S_OK;
}

/**************************************************************************
 *  DataAdviseHolder Implementation
 */
typedef struct DataAdviseHolder
{
  ICOM_VTABLE(IDataAdviseHolder)* lpvtbl;

  DWORD ref;
} DataAdviseHolder;

/**************************************************************************
 *  DataAdviseHolder method prototypes
 */
static IDataAdviseHolder* DataAdviseHolder_Constructor();
static void               DataAdviseHolder_Destructor(DataAdviseHolder* ptrToDestroy);
static HRESULT WINAPI     DataAdviseHolder_QueryInterface(
			    IDataAdviseHolder*      iface,
			    REFIID                  riid,
			    void**                  ppvObject);
static ULONG WINAPI       DataAdviseHolder_AddRef( 
                            IDataAdviseHolder*      iface);
static ULONG WINAPI       DataAdviseHolder_Release( 
                            IDataAdviseHolder*      iface);
static HRESULT WINAPI     DataAdviseHolder_Advise( 
                            IDataAdviseHolder*      iface,
			    IDataObject*            pDataObject, 
			    FORMATETC*              pFetc, 
			    DWORD                   advf, 
			    IAdviseSink*            pAdvise, 
			    DWORD*                  pdwConnection);
static HRESULT WINAPI     DataAdviseHolder_Unadvise( 
                            IDataAdviseHolder*      iface,
			    DWORD                   dwConnection);
static HRESULT WINAPI     DataAdviseHolder_EnumAdvise( 
                            IDataAdviseHolder*      iface,       
			    IEnumSTATDATA**         ppenumAdvise);
static HRESULT WINAPI     DataAdviseHolder_SendOnDataChange( 
                            IDataAdviseHolder*      iface, 
			    IDataObject*            pDataObject, 
			    DWORD                   dwReserved, 
			    DWORD                   advf);

/**************************************************************************
 *  DataAdviseHolderImpl_VTable
 */
static struct ICOM_VTABLE(IDataAdviseHolder) DataAdviseHolderImpl_VTable =
{
  DataAdviseHolder_QueryInterface,
  DataAdviseHolder_AddRef,
  DataAdviseHolder_Release,
  DataAdviseHolder_Advise,
  DataAdviseHolder_Unadvise,
  DataAdviseHolder_EnumAdvise,
  DataAdviseHolder_SendOnDataChange
};

/******************************************************************************
 * DataAdviseHolder_Constructor
 */
static IDataAdviseHolder* DataAdviseHolder_Constructor()
{
  DataAdviseHolder* newHolder;

  newHolder = (DataAdviseHolder*)HeapAlloc(GetProcessHeap(),
					   0,
					   sizeof(DataAdviseHolder));
  
  newHolder->lpvtbl = &DataAdviseHolderImpl_VTable;
  newHolder->ref = 1;

  return (IDataAdviseHolder*)newHolder;
}

/******************************************************************************
 * DataAdviseHolder_Destructor
 */
static void DataAdviseHolder_Destructor(
  DataAdviseHolder* ptrToDestroy)
{
  HeapFree(GetProcessHeap(),
	   0,
	   ptrToDestroy);
}

/************************************************************************
 * DataAdviseHolder_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataAdviseHolder_QueryInterface(
  IDataAdviseHolder*      iface,
  REFIID                  riid,
  void**                  ppvObject)
{
  ICOM_THIS(DataAdviseHolder, iface); 

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (This==0) || (ppvObject==0) )
    return E_INVALIDARG;
  
  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;

  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if ( (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0) ||
       (memcmp(&IID_IDataAdviseHolder, riid, sizeof(IID_IDataAdviseHolder)) == 0)  )
  {
    *ppvObject = iface;
  }
  
  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
  {
    return E_NOINTERFACE;
  }
  
  /*
   * Query Interface always increases the reference count by one when it is
   * successful. 
   */
  IUnknown_AddRef((IUnknown*)*ppvObject);

  return S_OK;;  
}

/************************************************************************
 * DataAdviseHolder_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI       DataAdviseHolder_AddRef(                             
  IDataAdviseHolder*      iface)
{
  ICOM_THIS(DataAdviseHolder, iface); 

  This->ref++;

  return This->ref;
}

/************************************************************************
 * DataAdviseHolder_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataAdviseHolder_Release( 
  IDataAdviseHolder*      iface)
{
  ICOM_THIS(DataAdviseHolder, iface); 

  /*
   * Decrease the reference count on this object.
   */
  This->ref--;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (This->ref==0)
  {
    DataAdviseHolder_Destructor(This);

    return 0;
  }
  
  return This->ref;
}

static HRESULT WINAPI DataAdviseHolder_Advise(
  IDataAdviseHolder*      iface,
  IDataObject*            pDataObject, 
  FORMATETC*              pFetc, 
  DWORD                   advf, 
  IAdviseSink*            pAdvise, 
  DWORD*                  pdwConnection)
{
  FIXME(ole, "(): stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI     DataAdviseHolder_Unadvise( 
  IDataAdviseHolder*      iface,
  DWORD                   dwConnection)
{
  FIXME(ole, "(): stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI     DataAdviseHolder_EnumAdvise( 
  IDataAdviseHolder*      iface,       
  IEnumSTATDATA**         ppenumAdvise)
{
  FIXME(ole, "(): stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI     DataAdviseHolder_SendOnDataChange( 
  IDataAdviseHolder*      iface, 
  IDataObject*            pDataObject, 
  DWORD                   dwReserved, 
  DWORD                   advf)
{
  FIXME(ole, "(): stub\n");
  return E_NOTIMPL;
}

/***********************************************************************
 * API functions
 */

/***********************************************************************
 * CreateOleAdviseHolder [OLE32.59]
 */
HRESULT WINAPI CreateOleAdviseHolder(
  LPOLEADVISEHOLDER *ppOAHolder)
{
  TRACE(ole, "(%p)\n", ppOAHolder);

  /*
   * Sanity check,
   */
  if (ppOAHolder==NULL)
    return E_POINTER;

  *ppOAHolder = OleAdviseHolderImpl_Constructor ();

  if (*ppOAHolder != NULL)
    return S_OK;

  return E_OUTOFMEMORY;
}

/******************************************************************************
 *              CreateDataAdviseHolder        [OLE32.53]
 */
HRESULT WINAPI CreateDataAdviseHolder(
  LPDATAADVISEHOLDER* ppDAHolder)
{
  TRACE(ole,"(%p)\n", ppDAHolder);

  /*
   * Sanity check,
   */
  if (ppDAHolder==NULL)
    return E_POINTER;

  *ppDAHolder = DataAdviseHolder_Constructor();

  if (*ppDAHolder != NULL)
    return S_OK;

  return E_OUTOFMEMORY;
}

