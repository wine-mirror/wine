/* Direct Play 2,3,4 Implementation
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
#include "dplay.h"
#include "heap.h"
#include "debugtools.h"

#include "dpinit.h"
#include "dplayx_global.h"

/* FIXME: This stuff shouldn't really be here. It indicates a poor architectural coupling */
#include "dplobby.h"
extern HRESULT DPL_CreateCompoundAddress ( LPCDPCOMPOUNDADDRESSELEMENT lpElements, DWORD dwElementCount,
                                           LPVOID lpAddress, LPDWORD lpdwAddressSize, BOOL bAnsiInterface );

DEFAULT_DEBUG_CHANNEL(dplay)


/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectPlay2Impl IDirectPlay2AImpl;
typedef struct IDirectPlay2Impl IDirectPlay2Impl;
typedef struct IDirectPlay3Impl IDirectPlay3AImpl;
typedef struct IDirectPlay3Impl IDirectPlay3Impl;
typedef struct IDirectPlay4Impl IDirectPlay4AImpl;
typedef struct IDirectPlay4Impl IDirectPlay4Impl;

/*****************************************************************************
 * IDirectPlay implementation structure
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
typedef struct tagDirectPlayIUnknownData
{
  DWORD             ref;
  CRITICAL_SECTION  DP_lock;
} DirectPlayIUnknownData;

/* Contains all dp1 and dp2 data members */
typedef struct tagDirectPlay2Data
{
  BOOL dummy;
} DirectPlay2Data;

typedef struct tagDirectPlay3Data
{
  BOOL connectionInitialized;
} DirectPlay3Data;

typedef struct tagDirectPlay4Data
{
  BOOL dummy;
} DirectPlay4Data;

#define DP_IMPL_FIELDS \
  DirectPlayIUnknownData*  unk; \
  DirectPlay2Data*         dp2; \
  DirectPlay3Data*         dp3; \
  DirectPlay4Data*         dp4;

struct IDirectPlay2Impl
{
  ICOM_VFIELD(IDirectPlay2); 
  DP_IMPL_FIELDS
};

struct IDirectPlay3Impl
{
  ICOM_VFIELD(IDirectPlay3);
  DP_IMPL_FIELDS
};

struct IDirectPlay4Impl
{
  ICOM_VFIELD(IDirectPlay4);
  DP_IMPL_FIELDS
};

/* Forward declarations of virtual tables */
static ICOM_VTABLE(IDirectPlay2) directPlay2AVT;
static ICOM_VTABLE(IDirectPlay3) directPlay3AVT;
static ICOM_VTABLE(IDirectPlay4) directPlay4AVT;

static ICOM_VTABLE(IDirectPlay2) directPlay2WVT;
static ICOM_VTABLE(IDirectPlay3) directPlay3WVT;
static ICOM_VTABLE(IDirectPlay4) directPlay4WVT;

BOOL DP_CreateIUnknown( LPVOID lpDP )
{
  ICOM_THIS(IDirectPlay2AImpl,lpDP);

  This->unk = (DirectPlayIUnknownData*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                  sizeof( *(This->unk) ) );
  if ( This->unk == NULL )
  {
    return FALSE;
  }

  InitializeCriticalSection( &This->unk->DP_lock );

  IDirectPlay_AddRef( (LPDIRECTPLAY2A)lpDP );

  return TRUE;
}

BOOL DP_DestroyIUnknown( LPVOID lpDP )
{
  ICOM_THIS(IDirectPlay2AImpl,lpDP);

  DeleteCriticalSection( &This->unk->DP_lock );
  HeapFree( GetProcessHeap(), 0, This->unk );

  return TRUE;
}

BOOL DP_CreateDirectPlay2( LPVOID lpDP )
{
  ICOM_THIS(IDirectPlay2AImpl,lpDP);

  This->dp2 = (DirectPlay2Data*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           sizeof( *(This->dp2) ) );
  if ( This->dp2 == NULL )
  {
    return FALSE;
  }

  return TRUE;
}

BOOL DP_DestroyDirectPlay2( LPVOID lpDP )
{
  ICOM_THIS(IDirectPlay2AImpl,lpDP);

  /* Delete the contents */
  HeapFree( GetProcessHeap(), 0, This->dp2 );

  return TRUE;
}

BOOL DP_CreateDirectPlay3( LPVOID lpDP )
{
  ICOM_THIS(IDirectPlay3AImpl,lpDP);

  This->dp3 = (DirectPlay3Data*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           sizeof( *(This->dp3) ) );
  if ( This->dp3 == NULL )
  {
    return FALSE;
  }

  This->dp3->connectionInitialized = FALSE;

  return TRUE;
}

BOOL DP_DestroyDirectPlay3( LPVOID lpDP )
{
  ICOM_THIS(IDirectPlay3AImpl,lpDP);

  /* Delete the contents */
  HeapFree( GetProcessHeap(), 0, This->dp3 );

  return TRUE;
}

BOOL DP_CreateDirectPlay4( LPVOID lpDP )
{
  ICOM_THIS(IDirectPlay4AImpl,lpDP);

  This->dp4 = (DirectPlay4Data*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           sizeof( *(This->dp4) ) );
  if ( This->dp4 == NULL )
  {
    return FALSE;
  }

  return TRUE;
}

BOOL DP_DestroyDirectPlay4( LPVOID lpDP )
{
  ICOM_THIS(IDirectPlay3AImpl,lpDP);

  /* Delete the contents */
  HeapFree( GetProcessHeap(), 0, This->dp4 );

  return TRUE;
}


/* Get a new interface. To be used by QueryInterface. */ 
extern 
HRESULT directPlay_QueryInterface 
         ( REFIID riid, LPVOID* ppvObj )
{

  if( IsEqualGUID( &IID_IDirectPlay2, riid ) )
  {
    *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof( IDirectPlay2Impl ) );

    if( *ppvObj == NULL )
    {
       return DPERR_OUTOFMEMORY;
    }

    /* new scope for variable declaration */
    {
      ICOM_THIS(IDirectPlay2Impl,*ppvObj);

      ICOM_VTBL(This) = &directPlay2WVT;

      if ( DP_CreateIUnknown( (LPVOID)This ) &&
           DP_CreateDirectPlay2( (LPVOID)This )
         )
      {
        return S_OK;
      }

    }

    goto error; 
  } 
  else if( IsEqualGUID( &IID_IDirectPlay2A, riid ) )
  {
    *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof( IDirectPlay2AImpl ) );

    if( *ppvObj == NULL )
    {
       return DPERR_OUTOFMEMORY;
    }

    /* new scope for variable declaration */
    {
      ICOM_THIS(IDirectPlay2AImpl,*ppvObj);

      ICOM_VTBL(This) = &directPlay2AVT;

      if ( DP_CreateIUnknown( (LPVOID)This ) &&
           DP_CreateDirectPlay2( (LPVOID)This )
         )
      {
        return S_OK;
      }

    }

    goto error;
  }
  else if( IsEqualGUID( &IID_IDirectPlay3, riid ) )
  {
    *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof( IDirectPlay3Impl ) );

    if( *ppvObj == NULL )
    {
       return DPERR_OUTOFMEMORY;
    }

    /* new scope for variable declaration */
    {
      ICOM_THIS(IDirectPlay3Impl,*ppvObj);

      ICOM_VTBL(This) = &directPlay3WVT;

      if ( DP_CreateIUnknown( (LPVOID)This ) &&
           DP_CreateDirectPlay2( (LPVOID)This ) &&
           DP_CreateDirectPlay3( (LPVOID)This )
         )
      {
        return S_OK;
      }

    }

    goto error;
  }
  else if( IsEqualGUID( &IID_IDirectPlay3A, riid ) )
  {
    *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof( IDirectPlay3AImpl ) );

    if( *ppvObj == NULL )
    {
       return DPERR_OUTOFMEMORY;
    }

    /* new scope for variable declaration */
    {
      ICOM_THIS(IDirectPlay3AImpl,*ppvObj);

      ICOM_VTBL(This) = &directPlay3AVT;

      if ( DP_CreateIUnknown( (LPVOID)This ) &&
           DP_CreateDirectPlay2( (LPVOID)This ) &&
           DP_CreateDirectPlay3( (LPVOID)This )
         )
      {
        return S_OK;
      }

    }

    goto error;
  }
  else if( IsEqualGUID( &IID_IDirectPlay4, riid ) )
  {
    *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof( IDirectPlay4Impl ) );

    if( *ppvObj == NULL )
    {
       return DPERR_OUTOFMEMORY;
    }

    /* new scope for variable declaration */
    {
      ICOM_THIS(IDirectPlay4Impl,*ppvObj);

      ICOM_VTBL(This) = &directPlay4WVT;

      if ( DP_CreateIUnknown( (LPVOID)This ) &&
           DP_CreateDirectPlay2( (LPVOID)This ) &&
           DP_CreateDirectPlay3( (LPVOID)This ) &&
           DP_CreateDirectPlay4( (LPVOID)This )
         )
      {
        return S_OK;
      }

    }

    goto error;
  }
  else if( IsEqualGUID( &IID_IDirectPlay4A, riid ) )
  {
    *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof( IDirectPlay4AImpl ) );

    if( *ppvObj == NULL )
    {
       return DPERR_OUTOFMEMORY;
    }

    /* new scope for variable declaration */
    {
      ICOM_THIS(IDirectPlay4AImpl,*ppvObj);

      ICOM_VTBL(This) = &directPlay4AVT;

      if ( DP_CreateIUnknown( (LPVOID)This ) &&
           DP_CreateDirectPlay2( (LPVOID)This ) &&
           DP_CreateDirectPlay3( (LPVOID)This ) &&
           DP_CreateDirectPlay4( (LPVOID)This )
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

    DP_DestroyDirectPlay4( *ppvObj );
    DP_DestroyDirectPlay3( *ppvObj );
    DP_DestroyDirectPlay2( *ppvObj );
    DP_DestroyIUnknown( *ppvObj );
    HeapFree( GetProcessHeap(), 0, *ppvObj );

    *ppvObj = NULL;
    return DPERR_NOMEMORY;

}


/* Direct Play methods */
static HRESULT WINAPI DirectPlay2W_QueryInterface
         ( LPDIRECTPLAY2 iface, REFIID riid, LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  TRACE("(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay2, riid )
    )
  {
    IDirectPlayX_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }
  return directPlay_QueryInterface( riid, ppvObj );
}

static HRESULT WINAPI DirectPlay2A_QueryInterface
         ( LPDIRECTPLAY2A iface, REFIID riid, LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  TRACE("(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay2A, riid )
    )
  {
    IDirectPlayX_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlay_QueryInterface( riid, ppvObj );
}

static HRESULT WINAPI DirectPlay3WImpl_QueryInterface
         ( LPDIRECTPLAY3 iface, REFIID riid, LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  TRACE("(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay3, riid )
    )
  {
    IDirectPlayX_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlay_QueryInterface( riid, ppvObj );
}

static HRESULT WINAPI DirectPlay3AImpl_QueryInterface
         ( LPDIRECTPLAY3A iface, REFIID riid, LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  TRACE("(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay3A, riid )
    )
  {
    IDirectPlayX_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlay_QueryInterface( riid, ppvObj );
}

static HRESULT WINAPI DirectPlay4WImpl_QueryInterface
         ( LPDIRECTPLAY4 iface, REFIID riid, LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  TRACE("(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay4, riid )
    )
  {
    IDirectPlayX_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlay_QueryInterface( riid, ppvObj );
}


static HRESULT WINAPI DirectPlay4AImpl_QueryInterface
         ( LPDIRECTPLAY4A iface, REFIID riid, LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  TRACE("(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay4A, riid )
    )
  {
    IDirectPlayX_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlay_QueryInterface( riid, ppvObj );
}


/* Shared between all dplay types */
static ULONG WINAPI DirectPlay2AImpl_AddRef
         ( LPDIRECTPLAY3 iface )
{
  ULONG refCount;
  ICOM_THIS(IDirectPlay3Impl,iface);

  EnterCriticalSection( &This->unk->DP_lock ); 
  {
    refCount = ++(This->unk->ref);
  }  
  LeaveCriticalSection( &This->unk->DP_lock );

  TRACE("ref count incremented to %lu for %p\n", refCount, This );

  return refCount;
}

static ULONG WINAPI DirectPlay2AImpl_Release
( LPDIRECTPLAY3 iface )
{
  ULONG refCount;

  ICOM_THIS(IDirectPlay3Impl,iface);

  EnterCriticalSection( &This->unk->DP_lock );
  {
    refCount = --(This->unk->ref);
  }
  LeaveCriticalSection( &This->unk->DP_lock );

  TRACE("ref count decremeneted to %lu for %p\n", refCount, This );

  /* Deallocate if this is the last reference to the object */
  if( refCount == 0 )
  {
     DP_DestroyDirectPlay4( This );
     DP_DestroyDirectPlay3( This );
     DP_DestroyDirectPlay2( This );
     DP_DestroyIUnknown( This );
     HeapFree( GetProcessHeap(), 0, This );
  }

  return refCount;
}

static HRESULT WINAPI DirectPlay2AImpl_AddPlayerToGroup
          ( LPDIRECTPLAY2A iface, DPID idGroup, DPID idPlayer )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idGroup, idPlayer );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_AddPlayerToGroup
          ( LPDIRECTPLAY2 iface, DPID idGroup, DPID idPlayer )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idGroup, idPlayer );
  return DP_OK;
}


static HRESULT WINAPI DirectPlay2AImpl_Close
          ( LPDIRECTPLAY2A iface )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(): stub\n", This );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_Close
          ( LPDIRECTPLAY2 iface )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(): stub\n", This );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_CreateGroup
          ( LPDIRECTPLAY2A iface, LPDPID lpidGroup, LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p,%p,0x%08lx,0x%08lx): stub\n", This, lpidGroup, lpGroupName, lpData, dwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_CreateGroup
          ( LPDIRECTPLAY2 iface, LPDPID lpidGroup, LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p,%p,0x%08lx,0x%08lx): stub\n", This, lpidGroup, lpGroupName, lpData, dwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_CreatePlayer
          ( LPDIRECTPLAY2A iface, LPDPID lpidPlayer, LPDPNAME lpPlayerName, HANDLE hEvent, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p,%d,%p,0x%08lx,0x%08lx): stub\n", This, lpidPlayer, lpPlayerName, hEvent, lpData, dwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_CreatePlayer
          ( LPDIRECTPLAY2 iface, LPDPID lpidPlayer, LPDPNAME lpPlayerName, HANDLE hEvent, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p,%d,%p,0x%08lx,0x%08lx): stub\n", This, lpidPlayer, lpPlayerName, hEvent, lpData, dwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_DeletePlayerFromGroup
          ( LPDIRECTPLAY2A iface, DPID idGroup, DPID idPlayer )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idGroup, idPlayer );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_DeletePlayerFromGroup
          ( LPDIRECTPLAY2 iface, DPID idGroup, DPID idPlayer )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idGroup, idPlayer );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_DestroyGroup
          ( LPDIRECTPLAY2A iface, DPID idGroup )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx): stub\n", This, idGroup );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_DestroyGroup
          ( LPDIRECTPLAY2 iface, DPID idGroup )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx): stub\n", This, idGroup );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_DestroyPlayer
          ( LPDIRECTPLAY2A iface, DPID idPlayer )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx): stub\n", This, idPlayer );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_DestroyPlayer
          ( LPDIRECTPLAY2 iface, DPID idPlayer )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx): stub\n", This, idPlayer );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_EnumGroupPlayers
          ( LPDIRECTPLAY2A iface, DPID idGroup, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub\n", This, idGroup, lpguidInstance, lpEnumPlayersCallback2, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_EnumGroupPlayers
          ( LPDIRECTPLAY2 iface, DPID idGroup, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub\n", This, idGroup, lpguidInstance, lpEnumPlayersCallback2, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_EnumGroups
          ( LPDIRECTPLAY2A iface, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p,%p,0x%08lx): stub\n", This, lpguidInstance, lpEnumPlayersCallback2, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_EnumGroups
          ( LPDIRECTPLAY2 iface, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p,%p,0x%08lx): stub\n", This, lpguidInstance, lpEnumPlayersCallback2, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_EnumPlayers
          ( LPDIRECTPLAY2A iface, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p,%p,0x%08lx): stub\n", This, lpguidInstance, lpEnumPlayersCallback2, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_EnumPlayers
          ( LPDIRECTPLAY2 iface, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p,%p,0x%08lx): stub\n", This, lpguidInstance, lpEnumPlayersCallback2, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_EnumSessions
          ( LPDIRECTPLAY2A iface, LPDPSESSIONDESC2 lpsd, DWORD dwTimeout, LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,0x%08lx,%p,%p,0x%08lx): stub\n", This, lpsd, dwTimeout, lpEnumSessionsCallback2, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_EnumSessions
          ( LPDIRECTPLAY2 iface, LPDPSESSIONDESC2 lpsd, DWORD dwTimeout, LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,0x%08lx,%p,%p,0x%08lx): stub\n", This, lpsd, dwTimeout, lpEnumSessionsCallback2, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetCaps
          ( LPDIRECTPLAY2A iface, LPDPCAPS lpDPCaps, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,0x%08lx): stub\n", This, lpDPCaps, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_GetCaps
          ( LPDIRECTPLAY2 iface, LPDPCAPS lpDPCaps, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,0x%08lx): stub\n", This, lpDPCaps, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetGroupData
          ( LPDIRECTPLAY2 iface, DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p,0x%08lx): stub\n", This, idGroup, lpData, lpdwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_GetGroupData
          ( LPDIRECTPLAY2 iface, DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p,0x%08lx): stub\n", This, idGroup, lpData, lpdwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetGroupName
          ( LPDIRECTPLAY2A iface, DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, idGroup, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_GetGroupName
          ( LPDIRECTPLAY2 iface, DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, idGroup, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetMessageCount
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPDWORD lpdwCount )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idPlayer, lpdwCount );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_GetMessageCount
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPDWORD lpdwCount )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idPlayer, lpdwCount );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetPlayerAddress
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, idPlayer, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_GetPlayerAddress
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, idPlayer, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetPlayerCaps
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPDPCAPS lpPlayerCaps, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,0x%08lx): stub\n", This, idPlayer, lpPlayerCaps, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_GetPlayerCaps
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPDPCAPS lpPlayerCaps, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,0x%08lx): stub\n", This, idPlayer, lpPlayerCaps, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetPlayerData
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p,0x%08lx): stub\n", This, idPlayer, lpData, lpdwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_GetPlayerData
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p,0x%08lx): stub\n", This, idPlayer, lpData, lpdwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetPlayerName
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, idPlayer, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_GetPlayerName
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, idPlayer, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetSessionDesc
          ( LPDIRECTPLAY2A iface, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p): stub\n", This, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_GetSessionDesc
          ( LPDIRECTPLAY2 iface, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p): stub\n", This, lpData, lpdwDataSize );
  return DP_OK;
}

/* Intended only for COM compatibility. Always returns an error. */
static HRESULT WINAPI DirectPlay2AImpl_Initialize
          ( LPDIRECTPLAY2A iface, LPGUID lpGUID )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  TRACE("(%p)->(%p): stub\n", This, lpGUID );
  return DPERR_ALREADYINITIALIZED;
}

/* Intended only for COM compatibility. Always returns an error. */
static HRESULT WINAPI DirectPlay2WImpl_Initialize
          ( LPDIRECTPLAY2 iface, LPGUID lpGUID )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  TRACE("(%p)->(%p): stub\n", This, lpGUID );
  return DPERR_ALREADYINITIALIZED;
}


static HRESULT WINAPI DirectPlay2AImpl_Open
          ( LPDIRECTPLAY2A iface, LPDPSESSIONDESC2 lpsd, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,0x%08lx): stub\n", This, lpsd, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_Open
          ( LPDIRECTPLAY2 iface, LPDPSESSIONDESC2 lpsd, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,0x%08lx): stub\n", This, lpsd, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_Receive
          ( LPDIRECTPLAY2A iface, LPDPID lpidFrom, LPDPID lpidTo, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p,0x%08lx,%p,%p): stub\n", This, lpidFrom, lpidTo, dwFlags, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_Receive
          ( LPDIRECTPLAY2 iface, LPDPID lpidFrom, LPDPID lpidTo, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,%p,0x%08lx,%p,%p): stub\n", This, lpidFrom, lpidTo, dwFlags, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_Send
          ( LPDIRECTPLAY2A iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,0x%08lx): stub\n", This, idFrom, idTo, dwFlags, lpData, dwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_Send
          ( LPDIRECTPLAY2 iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,0x%08lx): stub\n", This, idFrom, idTo, dwFlags, lpData, dwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_SetGroupData
          ( LPDIRECTPLAY2A iface, DPID idGroup, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub\n", This, idGroup, lpData, dwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_SetGroupData
          ( LPDIRECTPLAY2 iface, DPID idGroup, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{   
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub\n", This, idGroup, lpData, dwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_SetGroupName
          ( LPDIRECTPLAY2A iface, DPID idGroup, LPDPNAME lpGroupName, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,0x%08lx): stub\n", This, idGroup, lpGroupName, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_SetGroupName
          ( LPDIRECTPLAY2 iface, DPID idGroup, LPDPNAME lpGroupName, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,0x%08lx): stub\n", This, idGroup, lpGroupName, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_SetPlayerData
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub\n", This, idPlayer, lpData, dwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_SetPlayerData
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub\n", This, idPlayer, lpData, dwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_SetPlayerName
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPDPNAME lpPlayerName, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,0x%08lx): stub\n", This, idPlayer, lpPlayerName, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_SetPlayerName
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPDPNAME lpPlayerName, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,0x%08lx): stub\n", This, idPlayer, lpPlayerName, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_SetSessionDesc
          ( LPDIRECTPLAY2A iface, LPDPSESSIONDESC2 lpSessDesc, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,0x%08lx): stub\n", This, lpSessDesc, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_SetSessionDesc
          ( LPDIRECTPLAY2 iface, LPDPSESSIONDESC2 lpSessDesc, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  FIXME("(%p)->(%p,0x%08lx): stub\n", This, lpSessDesc, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_AddGroupToGroup
          ( LPDIRECTPLAY3A iface, DPID idParentGroup, DPID idGroup )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idParentGroup, idGroup );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_AddGroupToGroup
          ( LPDIRECTPLAY3 iface, DPID idParentGroup, DPID idGroup )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idParentGroup, idGroup );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_CreateGroupInGroup
          ( LPDIRECTPLAY3A iface, DPID idParentGroup, LPDPID lpidGroup, LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p,%p,0x%08lx,0x%08lx): stub\n", This, idParentGroup, lpidGroup, lpGroupName, lpData, dwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_CreateGroupInGroup
          ( LPDIRECTPLAY3 iface, DPID idParentGroup, LPDPID lpidGroup, LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p,%p,0x%08lx,0x%08lx): stub\n", This, idParentGroup, lpidGroup, lpGroupName, lpData, dwDataSize, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_DeleteGroupFromGroup
          ( LPDIRECTPLAY3A iface, DPID idParentGroup, DPID idGroup )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idParentGroup, idGroup );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_DeleteGroupFromGroup
          ( LPDIRECTPLAY3 iface, DPID idParentGroup, DPID idGroup )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idParentGroup, idGroup );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_EnumConnections
          ( LPDIRECTPLAY3A iface, LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  TRACE("(%p)->(%p,%p,%p,0x%08lx)\n", This, lpguidApplication, lpEnumCallback, lpContext, dwFlags );

  /* A default dwFlags (0) is backwards compatible -- DPCONNECTION_DIRECTPLAY */
  if( dwFlags == 0 )
  {
    dwFlags = DPCONNECTION_DIRECTPLAY;
  }

  if( ! ( ( dwFlags & DPCONNECTION_DIRECTPLAY ) ||
          ( dwFlags & DPCONNECTION_DIRECTPLAYLOBBY ) )
    ) 
  {
    return DPERR_INVALIDFLAGS;
  }

  if( !lpEnumCallback || !*lpEnumCallback )
  {
     return DPERR_INVALIDPARAMS;
  }

  /* Enumerate DirectPlay service providers */
  if( dwFlags & DPCONNECTION_DIRECTPLAY )
  {
    HKEY hkResult;
    LPCSTR searchSubKey    = "SOFTWARE\\Microsoft\\DirectPlay\\Service Providers";
    LPSTR guidDataSubKey   = "Guid";
    DWORD dwIndex, sizeOfSubKeyName=50;
    char subKeyName[51]; 

    /* Need to loop over the service providers in the registry */
    if( RegOpenKeyExA( HKEY_LOCAL_MACHINE, searchSubKey,
                         0, KEY_ENUMERATE_SUB_KEYS, &hkResult ) != ERROR_SUCCESS )
    {
      /* Hmmm. Does this mean that there are no service providers? */
      ERR(": no service providers?\n");
      return DP_OK;
    }


    /* Traverse all the service providers we have available */
    for( dwIndex=0;
         RegEnumKeyA( hkResult, dwIndex, subKeyName, sizeOfSubKeyName ) != ERROR_NO_MORE_ITEMS;
         ++dwIndex )
    {

      HKEY     hkServiceProvider;
      GUID     serviceProviderGUID;
      DWORD    returnTypeGUID, sizeOfReturnBuffer = 50;
      char     returnBuffer[51];
      LPWSTR   lpWGUIDString;
      DPNAME   dpName;
      HRESULT  hr;

      DPCOMPOUNDADDRESSELEMENT dpCompoundAddress;
      LPVOID                   lpAddressBuffer = NULL;
      DWORD                    dwAddressBufferSize = 0;

      TRACE(" this time through: %s\n", subKeyName );

      /* Get a handle for this particular service provider */
      if( RegOpenKeyExA( hkResult, subKeyName, 0, KEY_QUERY_VALUE,
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

      /* Fill in the DPNAME struct for the service provider */
      dpName.dwSize             = sizeof( dpName );
      dpName.dwFlags            = 0;
      dpName.psn.lpszShortNameA = subKeyName;
      dpName.pln.lpszLongNameA  = NULL;

      /* Create the compound address for the service provider. 
         NOTE: This is a gruesome architectural scar right now. DP uses DPL and DPL uses DP
               nast stuff. This may be why the native dll just gets around this little bit by
               allocating an 80 byte buffer which isn't even a filled with a valid compound 
               address. Oh well. Creating a proper compound address is the way to go anyways 
               despite this method taking slightly more heap space and realtime :) */
      dpCompoundAddress.guidDataType = DPAID_ServiceProvider;
      dpCompoundAddress.dwDataSize   = sizeof( GUID );
      dpCompoundAddress.lpData       = &serviceProviderGUID; 

      if( ( hr = DPL_CreateCompoundAddress( &dpCompoundAddress, 1, lpAddressBuffer, 
                                     &dwAddressBufferSize, TRUE ) ) != DPERR_BUFFERTOOSMALL )
      {
        ERR( "can't get buffer size: %s\n", DPLAYX_HresultToString( hr ) );
        return hr;
      }

      /* Now allocate the buffer */
      lpAddressBuffer = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwAddressBufferSize );

      if( ( hr = DPL_CreateCompoundAddress( &dpCompoundAddress, 1, lpAddressBuffer,
                                     &dwAddressBufferSize, TRUE ) ) != DP_OK )
      {
        ERR( "can't create address: %s\n", DPLAYX_HresultToString( hr ) );
        return hr;
      }

      /* The enumeration will return FALSE if we are not to continue */
      if( !lpEnumCallback( &serviceProviderGUID, lpAddressBuffer, dwAddressBufferSize, 
                           &dpName, DPCONNECTION_DIRECTPLAY, lpContext ) )
      {
         WARN("lpEnumCallback returning FALSE\n" );

         return DP_OK;
      }
    }
  }

  /* Enumerate DirectPlayLobby service providers */
  if( dwFlags & DPCONNECTION_DIRECTPLAYLOBBY )
  {
    FIXME( "DPCONNECTION_DIRECTPLAYLOBBY flag not handled\n" );
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_EnumConnections
          ( LPDIRECTPLAY3 iface, LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags )
{ 
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(%p,%p,%p,0x%08lx): stub\n", This, lpguidApplication, lpEnumCallback, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_EnumGroupsInGroup
          ( LPDIRECTPLAY3A iface, DPID idGroup, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumCallback, LPVOID lpContext, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub\n", This, idGroup, lpguidInstance, lpEnumCallback, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_EnumGroupsInGroup
          ( LPDIRECTPLAY3 iface, DPID idGroup, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumCallback, LPVOID lpContext, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub\n", This, idGroup, lpguidInstance, lpEnumCallback, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_GetGroupConnectionSettings
          ( LPDIRECTPLAY3A iface, DWORD dwFlags, DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,%p,%p): stub\n", This, dwFlags, idGroup, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_GetGroupConnectionSettings
          ( LPDIRECTPLAY3 iface, DWORD dwFlags, DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,%p,%p): stub\n", This, dwFlags, idGroup, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_InitializeConnection
          ( LPDIRECTPLAY3A iface, LPVOID lpConnection, DWORD dwFlags )
{
  HMODULE hServiceProvider;
  typedef DWORD (WINAPI *SP_SPInit)(LPVOID lpCompoundAddress, ...); /* FIXME: How many arguments? */
  SP_SPInit SPInit;

  ICOM_THIS(IDirectPlay3Impl,iface);

  FIXME("(%p)->(%p,0x%08lx): stub\n", This, lpConnection, dwFlags );

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDFLAGS;
  }

  if( This->dp3->connectionInitialized == TRUE )
  {
    return DPERR_ALREADYINITIALIZED;
  }

  /* Parse lpConnection as a compound address for the service provider */
  /* Take service provider GUID and find the path to it */

  /* FIXME: Hard coded to only load the tcp/ip service provider for now... */
  hServiceProvider = LoadLibraryA( "dpwsockx.dll" );

  if( hServiceProvider == 0 )
  {
    ERR( "Unable to load service provider\n" );
    return DPERR_UNAVAILABLE; 
  }
  
  /* Initialize the service provider by calling SPInit */
  SPInit = (SP_SPInit)GetProcAddress( hServiceProvider, "SPInit" );

  if( SPInit == NULL )
  {
    ERR( "Service provider doesn't provide SPInit interface?\n" );
  }  

#if 0
  /* NOTE: This will crash until I know what parameters/interface this has */
  /* FIXME: Take a guess that we just pass the compound address to the SP */
  /* Hmmm...how to say which parameters need to be gotten from the SP. They must
     come from the compound address, but how do we communicate what's required? */
  dwReturnValue = (*SPInit)( lpConnection );
#endif

  /* This interface is now initialized */
  This->dp3->connectionInitialized = TRUE;

  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_InitializeConnection
          ( LPDIRECTPLAY3 iface, LPVOID lpConnection, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(%p,0x%08lx): stub\n", This, lpConnection, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_SecureOpen
          ( LPDIRECTPLAY3A iface, LPCDPSESSIONDESC2 lpsd, DWORD dwFlags, LPCDPSECURITYDESC lpSecurity, LPCDPCREDENTIALS lpCredentials )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(%p,0x%08lx,%p,%p): stub\n", This, lpsd, dwFlags, lpSecurity, lpCredentials );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_SecureOpen
          ( LPDIRECTPLAY3 iface, LPCDPSESSIONDESC2 lpsd, DWORD dwFlags, LPCDPSECURITYDESC lpSecurity, LPCDPCREDENTIALS lpCredentials )
{   
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(%p,0x%08lx,%p,%p): stub\n", This, lpsd, dwFlags, lpSecurity, lpCredentials );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_SendChatMessage
          ( LPDIRECTPLAY3A iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPDPCHAT lpChatMessage )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx,%p): stub\n", This, idFrom, idTo, dwFlags, lpChatMessage );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_SendChatMessage
          ( LPDIRECTPLAY3 iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPDPCHAT lpChatMessage )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx,%p): stub\n", This, idFrom, idTo, dwFlags, lpChatMessage );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_SetGroupConnectionSettings
          ( LPDIRECTPLAY3A iface, DWORD dwFlags, DPID idGroup, LPDPLCONNECTION lpConnection )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,%p): stub\n", This, dwFlags, idGroup, lpConnection );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_SetGroupConnectionSettings
          ( LPDIRECTPLAY3 iface, DWORD dwFlags, DPID idGroup, LPDPLCONNECTION lpConnection )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,%p): stub\n", This, dwFlags, idGroup, lpConnection );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_StartSession
          ( LPDIRECTPLAY3A iface, DWORD dwFlags, DPID idGroup )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, dwFlags, idGroup );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_StartSession
          ( LPDIRECTPLAY3 iface, DWORD dwFlags, DPID idGroup )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, dwFlags, idGroup );
  return DP_OK;
}
 
static HRESULT WINAPI DirectPlay3AImpl_GetGroupFlags
          ( LPDIRECTPLAY3A iface, DPID idGroup, LPDWORD lpdwFlags )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idGroup, lpdwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_GetGroupFlags
          ( LPDIRECTPLAY3 iface, DPID idGroup, LPDWORD lpdwFlags )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idGroup, lpdwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_GetGroupParent
          ( LPDIRECTPLAY3A iface, DPID idGroup, LPDPID lpidGroup )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idGroup, lpidGroup );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_GetGroupParent
          ( LPDIRECTPLAY3 iface, DPID idGroup, LPDPID lpidGroup )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idGroup, lpidGroup );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_GetPlayerAccount
          ( LPDIRECTPLAY3A iface, DPID idPlayer, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,%p,%p): stub\n", This, idPlayer, dwFlags, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_GetPlayerAccount
          ( LPDIRECTPLAY3 iface, DPID idPlayer, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,%p,%p): stub\n", This, idPlayer, dwFlags, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_GetPlayerFlags
          ( LPDIRECTPLAY3A iface, DPID idPlayer, LPDWORD lpdwFlags )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idPlayer, lpdwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_GetPlayerFlags
          ( LPDIRECTPLAY3 iface, DPID idPlayer, LPDWORD lpdwFlags )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idPlayer, lpdwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4AImpl_GetGroupOwner
          ( LPDIRECTPLAY4A iface, DPID idGroup, LPDPID lpidGroupOwner )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idGroup, lpidGroupOwner );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4WImpl_GetGroupOwner
          ( LPDIRECTPLAY4 iface, DPID idGroup, LPDPID lpidGroupOwner )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idGroup, lpidGroupOwner );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4AImpl_SetGroupOwner
          ( LPDIRECTPLAY4A iface, DPID idGroup , DPID idGroupOwner )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idGroup, idGroupOwner );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4WImpl_SetGroupOwner
          ( LPDIRECTPLAY4 iface, DPID idGroup , DPID idGroupOwner )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idGroup, idGroupOwner );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4AImpl_SendEx
          ( LPDIRECTPLAY4A iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize, DWORD dwPriority, DWORD dwTimeout, LPVOID lpContext, LPDWORD lpdwMsgID )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,0x%08lx,0x%08lx,0x%08lx,%p,%p): stub\n", This, idFrom, idTo, dwFlags, lpData, dwDataSize, dwPriority, dwTimeout, lpContext, lpdwMsgID );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4WImpl_SendEx
          ( LPDIRECTPLAY4 iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize, DWORD dwPriority, DWORD dwTimeout, LPVOID lpContext, LPDWORD lpdwMsgID )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,0x%08lx,0x%08lx,0x%08lx,%p,%p): stub\n", This, idFrom, idTo, dwFlags, lpData, dwDataSize, dwPriority, dwTimeout, lpContext, lpdwMsgID );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4AImpl_GetMessageQueue
          ( LPDIRECTPLAY4A iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPDWORD lpdwNumMsgs, LPDWORD lpdwNumBytes )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,%p): stub\n", This, idFrom, idTo, dwFlags, lpdwNumMsgs, lpdwNumBytes );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4WImpl_GetMessageQueue
          ( LPDIRECTPLAY4 iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPDWORD lpdwNumMsgs, LPDWORD lpdwNumBytes )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,%p): stub\n", This, idFrom, idTo, dwFlags, lpdwNumMsgs, lpdwNumBytes );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4AImpl_CancelMessage
          ( LPDIRECTPLAY4A iface, DWORD dwMsgID, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, dwMsgID, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4WImpl_CancelMessage
          ( LPDIRECTPLAY4 iface, DWORD dwMsgID, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, dwMsgID, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4AImpl_CancelPriority
          ( LPDIRECTPLAY4A iface, DWORD dwMinPriority, DWORD dwMaxPriority, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx): stub\n", This, dwMinPriority, dwMaxPriority, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4WImpl_CancelPriority
          ( LPDIRECTPLAY4 iface, DWORD dwMinPriority, DWORD dwMaxPriority, DWORD dwFlags )
{
  ICOM_THIS(IDirectPlay4Impl,iface);
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx): stub\n", This, dwMinPriority, dwMaxPriority, dwFlags );
  return DP_OK;
}

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay2WVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif

static ICOM_VTABLE(IDirectPlay2) directPlay2WVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DirectPlay2W_QueryInterface,
  XCAST(AddRef)DirectPlay2AImpl_AddRef,
  XCAST(Release)DirectPlay2AImpl_Release,

  DirectPlay2WImpl_AddPlayerToGroup,
  DirectPlay2WImpl_Close,
  DirectPlay2WImpl_CreateGroup,
  DirectPlay2WImpl_CreatePlayer,
  DirectPlay2WImpl_DeletePlayerFromGroup,
  DirectPlay2WImpl_DestroyGroup,
  DirectPlay2WImpl_DestroyPlayer,
  DirectPlay2WImpl_EnumGroupPlayers,
  DirectPlay2WImpl_EnumGroups,
  DirectPlay2WImpl_EnumPlayers,
  DirectPlay2WImpl_EnumSessions,
  DirectPlay2WImpl_GetCaps,
  DirectPlay2WImpl_GetGroupData,
  DirectPlay2WImpl_GetGroupName,
  DirectPlay2WImpl_GetMessageCount,
  DirectPlay2WImpl_GetPlayerAddress,
  DirectPlay2WImpl_GetPlayerCaps,
  DirectPlay2WImpl_GetPlayerData,
  DirectPlay2WImpl_GetPlayerName,
  DirectPlay2WImpl_GetSessionDesc,
  DirectPlay2WImpl_Initialize,
  DirectPlay2WImpl_Open,
  DirectPlay2WImpl_Receive,
  DirectPlay2WImpl_Send,
  DirectPlay2WImpl_SetGroupData,
  DirectPlay2WImpl_SetGroupName,
  DirectPlay2WImpl_SetPlayerData,
  DirectPlay2WImpl_SetPlayerName,
  DirectPlay2WImpl_SetSessionDesc
};
#undef XCAST

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay2AVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif

static ICOM_VTABLE(IDirectPlay2) directPlay2AVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DirectPlay2A_QueryInterface,
  XCAST(AddRef)DirectPlay2AImpl_AddRef,
  XCAST(Release)DirectPlay2AImpl_Release,

  DirectPlay2AImpl_AddPlayerToGroup,
  DirectPlay2AImpl_Close,
  DirectPlay2AImpl_CreateGroup,
  DirectPlay2AImpl_CreatePlayer,
  DirectPlay2AImpl_DeletePlayerFromGroup,
  DirectPlay2AImpl_DestroyGroup,
  DirectPlay2AImpl_DestroyPlayer,
  DirectPlay2AImpl_EnumGroupPlayers,
  DirectPlay2AImpl_EnumGroups,
  DirectPlay2AImpl_EnumPlayers,
  DirectPlay2AImpl_EnumSessions,
  DirectPlay2AImpl_GetCaps,
  DirectPlay2AImpl_GetGroupData,
  DirectPlay2AImpl_GetGroupName,
  DirectPlay2AImpl_GetMessageCount,
  DirectPlay2AImpl_GetPlayerAddress,
  DirectPlay2AImpl_GetPlayerCaps,
  DirectPlay2AImpl_GetPlayerData,
  DirectPlay2AImpl_GetPlayerName,
  DirectPlay2AImpl_GetSessionDesc,
  DirectPlay2AImpl_Initialize,
  DirectPlay2AImpl_Open,
  DirectPlay2AImpl_Receive,
  DirectPlay2AImpl_Send,
  DirectPlay2AImpl_SetGroupData,
  DirectPlay2AImpl_SetGroupName,
  DirectPlay2AImpl_SetPlayerData,
  DirectPlay2AImpl_SetPlayerName,
  DirectPlay2AImpl_SetSessionDesc
};
#undef XCAST


/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay3AVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif

static ICOM_VTABLE(IDirectPlay3) directPlay3AVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DirectPlay3AImpl_QueryInterface,
  XCAST(AddRef)DirectPlay2AImpl_AddRef,
  XCAST(Release)DirectPlay2AImpl_Release,

  XCAST(AddPlayerToGroup)DirectPlay2AImpl_AddPlayerToGroup,
  XCAST(Close)DirectPlay2AImpl_Close,
  XCAST(CreateGroup)DirectPlay2AImpl_CreateGroup,
  XCAST(CreatePlayer)DirectPlay2AImpl_CreatePlayer,
  XCAST(DeletePlayerFromGroup)DirectPlay2AImpl_DeletePlayerFromGroup,
  XCAST(DestroyGroup)DirectPlay2AImpl_DestroyGroup,
  XCAST(DestroyPlayer)DirectPlay2AImpl_DestroyPlayer,
  XCAST(EnumGroupPlayers)DirectPlay2AImpl_EnumGroupPlayers,
  XCAST(EnumGroups)DirectPlay2AImpl_EnumGroups,
  XCAST(EnumPlayers)DirectPlay2AImpl_EnumPlayers,
  XCAST(EnumSessions)DirectPlay2AImpl_EnumSessions,
  XCAST(GetCaps)DirectPlay2AImpl_GetCaps,
  XCAST(GetGroupData)DirectPlay2AImpl_GetGroupData,
  XCAST(GetGroupName)DirectPlay2AImpl_GetGroupName,
  XCAST(GetMessageCount)DirectPlay2AImpl_GetMessageCount,
  XCAST(GetPlayerAddress)DirectPlay2AImpl_GetPlayerAddress,
  XCAST(GetPlayerCaps)DirectPlay2AImpl_GetPlayerCaps,
  XCAST(GetPlayerData)DirectPlay2AImpl_GetPlayerData,
  XCAST(GetPlayerName)DirectPlay2AImpl_GetPlayerName,
  XCAST(GetSessionDesc)DirectPlay2AImpl_GetSessionDesc,
  XCAST(Initialize)DirectPlay2AImpl_Initialize,
  XCAST(Open)DirectPlay2AImpl_Open,
  XCAST(Receive)DirectPlay2AImpl_Receive,
  XCAST(Send)DirectPlay2AImpl_Send,
  XCAST(SetGroupData)DirectPlay2AImpl_SetGroupData,
  XCAST(SetGroupName)DirectPlay2AImpl_SetGroupName,
  XCAST(SetPlayerData)DirectPlay2AImpl_SetPlayerData,
  XCAST(SetPlayerName)DirectPlay2AImpl_SetPlayerName,
  XCAST(SetSessionDesc)DirectPlay2AImpl_SetSessionDesc,

  DirectPlay3AImpl_AddGroupToGroup,
  DirectPlay3AImpl_CreateGroupInGroup,
  DirectPlay3AImpl_DeleteGroupFromGroup,
  DirectPlay3AImpl_EnumConnections,
  DirectPlay3AImpl_EnumGroupsInGroup,
  DirectPlay3AImpl_GetGroupConnectionSettings,
  DirectPlay3AImpl_InitializeConnection,
  DirectPlay3AImpl_SecureOpen,
  DirectPlay3AImpl_SendChatMessage,
  DirectPlay3AImpl_SetGroupConnectionSettings,
  DirectPlay3AImpl_StartSession,
  DirectPlay3AImpl_GetGroupFlags,
  DirectPlay3AImpl_GetGroupParent,
  DirectPlay3AImpl_GetPlayerAccount,
  DirectPlay3AImpl_GetPlayerFlags
};
#undef XCAST

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay3WVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif
static ICOM_VTABLE(IDirectPlay3) directPlay3WVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DirectPlay3WImpl_QueryInterface,
  XCAST(AddRef)DirectPlay2AImpl_AddRef,
  XCAST(Release)DirectPlay2AImpl_Release,

  XCAST(AddPlayerToGroup)DirectPlay2WImpl_AddPlayerToGroup,
  XCAST(Close)DirectPlay2WImpl_Close,
  XCAST(CreateGroup)DirectPlay2WImpl_CreateGroup,
  XCAST(CreatePlayer)DirectPlay2WImpl_CreatePlayer,
  XCAST(DeletePlayerFromGroup)DirectPlay2WImpl_DeletePlayerFromGroup,
  XCAST(DestroyGroup)DirectPlay2WImpl_DestroyGroup,
  XCAST(DestroyPlayer)DirectPlay2WImpl_DestroyPlayer,
  XCAST(EnumGroupPlayers)DirectPlay2WImpl_EnumGroupPlayers,
  XCAST(EnumGroups)DirectPlay2WImpl_EnumGroups,
  XCAST(EnumPlayers)DirectPlay2WImpl_EnumPlayers,
  XCAST(EnumSessions)DirectPlay2WImpl_EnumSessions,
  XCAST(GetCaps)DirectPlay2WImpl_GetCaps,
  XCAST(GetGroupData)DirectPlay2WImpl_GetGroupData,
  XCAST(GetGroupName)DirectPlay2WImpl_GetGroupName,
  XCAST(GetMessageCount)DirectPlay2WImpl_GetMessageCount,
  XCAST(GetPlayerAddress)DirectPlay2WImpl_GetPlayerAddress,
  XCAST(GetPlayerCaps)DirectPlay2WImpl_GetPlayerCaps,
  XCAST(GetPlayerData)DirectPlay2WImpl_GetPlayerData,
  XCAST(GetPlayerName)DirectPlay2WImpl_GetPlayerName,
  XCAST(GetSessionDesc)DirectPlay2WImpl_GetSessionDesc,
  XCAST(Initialize)DirectPlay2WImpl_Initialize,
  XCAST(Open)DirectPlay2WImpl_Open,
  XCAST(Receive)DirectPlay2WImpl_Receive,
  XCAST(Send)DirectPlay2WImpl_Send,
  XCAST(SetGroupData)DirectPlay2WImpl_SetGroupData,
  XCAST(SetGroupName)DirectPlay2WImpl_SetGroupName,
  XCAST(SetPlayerData)DirectPlay2WImpl_SetPlayerData,
  XCAST(SetPlayerName)DirectPlay2WImpl_SetPlayerName,
  XCAST(SetSessionDesc)DirectPlay2WImpl_SetSessionDesc,

  DirectPlay3WImpl_AddGroupToGroup,
  DirectPlay3WImpl_CreateGroupInGroup,
  DirectPlay3WImpl_DeleteGroupFromGroup,
  DirectPlay3WImpl_EnumConnections,
  DirectPlay3WImpl_EnumGroupsInGroup,
  DirectPlay3WImpl_GetGroupConnectionSettings,
  DirectPlay3WImpl_InitializeConnection,
  DirectPlay3WImpl_SecureOpen,
  DirectPlay3WImpl_SendChatMessage,
  DirectPlay3WImpl_SetGroupConnectionSettings,
  DirectPlay3WImpl_StartSession,
  DirectPlay3WImpl_GetGroupFlags,
  DirectPlay3WImpl_GetGroupParent,
  DirectPlay3WImpl_GetPlayerAccount,
  DirectPlay3WImpl_GetPlayerFlags
};
#undef XCAST

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay4WVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif
static ICOM_VTABLE(IDirectPlay4) directPlay4WVT =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DirectPlay4WImpl_QueryInterface,
  XCAST(AddRef)DirectPlay2AImpl_AddRef,
  XCAST(Release)DirectPlay2AImpl_Release,

  XCAST(AddPlayerToGroup)DirectPlay2WImpl_AddPlayerToGroup,
  XCAST(Close)DirectPlay2WImpl_Close,
  XCAST(CreateGroup)DirectPlay2WImpl_CreateGroup,
  XCAST(CreatePlayer)DirectPlay2WImpl_CreatePlayer,
  XCAST(DeletePlayerFromGroup)DirectPlay2WImpl_DeletePlayerFromGroup,
  XCAST(DestroyGroup)DirectPlay2WImpl_DestroyGroup,
  XCAST(DestroyPlayer)DirectPlay2WImpl_DestroyPlayer,
  XCAST(EnumGroupPlayers)DirectPlay2WImpl_EnumGroupPlayers,
  XCAST(EnumGroups)DirectPlay2WImpl_EnumGroups,
  XCAST(EnumPlayers)DirectPlay2WImpl_EnumPlayers,
  XCAST(EnumSessions)DirectPlay2WImpl_EnumSessions,
  XCAST(GetCaps)DirectPlay2WImpl_GetCaps,
  XCAST(GetGroupData)DirectPlay2WImpl_GetGroupData,
  XCAST(GetGroupName)DirectPlay2WImpl_GetGroupName,
  XCAST(GetMessageCount)DirectPlay2WImpl_GetMessageCount,
  XCAST(GetPlayerAddress)DirectPlay2WImpl_GetPlayerAddress,
  XCAST(GetPlayerCaps)DirectPlay2WImpl_GetPlayerCaps,
  XCAST(GetPlayerData)DirectPlay2WImpl_GetPlayerData,
  XCAST(GetPlayerName)DirectPlay2WImpl_GetPlayerName,
  XCAST(GetSessionDesc)DirectPlay2WImpl_GetSessionDesc,
  XCAST(Initialize)DirectPlay2WImpl_Initialize,
  XCAST(Open)DirectPlay2WImpl_Open,
  XCAST(Receive)DirectPlay2WImpl_Receive,
  XCAST(Send)DirectPlay2WImpl_Send,
  XCAST(SetGroupData)DirectPlay2WImpl_SetGroupData,
  XCAST(SetGroupName)DirectPlay2WImpl_SetGroupName,
  XCAST(SetPlayerData)DirectPlay2WImpl_SetPlayerData,
  XCAST(SetPlayerName)DirectPlay2WImpl_SetPlayerName,
  XCAST(SetSessionDesc)DirectPlay2WImpl_SetSessionDesc,

  XCAST(AddGroupToGroup)DirectPlay3WImpl_AddGroupToGroup,
  XCAST(CreateGroupInGroup)DirectPlay3WImpl_CreateGroupInGroup,
  XCAST(DeleteGroupFromGroup)DirectPlay3WImpl_DeleteGroupFromGroup,
  XCAST(EnumConnections)DirectPlay3WImpl_EnumConnections,
  XCAST(EnumGroupsInGroup)DirectPlay3WImpl_EnumGroupsInGroup,
  XCAST(GetGroupConnectionSettings)DirectPlay3WImpl_GetGroupConnectionSettings,
  XCAST(InitializeConnection)DirectPlay3WImpl_InitializeConnection,
  XCAST(SecureOpen)DirectPlay3WImpl_SecureOpen,
  XCAST(SendChatMessage)DirectPlay3WImpl_SendChatMessage,
  XCAST(SetGroupConnectionSettings)DirectPlay3WImpl_SetGroupConnectionSettings,
  XCAST(StartSession)DirectPlay3WImpl_StartSession,
  XCAST(GetGroupFlags)DirectPlay3WImpl_GetGroupFlags,
  XCAST(GetGroupParent)DirectPlay3WImpl_GetGroupParent,
  XCAST(GetPlayerAccount)DirectPlay3WImpl_GetPlayerAccount,
  XCAST(GetPlayerFlags)DirectPlay3WImpl_GetPlayerFlags,

  DirectPlay4WImpl_GetGroupOwner,
  DirectPlay4WImpl_SetGroupOwner,
  DirectPlay4WImpl_SendEx,
  DirectPlay4WImpl_GetMessageQueue,
  DirectPlay4WImpl_CancelMessage,
  DirectPlay4WImpl_CancelPriority
};
#undef XCAST


/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay4AVT.fn##fun))
#else
# define XCAST(fun)     (void*)
#endif
static ICOM_VTABLE(IDirectPlay4) directPlay4AVT =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DirectPlay4AImpl_QueryInterface,
  XCAST(AddRef)DirectPlay2AImpl_AddRef,
  XCAST(Release)DirectPlay2AImpl_Release,

  XCAST(AddPlayerToGroup)DirectPlay2AImpl_AddPlayerToGroup,
  XCAST(Close)DirectPlay2AImpl_Close,
  XCAST(CreateGroup)DirectPlay2AImpl_CreateGroup,
  XCAST(CreatePlayer)DirectPlay2AImpl_CreatePlayer,
  XCAST(DeletePlayerFromGroup)DirectPlay2AImpl_DeletePlayerFromGroup,
  XCAST(DestroyGroup)DirectPlay2AImpl_DestroyGroup,
  XCAST(DestroyPlayer)DirectPlay2AImpl_DestroyPlayer,
  XCAST(EnumGroupPlayers)DirectPlay2AImpl_EnumGroupPlayers,
  XCAST(EnumGroups)DirectPlay2AImpl_EnumGroups,
  XCAST(EnumPlayers)DirectPlay2AImpl_EnumPlayers,
  XCAST(EnumSessions)DirectPlay2AImpl_EnumSessions,
  XCAST(GetCaps)DirectPlay2AImpl_GetCaps,
  XCAST(GetGroupData)DirectPlay2AImpl_GetGroupData,
  XCAST(GetGroupName)DirectPlay2AImpl_GetGroupName,
  XCAST(GetMessageCount)DirectPlay2AImpl_GetMessageCount,
  XCAST(GetPlayerAddress)DirectPlay2AImpl_GetPlayerAddress,
  XCAST(GetPlayerCaps)DirectPlay2AImpl_GetPlayerCaps,
  XCAST(GetPlayerData)DirectPlay2AImpl_GetPlayerData,
  XCAST(GetPlayerName)DirectPlay2AImpl_GetPlayerName,
  XCAST(GetSessionDesc)DirectPlay2AImpl_GetSessionDesc,
  XCAST(Initialize)DirectPlay2AImpl_Initialize,
  XCAST(Open)DirectPlay2AImpl_Open,
  XCAST(Receive)DirectPlay2AImpl_Receive,
  XCAST(Send)DirectPlay2AImpl_Send,
  XCAST(SetGroupData)DirectPlay2AImpl_SetGroupData,
  XCAST(SetGroupName)DirectPlay2AImpl_SetGroupName,
  XCAST(SetPlayerData)DirectPlay2AImpl_SetPlayerData,
  XCAST(SetPlayerName)DirectPlay2AImpl_SetPlayerName,
  XCAST(SetSessionDesc)DirectPlay2AImpl_SetSessionDesc,

  XCAST(AddGroupToGroup)DirectPlay3AImpl_AddGroupToGroup,
  XCAST(CreateGroupInGroup)DirectPlay3AImpl_CreateGroupInGroup,
  XCAST(DeleteGroupFromGroup)DirectPlay3AImpl_DeleteGroupFromGroup,
  XCAST(EnumConnections)DirectPlay3AImpl_EnumConnections,
  XCAST(EnumGroupsInGroup)DirectPlay3AImpl_EnumGroupsInGroup,
  XCAST(GetGroupConnectionSettings)DirectPlay3AImpl_GetGroupConnectionSettings,
  XCAST(InitializeConnection)DirectPlay3AImpl_InitializeConnection,
  XCAST(SecureOpen)DirectPlay3AImpl_SecureOpen,
  XCAST(SendChatMessage)DirectPlay3AImpl_SendChatMessage,
  XCAST(SetGroupConnectionSettings)DirectPlay3AImpl_SetGroupConnectionSettings,
  XCAST(StartSession)DirectPlay3AImpl_StartSession,
  XCAST(GetGroupFlags)DirectPlay3AImpl_GetGroupFlags,
  XCAST(GetGroupParent)DirectPlay3AImpl_GetGroupParent,
  XCAST(GetPlayerAccount)DirectPlay3AImpl_GetPlayerAccount,
  XCAST(GetPlayerFlags)DirectPlay3AImpl_GetPlayerFlags,

  DirectPlay4AImpl_GetGroupOwner,
  DirectPlay4AImpl_SetGroupOwner,
  DirectPlay4AImpl_SendEx,
  DirectPlay4AImpl_GetMessageQueue,
  DirectPlay4AImpl_CancelMessage,
  DirectPlay4AImpl_CancelPriority
};
#undef XCAST


/***************************************************************************
 *  DirectPlayEnumerateA (DPLAYX.2) 
 *
 *  The pointer to the structure lpContext will be filled with the 
 *  appropriate data for each service offered by the OS. These services are
 *  not necessarily available on this particular machine but are defined
 *  as simple service providers under the "Service Providers" registry key.
 *  This structure is then passed to lpEnumCallback for each of the different 
 *  services. 
 *
 *  This API is useful only for applications written using DirectX3 or
 *  worse. It is superceeded by IDirectPlay3::EnumConnections which also
 *  gives information on the actual connections.
 *
 * defn of a service provider:
 * A dynamic-link library used by DirectPlay to communicate over a network. 
 * The service provider contains all the network-specific code required
 * to send and receive messages. Online services and network operators can
 * supply service providers to use specialized hardware, protocols, communications
 * media, and network resources. 
 *
 * TODO: Allocate string buffer space from the heap (length from reg)
 *       Pass real device driver numbers...
 *       Get the GUID properly...
 */
HRESULT WINAPI DirectPlayEnumerateA( LPDPENUMDPCALLBACKA lpEnumCallback,
                                     LPVOID lpContext )
{

  HKEY   hkResult; 
  LPCSTR searchSubKey    = "SOFTWARE\\Microsoft\\DirectPlay\\Service Providers";
  DWORD  dwIndex;
  DWORD  sizeOfSubKeyName=50;
  char   subKeyName[51]; 

  TRACE(": lpEnumCallback=%p lpContext=%p\n", lpEnumCallback, lpContext );

  if( !lpEnumCallback || !*lpEnumCallback )
  {
     return DPERR_INVALIDPARAMS;
  }

  /* Need to loop over the service providers in the registry */
  if( RegOpenKeyExA( HKEY_LOCAL_MACHINE, searchSubKey,
                       0, KEY_ENUMERATE_SUB_KEYS, &hkResult ) != ERROR_SUCCESS )
  {
    /* Hmmm. Does this mean that there are no service providers? */ 
    ERR(": no service providers?\n");
    return DP_OK; 
  }

  /* Traverse all the service providers we have available */
  for( dwIndex=0;
       RegEnumKeyA( hkResult, dwIndex, subKeyName, sizeOfSubKeyName ) != ERROR_NO_MORE_ITEMS;
       ++dwIndex )
  {
    LPSTR    majVerDataSubKey = "dwReserved1";  
    LPSTR    minVerDataSubKey = "dwReserved2";  
    LPSTR    guidDataSubKey   = "Guid";
    HKEY     hkServiceProvider;
    GUID     serviceProviderGUID;
    DWORD    returnTypeGUID, returnTypeReserved, sizeOfReturnBuffer = 50;
    char     returnBuffer[51];
    DWORD    majVersionNum , minVersionNum = 0;
    LPWSTR   lpWGUIDString; 

    TRACE(" this time through: %s\n", subKeyName );

    /* Get a handle for this particular service provider */
    if( RegOpenKeyExA( hkResult, subKeyName, 0, KEY_QUERY_VALUE,
                         &hkServiceProvider ) != ERROR_SUCCESS )
    {
      ERR(": what the heck is going on?\n" );
      continue;
    }

    /* Get the GUID, Device major number and device minor number 
     * from the registry. 
     */
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

    /* FIXME: Need to know which of dwReserved1 and dwReserved2 are maj and min */

    sizeOfReturnBuffer = 50;
    if( RegQueryValueExA( hkServiceProvider, majVerDataSubKey,
                            NULL, &returnTypeReserved, returnBuffer,
                            &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
      ERR(": missing dwReserved1 registry data members\n") ;
      continue; 
    }

    majVersionNum = GET_DWORD( returnBuffer );

    sizeOfReturnBuffer = 50;
    if( RegQueryValueExA( hkServiceProvider, minVerDataSubKey,
                            NULL, &returnTypeReserved, returnBuffer,
                            &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
      ERR(": missing dwReserved2 registry data members\n") ;
      continue;
    }

    minVersionNum = GET_DWORD( returnBuffer );


    /* The enumeration will return FALSE if we are not to continue */
    if( !lpEnumCallback( &serviceProviderGUID , subKeyName,
                         majVersionNum, minVersionNum, lpContext ) )
    {
      WARN("lpEnumCallback returning FALSE\n" );
      break;
    }
  }

  return DP_OK;

}

/***************************************************************************
 *  DirectPlayEnumerateW (DPLAYX.3)
 *
 */
HRESULT WINAPI DirectPlayEnumerateW( LPDPENUMDPCALLBACKW lpEnumCallback, LPVOID lpContext )
{

  FIXME(":stub\n");

  return DPERR_OUTOFMEMORY; 

}

/***************************************************************************
 *  DirectPlayCreate (DPLAYX.1) (DPLAY.1)
 *
 */
HRESULT WINAPI DirectPlayCreate
( LPGUID lpGUID, LPDIRECTPLAY2 *lplpDP, IUnknown *pUnk)
{
  TRACE( "lpGUID=%s lplpDP=%p pUnk=%p\n", debugstr_guid(lpGUID), lplpDP, pUnk );

  if( pUnk != NULL )
  {
    return CLASS_E_NOAGGREGATION;
  }


  /* Create an IDirectPlay object. We don't support that so we'll cheat and
     give them an IDirectPlay2A object and hope that doesn't cause problems */
  if( directPlay_QueryInterface( &IID_IDirectPlay2A, (LPVOID*)lplpDP ) != DP_OK )
  {
    return DPERR_UNAVAILABLE;
  } 

  if( IsEqualGUID( &GUID_NULL, lpGUID ) )
  {
    /* The GUID_NULL means don't bind a service provider. Just return the
       interface */ 
    return DP_OK;
  }


  /* Bind the desired service provider */
  if( ( IsEqualGUID( lpGUID, &DPSPGUID_MODEM ) ) ||
      ( IsEqualGUID( lpGUID, &DPSPGUID_SERIAL ) ) ||
      ( IsEqualGUID( lpGUID, &DPSPGUID_TCPIP ) ) ||
      ( IsEqualGUID( lpGUID, &DPSPGUID_IPX ) ) 
    )
  {
     FIXME( "Service provider binding not supported yet\n" );
     IDirectPlayX_Release( *lplpDP );
     *lplpDP = NULL;
     return DPERR_INVALIDPARAMS; 
  }

  ERR( "unknown Service Provider %s\n", debugstr_guid(lpGUID) );

  IDirectPlayX_Release( *lplpDP );
  *lplpDP = NULL;

  return DPERR_INVALIDPARAMS;
}
