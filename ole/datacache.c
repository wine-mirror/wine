/*
 *	OLE 2 Data cache
 *
 *      Copyright 1999  Francis Beaudet
 *
 * NOTES:
 *    The OLE2 data cache supports a whole whack of
 *    interfaces including:
 *       IDataObject, IPersistStorage, IViewObject2,
 *       IOleCache2 and IOleCacheControl.
 *
 *    All the implementation details are taken from: Inside OLE
 *    second edition by Kraig Brockschmidt,
 *
 * TODO
 *    Allmost everything is to be done. Handling the cached data, 
 *    handling the representations, drawing the representations of
 *    the cached data... yada yada.
 */
#include <assert.h>

#include "winuser.h"
#include "winerror.h"
#include "ole2.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(ole)

/****************************************************************************
 * AdviseSinkListNode
 */
typedef struct AdviseSinkListNode
{
  
} AdviseSinkListNode;

/****************************************************************************
 * DataCache
 */
struct DataCache
{
  /*
   * List all interface VTables here
   */
  ICOM_VTABLE(IDataObject)*      lpvtbl1; 
  ICOM_VTABLE(IUnknown)*         lpvtbl2;
  ICOM_VTABLE(IPersistStorage)*  lpvtbl3;
  ICOM_VTABLE(IViewObject2)*     lpvtbl4;  
  ICOM_VTABLE(IOleCache2)*       lpvtbl5;
  ICOM_VTABLE(IOleCacheControl)* lpvtbl6;

  /*
   * Reference count of this object
   */
  ULONG ref;

  /*
   * IUnknown implementation of the outer object.
   */
  IUnknown* outerUnknown;

  /*
   * This storage pointer is set through a call to
   * IPersistStorage_Load. This is where the visual
   * representation of the object is stored.
   */
  IStorage* presentationStorage;
};

typedef struct DataCache DataCache;

/*
 * Here, I define utility macros to help with the casting of the 
 * "this" parameter.
 * There is a version to accomodate all of the VTables implemented
 * by this object.
 */
#define _ICOM_THIS_From_IDataObject(class,name)       class* this = (class*)name;
#define _ICOM_THIS_From_NDIUnknown(class, name)       class* this = (class*)(((void*)name)-sizeof(void*)); 
#define _ICOM_THIS_From_IPersistStorage(class, name)  class* this = (class*)(((void*)name)-2*sizeof(void*)); 
#define _ICOM_THIS_From_IViewObject2(class, name)     class* this = (class*)(((void*)name)-3*sizeof(void*)); 
#define _ICOM_THIS_From_IOleCache2(class, name)       class* this = (class*)(((void*)name)-4*sizeof(void*)); 
#define _ICOM_THIS_From_IOleCacheControl(class, name) class* this = (class*)(((void*)name)-5*sizeof(void*)); 

/*
 * Prototypes for the methods of the DataCache class.
 */
static DataCache* DataCache_Construct(REFCLSID  clsid,
				      LPUNKNOWN pUnkOuter);
static void       DataCache_Destroy(DataCache* ptrToDestroy);

/*
 * Prototypes for the methods of the DataCache class
 * that implement non delegating IUnknown methods.
 */
static HRESULT WINAPI DataCache_NDIUnknown_QueryInterface(
            IUnknown*      iface,
            REFIID         riid,
            void**         ppvObject);
static ULONG WINAPI DataCache_NDIUnknown_AddRef( 
            IUnknown*      iface);
static ULONG WINAPI DataCache_NDIUnknown_Release( 
            IUnknown*      iface);

/*
 * Prototypes for the methods of the DataCache class
 * that implement IDataObject methods.
 */
static HRESULT WINAPI DataCache_IDataObject_QueryInterface(
            IDataObject*     iface,
            REFIID           riid,
            void**           ppvObject);
static ULONG WINAPI DataCache_IDataObject_AddRef( 
            IDataObject*     iface);
static ULONG WINAPI DataCache_IDataObject_Release( 
            IDataObject*     iface);
static HRESULT WINAPI DataCache_GetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetcIn, 
	    STGMEDIUM*       pmedium);
static HRESULT WINAPI DataCache_GetDataHere(
	    IDataObject*     iface, 
	    LPFORMATETC      pformatetc,
	    STGMEDIUM*       pmedium);
static HRESULT WINAPI DataCache_QueryGetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc);
static HRESULT WINAPI DataCache_GetCanonicalFormatEtc(
	    IDataObject*     iface, 
	    LPFORMATETC      pformatectIn, 
	    LPFORMATETC      pformatetcOut);
static HRESULT WINAPI DataCache_IDataObject_SetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc, 
	    STGMEDIUM*       pmedium, 
	    BOOL             fRelease);
static HRESULT WINAPI DataCache_EnumFormatEtc(
	    IDataObject*     iface,       
	    DWORD            dwDirection,
	    IEnumFORMATETC** ppenumFormatEtc);
static HRESULT WINAPI DataCache_DAdvise(
	    IDataObject*     iface, 
	    FORMATETC*       pformatetc, 
	    DWORD            advf, 
	    IAdviseSink*     pAdvSink, 
	    DWORD*           pdwConnection);
static HRESULT WINAPI DataCache_DUnadvise(
	    IDataObject*     iface,
	    DWORD            dwConnection);
static HRESULT WINAPI DataCache_EnumDAdvise(
	    IDataObject*     iface,
	    IEnumSTATDATA**  ppenumAdvise);

/*
 * Prototypes for the methods of the DataCache class
 * that implement IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_IPersistStorage_QueryInterface(
            IPersistStorage* iface,
            REFIID           riid,
            void**           ppvObject);
static ULONG WINAPI DataCache_IPersistStorage_AddRef( 
            IPersistStorage* iface);
static ULONG WINAPI DataCache_IPersistStorage_Release( 
            IPersistStorage* iface);
static HRESULT WINAPI DataCache_GetClassID( 
            const IPersistStorage* iface,
	    CLSID*           pClassID);
static HRESULT WINAPI DataCache_IsDirty( 
            IPersistStorage* iface);
static HRESULT WINAPI DataCache_InitNew( 
            IPersistStorage* iface, 
	    IStorage*        pStg);
static HRESULT WINAPI DataCache_Load( 
            IPersistStorage* iface,
	    IStorage*        pStg);
static HRESULT WINAPI DataCache_Save( 
            IPersistStorage* iface,
	    IStorage*        pStg, 
	    BOOL             fSameAsLoad);
static HRESULT WINAPI DataCache_SaveCompleted( 
            IPersistStorage* iface,  
	    IStorage*        pStgNew);
static HRESULT WINAPI DataCache_HandsOffStorage(
            IPersistStorage* iface);

/*
 * Prototypes for the methods of the DataCache class
 * that implement IViewObject2 methods.
 */
static HRESULT WINAPI DataCache_IViewObject2_QueryInterface(
            IViewObject2* iface,
            REFIID           riid,
            void**           ppvObject);
static ULONG WINAPI DataCache_IViewObject2_AddRef( 
            IViewObject2* iface);
static ULONG WINAPI DataCache_IViewObject2_Release( 
            IViewObject2* iface);
static HRESULT WINAPI DataCache_Draw(
            IViewObject2*    iface,
	    DWORD            dwDrawAspect,
	    LONG             lindex,
	    void*            pvAspect,
	    DVTARGETDEVICE*  ptd, 
	    HDC              hdcTargetDev, 
	    HDC              hdcDraw,
	    LPCRECTL         lprcBounds,
	    LPCRECTL         lprcWBounds,
	    IVO_ContCallback pfnContinue,
	    DWORD            dwContinue);
static HRESULT WINAPI DataCache_GetColorSet(
            IViewObject2*   iface, 
	    DWORD           dwDrawAspect, 
	    LONG            lindex, 
	    void*           pvAspect, 
	    DVTARGETDEVICE* ptd, 
	    HDC             hicTargetDevice, 
	    LOGPALETTE**    ppColorSet);
static HRESULT WINAPI DataCache_Freeze(
            IViewObject2*   iface,
	    DWORD           dwDrawAspect,
	    LONG            lindex,
	    void*           pvAspect, 
	    DWORD*          pdwFreeze);
static HRESULT WINAPI DataCache_Unfreeze(
            IViewObject2*   iface,
	    DWORD           dwFreeze);
static HRESULT WINAPI DataCache_SetAdvise(
            IViewObject2*   iface,
	    DWORD           aspects, 
	    DWORD           advf, 
	    IAdviseSink*    pAdvSink);
static HRESULT WINAPI DataCache_GetAdvise(
            IViewObject2*   iface, 
	    DWORD*          pAspects, 
	    DWORD*          pAdvf, 
	    IAdviseSink**   ppAdvSink);
static HRESULT WINAPI DataCache_GetExtent(
            IViewObject2*   iface, 
	    DWORD           dwDrawAspect, 
	    LONG            lindex, 
	    DVTARGETDEVICE* ptd, 
	    LPSIZEL         lpsizel);

/*
 * Prototypes for the methods of the DataCache class
 * that implement IOleCache2 methods.
 */
static HRESULT WINAPI DataCache_IOleCache2_QueryInterface(
            IOleCache2*     iface,
            REFIID          riid,
            void**          ppvObject);
static ULONG WINAPI DataCache_IOleCache2_AddRef( 
            IOleCache2*     iface);
static ULONG WINAPI DataCache_IOleCache2_Release( 
            IOleCache2*     iface);
static HRESULT WINAPI DataCache_Cache(
            IOleCache2*     iface,
	    FORMATETC*      pformatetc,
	    DWORD           advf,
	    DWORD*          pdwConnection);
static HRESULT WINAPI DataCache_Uncache(
	    IOleCache2*     iface,
	    DWORD           dwConnection);
static HRESULT WINAPI DataCache_EnumCache(
            IOleCache2*     iface,
	    IEnumSTATDATA** ppenumSTATDATA);
static HRESULT WINAPI DataCache_InitCache(
	    IOleCache2*     iface,
	    IDataObject*    pDataObject);
static HRESULT WINAPI DataCache_IOleCache2_SetData(
            IOleCache2*     iface,
	    FORMATETC*      pformatetc,
	    STGMEDIUM*      pmedium,
	    BOOL            fRelease);
static HRESULT WINAPI DataCache_UpdateCache(
            IOleCache2*     iface,
	    LPDATAOBJECT    pDataObject, 
	    DWORD           grfUpdf,
	    LPVOID          pReserved);
static HRESULT WINAPI DataCache_DiscardCache(
            IOleCache2*     iface,
	    DWORD           dwDiscardOptions);

/*
 * Prototypes for the methods of the DataCache class
 * that implement IOleCacheControl methods.
 */
static HRESULT WINAPI DataCache_IOleCacheControl_QueryInterface(
            IOleCacheControl* iface,
            REFIID            riid,
            void**            ppvObject);
static ULONG WINAPI DataCache_IOleCacheControl_AddRef( 
            IOleCacheControl* iface);
static ULONG WINAPI DataCache_IOleCacheControl_Release( 
            IOleCacheControl* iface);
static HRESULT WINAPI DataCache_OnRun(
	    IOleCacheControl* iface,
	    LPDATAOBJECT      pDataObject);
static HRESULT WINAPI DataCache_OnStop(
	    IOleCacheControl* iface);

/*
 * Virtual function tables for the DataCache class.
 */
static ICOM_VTABLE(IUnknown) DataCache_NDIUnknown_VTable =
{
  DataCache_NDIUnknown_QueryInterface,
  DataCache_NDIUnknown_AddRef,
  DataCache_NDIUnknown_Release
};

static ICOM_VTABLE(IDataObject) DataCache_IDataObject_VTable =
{
  DataCache_IDataObject_QueryInterface,
  DataCache_IDataObject_AddRef,
  DataCache_IDataObject_Release,
  DataCache_GetData,
  DataCache_GetDataHere,
  DataCache_QueryGetData,
  DataCache_GetCanonicalFormatEtc,
  DataCache_IDataObject_SetData,
  DataCache_EnumFormatEtc,
  DataCache_DAdvise,
  DataCache_DUnadvise,
  DataCache_EnumDAdvise
};

static ICOM_VTABLE(IPersistStorage) DataCache_IPersistStorage_VTable =
{
  DataCache_IPersistStorage_QueryInterface,
  DataCache_IPersistStorage_AddRef,
  DataCache_IPersistStorage_Release,
  DataCache_GetClassID,
  DataCache_IsDirty,
  DataCache_InitNew,
  DataCache_Load,
  DataCache_Save,
  DataCache_SaveCompleted,
  DataCache_HandsOffStorage
};

static ICOM_VTABLE(IViewObject2) DataCache_IViewObject2_VTable =
{
  DataCache_IViewObject2_QueryInterface,
  DataCache_IViewObject2_AddRef,
  DataCache_IViewObject2_Release,
  DataCache_Draw,
  DataCache_GetColorSet,
  DataCache_Freeze,
  DataCache_Unfreeze,
  DataCache_SetAdvise,
  DataCache_GetAdvise,
  DataCache_GetExtent
};

static ICOM_VTABLE(IOleCache2) DataCache_IOleCache2_VTable =
{
  DataCache_IOleCache2_QueryInterface,
  DataCache_IOleCache2_AddRef,
  DataCache_IOleCache2_Release,
  DataCache_Cache,
  DataCache_Uncache,
  DataCache_EnumCache,
  DataCache_InitCache,
  DataCache_IOleCache2_SetData,
  DataCache_UpdateCache,
  DataCache_DiscardCache
};

static ICOM_VTABLE(IOleCacheControl) DataCache_IOleCacheControl_VTable =
{
  DataCache_IOleCacheControl_QueryInterface,
  DataCache_IOleCacheControl_AddRef,
  DataCache_IOleCacheControl_Release,
  DataCache_OnRun,
  DataCache_OnStop
};

/******************************************************************************
 *              CreateDataCache        [OLE32.54]
 */
HRESULT WINAPI CreateDataCache(
  LPUNKNOWN pUnkOuter, 
  REFCLSID  rclsid, 
  REFIID    riid, 
  LPVOID*   ppvObj)
{
  DataCache* newCache = NULL;
  HRESULT    hr       = S_OK;
  char       xclsid[50];
  char       xriid[50];

  WINE_StringFromCLSID((LPCLSID)rclsid,xclsid);
  WINE_StringFromCLSID((LPCLSID)riid,xriid);

  TRACE(ole, "(%s, %p, %s, %p)\n", xclsid, pUnkOuter, xriid, ppvObj);

  /*
   * Sanity check
   */
  if (ppvObj==0)
    return E_POINTER;

  *ppvObj = 0;

  /*
   * If this cache is constructed for aggregation, make sure
   * the caller is requesting the IUnknown interface.
   * This is necessary because it's the only time the non-delegating
   * IUnknown pointer can be returned to the outside.
   */
  if ( (pUnkOuter!=NULL) && 
       (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) != 0) )
    return CLASS_E_NOAGGREGATION;

  /*
   * Try to construct a new instance of the class.
   */
  newCache = DataCache_Construct(rclsid, 
				 pUnkOuter);

  if (newCache == 0)
    return E_OUTOFMEMORY;

  /*
   * Make sure it supports the interface required by the caller.
   */
  hr = IUnknown_QueryInterface((IUnknown*)&(newCache->lpvtbl2), riid, ppvObj);

  /*
   * Release the reference obtained in the constructor. If
   * the QueryInterface was unsuccessful, it will free the class.
   */
  IUnknown_Release((IUnknown*)&(newCache->lpvtbl2));

  return hr;
}

/*********************************************************
 * Method implementation for DataCache class.
 */
static DataCache* DataCache_Construct(
  REFCLSID  clsid,
  LPUNKNOWN pUnkOuter)
{
  DataCache* newObject = 0;

  /*
   * Allocate space for the object.
   */
  newObject = HeapAlloc(GetProcessHeap(), 0, sizeof(DataCache));

  if (newObject==0)
    return newObject;
  
  /*
   * Initialize the virtual function table.
   */
  newObject->lpvtbl1 = &DataCache_IDataObject_VTable;
  newObject->lpvtbl2 = &DataCache_NDIUnknown_VTable;
  newObject->lpvtbl3 = &DataCache_IPersistStorage_VTable;
  newObject->lpvtbl4 = &DataCache_IViewObject2_VTable;
  newObject->lpvtbl5 = &DataCache_IOleCache2_VTable;
  newObject->lpvtbl6 = &DataCache_IOleCacheControl_VTable;
  
  /*
   * Start with one reference count. The caller of this function 
   * must release the interface pointer when it is done.
   */
  newObject->ref = 1;

  /*
   * Initialize the outer unknown
   * We don't keep a reference on the outer unknown since, the way 
   * aggregation works, our lifetime is at least as large as it's
   * lifetime.
   */
  if (pUnkOuter==NULL)
    pUnkOuter = (IUnknown*)&(newObject->lpvtbl2);

  newObject->outerUnknown = pUnkOuter;

  /*
   * Initialize the other members of the structure.
   */
  newObject->presentationStorage = NULL;

  return newObject;
}

static void DataCache_Destroy(
  DataCache* ptrToDestroy)
{
  if (ptrToDestroy->presentationStorage != NULL)
  {
    IStorage_Release(ptrToDestroy->presentationStorage);
    ptrToDestroy->presentationStorage = NULL;
  }

  /*
   * Free the datacache pointer.
   */
  HeapFree(GetProcessHeap(), 0, ptrToDestroy);
}

/*********************************************************
 * Method implementation for the  non delegating IUnknown
 * part of the DataCache class.
 */

/************************************************************************
 * DefaultHandler_NDIUnknown_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate it's implementation
 * to the outer unknown.
 */
static HRESULT WINAPI DataCache_NDIUnknown_QueryInterface(
            IUnknown*      iface,
            REFIID         riid,
            void**         ppvObject)
{
  _ICOM_THIS_From_NDIUnknown(DataCache, iface);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (this==0) || (ppvObject==0) )
    return E_INVALIDARG;
  
  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;

  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0) 
  {
    *ppvObject = iface;
  }
  else if (memcmp(&IID_IDataObject, riid, sizeof(IID_IDataObject)) == 0) 
  {
    *ppvObject = (IDataObject*)&(this->lpvtbl1);
  }
  else if ( (memcmp(&IID_IPersistStorage, riid, sizeof(IID_IPersistStorage)) == 0)  ||
	    (memcmp(&IID_IPersist, riid, sizeof(IID_IPersist)) == 0) )
  {
    *ppvObject = (IPersistStorage*)&(this->lpvtbl3);
  }
  else if ( (memcmp(&IID_IViewObject, riid, sizeof(IID_IViewObject)) == 0) ||
	    (memcmp(&IID_IViewObject2, riid, sizeof(IID_IViewObject2)) == 0) )
  {
    *ppvObject = (IViewObject2*)&(this->lpvtbl4);
  }
  else if ( (memcmp(&IID_IOleCache, riid, sizeof(IID_IOleCache)) == 0) ||
	    (memcmp(&IID_IOleCache2, riid, sizeof(IID_IOleCache2)) == 0) )
  {
    *ppvObject = (IOleCache2*)&(this->lpvtbl5);
  }
  else if (memcmp(&IID_IOleCacheControl, riid, sizeof(IID_IOleCacheControl)) == 0) 
  {
    *ppvObject = (IOleCacheControl*)&(this->lpvtbl6);
  }

  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
  {
    char clsid[50];

    WINE_StringFromCLSID((LPCLSID)riid,clsid);
    
    WARN(ole, 
	 "() : asking for un supported interface %s\n", 
	 clsid);

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
 * DataCache_NDIUnknown_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate it's implementation
 * to the outer unknown.
 */
static ULONG WINAPI DataCache_NDIUnknown_AddRef( 
            IUnknown*      iface)
{
  _ICOM_THIS_From_NDIUnknown(DataCache, iface);

  this->ref++;

  return this->ref;
}

/************************************************************************
 * DataCache_NDIUnknown_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate it's implementation
 * to the outer unknown.
 */
static ULONG WINAPI DataCache_NDIUnknown_Release( 
            IUnknown*      iface)
{
  _ICOM_THIS_From_NDIUnknown(DataCache, iface);

  /*
   * Decrease the reference count on this object.
   */
  this->ref--;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (this->ref==0)
  {
    DataCache_Destroy(this);

    return 0;
  }
  
  return this->ref;
}

/*********************************************************
 * Method implementation for the IDataObject
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IDataObject_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataCache_IDataObject_QueryInterface(
            IDataObject*     iface,
            REFIID           riid,
            void**           ppvObject)
{
  _ICOM_THIS_From_IDataObject(DataCache, iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);  
}

/************************************************************************
 * DataCache_IDataObject_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IDataObject_AddRef( 
            IDataObject*     iface)
{
  _ICOM_THIS_From_IDataObject(DataCache, iface);

  return IUnknown_AddRef(this->outerUnknown);  
}

/************************************************************************
 * DataCache_IDataObject_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IDataObject_Release( 
            IDataObject*     iface)
{
  _ICOM_THIS_From_IDataObject(DataCache, iface);

  return IUnknown_Release(this->outerUnknown);  
}

static HRESULT WINAPI DataCache_GetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetcIn, 
	    STGMEDIUM*       pmedium)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_GetDataHere(
	    IDataObject*     iface, 
	    LPFORMATETC      pformatetc,
	    STGMEDIUM*       pmedium)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_QueryGetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_EnumFormatEtc (IDataObject)
 *
 * The data cache doesn't implement this method.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_GetCanonicalFormatEtc(
	    IDataObject*     iface, 
	    LPFORMATETC      pformatectIn, 
	    LPFORMATETC      pformatetcOut)
{
  TRACE(ole,"()\n");
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_IDataObject_SetData (IDataObject)
 *
 * This method is delegated to the IOleCache2 implementation.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_IDataObject_SetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc, 
	    STGMEDIUM*       pmedium, 
	    BOOL             fRelease)
{
  IOleCache2* oleCache = NULL;
  HRESULT     hres;

  TRACE(ole,"(%p, %p, %p, %d)\n", iface, pformatetc, pmedium, fRelease);

  hres = IDataObject_QueryInterface(iface, &IID_IOleCache2, (void**)&oleCache);

  if (FAILED(hres))
    return E_UNEXPECTED;

  hres = IOleCache2_SetData(oleCache, pformatetc, pmedium, fRelease);

  IOleCache2_Release(oleCache);

  return hres;;
}

/************************************************************************
 * DataCache_EnumFormatEtc (IDataObject)
 *
 * The data cache doesn't implement this method.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_EnumFormatEtc(
	    IDataObject*     iface,       
	    DWORD            dwDirection,
	    IEnumFORMATETC** ppenumFormatEtc)
{
  TRACE(ole,"()\n");
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_DAdvise (IDataObject)
 *
 * The data cache doesn't support connections.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_DAdvise(
	    IDataObject*     iface, 
	    FORMATETC*       pformatetc, 
	    DWORD            advf, 
	    IAdviseSink*     pAdvSink, 
	    DWORD*           pdwConnection)
{
  TRACE(ole,"()\n");
  return OLE_E_ADVISENOTSUPPORTED;
}

/************************************************************************
 * DataCache_DUnadvise (IDataObject)
 *
 * The data cache doesn't support connections.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_DUnadvise(
	    IDataObject*     iface,
	    DWORD            dwConnection)
{
  TRACE(ole,"()\n");
  return OLE_E_NOCONNECTION;
}

/************************************************************************
 * DataCache_EnumDAdvise (IDataObject)
 *
 * The data cache doesn't support connections.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_EnumDAdvise(
	    IDataObject*     iface,
	    IEnumSTATDATA**  ppenumAdvise)
{
  TRACE(ole,"()\n");
  return OLE_E_ADVISENOTSUPPORTED;
}

/*********************************************************
 * Method implementation for the IDataObject
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IPersistStorage_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataCache_IPersistStorage_QueryInterface(
            IPersistStorage* iface,
            REFIID           riid,
            void**           ppvObject)
{
  _ICOM_THIS_From_IPersistStorage(DataCache, iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);  
}

/************************************************************************
 * DataCache_IPersistStorage_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IPersistStorage_AddRef( 
            IPersistStorage* iface)
{
  _ICOM_THIS_From_IPersistStorage(DataCache, iface);

  return IUnknown_AddRef(this->outerUnknown);  
}

/************************************************************************
 * DataCache_IPersistStorage_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IPersistStorage_Release( 
            IPersistStorage* iface)
{
  _ICOM_THIS_From_IPersistStorage(DataCache, iface);

  return IUnknown_Release(this->outerUnknown);  
}

/************************************************************************
 * DataCache_GetClassID (IPersistStorage)
 *
 * The data cache doesn't implement this method.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_GetClassID( 
            const IPersistStorage* iface,
	    CLSID*           pClassID)
{
  TRACE(ole,"(%p, %p)\n", iface, pClassID);
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_IsDirty( 
            IPersistStorage* iface)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_InitNew (IPersistStorage)
 *
 * The data cache implementation of IPersistStorage_InitNew simply stores
 * the storage pointer.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_InitNew( 
            IPersistStorage* iface, 
	    IStorage*        pStg)
{
  TRACE(ole, "(%p, %p)\n", iface, pStg);

  return DataCache_Load(iface, pStg);
}

/************************************************************************
 * DataCache_Load (IPersistStorage)
 *
 * The data cache implementation of IPersistStorage_Load doesn't 
 * actually load anything. Instead, it holds on to the storage pointer
 * and it will load the presentation information when the 
 * IDataObject_GetData or IViewObject2_Draw methods are called.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_Load( 
            IPersistStorage* iface,
	    IStorage*        pStg)
{
  _ICOM_THIS_From_IPersistStorage(DataCache, iface);

  TRACE(ole, "(%p, %p)\n", iface, pStg);

  if (this->presentationStorage != NULL)
  {
    IStorage_Release(this->presentationStorage);
  }

  this->presentationStorage = pStg;

  if (this->presentationStorage != NULL)
  {
    IStorage_AddRef(this->presentationStorage);
  }

  return S_OK;
}

static HRESULT WINAPI DataCache_Save( 
            IPersistStorage* iface,
	    IStorage*        pStg, 
	    BOOL             fSameAsLoad)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_SaveCompleted (IPersistStorage)
 *
 * This method is called to tell the cache to release the storage
 * pointer it's currentlu holding.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_SaveCompleted( 
            IPersistStorage* iface,  
	    IStorage*        pStgNew)
{
  TRACE(ole, "(%p, %p)\n", iface, pStgNew);

  /*
   * First, make sure we get our hands off any storage we have.
   */
  DataCache_HandsOffStorage(iface);

  /*
   * Then, attach to the new storage.
   */
  DataCache_Load(iface, pStgNew);

  return S_OK;
}

/************************************************************************
 * DataCache_HandsOffStorage (IPersistStorage)
 *
 * This method is called to tell the cache to release the storage
 * pointer it's currentlu holding.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_HandsOffStorage(
            IPersistStorage* iface)
{
  _ICOM_THIS_From_IPersistStorage(DataCache, iface);

  TRACE(ole,"\n");

  if (this->presentationStorage != NULL)
  {
    IStorage_Release(this->presentationStorage);
    this->presentationStorage = NULL;
  }

  return S_OK;
}

/*********************************************************
 * Method implementation for the IViewObject2
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IViewObject2_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataCache_IViewObject2_QueryInterface(
            IViewObject2* iface,
            REFIID           riid,
            void**           ppvObject)
{
  _ICOM_THIS_From_IViewObject2(DataCache, iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);  
}

/************************************************************************
 * DataCache_IViewObject2_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IViewObject2_AddRef( 
            IViewObject2* iface)
{
  _ICOM_THIS_From_IViewObject2(DataCache, iface);

  return IUnknown_AddRef(this->outerUnknown);  
}

/************************************************************************
 * DataCache_IViewObject2_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IViewObject2_Release( 
            IViewObject2* iface)
{
  _ICOM_THIS_From_IViewObject2(DataCache, iface);

  return IUnknown_Release(this->outerUnknown);  
}

static HRESULT WINAPI DataCache_Draw(
            IViewObject2*    iface,
	    DWORD            dwDrawAspect,
	    LONG             lindex,
	    void*            pvAspect,
	    DVTARGETDEVICE*  ptd, 
	    HDC              hdcTargetDev, 
	    HDC              hdcDraw,
	    LPCRECTL         lprcBounds,
	    LPCRECTL         lprcWBounds,
	    IVO_ContCallback pfnContinue,
	    DWORD            dwContinue)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_GetColorSet(
            IViewObject2*   iface, 
	    DWORD           dwDrawAspect, 
	    LONG            lindex, 
	    void*           pvAspect, 
	    DVTARGETDEVICE* ptd, 
	    HDC             hicTargetDevice, 
	    LOGPALETTE**    ppColorSet)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_Freeze(
            IViewObject2*   iface,
	    DWORD           dwDrawAspect,
	    LONG            lindex,
	    void*           pvAspect, 
	    DWORD*          pdwFreeze)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_Unfreeze(
            IViewObject2*   iface,
	    DWORD           dwFreeze)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_SetAdvise(
            IViewObject2*   iface,
	    DWORD           aspects, 
	    DWORD           advf, 
	    IAdviseSink*    pAdvSink)
{
  FIXME(ole,"stub\n");
  return S_OK;
}

static HRESULT WINAPI DataCache_GetAdvise(
            IViewObject2*   iface, 
	    DWORD*          pAspects, 
	    DWORD*          pAdvf, 
	    IAdviseSink**   ppAdvSink)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_GetExtent(
            IViewObject2*   iface, 
	    DWORD           dwDrawAspect, 
	    LONG            lindex, 
	    DVTARGETDEVICE* ptd, 
	    LPSIZEL         lpsizel)
{
  lpsizel->cx = 5000;
  lpsizel->cy = 5000;

  FIXME(ole,"stub\n");

  return S_OK;
}


/*********************************************************
 * Method implementation for the IOleCache2
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IOleCache2_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataCache_IOleCache2_QueryInterface(
            IOleCache2*     iface,
            REFIID          riid,
            void**          ppvObject)
{
  _ICOM_THIS_From_IOleCache2(DataCache, iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);  
}

/************************************************************************
 * DataCache_IOleCache2_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCache2_AddRef( 
            IOleCache2*     iface)
{
  _ICOM_THIS_From_IOleCache2(DataCache, iface);

  return IUnknown_AddRef(this->outerUnknown);  
}

/************************************************************************
 * DataCache_IOleCache2_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCache2_Release( 
            IOleCache2*     iface)
{
  _ICOM_THIS_From_IOleCache2(DataCache, iface);

  return IUnknown_Release(this->outerUnknown);  
}

static HRESULT WINAPI DataCache_Cache(
            IOleCache2*     iface,
	    FORMATETC*      pformatetc,
	    DWORD           advf,
	    DWORD*          pdwConnection)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_Uncache(
	    IOleCache2*     iface,
	    DWORD           dwConnection)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_EnumCache(
            IOleCache2*     iface,
	    IEnumSTATDATA** ppenumSTATDATA)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_InitCache(
	    IOleCache2*     iface,
	    IDataObject*    pDataObject)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_IOleCache2_SetData(
            IOleCache2*     iface,
	    FORMATETC*      pformatetc,
	    STGMEDIUM*      pmedium,
	    BOOL            fRelease)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_UpdateCache(
            IOleCache2*     iface,
	    LPDATAOBJECT    pDataObject, 
	    DWORD           grfUpdf,
	    LPVOID          pReserved)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_DiscardCache(
            IOleCache2*     iface,
	    DWORD           dwDiscardOptions)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}


/*********************************************************
 * Method implementation for the IOleCacheControl
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IOleCacheControl_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataCache_IOleCacheControl_QueryInterface(
            IOleCacheControl* iface,
            REFIID            riid,
            void**            ppvObject)
{
  _ICOM_THIS_From_IOleCacheControl(DataCache, iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);  
}

/************************************************************************
 * DataCache_IOleCacheControl_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCacheControl_AddRef( 
            IOleCacheControl* iface)
{
  _ICOM_THIS_From_IOleCacheControl(DataCache, iface);

  return IUnknown_AddRef(this->outerUnknown);  
}

/************************************************************************
 * DataCache_IOleCacheControl_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCacheControl_Release( 
            IOleCacheControl* iface)
{
  _ICOM_THIS_From_IOleCacheControl(DataCache, iface);

  return IUnknown_Release(this->outerUnknown);  
}

static HRESULT WINAPI DataCache_OnRun(
	    IOleCacheControl* iface,
	    LPDATAOBJECT      pDataObject)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_OnStop(
	    IOleCacheControl* iface)
{
  FIXME(ole,"stub\n");
  return E_NOTIMPL;
}


