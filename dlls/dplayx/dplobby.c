/* Direct Play Lobby 2 & 3 Implementation
 *
 * Copyright 1998,1999,2000 - Peter Hunnisett
 *
 * <presently under construction - contact hunnise@nortelnetworks.com>
 *
 */
#include <string.h>

#include "winerror.h"
#include "winnt.h"
#include "winreg.h"
#include "heap.h"
#include "debugtools.h"

#include "dplobby.h"
#include "dpinit.h"
#include "dplayx_global.h"

DEFAULT_DEBUG_CHANNEL(dplay)


/* Forward declarations for this module helper methods */
HRESULT DPL_CreateCompoundAddress ( LPCDPCOMPOUNDADDRESSELEMENT lpElements, DWORD dwElementCount,
                                    LPVOID lpAddress, LPDWORD lpdwAddressSize, BOOL bAnsiInterface );

HRESULT DPL_CreateAddress( REFGUID guidSP, REFGUID guidDataType, LPCVOID lpData, DWORD dwDataSize, 
                           LPVOID lpAddress, LPDWORD lpdwAddressSize, BOOL bAnsiInterface );



static HRESULT DPL_EnumAddress( LPDPENUMADDRESSCALLBACK lpEnumAddressCallback, LPCVOID lpAddress,
                                DWORD dwAddressSize, LPVOID lpContext );


/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectPlayLobbyImpl  IDirectPlayLobbyAImpl;
typedef struct IDirectPlayLobbyImpl  IDirectPlayLobbyWImpl;
typedef struct IDirectPlayLobby2Impl IDirectPlayLobby2AImpl;
typedef struct IDirectPlayLobby2Impl IDirectPlayLobby2WImpl;
typedef struct IDirectPlayLobby3Impl IDirectPlayLobby3AImpl;
typedef struct IDirectPlayLobby3Impl IDirectPlayLobby3WImpl;

/*****************************************************************************
 * IDirectPlayLobby {1,2,3} implementation structure
 * 
 * The philosophy behind this extra pointer derefernce is that I wanted to 
 * have the same structure for all types of objects without having to do
 * alot of casting. I also only wanted to implement an interface in the 
 * object it was "released" with IUnknown interface being implemented in the 1 version.
 * Of course, with these new interfaces comes the data required to keep the state required
 * by these interfaces. So, basically, the pointers contain the data associated with
 * a release. If you use the data associated with release 3 in a release 2 object, you'll
 * get a run time trap, as that won't have any data.
 *
 */

typedef struct tagDirectPlayLobbyIUnknownData
{
  DWORD             ref;
  CRITICAL_SECTION  DPL_lock;
} DirectPlayLobbyIUnknownData;

typedef struct tagDirectPlayLobbyData
{
  HKEY hkCallbackKeyHack;
} DirectPlayLobbyData;

typedef struct tagDirectPlayLobby2Data
{
  BOOL dummy;
} DirectPlayLobby2Data;

typedef struct tagDirectPlayLobby3Data
{
  BOOL dummy;
} DirectPlayLobby3Data;

#define DPL_IMPL_FIELDS \
 DirectPlayLobbyIUnknownData*  unk; \
 DirectPlayLobbyData*          dpl; \
 DirectPlayLobby2Data*         dpl2; \
 DirectPlayLobby3Data*         dpl3;

struct IDirectPlayLobbyImpl
{
    ICOM_VFIELD(IDirectPlayLobby);
    DPL_IMPL_FIELDS
};

struct IDirectPlayLobby2Impl
{
    ICOM_VFIELD(IDirectPlayLobby2);
    DPL_IMPL_FIELDS
};

struct IDirectPlayLobby3Impl
{
    ICOM_VFIELD(IDirectPlayLobby3);
    DPL_IMPL_FIELDS
};


/* Forward declarations of virtual tables */
static ICOM_VTABLE(IDirectPlayLobby)  directPlayLobbyWVT;
static ICOM_VTABLE(IDirectPlayLobby2) directPlayLobby2WVT;
static ICOM_VTABLE(IDirectPlayLobby3) directPlayLobby3WVT;

static ICOM_VTABLE(IDirectPlayLobby)  directPlayLobbyAVT;
static ICOM_VTABLE(IDirectPlayLobby2) directPlayLobby2AVT;
static ICOM_VTABLE(IDirectPlayLobby3) directPlayLobby3AVT;




/* The COM interface for upversioning an interface
 * We've been given a GUID (riid) and we need to replace the present
 * interface with that of the requested interface.
 *
 * Snip from some Microsoft document:
 * There are four requirements for implementations of QueryInterface (In these
 * cases, "must succeed" means "must succeed barring catastrophic failure."):
 *
 *  * The set of interfaces accessible on an object through
 *    IUnknown::QueryInterface must be static, not dynamic. This means that
 *    if a call to QueryInterface for a pointer to a specified interface
 *    succeeds the first time, it must succeed again, and if it fails the
 *    first time, it must fail on all subsequent queries.
 *  * It must be symmetric ~W if a client holds a pointer to an interface on
 *    an object, and queries for that interface, the call must succeed.
 *  * It must be reflexive ~W if a client holding a pointer to one interface
 *    queries successfully for another, a query through the obtained pointer
 *    for the first interface must succeed.
 *  * It must be transitive ~W if a client holding a pointer to one interface
 *    queries successfully for a second, and through that pointer queries
 *    successfully for a third interface, a query for the first interface
 *    through the pointer for the third interface must succeed.
 *
 *  As you can see, this interface doesn't qualify but will most likely
 *  be good enough for the time being.
 */


BOOL DPL_CreateIUnknown( LPVOID lpDPL ) 
{
  ICOM_THIS(IDirectPlayLobbyAImpl,lpDPL);

  This->unk = (DirectPlayLobbyIUnknownData*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                                       sizeof( *(This->unk) ) ); 
  if ( This->unk == NULL )
  {
    return FALSE; 
  }

  InitializeCriticalSection( &This->unk->DPL_lock );

  IDirectPlayLobby_AddRef( (LPDIRECTPLAYLOBBYA)lpDPL );

  return TRUE;
}

BOOL DPL_DestroyIUnknown( LPVOID lpDPL )
{
  ICOM_THIS(IDirectPlayLobbyAImpl,lpDPL);

  DeleteCriticalSection( &This->unk->DPL_lock );
  HeapFree( GetProcessHeap(), 0, This->unk );

  return TRUE;
}

BOOL DPL_CreateLobby1( LPVOID lpDPL )
{
  ICOM_THIS(IDirectPlayLobbyAImpl,lpDPL);

  This->dpl = (DirectPlayLobbyData*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                               sizeof( *(This->dpl) ) );
  if ( This->dpl == NULL )
  {
    return FALSE;
  }

  return TRUE;  
}

BOOL DPL_DestroyLobby1( LPVOID lpDPL )
{
  ICOM_THIS(IDirectPlayLobbyAImpl,lpDPL);

  /* Delete the contents */
  HeapFree( GetProcessHeap(), 0, This->dpl );

  return TRUE;
}

BOOL DPL_CreateLobby2( LPVOID lpDPL )
{
  ICOM_THIS(IDirectPlayLobby2AImpl,lpDPL);

  This->dpl2 = (DirectPlayLobby2Data*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                 sizeof( *(This->dpl2) ) );
  if ( This->dpl2 == NULL )
  {
    return FALSE;
  }

  return TRUE;
}

BOOL DPL_DestroyLobby2( LPVOID lpDPL )
{
  ICOM_THIS(IDirectPlayLobby2AImpl,lpDPL);

  HeapFree( GetProcessHeap(), 0, This->dpl2 );

  return TRUE;
}

BOOL DPL_CreateLobby3( LPVOID lpDPL )
{
  ICOM_THIS(IDirectPlayLobby3AImpl,lpDPL);

  This->dpl3 = (DirectPlayLobby3Data*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                 sizeof( *(This->dpl3) ) );
  if ( This->dpl3 == NULL )
  {
    return FALSE;
  }

  return TRUE;
}

BOOL DPL_DestroyLobby3( LPVOID lpDPL )
{
  ICOM_THIS(IDirectPlayLobby3AImpl,lpDPL);

  HeapFree( GetProcessHeap(), 0, This->dpl3 );

  return TRUE;
}


/* Helper function for DirectPlayLobby  QueryInterface */ 
extern 
HRESULT directPlayLobby_QueryInterface
         ( REFIID riid, LPVOID* ppvObj )
{
  if( IsEqualGUID( &IID_IDirectPlayLobby, riid ) )
  {
    *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof( IDirectPlayLobbyWImpl ) );

    if( *ppvObj == NULL )
    {
      return E_OUTOFMEMORY;
    }
    
    /* new scope for variable declaration */
    {
      ICOM_THIS(IDirectPlayLobbyWImpl,*ppvObj);

      ICOM_VTBL(This) = &directPlayLobbyWVT;

      if ( DPL_CreateIUnknown( (LPVOID)This ) &&
           DPL_CreateLobby1( (LPVOID)This ) 
         )
      { 
        return S_OK;
      }

    }

    goto error;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobbyA, riid ) )
  {
    *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof( IDirectPlayLobbyAImpl ) );

    if( *ppvObj == NULL )
    {
      return E_OUTOFMEMORY;
    }

    {
      ICOM_THIS(IDirectPlayLobbyAImpl,*ppvObj);

      ICOM_VTBL(This) = &directPlayLobbyAVT;

      if ( DPL_CreateIUnknown( (LPVOID)This ) &&
           DPL_CreateLobby1( (LPVOID)This ) 
         )
      { 
        return S_OK;
      }
    }

    goto error;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2, riid ) )
  {
    *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof( IDirectPlayLobby2WImpl ) );

    if( *ppvObj == NULL )
    {
      return E_OUTOFMEMORY;
    }

    {
      ICOM_THIS(IDirectPlayLobby2WImpl,*ppvObj);

      ICOM_VTBL(This) = &directPlayLobby2WVT;

      if ( DPL_CreateIUnknown( (LPVOID)This ) &&
           DPL_CreateLobby1( (LPVOID)This ) &&
           DPL_CreateLobby2( (LPVOID)This )
         )
      {
        return S_OK;
      }
    }

    goto error;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2A, riid ) )
  {
    *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof( IDirectPlayLobby2AImpl ) );

    if( *ppvObj == NULL )
    {
      return E_OUTOFMEMORY;
    }

    {
      ICOM_THIS(IDirectPlayLobby2AImpl,*ppvObj);

      ICOM_VTBL(This) = &directPlayLobby2AVT;

      if ( DPL_CreateIUnknown( (LPVOID)This ) &&
           DPL_CreateLobby1( (LPVOID)This )  &&
           DPL_CreateLobby2( (LPVOID)This )
         )
      {
        return S_OK;
      }
    }

    goto error;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby3, riid ) )
  {
    *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof( IDirectPlayLobby3WImpl ) );

    if( *ppvObj == NULL )
    {
      return E_OUTOFMEMORY;
    }

    {
      ICOM_THIS(IDirectPlayLobby3WImpl,*ppvObj);

      ICOM_VTBL(This) = &directPlayLobby3WVT;

      if ( DPL_CreateIUnknown( *ppvObj ) &&
           DPL_CreateLobby1( *ppvObj ) &&
           DPL_CreateLobby2( *ppvObj ) &&
           DPL_CreateLobby3( *ppvObj )
         )
      {
        return S_OK;
      }
    }    
 
    goto error;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby3A, riid ) )
  {
     *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                          sizeof( IDirectPlayLobby3AImpl ) );

    if( *ppvObj == NULL )
    {
      return E_OUTOFMEMORY;
    }

    {
      ICOM_THIS(IDirectPlayLobby3AImpl,*ppvObj);

      ICOM_VTBL(This) = &directPlayLobby3AVT;

      if ( DPL_CreateIUnknown( *ppvObj ) &&
           DPL_CreateLobby1( *ppvObj ) &&
           DPL_CreateLobby2( *ppvObj ) &&
           DPL_CreateLobby3( *ppvObj )
         )
      {
        return S_OK;
      }
    }

    goto error;
  }

  /* Unsupported interface */
  *ppvObj = NULL;
  return E_NOINTERFACE;

error:

    DPL_DestroyLobby3( *ppvObj );
    DPL_DestroyLobby2( *ppvObj );
    DPL_DestroyLobby1( *ppvObj );
    DPL_DestroyIUnknown( *ppvObj );
    HeapFree( GetProcessHeap(), 0, *ppvObj );

    *ppvObj = NULL;
    return DPERR_NOMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyAImpl_QueryInterface
( LPDIRECTPLAYLOBBYA iface,
  REFIID riid,
  LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlayLobbyAImpl,iface);
  TRACE("(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid )  ||
      IsEqualGUID( &IID_IDirectPlayLobbyA, riid )
    )
  {
    IDirectPlayLobby_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlayLobby_QueryInterface( riid, ppvObj );

}

static HRESULT WINAPI IDirectPlayLobbyW_QueryInterface
( LPDIRECTPLAYLOBBY iface,
  REFIID riid,
  LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlayLobbyWImpl,iface);
  TRACE("(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid )  ||
      IsEqualGUID( &IID_IDirectPlayLobby, riid )
    )
  {
    IDirectPlayLobby_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlayLobby_QueryInterface( riid, ppvObj );
}


static HRESULT WINAPI IDirectPlayLobby2AImpl_QueryInterface
( LPDIRECTPLAYLOBBY2A iface,
  REFIID riid,
  LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlayLobby2AImpl,iface);
  TRACE("(%p)->(%p,%p)\n", This, riid, ppvObj );

  /* Compare riids. We know this object is a direct play lobby 2A object.
     If we are asking about the same type of interface we're fine.
   */
  if( IsEqualGUID( &IID_IUnknown, riid )  ||
      IsEqualGUID( &IID_IDirectPlayLobby2A, riid )
    )
  {
    IDirectPlayLobby_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }
  return directPlayLobby_QueryInterface( riid, ppvObj ); 
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_QueryInterface
( LPDIRECTPLAYLOBBY2 iface,
  REFIID riid,
  LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlayLobby2WImpl,iface);

  /* Compare riids. We know this object is a direct play lobby 2 object.
     If we are asking about the same type of interface we're fine.
   */
  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlayLobby2, riid ) 
    )
  {
    IDirectPlayLobby_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlayLobby_QueryInterface( riid, ppvObj ); 

}

static HRESULT WINAPI IDirectPlayLobby3AImpl_QueryInterface
( LPDIRECTPLAYLOBBY3A iface,
  REFIID riid,
  LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlayLobby3AImpl,iface);

  /* Compare riids. We know this object is a direct play lobby 3 object.
     If we are asking about the same type of interface we're fine.
   */
  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlayLobby3A, riid )
    )
  {
    IDirectPlayLobby_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlayLobby_QueryInterface( riid, ppvObj );

}

static HRESULT WINAPI IDirectPlayLobby3WImpl_QueryInterface
( LPDIRECTPLAYLOBBY3 iface,
  REFIID riid,
  LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlayLobby3WImpl,iface);

  /* Compare riids. We know this object is a direct play lobby 3 object.
     If we are asking about the same type of interface we're fine.
   */
  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlayLobby3, riid )
    )
  {
    IDirectPlayLobby_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlayLobby_QueryInterface( riid, ppvObj );

}

/* 
 * Simple procedure. Just increment the reference count to this
 * structure and return the new reference count.
 */
static ULONG WINAPI IDirectPlayLobbyImpl_AddRef
( LPDIRECTPLAYLOBBY iface )
{
  ULONG refCount;
  ICOM_THIS(IDirectPlayLobbyWImpl,iface);

  EnterCriticalSection( &This->unk->DPL_lock );
  {
    refCount = ++(This->unk->ref);
  }
  LeaveCriticalSection( &This->unk->DPL_lock );

  TRACE("ref count incremented to %lu for %p\n", refCount, This );

  return refCount;
}

/*
 * Simple COM procedure. Decrease the reference count to this object.
 * If the object no longer has any reference counts, free up the associated
 * memory.
 */
static ULONG WINAPI IDirectPlayLobbyAImpl_Release
( LPDIRECTPLAYLOBBYA iface )
{
  ULONG refCount;

  ICOM_THIS(IDirectPlayLobbyAImpl,iface);

  EnterCriticalSection( &This->unk->DPL_lock );
  {
    refCount = --(This->unk->ref);
  }
  LeaveCriticalSection( &This->unk->DPL_lock );

  TRACE("ref count decremeneted to %lu for %p\n", refCount, This );

  /* Deallocate if this is the last reference to the object */
  if( refCount )
  {
     DPL_DestroyLobby3( This );
     DPL_DestroyLobby2( This );
     DPL_DestroyLobby1( This );
     DPL_DestroyIUnknown( This );
     HeapFree( GetProcessHeap(), 0, This );
  }

  return refCount;
}


/********************************************************************
 * 
 * Connects an application to the session specified by the DPLCONNECTION
 * structure currently stored with the DirectPlayLobby object.
 *
 * Returns a IDirectPlay interface.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_Connect
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  LPDIRECTPLAY2A* lplpDP,
  IUnknown* pUnk)
{
  ICOM_THIS(IDirectPlayLobbyAImpl,iface);

  LPDIRECTPLAY2A      lpDirectPlay2A;
  /* LPDIRECTPLAY3A      lpDirectPlay3A; */
  /* LPDIRECTPLAYLOBBY2A lpDirectPlayLobby2A; */
  HRESULT             rc;

  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, dwFlags, lplpDP, pUnk );

  if( dwFlags || pUnk )
  {
     return DPERR_INVALIDPARAMS;
  }

  /* Create the DirectPlay interface */
  if( ( rc = directPlay_QueryInterface( &IID_IDirectPlay2A, (LPVOID*)lplpDP ) ) != DP_OK )
  {
     ERR("error creating Direct Play 2A interface. Return Code = 0x%lx.\n", rc );
     return rc;
  }

  lpDirectPlay2A = *lplpDP;

  /* - Need to call IDirectPlay::EnumConnections with the service provider to get that good information
   * - Need to call CreateAddress to create the lpConnection param for IDirectPlay::InitializeConnection
   * - Call IDirectPlay::InitializeConnection
   * - Call IDirectPlay::Open 
   */
#if 0
  IDirectPlayLobby_EnumAddress( iface, RunApplicationA_Callback, 
                                lpConn->lpAddress, lpConn->dwAddressSize, NULL );
#endif


  return DP_OK;

}

static HRESULT WINAPI IDirectPlayLobbyWImpl_Connect
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  LPDIRECTPLAY2* lplpDP,
  IUnknown* pUnk)
{
  ICOM_THIS(IDirectPlayLobbyWImpl,iface);
  LPDIRECTPLAY2* directPlay2W;
  HRESULT        createRC;

  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, dwFlags, lplpDP, pUnk );

  if( dwFlags || pUnk )
  {
     return DPERR_INVALIDPARAMS;
  }

  /* Create the DirectPlay interface */
  if( ( createRC = directPlay_QueryInterface( &IID_IDirectPlay2, (LPVOID*)lplpDP ) ) != DP_OK )
  {
     ERR("error creating Direct Play 2W interface. Return Code = 0x%lx.\n", createRC );
     return createRC;
  } 

  /* This should invoke IDirectPlay3::InitializeConnection IDirectPlay3::Open */  
  directPlay2W = lplpDP; 
 
  return DP_OK;

}

/********************************************************************
 *
 * Creates a DirectPlay Address, given a service provider-specific network
 * address. 
 * Returns an address contains the globally unique identifier
 * (GUID) of the service provider and data that the service provider can
 * interpret as a network address.
 *
 * NOTE: It appears that this method is supposed to be really really stupid
 *       with no error checking on the contents.
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_CreateAddress
( LPDIRECTPLAYLOBBYA iface,
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData, 
  DWORD dwDataSize,
  LPVOID lpAddress, 
  LPDWORD lpdwAddressSize )
{
  return DPL_CreateAddress( guidSP, guidDataType, lpData, dwDataSize, 
                            lpAddress, lpdwAddressSize, TRUE ); 
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_CreateAddress
( LPDIRECTPLAYLOBBY iface,
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData,
  DWORD dwDataSize,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  return DPL_CreateAddress( guidSP, guidDataType, lpData, dwDataSize,
                            lpAddress, lpdwAddressSize, FALSE );
}

HRESULT DPL_CreateAddress(
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData,
  DWORD dwDataSize,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize,
  BOOL bAnsiInterface )
{
  const DWORD dwNumAddElements = 2; /* Service Provide & address data type */
  DPCOMPOUNDADDRESSELEMENT addressElements[ 2 /* dwNumAddElements */ ];

  TRACE( "(%p)->(%p,%p,0x%08lx,%p,%p,%d)\n", guidSP, guidDataType, lpData, dwDataSize, 
                                             lpAddress, lpdwAddressSize, bAnsiInterface );

  addressElements[ 0 ].guidDataType = DPAID_ServiceProvider;
  addressElements[ 0 ].dwDataSize = sizeof( GUID );
  addressElements[ 0 ].lpData = (LPVOID)guidSP;

  addressElements[ 1 ].guidDataType = *guidDataType;
  addressElements[ 1 ].dwDataSize = dwDataSize;
  addressElements[ 1 ].lpData = (LPVOID)lpData;

  /* Call CreateCompoundAddress to cut down on code.
     NOTE: We can do this because we don't support DPL 1 interfaces! */
  return DPL_CreateCompoundAddress( addressElements, dwNumAddElements,
                                    lpAddress, lpdwAddressSize, bAnsiInterface );
}



/********************************************************************
 *
 * Parses out chunks from the DirectPlay Address buffer by calling the
 * given callback function, with lpContext, for each of the chunks.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_EnumAddress
( LPDIRECTPLAYLOBBYA iface,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext )
{
  ICOM_THIS(IDirectPlayLobbyAImpl,iface);

  TRACE("(%p)->(%p,%p,0x%08lx,%p)\n", This, lpEnumAddressCallback, lpAddress, 
                                      dwAddressSize, lpContext );

  return DPL_EnumAddress( lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );
}
  
static HRESULT WINAPI IDirectPlayLobbyWImpl_EnumAddress
( LPDIRECTPLAYLOBBY iface,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext )
{
  ICOM_THIS(IDirectPlayLobbyWImpl,iface);

  TRACE("(%p)->(%p,%p,0x%08lx,%p)\n", This, lpEnumAddressCallback, lpAddress,     
                                      dwAddressSize, lpContext );

  return DPL_EnumAddress( lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );
}

static HRESULT DPL_EnumAddress( LPDPENUMADDRESSCALLBACK lpEnumAddressCallback, LPCVOID lpAddress,
                                DWORD dwAddressSize, LPVOID lpContext )
{ 
  DWORD dwTotalSizeEnumerated = 0;

  /* FIXME: First chunk is always the total size chunk - Should we report it? */ 

  while ( dwTotalSizeEnumerated < dwAddressSize )
  {
    LPDPADDRESS lpElements = (LPDPADDRESS)lpAddress;
    DWORD dwSizeThisEnumeration; 

    /* Invoke the enum method. If false is returned, stop enumeration */
    if ( !lpEnumAddressCallback( &lpElements->guidDataType, lpElements->dwDataSize, 
                                 lpElements + sizeof( DPADDRESS ), lpContext ) )
    {
      break;
    }

    dwSizeThisEnumeration  = sizeof( DPADDRESS ) + lpElements->dwDataSize;
    lpAddress = (char *) lpAddress + dwSizeThisEnumeration;
    dwTotalSizeEnumerated += dwSizeThisEnumeration;
  }

  return DP_OK;
}

/********************************************************************
 *
 * Enumerates all the address types that a given service provider needs to
 * build the DirectPlay Address.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_EnumAddressTypes
( LPDIRECTPLAYLOBBYA iface,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP,
  LPVOID lpContext,
  DWORD dwFlags )
{
  ICOM_THIS(IDirectPlayLobbyAImpl,iface);

  HKEY   hkResult;
  LPCSTR searchSubKey    = "SOFTWARE\\Microsoft\\DirectPlay\\Service Providers";
  DWORD  dwIndex, sizeOfSubKeyName=50;
  char   subKeyName[51];
  FILETIME filetime;

  TRACE(" (%p)->(%p,%p,%p,0x%08lx)\n", This, lpEnumAddressTypeCallback, guidSP, lpContext, dwFlags );

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDPARAMS;
  }

  if( !lpEnumAddressTypeCallback || !*lpEnumAddressTypeCallback )
  {
     return DPERR_INVALIDPARAMS;
  }

  if( guidSP == NULL )
  {
    return DPERR_INVALIDOBJECT;
  }

    /* Need to loop over the service providers in the registry */
    if( RegOpenKeyExA( HKEY_LOCAL_MACHINE, searchSubKey,
                         0, KEY_READ, &hkResult ) != ERROR_SUCCESS )
    {
      /* Hmmm. Does this mean that there are no service providers? */
      ERR(": no service providers?\n");
      return DP_OK;
    }

    /* Traverse all the service providers we have available */
    for( dwIndex=0;
         RegEnumKeyExA( hkResult, dwIndex, subKeyName, &sizeOfSubKeyName,
                        NULL, NULL, NULL, &filetime ) != ERROR_NO_MORE_ITEMS;
         ++dwIndex, sizeOfSubKeyName=50 )
    {

      HKEY     hkServiceProvider, hkServiceProviderAt;
      GUID     serviceProviderGUID;
      DWORD    returnTypeGUID, sizeOfReturnBuffer = 50;
      char     atSubKey[51];
      char     returnBuffer[51];
      LPWSTR   lpWGUIDString;
      DWORD    dwAtIndex;
      LPSTR    atKey = "Address Types";
      LPSTR    guidDataSubKey   = "Guid";
      FILETIME filetime;


      TRACE(" this time through: %s\n", subKeyName );

      /* Get a handle for this particular service provider */
      if( RegOpenKeyExA( hkResult, subKeyName, 0, KEY_READ,
                         &hkServiceProvider ) != ERROR_SUCCESS )
      {
         ERR(": what the heck is going on?\n" );
         continue;
      }

      if( RegQueryValueExA( hkServiceProvider, guidDataSubKey,
                            NULL, &returnTypeGUID, returnBuffer,
                            &sizeOfReturnBuffer ) != ERROR_SUCCESS )
      {
        ERR(": missing GUID registry data members\n" );
        continue;
      }

      /* FIXME: Check return types to ensure we're interpreting data right */
      lpWGUIDString = HEAP_strdupAtoW( GetProcessHeap(), 0, returnBuffer );
      CLSIDFromString( (LPCOLESTR)lpWGUIDString, &serviceProviderGUID );
      HeapFree( GetProcessHeap(), 0, lpWGUIDString );
      /* FIXME: Have I got a memory leak on the serviceProviderGUID? */

      /* Determine if this is the Service Provider that the user asked for */
      if( !IsEqualGUID( &serviceProviderGUID, guidSP ) )
      {
        continue;
      }

      /* Get a handle for this particular service provider */
      if( RegOpenKeyExA( hkServiceProvider, atKey, 0, KEY_READ,
                         &hkServiceProviderAt ) != ERROR_SUCCESS )
      {
        TRACE(": No Address Types registry data sub key/members\n" );
        break;
      }

      /* Traverse all the address type we have available */
      for( dwAtIndex=0;
           RegEnumKeyExA( hkServiceProviderAt, dwAtIndex, atSubKey, &sizeOfSubKeyName,
                          NULL, NULL, NULL, &filetime ) != ERROR_NO_MORE_ITEMS;
           ++dwAtIndex, sizeOfSubKeyName=50 )
      {
        TRACE( "Found Address Type GUID %s\n", atSubKey );

        /* FIXME: Check return types to ensure we're interpreting data right */
        lpWGUIDString = HEAP_strdupAtoW( GetProcessHeap(), 0, atSubKey );
        CLSIDFromString( (LPCOLESTR)lpWGUIDString, &serviceProviderGUID );
        HeapFree( GetProcessHeap(), 0, lpWGUIDString );
        /* FIXME: Have I got a memory leak on the serviceProviderGUID? */
    
        /* The enumeration will return FALSE if we are not to continue */
        if( !lpEnumAddressTypeCallback( &serviceProviderGUID, lpContext, 0 ) )
        {
           WARN("lpEnumCallback returning FALSE\n" );
           break; /* FIXME: This most likely has to break from the procedure...*/
        }

      }

      /* We only enumerate address types for 1 GUID. We've found it, so quit looking */
      break;
    } 

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_EnumAddressTypes
( LPDIRECTPLAYLOBBY iface,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Enumerates what applications are registered with DirectPlay by
 * invoking the callback function with lpContext.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyWImpl_EnumLocalApplications
( LPDIRECTPLAYLOBBY iface,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK lpEnumLocalAppCallback,
  LPVOID lpContext,
  DWORD dwFlags )
{
  ICOM_THIS(IDirectPlayLobbyWImpl,iface);

  FIXME("(%p)->(%p,%p,0x%08lx):stub\n", This, lpEnumLocalAppCallback, lpContext, dwFlags );

  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyAImpl_EnumLocalApplications
( LPDIRECTPLAYLOBBYA iface,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK lpEnumLocalAppCallback,
  LPVOID lpContext,
  DWORD dwFlags )
{
  ICOM_THIS(IDirectPlayLobbyAImpl,iface);

  HKEY hkResult;
  LPCSTR searchSubKey    = "SOFTWARE\\Microsoft\\DirectPlay\\Applications";
  LPSTR guidDataSubKey   = "Guid";
  DWORD dwIndex, sizeOfSubKeyName=50;
  char subKeyName[51];
  FILETIME filetime;

  TRACE("(%p)->(%p,%p,0x%08lx)\n", This, lpEnumLocalAppCallback, lpContext, dwFlags );

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDPARAMS;
  }

  if( !lpEnumLocalAppCallback || !*lpEnumLocalAppCallback )
  {
     return DPERR_INVALIDPARAMS;
  }

  /* Need to loop over the service providers in the registry */
  if( RegOpenKeyExA( HKEY_LOCAL_MACHINE, searchSubKey,
                     0, KEY_READ, &hkResult ) != ERROR_SUCCESS )
  {
    /* Hmmm. Does this mean that there are no service providers? */
    ERR(": no service providers?\n");
    return DP_OK;
  }

  /* Traverse all registered applications */
  for( dwIndex=0;
       RegEnumKeyExA( hkResult, dwIndex, subKeyName, &sizeOfSubKeyName, NULL, NULL, NULL, &filetime ) != ERROR_NO_MORE_ITEMS;
       ++dwIndex, sizeOfSubKeyName=50 )
  {

    HKEY       hkServiceProvider;
    GUID       serviceProviderGUID;
    DWORD      returnTypeGUID, sizeOfReturnBuffer = 50;
    char       returnBuffer[51];
    LPWSTR     lpWGUIDString;
    DPLAPPINFO dplAppInfo;

    TRACE(" this time through: %s\n", subKeyName );

    /* Get a handle for this particular service provider */
    if( RegOpenKeyExA( hkResult, subKeyName, 0, KEY_READ,
                       &hkServiceProvider ) != ERROR_SUCCESS )
    {
       ERR(": what the heck is going on?\n" );
       continue;
    }

    if( RegQueryValueExA( hkServiceProvider, guidDataSubKey,
                          NULL, &returnTypeGUID, returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
      ERR(": missing GUID registry data members\n" );
      continue;
    }

    /* FIXME: Check return types to ensure we're interpreting data right */
    lpWGUIDString = HEAP_strdupAtoW( GetProcessHeap(), 0, returnBuffer );
    CLSIDFromString( (LPCOLESTR)lpWGUIDString, &serviceProviderGUID );
    HeapFree( GetProcessHeap(), 0, lpWGUIDString );
    /* FIXME: Have I got a memory leak on the serviceProviderGUID? */

    dplAppInfo.dwSize               = sizeof( dplAppInfo );
    dplAppInfo.guidApplication      = serviceProviderGUID;
    dplAppInfo.appName.lpszAppNameA = subKeyName; 

    EnterCriticalSection( &This->unk->DPL_lock );
 
    memcpy( &This->dpl->hkCallbackKeyHack, &hkServiceProvider, sizeof( hkServiceProvider ) );

    if( !lpEnumLocalAppCallback( &dplAppInfo, lpContext, dwFlags ) )
    {
       LeaveCriticalSection( &This->unk->DPL_lock );
       break;
    }

    LeaveCriticalSection( &This->unk->DPL_lock );
  }

  return DP_OK;
}

/********************************************************************
 *
 * Retrieves the DPLCONNECTION structure that contains all the information
 * needed to start and connect an application. This was generated using
 * either the RunApplication or SetConnectionSettings methods.
 *
 * NOTES: If lpData is NULL then just return lpdwDataSize. This allows
 *        the data structure to be allocated by our caller which can then
 *        call this procedure/method again with a valid data pointer.
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_GetConnectionSettings
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlayLobbyAImpl,iface);
  HRESULT hr; 

  TRACE("(%p)->(0x%08lx,%p,%p)\n", This, dwAppID, lpData, lpdwDataSize );

  EnterCriticalSection( &This->unk->DPL_lock );

  hr = DPLAYX_GetConnectionSettingsA( dwAppID, lpData, lpdwDataSize );

  LeaveCriticalSection( &This->unk->DPL_lock );

  return hr;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_GetConnectionSettings
( LPDIRECTPLAYLOBBY iface,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlayLobbyWImpl,iface);
  HRESULT hr;

  TRACE("(%p)->(0x%08lx,%p,%p)\n", This, dwAppID, lpData, lpdwDataSize );
 
  EnterCriticalSection( &This->unk->DPL_lock );

  hr = DPLAYX_GetConnectionSettingsW( dwAppID, lpData, lpdwDataSize );

  LeaveCriticalSection( &This->unk->DPL_lock );

  return hr;
}

/********************************************************************
 *
 * Retrieves the message sent between a lobby client and a DirectPlay 
 * application. All messages are queued until received.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlayLobbyAImpl,iface);
  FIXME(":stub %p %08lx %08lx %p %p %p\n", This, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlayLobbyWImpl,iface);
  FIXME(":stub %p %08lx %08lx %p %p %p\n", This, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
}

typedef struct tagRunApplicationEnumStruct
{
  IDirectPlayLobbyAImpl* This;

  GUID  appGUID;
  LPSTR lpszPath;
  LPSTR lpszFileName;
  LPSTR lpszCommandLine;
  LPSTR lpszCurrentDirectory;
} RunApplicationEnumStruct, *lpRunApplicationEnumStruct;

/* To be called by RunApplication to find how to invoke the function */
static BOOL CALLBACK RunApplicationA_EnumLocalApplications
( LPCDPLAPPINFO   lpAppInfo,
  LPVOID          lpContext,
  DWORD           dwFlags )
{
  lpRunApplicationEnumStruct lpData = (lpRunApplicationEnumStruct)lpContext;

  if( IsEqualGUID( &lpAppInfo->guidApplication, &lpData->appGUID ) )
  {
    char  returnBuffer[200];
    DWORD returnType, sizeOfReturnBuffer;
    LPSTR clSubKey   = "CommandLine";
    LPSTR cdSubKey   = "CurrentDirectory";  
    LPSTR fileSubKey = "File";
    LPSTR pathSubKey = "Path";

    /* FIXME: Lazy man hack - dplay struct has the present reg key saved */ 

    sizeOfReturnBuffer = 200;
    
    /* Get all the appropriate data from the registry */
    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, clSubKey,
                          NULL, &returnType, returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing CommandLine registry data member\n" );
    }
    else
    {
      lpData->lpszCommandLine = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, returnBuffer ); 
    }

    sizeOfReturnBuffer = 200;

    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, cdSubKey,
                          NULL, &returnType, returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing CurrentDirectory registry data member\n" );
    }
    else
    {
      lpData->lpszCurrentDirectory = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, returnBuffer );
    }

    sizeOfReturnBuffer = 200;

    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, fileSubKey,
                          NULL, &returnType, returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing File registry data member\n" );
    }
    else
    {
      lpData->lpszFileName = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, returnBuffer );
    }

    sizeOfReturnBuffer = 200;

    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, pathSubKey,
                          NULL, &returnType, returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing Path registry data member\n" );
    }
    else
    {
      lpData->lpszPath = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, returnBuffer );
    }

    return FALSE; /* No need to keep going as we found what we wanted */
  }

  return TRUE; /* Keep enumerating, haven't found the application yet */ 
}

/********************************************************************
 *
 * Starts an application and passes to it all the information to
 * connect to a session.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_RunApplication
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  ICOM_THIS(IDirectPlayLobbyAImpl,iface);
  HRESULT hr;
  RunApplicationEnumStruct enumData;
  char temp[200];
  STARTUPINFOA startupInfo;
  PROCESS_INFORMATION newProcessInfo;
  LPSTR appName;

  TRACE( "(%p)->(0x%08lx,%p,%p,%x)\n", This, dwFlags, lpdwAppID, lpConn, hReceiveEvent );

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDPARAMS;
  }

  EnterCriticalSection( &This->unk->DPL_lock );

  ZeroMemory( &enumData, sizeof( enumData ) );
  enumData.This    = This;
  enumData.appGUID = lpConn->lpSessionDesc->guidApplication;

  /* Our callback function will fill up the enumData structure with all the information 
     required to start a new process */
  IDirectPlayLobby_EnumLocalApplications( iface, RunApplicationA_EnumLocalApplications,
                                          (LPVOID)(&enumData), 0 );

  /* First the application name */
  strcpy( temp, enumData.lpszPath );
  strcat( temp, "\\" );
  strcat( temp, enumData.lpszFileName ); 
  HeapFree( GetProcessHeap(), 0, enumData.lpszPath );   
  HeapFree( GetProcessHeap(), 0, enumData.lpszFileName );
  appName = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, temp );

  /* Now the command line */
  strcat( temp, " " ); 
  strcat( temp, enumData.lpszCommandLine );
  HeapFree( GetProcessHeap(), 0, enumData.lpszCommandLine );
  enumData.lpszCommandLine = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, temp );

  ZeroMemory( &startupInfo, sizeof( startupInfo ) );
  startupInfo.cb = sizeof( startupInfo );
  /* FIXME: Should any fields be filled in? */

  ZeroMemory( &newProcessInfo, sizeof( newProcessInfo ) );

  if( !CreateProcessA( appName,
                       enumData.lpszCommandLine,
                       NULL,
                       NULL,
                       FALSE,
                       CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_CONSOLE | CREATE_SUSPENDED, /* Creation Flags */
                       NULL,
                       enumData.lpszCurrentDirectory,
                       &startupInfo,
                       &newProcessInfo
                     )
    )
  {
    FIXME( "Failed to create process for app %s\n", appName );

    HeapFree( GetProcessHeap(), 0, appName );
    HeapFree( GetProcessHeap(), 0, enumData.lpszCommandLine );
    HeapFree( GetProcessHeap(), 0, enumData.lpszCurrentDirectory );

    return DPERR_CANTCREATEPROCESS; 
  } 

  HeapFree( GetProcessHeap(), 0, appName );
  HeapFree( GetProcessHeap(), 0, enumData.lpszCommandLine );
  HeapFree( GetProcessHeap(), 0, enumData.lpszCurrentDirectory );

  /* Reserve this global application id! */
  if( !DPLAYX_CreateLobbyApplication( newProcessInfo.dwProcessId, hReceiveEvent ) )
  {
    ERR( "Unable to create global application data\n" );
  }

  hr = IDirectPlayLobby_SetConnectionSettings( iface, 0, newProcessInfo.dwProcessId, lpConn );
 
  if( hr != DP_OK )
  {
    FIXME( "SetConnectionSettings failure %s\n", DPLAYX_HresultToString( hr ) );
    return hr;
  }

  /* Everything seems to have been set correctly, update the dwAppID */
  *lpdwAppID = newProcessInfo.dwProcessId;

  /* Unsuspend the process */ 
  ResumeThread( newProcessInfo.dwThreadId );

  LeaveCriticalSection( &This->unk->DPL_lock );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_RunApplication
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  ICOM_THIS(IDirectPlayLobbyWImpl,iface);
  FIXME( "(%p)->(0x%08lx,%p,%p,%p):stub\n", This, dwFlags, lpdwAppID, lpConn, (void *)hReceiveEvent );
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Sends a message between the application and the lobby client.
 * All messages are queued until received.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_SendLobbyMessage
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_SendLobbyMessage
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Modifies the DPLCONNECTION structure to contain all information
 * needed to start and connect an application.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyWImpl_SetConnectionSettings
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  ICOM_THIS(IDirectPlayLobbyWImpl,iface);
  HRESULT hr;

  TRACE("(%p)->(0x%08lx,0x%08lx,%p)\n", This, dwFlags, dwAppID, lpConn );

  EnterCriticalSection( &This->unk->DPL_lock );

  hr = DPLAYX_SetConnectionSettingsW( dwFlags, dwAppID, lpConn );

  LeaveCriticalSection( &This->unk->DPL_lock );

  return hr;
}

static HRESULT WINAPI IDirectPlayLobbyAImpl_SetConnectionSettings
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  ICOM_THIS(IDirectPlayLobbyAImpl,iface);
  HRESULT hr;

  TRACE("(%p)->(0x%08lx,0x%08lx,%p)\n", This, dwFlags, dwAppID, lpConn );

  EnterCriticalSection( &This->unk->DPL_lock );

  hr = DPLAYX_SetConnectionSettingsA( dwFlags, dwAppID, lpConn );

  LeaveCriticalSection( &This->unk->DPL_lock );

  return hr;
}

/********************************************************************
 *
 * Registers an event that will be set when a lobby message is received.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}


/* DPL 2 methods */

/********************************************************************
 *
 * Registers an event that will be set when a lobby message is received.
 *
 */
static HRESULT WINAPI IDirectPlayLobby2WImpl_CreateCompoundAddress
( LPDIRECTPLAYLOBBY2 iface,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  return DPL_CreateCompoundAddress( lpElements, dwElementCount, lpAddress, lpdwAddressSize, FALSE );
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_CreateCompoundAddress
( LPDIRECTPLAYLOBBY2A iface,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  return DPL_CreateCompoundAddress( lpElements, dwElementCount, lpAddress, lpdwAddressSize, TRUE );
}

HRESULT DPL_CreateCompoundAddress
( LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize,
  BOOL bAnsiInterface )
{
  DWORD dwSizeRequired = 0;
  DWORD dwElements;
  LPCDPCOMPOUNDADDRESSELEMENT lpOrigElements = lpElements;

  TRACE("(%p,0x%08lx,%p,%p)\n", lpElements, dwElementCount, lpAddress, lpdwAddressSize );

  /* Parameter check */
  if( ( lpElements == NULL ) ||
      ( dwElementCount == 0 )   /* FIXME: Not sure if this is a failure case */
    )
  {
    return DPERR_INVALIDPARAMS;
  }

  /* Add the total size chunk */
  dwSizeRequired += sizeof( DPADDRESS ) + sizeof( DWORD );

  /* Calculate the size of the buffer required */
  for ( dwElements = dwElementCount; dwElements > 0; --dwElements, ++lpElements ) 
  {
    if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ServiceProvider ) ) ||
         ( IsEqualGUID( &lpElements->guidDataType, &DPAID_LobbyProvider ) )
       )
    {
      dwSizeRequired += sizeof( DPADDRESS ) + sizeof( GUID ); 
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Phone ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Modem ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INet ) )
            )
    {
      if( !bAnsiInterface )
      { 
        ERR( "Ansi GUIDs used for unicode interface\n" );
        return DPERR_INVALIDFLAGS;
      }

      dwSizeRequired += sizeof( DPADDRESS ) + lpElements->dwDataSize;
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_PhoneW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ModemW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetW ) )
            )
    {
      if( bAnsiInterface )
      {
        ERR( "Unicode GUIDs used for ansi interface\n" );
        return DPERR_INVALIDFLAGS;
      }

      FIXME( "Right size for unicode interface?\n" );
      dwSizeRequired += sizeof( DPADDRESS ) + lpElements->dwDataSize * sizeof( WCHAR );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetPort ) )
    {
      dwSizeRequired += sizeof( DPADDRESS ) + sizeof( WORD );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ComPort ) )
    {
      FIXME( "Right size for unicode interface?\n" );
      dwSizeRequired += sizeof( DPADDRESS ) + sizeof( DPCOMPORTADDRESS ); /* FIXME: Right size? */
    }
    else
    {
      ERR( "Unknown GUID %s\n", debugstr_guid(&lpElements->guidDataType) );
      return DPERR_INVALIDFLAGS; 
    }
  }

  /* The user wants to know how big a buffer to allocate for us */
  if( ( lpAddress == NULL ) ||
      ( *lpdwAddressSize < dwSizeRequired ) 
    )
  {
    *lpdwAddressSize = dwSizeRequired; 
    return DPERR_BUFFERTOOSMALL;
  }  

  /* Add the total size chunk */
  {
    LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

    lpdpAddress->guidDataType = DPAID_TotalSize;
    lpdpAddress->dwDataSize = sizeof( DWORD );
    lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

    *(LPDWORD)lpAddress = dwSizeRequired;
    lpAddress = (char *) lpAddress + sizeof( DWORD );
  }

  /* Calculate the size of the buffer required */
  for( dwElements = dwElementCount, lpElements = lpOrigElements; 
       dwElements > 0; 
       --dwElements, ++lpElements ) 
  {
    if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ServiceProvider ) ) ||
         ( IsEqualGUID( &lpElements->guidDataType, &DPAID_LobbyProvider ) )
       )
    {
      LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

      lpdpAddress->guidDataType = lpElements->guidDataType;
      lpdpAddress->dwDataSize = sizeof( GUID );
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      *((LPGUID)lpAddress) = *((LPGUID)lpElements->lpData);
      lpAddress = (char *) lpAddress + sizeof( GUID );
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Phone ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Modem ) ) || 
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INet ) )
            )
    {
      LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

      lpdpAddress->guidDataType = lpElements->guidDataType;
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      lstrcpynA( (LPSTR)lpAddress, 
                 (LPCSTR)lpElements->lpData, 
                 lpElements->dwDataSize );
      lpAddress = (char *) lpAddress + lpElements->dwDataSize;
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_PhoneW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ModemW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetW ) )
            )
    {
      LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

      lpdpAddress->guidDataType = lpElements->guidDataType;
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      lstrcpynW( (LPWSTR)lpAddress,
                 (LPCWSTR)lpElements->lpData,
                 lpElements->dwDataSize );
      lpAddress = (char *) lpAddress + lpElements->dwDataSize * sizeof( WCHAR );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetPort ) )
    {
      LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

      lpdpAddress->guidDataType = lpElements->guidDataType;
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      *((LPWORD)lpAddress) = *((LPWORD)lpElements->lpData);
      lpAddress = (char *) lpAddress + sizeof( WORD );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ComPort ) )
    {
      LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

      lpdpAddress->guidDataType = lpElements->guidDataType;
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      memcpy( lpAddress, lpElements->lpData, sizeof( DPADDRESS ) ); 
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );
    }
  }

  return DP_OK;
}

/* DPL 3 methods */

static HRESULT WINAPI IDirectPlayLobby3WImpl_ConnectEx
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags, REFIID riid, LPVOID* lplpDP, IUnknown* pUnk )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_ConnectEx
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags, REFIID riid, LPVOID* lplpDP, IUnknown* pUnk )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3WImpl_RegisterApplication
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags, LPDPAPPLICATIONDESC lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_RegisterApplication
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags, LPDPAPPLICATIONDESC lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3WImpl_UnregisterApplication
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags, REFGUID lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_UnregisterApplication
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags, REFGUID lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3WImpl_WaitForConnectionSettings
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_WaitForConnectionSettings
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags )
{
  FIXME(":stub\n");
  return DP_OK;
}


/* Virtual Table definitions for DPL{1,2,3}{A,W} */

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobbyAVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif

/* Direct Play Lobby 1 (ascii) Virtual Table for methods */
/* All lobby 1 methods are exactly the same except QueryInterface */
static struct ICOM_VTABLE(IDirectPlayLobby) directPlayLobbyAVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

  IDirectPlayLobbyAImpl_QueryInterface,
  XCAST(AddRef)IDirectPlayLobbyImpl_AddRef,
  XCAST(Release)IDirectPlayLobbyAImpl_Release,

  IDirectPlayLobbyAImpl_Connect,
  IDirectPlayLobbyAImpl_CreateAddress,
  IDirectPlayLobbyAImpl_EnumAddress,
  IDirectPlayLobbyAImpl_EnumAddressTypes,
  IDirectPlayLobbyAImpl_EnumLocalApplications,
  IDirectPlayLobbyAImpl_GetConnectionSettings,
  IDirectPlayLobbyAImpl_ReceiveLobbyMessage,
  IDirectPlayLobbyAImpl_RunApplication,
  IDirectPlayLobbyAImpl_SendLobbyMessage,
  IDirectPlayLobbyAImpl_SetConnectionSettings,
  IDirectPlayLobbyAImpl_SetLobbyMessageEvent
};
#undef XCAST


/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobbyWVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif

/* Direct Play Lobby 1 (unicode) Virtual Table for methods */
static ICOM_VTABLE(IDirectPlayLobby) directPlayLobbyWVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

  IDirectPlayLobbyW_QueryInterface,
  XCAST(AddRef)IDirectPlayLobbyImpl_AddRef,
  XCAST(Release)IDirectPlayLobbyAImpl_Release,

  IDirectPlayLobbyWImpl_Connect,
  IDirectPlayLobbyWImpl_CreateAddress, 
  IDirectPlayLobbyWImpl_EnumAddress,
  IDirectPlayLobbyWImpl_EnumAddressTypes,
  IDirectPlayLobbyWImpl_EnumLocalApplications,
  IDirectPlayLobbyWImpl_GetConnectionSettings,
  IDirectPlayLobbyWImpl_ReceiveLobbyMessage,
  IDirectPlayLobbyWImpl_RunApplication,
  IDirectPlayLobbyWImpl_SendLobbyMessage,
  IDirectPlayLobbyWImpl_SetConnectionSettings,
  IDirectPlayLobbyWImpl_SetLobbyMessageEvent
};
#undef XCAST

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobby2AVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif

/* Direct Play Lobby 2 (ascii) Virtual Table for methods */
static ICOM_VTABLE(IDirectPlayLobby2) directPlayLobby2AVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

  IDirectPlayLobby2AImpl_QueryInterface,
  XCAST(AddRef)IDirectPlayLobbyImpl_AddRef,
  XCAST(Release)IDirectPlayLobbyAImpl_Release,

  XCAST(Connect)IDirectPlayLobbyAImpl_Connect,
  XCAST(CreateAddress)IDirectPlayLobbyAImpl_CreateAddress,
  XCAST(EnumAddress)IDirectPlayLobbyAImpl_EnumAddress,
  XCAST(EnumAddressTypes)IDirectPlayLobbyAImpl_EnumAddressTypes,
  XCAST(EnumLocalApplications)IDirectPlayLobbyAImpl_EnumLocalApplications,
  XCAST(GetConnectionSettings)IDirectPlayLobbyAImpl_GetConnectionSettings,
  XCAST(ReceiveLobbyMessage)IDirectPlayLobbyAImpl_ReceiveLobbyMessage,
  XCAST(RunApplication)IDirectPlayLobbyAImpl_RunApplication,
  XCAST(SendLobbyMessage)IDirectPlayLobbyAImpl_SendLobbyMessage,
  XCAST(SetConnectionSettings)IDirectPlayLobbyAImpl_SetConnectionSettings,
  XCAST(SetLobbyMessageEvent)IDirectPlayLobbyAImpl_SetLobbyMessageEvent,

  IDirectPlayLobby2AImpl_CreateCompoundAddress 
};
#undef XCAST

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobby2AVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif

/* Direct Play Lobby 2 (unicode) Virtual Table for methods */
static ICOM_VTABLE(IDirectPlayLobby2) directPlayLobby2WVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

  IDirectPlayLobby2WImpl_QueryInterface,
  XCAST(AddRef)IDirectPlayLobbyImpl_AddRef, 
  XCAST(Release)IDirectPlayLobbyAImpl_Release,

  XCAST(Connect)IDirectPlayLobbyWImpl_Connect,
  XCAST(CreateAddress)IDirectPlayLobbyWImpl_CreateAddress,
  XCAST(EnumAddress)IDirectPlayLobbyWImpl_EnumAddress,
  XCAST(EnumAddressTypes)IDirectPlayLobbyWImpl_EnumAddressTypes,
  XCAST(EnumLocalApplications)IDirectPlayLobbyWImpl_EnumLocalApplications,
  XCAST(GetConnectionSettings)IDirectPlayLobbyWImpl_GetConnectionSettings,
  XCAST(ReceiveLobbyMessage)IDirectPlayLobbyWImpl_ReceiveLobbyMessage,
  XCAST(RunApplication)IDirectPlayLobbyWImpl_RunApplication,
  XCAST(SendLobbyMessage)IDirectPlayLobbyWImpl_SendLobbyMessage,
  XCAST(SetConnectionSettings)IDirectPlayLobbyWImpl_SetConnectionSettings,
  XCAST(SetLobbyMessageEvent)IDirectPlayLobbyWImpl_SetLobbyMessageEvent,

  IDirectPlayLobby2WImpl_CreateCompoundAddress
};
#undef XCAST

/* Direct Play Lobby 3 (ascii) Virtual Table for methods */

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobby3AVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif

static ICOM_VTABLE(IDirectPlayLobby3) directPlayLobby3AVT =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  IDirectPlayLobby3AImpl_QueryInterface,
  XCAST(AddRef)IDirectPlayLobbyImpl_AddRef,
  XCAST(Release)IDirectPlayLobbyAImpl_Release,

  XCAST(Connect)IDirectPlayLobbyAImpl_Connect,
  XCAST(CreateAddress)IDirectPlayLobbyAImpl_CreateAddress,
  XCAST(EnumAddress)IDirectPlayLobbyAImpl_EnumAddress,
  XCAST(EnumAddressTypes)IDirectPlayLobbyAImpl_EnumAddressTypes,
  XCAST(EnumLocalApplications)IDirectPlayLobbyAImpl_EnumLocalApplications,
  XCAST(GetConnectionSettings)IDirectPlayLobbyAImpl_GetConnectionSettings,
  XCAST(ReceiveLobbyMessage)IDirectPlayLobbyAImpl_ReceiveLobbyMessage,
  XCAST(RunApplication)IDirectPlayLobbyAImpl_RunApplication,
  XCAST(SendLobbyMessage)IDirectPlayLobbyAImpl_SendLobbyMessage,
  XCAST(SetConnectionSettings)IDirectPlayLobbyAImpl_SetConnectionSettings,
  XCAST(SetLobbyMessageEvent)IDirectPlayLobbyAImpl_SetLobbyMessageEvent,

  XCAST(CreateCompoundAddress)IDirectPlayLobby2AImpl_CreateCompoundAddress,

  IDirectPlayLobby3AImpl_ConnectEx,
  IDirectPlayLobby3AImpl_RegisterApplication,
  IDirectPlayLobby3AImpl_UnregisterApplication,
  IDirectPlayLobby3AImpl_WaitForConnectionSettings
};
#undef XCAST

/* Direct Play Lobby 3 (unicode) Virtual Table for methods */

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobby3WVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif

static ICOM_VTABLE(IDirectPlayLobby3) directPlayLobby3WVT =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  IDirectPlayLobby3WImpl_QueryInterface,
  XCAST(AddRef)IDirectPlayLobbyImpl_AddRef,
  XCAST(Release)IDirectPlayLobbyAImpl_Release,

  XCAST(Connect)IDirectPlayLobbyWImpl_Connect,
  XCAST(CreateAddress)IDirectPlayLobbyWImpl_CreateAddress,
  XCAST(EnumAddress)IDirectPlayLobbyWImpl_EnumAddress,
  XCAST(EnumAddressTypes)IDirectPlayLobbyWImpl_EnumAddressTypes,
  XCAST(EnumLocalApplications)IDirectPlayLobbyWImpl_EnumLocalApplications,
  XCAST(GetConnectionSettings)IDirectPlayLobbyWImpl_GetConnectionSettings,
  XCAST(ReceiveLobbyMessage)IDirectPlayLobbyWImpl_ReceiveLobbyMessage,
  XCAST(RunApplication)IDirectPlayLobbyWImpl_RunApplication,
  XCAST(SendLobbyMessage)IDirectPlayLobbyWImpl_SendLobbyMessage,
  XCAST(SetConnectionSettings)IDirectPlayLobbyWImpl_SetConnectionSettings,
  XCAST(SetLobbyMessageEvent)IDirectPlayLobbyWImpl_SetLobbyMessageEvent,

  XCAST(CreateCompoundAddress)IDirectPlayLobby2WImpl_CreateCompoundAddress,

  IDirectPlayLobby3WImpl_ConnectEx,
  IDirectPlayLobby3WImpl_RegisterApplication,
  IDirectPlayLobby3WImpl_UnregisterApplication,
  IDirectPlayLobby3WImpl_WaitForConnectionSettings
};
#undef XCAST


/*********************************************************
 *
 * Direct Play and Direct Play Lobby Interface Implementation 
 * 
 *********************************************************/ 

/***************************************************************************
 *  DirectPlayLobbyCreateA   (DPLAYX.4)
 *
 */
HRESULT WINAPI DirectPlayLobbyCreateA( LPGUID lpGUIDDSP,
                                       LPDIRECTPLAYLOBBYA *lplpDPL,
                                       IUnknown *lpUnk, 
                                       LPVOID lpData,
                                       DWORD dwDataSize )
{
  TRACE("lpGUIDDSP=%p lplpDPL=%p lpUnk=%p lpData=%p dwDataSize=%08lx\n",
        lpGUIDDSP,lplpDPL,lpUnk,lpData,dwDataSize);

  /* Parameter Check: lpGUIDSP, lpUnk & lpData must be NULL. dwDataSize must
   * equal 0. These fields are mostly for future expansion.
   */
  if ( lpGUIDDSP || lpUnk || lpData || dwDataSize )
  {
     *lplpDPL = NULL;
     return DPERR_INVALIDPARAMS;
  }

  return directPlayLobby_QueryInterface( &IID_IDirectPlayLobbyA, (void**)lplpDPL ); 
}

/***************************************************************************
 *  DirectPlayLobbyCreateW   (DPLAYX.5)
 *
 */
HRESULT WINAPI DirectPlayLobbyCreateW( LPGUID lpGUIDDSP, 
                                       LPDIRECTPLAYLOBBY *lplpDPL,
                                       IUnknown *lpUnk,
                                       LPVOID lpData, 
                                       DWORD dwDataSize )
{
  TRACE("lpGUIDDSP=%p lplpDPL=%p lpUnk=%p lpData=%p dwDataSize=%08lx\n",
        lpGUIDDSP,lplpDPL,lpUnk,lpData,dwDataSize);

  /* Parameter Check: lpGUIDSP, lpUnk & lpData must be NULL. dwDataSize must 
   * equal 0. These fields are mostly for future expansion.
   */
  if ( lpGUIDDSP || lpData || dwDataSize )
  {
     *lplpDPL = NULL;
     ERR("Bad parameters!\n" );
     return DPERR_INVALIDPARAMS;
  }

  if( lpUnk )
  {
     *lplpDPL = NULL;
     ERR("Bad parameters!\n" );
     return CLASS_E_NOAGGREGATION;
  }

  return directPlayLobby_QueryInterface( &IID_IDirectPlayLobby, (void**)lplpDPL );  

}
