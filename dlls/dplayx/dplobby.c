/* Direct Play Lobby 2 & 3 Implementation
 *
 * Copyright 1998,1999 - Peter Hunnisett
 *
 * <presently under construction - contact hunnise@nortelnetworks.com>
 *
 */
#include <string.h>
#include "winerror.h"
#include "winnt.h"
#include "winreg.h"
#include "dplobby.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(dplay)

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectPlayLobbyImpl IDirectPlayLobbyImpl;
typedef struct IDirectPlayLobbyImpl IDirectPlayLobbyAImpl;
typedef struct IDirectPlayLobbyImpl IDirectPlayLobbyWImpl;
typedef struct IDirectPlayLobbyImpl IDirectPlayLobby2Impl;
typedef struct IDirectPlayLobbyImpl IDirectPlayLobby2AImpl;
typedef struct IDirectPlayLobbyImpl IDirectPlayLobby3Impl;
typedef struct IDirectPlayLobbyImpl IDirectPlayLobby3AImpl;

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

/* FIXME: I don't think that everything belongs here...*/
typedef struct tagDirectPlayLobbyData
{
  DWORD                     dwConnFlags;
  DPSESSIONDESC2            sessionDesc;
  DPNAME                    playerName;
  GUID                      guidSP;
  LPVOID                    lpAddress;
  DWORD                     dwAddressSize;
} DirectPlayLobbyData;

typedef struct tagDirectPlayLobby2Data
{
  BOOL dummy;
} DirectPlayLobby2Data;

typedef struct tagDirectPlayLobby3Data
{
  BOOL dummy;
} DirectPlayLobby3Data;

struct IDirectPlayLobbyImpl
{
    ICOM_VTABLE(IDirectPlayLobby)* lpvtbl;
 
    /* IUnknown fields */
    DirectPlayLobbyIUnknownData*  unk;

    /* IDirectPlayLobby 1 fields */
    DirectPlayLobbyData*          dpl;

    /* IDirectPlayLobby 2 fields */
    DirectPlayLobby2Data*         dpl2;

    /* IDirectPlayLobby 3 fields */
    DirectPlayLobby3Data*         dpl3;
};

/* Forward declarations of virtual tables */
static ICOM_VTABLE(IDirectPlayLobby) directPlayLobbyAVT;
static ICOM_VTABLE(IDirectPlayLobby) directPlayLobbyWVT;
static ICOM_VTABLE(IDirectPlayLobby2) directPlayLobby2AVT;
static ICOM_VTABLE(IDirectPlayLobby2) directPlayLobby2WVT;
static ICOM_VTABLE(IDirectPlayLobby3) directPlayLobby3AVT;
static ICOM_VTABLE(IDirectPlayLobby3) directPlayLobby3WVT;




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


BOOL DPL_CreateIUnknown( IDirectPlayLobbyImpl* lpDPL ) 
{
  lpDPL->unk = (DirectPlayLobbyIUnknownData*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                                        sizeof( *(lpDPL->unk) ) ); 
  if ( lpDPL->unk != NULL )
  {
    InitializeCriticalSection( &lpDPL->unk->DPL_lock );

    IDirectPlayLobby_AddRef( (IDirectPlayLobby*)lpDPL );

    return TRUE; 
  }

  return FALSE;
}

BOOL DPL_DestroyIUnknown( IDirectPlayLobbyImpl* lpDPL )
{
  DeleteCriticalSection( &lpDPL->unk->DPL_lock );
  HeapFree( GetProcessHeap(), 0, lpDPL->unk );

  return TRUE;
}

BOOL DPL_CreateLobby1( IDirectPlayLobbyImpl* lpDPL )
{
  lpDPL->dpl = (DirectPlayLobbyIUnknownData*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                        sizeof( *(lpDPL->dpl) ) );
  if ( lpDPL->dpl == NULL )
  {
    return FALSE;
  }

  /* Initialize the dwSize fields for internal structures */

  lpDPL->dpl->sessionDesc.dwSize = sizeof( lpDPL->dpl->sessionDesc ); 
  lpDPL->dpl->playerName.dwSize = sizeof( lpDPL->dpl->playerName );
  /* lpDPL->dpl->dwAddressSize = 0; Done in HeapAlloc */
  
  return TRUE;  
}

BOOL DPL_DestroyLobby1( IDirectPlayLobbyImpl* lpDPL )
{
  /* Delete the contents */
  HeapFree( GetProcessHeap(), 0, lpDPL->dpl->sessionDesc.sess.lpszSessionNameA );
  HeapFree( GetProcessHeap(), 0, lpDPL->dpl->sessionDesc.pass.lpszPasswordA );

  HeapFree( GetProcessHeap(), 0, lpDPL->dpl->playerName.psn.lpszShortNameA );
  HeapFree( GetProcessHeap(), 0, lpDPL->dpl->playerName.pln.lpszLongNameA );

  HeapFree( GetProcessHeap(), 0, lpDPL->dpl->lpAddress );

  HeapFree( GetProcessHeap(), 0, lpDPL->dpl );

  return TRUE;
}

BOOL DPL_CreateLobby2( IDirectPlayLobbyImpl* lpDPL )
{
  lpDPL->dpl2 = (DirectPlayLobbyIUnknownData*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                         sizeof( *(lpDPL->dpl2) ) );
  if ( lpDPL->dpl2 == NULL )
  {
    return FALSE;
  }

  return TRUE;
}

BOOL DPL_DestroyLobby2( IDirectPlayLobbyImpl* lpDPL )
{
  HeapFree( GetProcessHeap(), 0, lpDPL->dpl2 );

  return TRUE;
}

BOOL DPL_CreateLobby3( IDirectPlayLobbyImpl* lpDPL )
{
  lpDPL->dpl3 = (DirectPlayLobbyIUnknownData*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                         sizeof( *(lpDPL->dpl3) ) );
  if ( lpDPL->dpl3 == NULL )
  {
    return FALSE;
  }

  return TRUE;
}

BOOL DPL_DestroyLobby3( IDirectPlayLobbyImpl* lpDPL )
{
  HeapFree( GetProcessHeap(), 0, lpDPL->dpl3 );

  return TRUE;
}


/* Helper function for DirectPlayLobby  QueryInterface */ 
extern 
HRESULT directPlayLobby_QueryInterface
         ( REFIID riid, LPVOID* ppvObj )
{
  IDirectPlayLobby3AImpl* lpDPL = NULL;

  if( IsEqualGUID( &IID_IDirectPlayLobby, riid ) )
  {
    lpDPL = (IDirectPlayLobbyImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                              sizeof( *lpDPL ) );

    if( !lpDPL )
    {
      return E_OUTOFMEMORY;
    }

    lpDPL->lpvtbl = &directPlayLobbyWVT;

    if ( DPL_CreateIUnknown( lpDPL ) &&
         DPL_CreateLobby1( lpDPL ) 
       )
    { 
      *ppvObj = lpDPL;
      return S_OK;
    }
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobbyA, riid ) )
  {
    lpDPL = (IDirectPlayLobbyAImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                               sizeof( *lpDPL ) );

    if( !lpDPL )
    {
      return E_OUTOFMEMORY;
    }

    lpDPL->lpvtbl = &directPlayLobbyAVT;

    if ( DPL_CreateIUnknown( lpDPL ) &&
         DPL_CreateLobby1( lpDPL ) 
       )
    { 
      *ppvObj = lpDPL;
      return S_OK;
    }
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2, riid ) )
  {
    lpDPL = (IDirectPlayLobby2Impl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                               sizeof( *lpDPL ) );

    if( !lpDPL )
    {
      return E_OUTOFMEMORY;
    }

    lpDPL->lpvtbl = &directPlayLobby2WVT;

    if ( DPL_CreateIUnknown( lpDPL ) &&
         DPL_CreateLobby1( lpDPL ) &&
         DPL_CreateLobby2( lpDPL )
       )
    {
      *ppvObj = lpDPL;
      return S_OK;
    }
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2A, riid ) )
  {
    lpDPL = (IDirectPlayLobby2AImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                sizeof( *lpDPL ) );

    if( !lpDPL )
    {
      return E_OUTOFMEMORY;
    }

    lpDPL->lpvtbl = &directPlayLobby2AVT;

    if ( DPL_CreateIUnknown( lpDPL ) &&
         DPL_CreateLobby1( lpDPL )  &&
         DPL_CreateLobby2( lpDPL )
       )
    {
      *ppvObj = lpDPL;
      return S_OK;
    }
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby3, riid ) )
  {
    lpDPL = (IDirectPlayLobby3Impl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                               sizeof( *lpDPL ) );

    if( !lpDPL )
    {
      return E_OUTOFMEMORY;
    }

    lpDPL->lpvtbl = &directPlayLobby3WVT;

    if ( DPL_CreateIUnknown( lpDPL ) &&
         DPL_CreateLobby1( lpDPL ) &&
         DPL_CreateLobby2( lpDPL ) &&
         DPL_CreateLobby3( lpDPL )
       )
    {
      *ppvObj = lpDPL;
      return S_OK;
    }
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby3A, riid ) )
  {
     lpDPL = (IDirectPlayLobby3AImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                             sizeof( *lpDPL ) );

    if( !lpDPL )
    {
      return E_OUTOFMEMORY;
    }

    lpDPL->lpvtbl = &directPlayLobby3AVT;

    if ( DPL_CreateIUnknown( lpDPL ) &&
         DPL_CreateLobby1( lpDPL ) &&
         DPL_CreateLobby2( lpDPL ) &&
         DPL_CreateLobby3( lpDPL )
       )
    {
      *ppvObj = lpDPL;
      return S_OK;
    }
  }

  /* Check if we had problems creating an interface */
  if ( lpDPL != NULL )
  {
    DPL_DestroyLobby3( lpDPL );
    DPL_DestroyLobby2( lpDPL );
    DPL_DestroyLobby1( lpDPL );
    DPL_DestroyIUnknown( lpDPL );
    HeapFree( GetProcessHeap(), 0, lpDPL );

    *ppvObj = NULL;

    return DPERR_NOMEMORY;
  }
  else /* Unsupported interface */
  {
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }
}

static HRESULT WINAPI IDirectPlayLobbyAImpl_QueryInterface
( LPDIRECTPLAYLOBBYA iface,
  REFIID riid,
  LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
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
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
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
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
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
  ICOM_THIS(IDirectPlayLobby2Impl,iface);

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
  ICOM_THIS(IDirectPlayLobby3Impl,iface);

  /* Compare riids. We know this object is a direct play lobby 3 object.
     If we are asking about the same type of interface we're fine.
   */
  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlayLobby3A, riid )
    )
  {
    IDirectPlayLobby_AddRef( (IDirectPlayLobby*)This );
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
  ICOM_THIS(IDirectPlayLobby3Impl,iface);

  /* Compare riids. We know this object is a direct play lobby 3 object.
     If we are asking about the same type of interface we're fine.
   */
  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlayLobby3, riid )
    )
  {
    IDirectPlayLobby_AddRef( (IDirectPlayLobby*)This );
    *ppvObj = This;
    return S_OK;
  }

  return directPlayLobby_QueryInterface( riid, ppvObj );

}

/* 
 * Simple procedure. Just increment the reference count to this
 * structure and return the new reference count.
 */
static ULONG WINAPI IDirectPlayLobbyAImpl_AddRef
( LPDIRECTPLAYLOBBYA iface )
{
  ULONG refCount;
  ICOM_THIS(IDirectPlayLobby2Impl,iface);

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

  ICOM_THIS(IDirectPlayLobby2Impl,iface);

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
  ICOM_THIS(IDirectPlayLobbyImpl,iface);

  LPDIRECTPLAY2A      lpDirectPlay2A;
  LPDIRECTPLAY3A      lpDirectPlay3A;
  LPDIRECTPLAYLOBBY2A lpDirectPlayLobby2A;
  HRESULT             rc;

  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, dwFlags, lplpDP, pUnk );

  if( dwFlags || pUnk )
  {
     return DPERR_INVALIDPARAMS;
  }

  if( ( rc = DirectPlayCreate( (LPGUID)&IID_IDirectPlay2A, lplpDP, pUnk ) ) != DP_OK )
  {
     ERR("error creating Direct Play 2W interface. Return Code = 0x%lx.\n", rc );
     return rc;
  }

  lpDirectPlay2A = *lplpDP;

  /* This should invoke IDirectPlay3::InitializeConnection IDirectPlay3::Open */
  /* - Use This object to get a DPL2 object using QueryInterface
   * - Need to call CreateCompoundAddress to create the lpConnection param for IDirectPlay::InitializeConnection
   * - Call IDirectPlay::InitializeConnection
   * - Call IDirectPlay::Open 
   */
#if 0
  create_address:
  {
    DPCOMPOUNDADDRESSELEMENT compoundAddress;

    /* Get lobby version capable of supporting CreateCompoundAddress */ 
    if( ( rc = IDirectPlayLobby_QueryInterface( This, &IID_IDirectPlayLobby2A, &lpDirectPlayLobby2A ) ) != DP_OK )
    {
      return rc;
    }

    EnterCriticalSection( &This->unk->DPL_lock );

    /* Actually create the compound address */
    memcpy( compoundAddress.guidDataType, This->dpl->guidSP, sizeof( compoundAddress.guidDataType ) );

    LeaveCriticalSection( &This->unk->DPL_lock );

    rc = IDirectPlayLobby_CreateCompoundAddress( lpDirectPlayLobby2A, lpElements, dwElementCount, lpAddress, lpdwAddressSize ) 

    /* Free the lobby object since we're done with it */
    IDirectPlayLobby_Release( lpDirectPlayLobby2A );

    if( rc != DP_OK )
    {
      return rc; 
    }
  }

  if( ( rc = IDirectPlayX_QueryInterface( directPlay2A, &IID_IDirectPlay3A, &lpDirectPlay3A ) ) != DP_OK )
  {
    return rc;
  }
#endif

  return DP_OK;

}

static HRESULT WINAPI IDirectPlayLobbyWImpl_Connect
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  LPDIRECTPLAY2* lplpDP,
  IUnknown* pUnk)
{
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
  LPDIRECTPLAY2* directPlay2W;
  HRESULT        createRC;

  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, dwFlags, lplpDP, pUnk );

  if( dwFlags || pUnk )
  {
     return DPERR_INVALIDPARAMS;
  }

  if( ( createRC = DirectPlayCreate( (LPGUID)&IID_IDirectPlay2, lplpDP, pUnk ) ) != DP_OK )
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
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
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
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
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
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_EnumAddress
( LPDIRECTPLAYLOBBY iface,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
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
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
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
  ICOM_THIS(IDirectPlayLobbyImpl,iface);

  FIXME("(%p)->(%p,%p,0x%08lx):stub\n", This, lpEnumLocalAppCallback, lpContext, dwFlags );

  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyAImpl_EnumLocalApplications
( LPDIRECTPLAYLOBBYA iface,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK lpEnumLocalAppCallback,
  LPVOID lpContext,
  DWORD dwFlags )
{
  ICOM_THIS(IDirectPlayLobbyImpl,iface);

  FIXME("(%p)->(%p,%p,0x%08lx):stub\n", This, lpEnumLocalAppCallback, lpContext, dwFlags );

  return DPERR_OUTOFMEMORY;
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
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
  LPDPLCONNECTION lpDplConnection;

  FIXME(": semi stub (%p)->(0x%08lx,%p,%p)\n", This, dwAppID, lpData, lpdwDataSize );
 
  /* Application is requesting us to give the required size */
  if ( lpData == NULL )
  {
    *lpdwDataSize = sizeof( DPLCONNECTION );
    return DPERR_BUFFERTOOSMALL;
  }

  /* Let's check the size of the buffer that the application has allocated */
  if ( *lpdwDataSize < sizeof( DPLCONNECTION ) )
  {
    *lpdwDataSize = sizeof( DPLCONNECTION );
    return DPERR_BUFFERTOOSMALL;
  }

  /* FIXME: Who's supposed to own the data */

  /* Fill in the fields - let them just use the ptrs */
  lpDplConnection = (LPDPLCONNECTION)lpData;

  /* Copy everything we've got into here */
  /* Need to actually store the stuff here. Check if we've already allocated each field first. */
  lpDplConnection->dwFlags = This->dpl->dwConnFlags;

  /* Copy LPDPSESSIONDESC2 struct */
  lpDplConnection->lpSessionDesc = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( This->dpl->sessionDesc ) );
  memcpy( lpDplConnection->lpSessionDesc, &This->dpl->sessionDesc, sizeof( This->dpl->sessionDesc ) );

  if( This->dpl->sessionDesc.sess.lpszSessionName )
  {
    lpDplConnection->lpSessionDesc->sess.lpszSessionName = 
      HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, This->dpl->sessionDesc.sess.lpszSessionNameA );
  }

  if( This->dpl->sessionDesc.pass.lpszPassword )
  {
    lpDplConnection->lpSessionDesc->pass.lpszPassword = 
      HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, This->dpl->sessionDesc.pass.lpszPasswordA );
  }
     
  lpDplConnection->lpSessionDesc->dwReserved1 = This->dpl->sessionDesc.dwReserved1;
  lpDplConnection->lpSessionDesc->dwReserved2 = This->dpl->sessionDesc.dwReserved2;

  /* Copy DPNAME struct - seems to be optional - check for existance first */
  lpDplConnection->lpPlayerName = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( This->dpl->playerName ) );
  memcpy( lpDplConnection->lpPlayerName, &(This->dpl->playerName), sizeof( This->dpl->playerName ) );

  if( This->dpl->playerName.psn.lpszShortName )
  {
    lpDplConnection->lpPlayerName->psn.lpszShortName =
      HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, This->dpl->playerName.psn.lpszShortNameA );  
  }

  if( This->dpl->playerName.pln.lpszLongName )
  {
    lpDplConnection->lpPlayerName->pln.lpszLongName =
      HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, This->dpl->playerName.pln.lpszLongNameA );
  }

  memcpy( &lpDplConnection->guidSP, &This->dpl->guidSP, sizeof( This->dpl->guidSP ) );

  lpDplConnection->lpAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, This->dpl->dwAddressSize );
  memcpy( lpDplConnection->lpAddress, This->dpl->lpAddress, This->dpl->dwAddressSize );

  lpDplConnection->dwAddressSize = This->dpl->dwAddressSize;

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_GetConnectionSettings
( LPDIRECTPLAYLOBBY iface,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
  FIXME(":semi stub %p %08lx %p %p \n", This, dwAppID, lpData, lpdwDataSize );

  /* Application is requesting us to give the required size */ 
  if ( !lpData )
  {
    /* Let's check the size of the buffer that the application has allocated */
    if( *lpdwDataSize >= sizeof( DPLCONNECTION ) )
    {
      return DP_OK;  
    }
    else
    {
      *lpdwDataSize = sizeof( DPLCONNECTION );
      return DPERR_BUFFERTOOSMALL;
    }
  }

  /* Fill in the fields - see above */
  FIXME("stub\n" );

  return DP_OK;
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
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
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
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
  FIXME(":stub %p %08lx %08lx %p %p %p\n", This, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
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
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_RunApplication
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  FIXME(":stub\n");
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
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
  TRACE(": This=%p, dwFlags=%08lx, dwAppId=%08lx, lpConn=%p\n",
         This, dwFlags, dwAppID, lpConn );

  /* Paramater check */
  if( dwFlags || !This || !lpConn )
  {
    ERR("invalid parameters.\n");
    return DPERR_INVALIDPARAMS;
  }

  /* See if there is a connection associated with this request.
   * dwAppID == 0 indicates that this request isn't associated with a connection.
   */
  if( dwAppID )
  {
     FIXME(": Connection dwAppID=%08lx given. Not implemented yet.\n",
            dwAppID );

     /* Need to add a check for this application Id...*/
     return DPERR_NOTLOBBIED;
  }

  if(  lpConn->dwSize != sizeof(DPLCONNECTION) )
  {
    ERR(": old/new DPLCONNECTION type? Size=%08lx vs. expected=%ul bytes\n", 
         lpConn->dwSize, sizeof( DPLCONNECTION ) );
    return DPERR_INVALIDPARAMS;
  }

  /* Need to investigate the lpConn->lpSessionDesc to figure out
   * what type of session we need to join/create.
   */
  if(  (!lpConn->lpSessionDesc ) || 
       ( lpConn->lpSessionDesc->dwSize != sizeof( DPSESSIONDESC2 ) )
    )
  {
    ERR("DPSESSIONDESC passed in? Size=%08lx vs. expected=%ul bytes\n",
         lpConn->lpSessionDesc->dwSize, sizeof( DPSESSIONDESC2 ) );
    return DPERR_INVALIDPARAMS;
  }

  EnterCriticalSection( &This->unk->DPL_lock );

  /* Need to actually store the stuff here. Check if we've already allocated each field first. */
  This->dpl->dwConnFlags = lpConn->dwFlags;

  /* Copy LPDPSESSIONDESC2 struct - this is required */
  memcpy( &This->dpl->sessionDesc, lpConn->lpSessionDesc, sizeof( *(lpConn->lpSessionDesc) ) );

  /* FIXME: Can this just be shorted? Does it handle the NULL case correctly? */
  if( lpConn->lpSessionDesc->sess.lpszSessionName )
    This->dpl->sessionDesc.sess.lpszSessionName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->sess.lpszSessionName );
  else
    This->dpl->sessionDesc.sess.lpszSessionName = NULL;
 
  if( lpConn->lpSessionDesc->pass.lpszPassword )
    This->dpl->sessionDesc.pass.lpszPassword = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->pass.lpszPassword );
  else
    This->dpl->sessionDesc.pass.lpszPassword = NULL;

  This->dpl->sessionDesc.dwReserved1 = lpConn->lpSessionDesc->dwReserved1;
  This->dpl->sessionDesc.dwReserved2 = lpConn->lpSessionDesc->dwReserved2;

  /* Copy DPNAME struct - seems to be optional - check for existance first */
  if( lpConn->lpPlayerName )
  {
     memcpy( &This->dpl->playerName, lpConn->lpPlayerName, sizeof( *lpConn->lpPlayerName ) ); 

     if( lpConn->lpPlayerName->psn.lpszShortName )
       This->dpl->playerName.psn.lpszShortName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->psn.lpszShortName );

     if( lpConn->lpPlayerName->pln.lpszLongName )
       This->dpl->playerName.pln.lpszLongName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->pln.lpszLongName );

  }

  memcpy( &This->dpl->guidSP, &lpConn->guidSP, sizeof( lpConn->guidSP ) );  
  
  This->dpl->lpAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->dwAddressSize ); 
  memcpy( This->dpl->lpAddress, lpConn->lpAddress, lpConn->dwAddressSize );

  This->dpl->dwAddressSize = lpConn->dwAddressSize;

  LeaveCriticalSection( &This->unk->DPL_lock );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobbyAImpl_SetConnectionSettings
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
  TRACE(": This=%p, dwFlags=%08lx, dwAppId=%08lx, lpConn=%p\n",
         This, dwFlags, dwAppID, lpConn );

  /* Paramater check */
  if( dwFlags || !This || !lpConn )
  {
    ERR("invalid parameters.\n");
    return DPERR_INVALIDPARAMS;
  }

  /* See if there is a connection associated with this request.
   * dwAppID == 0 indicates that this request isn't associated with a connection.
   */
  if( dwAppID )
  {
     FIXME(": Connection dwAppID=%08lx given. Not implemented yet.\n",
            dwAppID );

     /* Need to add a check for this application Id...*/
     return DPERR_NOTLOBBIED;
  }

  if(  lpConn->dwSize != sizeof(DPLCONNECTION) )
  {
    ERR(": old/new DPLCONNECTION type? Size=%08lx vs. expected=%ul bytes\n",
         lpConn->dwSize, sizeof( DPLCONNECTION ) );
    return DPERR_INVALIDPARAMS;
  }

  /* Need to investigate the lpConn->lpSessionDesc to figure out
   * what type of session we need to join/create.
   */
  if(  (!lpConn->lpSessionDesc ) ||
       ( lpConn->lpSessionDesc->dwSize != sizeof( DPSESSIONDESC2 ) )
    )
  {
    ERR("DPSESSIONDESC passed in? Size=%08lx vs. expected=%ul bytes\n",
         lpConn->lpSessionDesc->dwSize, sizeof( DPSESSIONDESC2 ) );
    return DPERR_INVALIDPARAMS;
  }

  EnterCriticalSection( &This->unk->DPL_lock );

  /* Need to actually store the stuff here. Check if we've already allocated each field first. */
  This->dpl->dwConnFlags = lpConn->dwFlags;

  /* Copy LPDPSESSIONDESC2 struct - this is required */
  memcpy( &This->dpl->sessionDesc, lpConn->lpSessionDesc, sizeof( *(lpConn->lpSessionDesc) ) );

  /* FIXME: Can this just be shorted? Does it handle the NULL case correctly? */
  if( lpConn->lpSessionDesc->sess.lpszSessionNameA )
    This->dpl->sessionDesc.sess.lpszSessionNameA = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->sess.lpszSessionNameA );
  else
    This->dpl->sessionDesc.sess.lpszSessionNameA = NULL;

  if( lpConn->lpSessionDesc->pass.lpszPasswordA )
    This->dpl->sessionDesc.pass.lpszPasswordA = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->pass.lpszPasswordA );
  else
    This->dpl->sessionDesc.pass.lpszPasswordA = NULL;

  This->dpl->sessionDesc.dwReserved1 = lpConn->lpSessionDesc->dwReserved1;
  This->dpl->sessionDesc.dwReserved2 = lpConn->lpSessionDesc->dwReserved2;

  /* Copy DPNAME struct - seems to be optional - check for existance first */
  if( lpConn->lpPlayerName )
  {
     memcpy( &This->dpl->playerName, lpConn->lpPlayerName, sizeof( *lpConn->lpPlayerName ) );

     if( lpConn->lpPlayerName->psn.lpszShortNameA )
       This->dpl->playerName.psn.lpszShortNameA = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->psn.lpszShortNameA );

     if( lpConn->lpPlayerName->pln.lpszLongNameA )
       This->dpl->playerName.pln.lpszLongNameA = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->pln.lpszLongNameA );

  }

  memcpy( &This->dpl->guidSP, &lpConn->guidSP, sizeof( lpConn->guidSP ) );

  This->dpl->lpAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->dwAddressSize );
  memcpy( This->dpl->lpAddress, lpConn->lpAddress, lpConn->dwAddressSize );

  This->dpl->dwAddressSize = lpConn->dwAddressSize;

  LeaveCriticalSection( &This->unk->DPL_lock );

  return DP_OK;
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


/* DPL 2 methods - well actuall only one */

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
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_CreateCompoundAddress
( LPDIRECTPLAYLOBBY2A iface,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
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
  XCAST(AddRef)IDirectPlayLobbyAImpl_AddRef,
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
  XCAST(AddRef)IDirectPlayLobbyAImpl_AddRef,
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
  XCAST(AddRef)IDirectPlayLobbyAImpl_AddRef,
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
  XCAST(AddRef)IDirectPlayLobbyAImpl_AddRef, 
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
  XCAST(AddRef)IDirectPlayLobbyAImpl_AddRef,
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
  XCAST(AddRef)IDirectPlayLobbyAImpl_AddRef,
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
  if ( lpGUIDDSP || lpUnk || lpData || dwDataSize )
  {
     *lplpDPL = NULL;
     ERR("Bad parameters!\n" );
     return DPERR_INVALIDPARAMS;
  }

  return directPlayLobby_QueryInterface( &IID_IDirectPlayLobby, (void**)lplpDPL );  

}
