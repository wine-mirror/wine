/* Direct Play 3 and Direct Play Lobby 2 Implementation
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
#include "dplobby.h"
#include "heap.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(dplay)

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectPlayLobbyImpl IDirectPlayLobbyImpl;
typedef struct IDirectPlayLobbyImpl IDirectPlayLobbyAImpl;
typedef struct IDirectPlayLobbyImpl IDirectPlayLobbyWImpl;
typedef struct IDirectPlayLobby2Impl IDirectPlayLobby2Impl;
typedef struct IDirectPlayLobby2Impl IDirectPlayLobby2AImpl;
typedef struct IDirectPlay2Impl IDirectPlay2Impl;
typedef struct IDirectPlay2Impl IDirectPlay2AImpl;
typedef struct IDirectPlay3Impl IDirectPlay3Impl;
typedef struct IDirectPlay3Impl IDirectPlay3AImpl;

/*****************************************************************************
 * IDirectPlayLobby implementation structure
 */
struct IDirectPlayLobbyImpl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectPlayLobby)* lpvtbl;
    DWORD                          ref;
    /* IDirectPlayLobbyImpl fields */
    DWORD                    dwConnFlags;
    DPSESSIONDESC2           sessionDesc;
    DPNAME                   playerName;
    GUID                     guidSP;
    LPVOID                   lpAddress;
    DWORD                    dwAddressSize;
};

/*****************************************************************************
 * IDirectPlayLobby2 implementation structure
 */
struct IDirectPlayLobby2Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectPlayLobby2)* lpvtbl;
    DWORD                           ref;
    /* IDirectPlayLobby2Impl fields */
    DWORD                     dwConnFlags;
    DPSESSIONDESC2            lpSessionDesc;
    DPNAME                    lpPlayerName;
    GUID                      guidSP;
    LPVOID                    lpAddress;
    DWORD                     dwAddressSize;
};

/*****************************************************************************
 * IDirectPlay2 implementation structure
 */
struct IDirectPlay2Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectPlay2)* lpvtbl;
    DWORD                      ref;
    /* IDirectPlay2Impl fields */
    /* none */
};

/*****************************************************************************
 * IDirectPlay3 implementation structure
 */
struct IDirectPlay3Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectPlay3)* lpvtbl;
    DWORD                      ref;
    /* IDirectPlay3Impl fields */
    /* none */
};

/* Forward declarations of virtual tables */
static ICOM_VTABLE(IDirectPlayLobby) directPlayLobbyAVT;
static ICOM_VTABLE(IDirectPlayLobby) directPlayLobbyWVT;
static ICOM_VTABLE(IDirectPlayLobby2) directPlayLobby2AVT;
static ICOM_VTABLE(IDirectPlayLobby2) directPlayLobby2WVT;
static ICOM_VTABLE(IDirectPlay2) directPlay2WVT;
static ICOM_VTABLE(IDirectPlay2) directPlay2AVT;
static ICOM_VTABLE(IDirectPlay3) directPlay3WVT;
static ICOM_VTABLE(IDirectPlay3) directPlay3AVT;




/* Routine called when starting up the server thread */
DWORD DPLobby_Spawn_Server( LPVOID startData )
{
  DPSESSIONDESC2* lpSession = (DPSESSIONDESC2*) startData;
  DWORD sessionDwFlags = lpSession->dwFlags;
 
  TRACE( dplay, "spawing thread for lpConn=%p dwFlags=%08lx\n", lpSession, sessionDwFlags );
  FIXME( dplay, "thread needs something to do\n" ); 

/*for(;;)*/
  {
     
    /* Check out the connection flags to determine what to do. Ensure we have 
       no leftover bits in this structure */
    if( sessionDwFlags & DPSESSION_CLIENTSERVER )
    {
       /* This indicates that the application which is requesting the creation
        * of this session is going to be the server (application/player)
        */ 
       if( sessionDwFlags & DPSESSION_SECURESERVER )
       {
         sessionDwFlags &= ~DPSESSION_SECURESERVER; 
       }
       sessionDwFlags &= ~DPSESSION_CLIENTSERVER;  
    }

    if( sessionDwFlags & DPSESSION_JOINDISABLED )
    {
       sessionDwFlags &= ~DPSESSION_JOINDISABLED; 
    } 

    if( sessionDwFlags & DPSESSION_KEEPALIVE )
    {
       sessionDwFlags &= ~DPSESSION_KEEPALIVE;
    }

    if( sessionDwFlags & DPSESSION_MIGRATEHOST )
    {
       sessionDwFlags &= ~DPSESSION_MIGRATEHOST;
    }

    if( sessionDwFlags & DPSESSION_MULTICASTSERVER )
    {
       sessionDwFlags &= ~DPSESSION_MULTICASTSERVER;
    }

    if( sessionDwFlags & DPSESSION_NEWPLAYERSDISABLED )
    {
       sessionDwFlags &= ~DPSESSION_NEWPLAYERSDISABLED; 
    }

    if( sessionDwFlags & DPSESSION_NODATAMESSAGES )
    {
       sessionDwFlags &= ~DPSESSION_NODATAMESSAGES; 
    } 

    if( sessionDwFlags & DPSESSION_NOMESSAGEID )
    {
       sessionDwFlags &= ~DPSESSION_NOMESSAGEID;
    }

    if( sessionDwFlags & DPSESSION_PASSWORDREQUIRED )
    {
       sessionDwFlags &= ~DPSESSION_PASSWORDREQUIRED;
    }

  }

  ExitThread(0);
  return 0; 
}


/*********************************************************
 *
 * Direct Play and Direct Play Lobby Interface Implementation 
 * 
 *********************************************************/ 

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

/* Helper function for DirectPlayLobby  QueryInterface */ 
static HRESULT directPlayLobby_QueryInterface
         ( REFIID riid, LPVOID* ppvObj )
{

  if( IsEqualGUID( &IID_IDirectPlayLobby, riid ) )
  {
     IDirectPlayLobbyImpl* lpDpL = (IDirectPlayLobbyImpl*)(*ppvObj);

     lpDpL = (IDirectPlayLobbyImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           sizeof( *lpDpL ) );

    if( !lpDpL )
    {
      return E_NOINTERFACE;
    }

    lpDpL->lpvtbl = &directPlayLobbyWVT;
    lpDpL->ref    = 1;

    return S_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobbyA, riid ) )
  {
     IDirectPlayLobbyAImpl* lpDpL = (IDirectPlayLobbyAImpl*)(*ppvObj);

     lpDpL = (IDirectPlayLobbyAImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                            sizeof( *lpDpL ) );

    if( !lpDpL )
    {
      return E_NOINTERFACE;
    }

    lpDpL->lpvtbl = &directPlayLobbyAVT;
    lpDpL->ref    = 1;

    return S_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2, riid ) )
  {
     IDirectPlayLobby2Impl* lpDpL = (IDirectPlayLobby2Impl*)(*ppvObj);

     lpDpL = (IDirectPlayLobby2Impl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                             sizeof( *lpDpL ) );

    if( !lpDpL )
    {
      return E_NOINTERFACE;
    }

    lpDpL->lpvtbl = &directPlayLobby2WVT;
    lpDpL->ref    = 1;

    return S_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2A, riid ) )
  {
     IDirectPlayLobby2AImpl* lpDpL = (IDirectPlayLobby2AImpl*)(*ppvObj);

     lpDpL = (IDirectPlayLobby2AImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                             sizeof( *lpDpL ) );

    if( !lpDpL )
    {
      return E_NOINTERFACE;
    }

    lpDpL->lpvtbl = &directPlayLobby2AVT;
    lpDpL->ref    = 1;

    return S_OK;
  }

  /* Unknown interface */
  *ppvObj = NULL;
  return E_NOINTERFACE;
}
static HRESULT WINAPI IDirectPlayLobbyAImpl_QueryInterface
( LPDIRECTPLAYLOBBYA iface,
  REFIID riid,
  LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  TRACE( dplay, "(%p)->(%p,%p)\n", This, riid, ppvObj );

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
  TRACE( dplay, "(%p)->(%p,%p)\n", This, riid, ppvObj );

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
  TRACE( dplay, "(%p)->(%p,%p)\n", This, riid, ppvObj );

  /* Compare riids. We know this object is a direct play lobby 2A object.
     If we are asking about the same type of interface we're fine.
   */
  if( IsEqualGUID( &IID_IUnknown, riid )  ||
      IsEqualGUID( &IID_IDirectPlayLobby2A, riid )
    )
  {
    IDirectPlayLobby2_AddRef( iface );
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
    IDirectPlayLobby2_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlayLobby_QueryInterface( riid, ppvObj ); 

}

/* 
 * Simple procedure. Just increment the reference count to this
 * structure and return the new reference count.
 */
static ULONG WINAPI IDirectPlayLobby2AImpl_AddRef
( LPDIRECTPLAYLOBBY2A iface )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  ++(This->ref);
  TRACE( dplay,"ref count now %lu\n", This->ref );
  return (This->ref);
}

static ULONG WINAPI IDirectPlayLobby2WImpl_AddRef
( LPDIRECTPLAYLOBBY2 iface )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobby2AImpl_AddRef( (LPDIRECTPLAYLOBBY2) This );
}


/*
 * Simple COM procedure. Decrease the reference count to this object.
 * If the object no longer has any reference counts, free up the associated
 * memory.
 */
static ULONG WINAPI IDirectPlayLobby2AImpl_Release
( LPDIRECTPLAYLOBBY2A iface )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  TRACE( dplay, "ref count decremeneted from %lu\n", This->ref );

  This->ref--;

  /* Deallocate if this is the last reference to the object */
  if( !(This->ref) )
  {
    FIXME( dplay, "memory leak\n" );
    /* Implement memory deallocation */

    HeapFree( GetProcessHeap(), 0, This );

    return 0;
  }

  return This->ref;
}

static ULONG WINAPI IDirectPlayLobby2WImpl_Release
( LPDIRECTPLAYLOBBY2 iface )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobby2AImpl_Release( (LPDIRECTPLAYLOBBY2A) This );
}


/********************************************************************
 * 
 * Connects an application to the session specified by the DPLCONNECTION
 * structure currently stored with the DirectPlayLobby object.
 *
 * Returns a IDirectPlay interface.
 *
 */
static HRESULT WINAPI IDirectPlayLobby2AImpl_Connect
( LPDIRECTPLAYLOBBY2A iface,
  DWORD dwFlags,
  LPDIRECTPLAY2* lplpDP,
  IUnknown* pUnk)
{
  FIXME( dplay, ": dwFlags=%08lx %p %p stub\n", dwFlags, lplpDP, pUnk );
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_Connect
( LPDIRECTPLAYLOBBY2 iface,
  DWORD dwFlags,
  LPDIRECTPLAY2* lplpDP,
  IUnknown* pUnk)
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  LPDIRECTPLAY2* directPlay2W;
  HRESULT        createRC;

  FIXME( dplay, "(%p)->(%08lx,%p,%p): stub\n", This, dwFlags, lplpDP, pUnk );

  if( dwFlags )
  {
     return DPERR_INVALIDPARAMS;
  }

  if( ( createRC = DirectPlayCreate( (LPGUID)&IID_IDirectPlayLobby2, lplpDP, pUnk ) ) != DP_OK )
  {
     ERR( dplay, "error creating Direct Play 2W interface. Return Code = %ld.\n", createRC );
     return createRC;
  } 

  /* This should invoke IDirectPlay3::InitializeConnection IDirectPlay3::Open */  
  directPlay2W = lplpDP; 
 
    


#if 0
  /* All the stuff below this is WRONG! */
  if( This->lpSession->dwFlags == DPLCONNECTION_CREATESESSION )
  {
    DWORD threadIdSink;

    /* Spawn a thread to deal with all of this and to handle the incomming requests */
    threadIdSink = CreateThread( NULL, 0, &DPLobby_Spawn_Server,
                                (LPVOID)This->lpSession->lpConn->lpSessionDesc, 0, &threadIdSink );  

  }
  else if ( This->lpSession->dwFlags == DPLCONNECTION_JOINSESSION ) 
  {
    /* Let's search for a matching session */
    FIXME( dplay, "joining session not yet supported.\n");
    return DPERR_OUTOFMEMORY;
  }
  else /* Unknown type of connection request */
  {
     ERR( dplay, ": Unknown connection request lpConn->dwFlags=%08lx\n",
          lpConn->dwFlags ); 

     return DPERR_OUTOFMEMORY;
  }

  /* This does the work of the following methods...
     IDirectPlay3::InitializeConnection,
     IDirectPlay3::EnumSessions,
     IDirectPlay3::Open
   */
  

#endif

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
static HRESULT WINAPI IDirectPlayLobby2AImpl_CreateAddress
( LPDIRECTPLAYLOBBY2A iface,
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData, 
  DWORD dwDataSize,
  LPVOID lpAddress, 
  LPDWORD lpdwAddressSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_CreateAddress
( LPDIRECTPLAYLOBBY2 iface,
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData,
  DWORD dwDataSize,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}


/********************************************************************
 *
 * Parses out chunks from the DirectPlay Address buffer by calling the
 * given callback function, with lpContext, for each of the chunks.
 *
 */
static HRESULT WINAPI IDirectPlayLobby2AImpl_EnumAddress
( LPDIRECTPLAYLOBBY2A iface,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_EnumAddress
( LPDIRECTPLAYLOBBY2 iface,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext )
{
  FIXME( dplay, ":stub\n");
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
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_EnumAddressTypes
( LPDIRECTPLAYLOBBY2A iface,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP, 
  LPVOID lpContext,
  DWORD dwFlags )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyAImpl_EnumAddressTypes( (LPDIRECTPLAYLOBBYA)This, lpEnumAddressTypeCallback,
                                             guidSP, lpContext, dwFlags );
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_EnumAddressTypes
( LPDIRECTPLAYLOBBY2 iface,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Enumerates what applications are registered with DirectPlay by
 * invoking the callback function with lpContext.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyW_EnumLocalApplications
( LPDIRECTPLAYLOBBY iface,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK a,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_EnumLocalApplications
( LPDIRECTPLAYLOBBY2 iface,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK a,
  LPVOID lpContext,
  DWORD dwFlags )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyW_EnumLocalApplications( (LPDIRECTPLAYLOBBY)This, a,
                                                  lpContext, dwFlags ); 
}

static HRESULT WINAPI IDirectPlayLobbyAImpl_EnumLocalApplications
( LPDIRECTPLAYLOBBYA iface,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK a,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_EnumLocalApplications
( LPDIRECTPLAYLOBBY2A iface,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK a,
  LPVOID lpContext,
  DWORD dwFlags )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyAImpl_EnumLocalApplications( (LPDIRECTPLAYLOBBYA)This, a,
                                                  lpContext, dwFlags ); 
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

  FIXME( dplay, ": semi stub (%p)->(0x%08lx,%p,%p)\n", This, dwAppID, lpData, lpdwDataSize );
 
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

  /* Fill in the fields - let them just use the ptrs */
  lpDplConnection = (LPDPLCONNECTION)lpData;

  /* Make sure we were given the right size */
  if( lpDplConnection->dwSize < sizeof( DPLCONNECTION ) )
  {
     ERR( dplay, "bad passed size 0x%08lx.\n", lpDplConnection->dwSize );
     return DPERR_INVALIDPARAMS;
  }

  /* Copy everything we've got into here */
  /* Need to actually store the stuff here. Check if we've already allocated each field first. */
  lpDplConnection->dwFlags = This->dwConnFlags;

  /* Copy LPDPSESSIONDESC2 struct */
  lpDplConnection->lpSessionDesc = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( This->sessionDesc ) );
  memcpy( lpDplConnection->lpSessionDesc, &(This->sessionDesc), sizeof( This->sessionDesc ) );

  if( This->sessionDesc.sess.lpszSessionName )
  {
    lpDplConnection->lpSessionDesc->sess.lpszSessionName = 
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, This->sessionDesc.sess.lpszSessionName );
  }

  if( This->sessionDesc.pass.lpszPassword )
  {
    lpDplConnection->lpSessionDesc->pass.lpszPassword = 
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, This->sessionDesc.pass.lpszPassword );
  }
     
  /* I don't know what to use the reserved for. We'll set it to 0 just for fun */
  This->sessionDesc.dwReserved1 = This->sessionDesc.dwReserved2 = 0;

  /* Copy DPNAME struct - seems to be optional - check for existance first */
  lpDplConnection->lpPlayerName = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( This->playerName ) );
  memcpy( lpDplConnection->lpPlayerName, &(This->playerName), sizeof( This->playerName ) );

  if( This->playerName.psn.lpszShortName )
  {
    lpDplConnection->lpPlayerName->psn.lpszShortName =
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, This->playerName.psn.lpszShortName );  
  }

  if( This->playerName.pln.lpszLongName )
  {
    lpDplConnection->lpPlayerName->pln.lpszLongName =
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, This->playerName.pln.lpszLongName );
  }



  memcpy( &(lpDplConnection->guidSP), &(This->guidSP), sizeof( This->guidSP ) );

  lpDplConnection->lpAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, This->dwAddressSize );
  memcpy( lpDplConnection->lpAddress, This->lpAddress, This->dwAddressSize );

  lpDplConnection->dwAddressSize = This->dwAddressSize;

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_GetConnectionSettings
( LPDIRECTPLAYLOBBY2A iface,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyAImpl_GetConnectionSettings( (LPDIRECTPLAYLOBBYA)This,
                                                  dwAppID, lpData, lpdwDataSize ); 
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_GetConnectionSettings
( LPDIRECTPLAYLOBBY iface,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
  FIXME( dplay, ":semi stub %p %08lx %p %p \n", This, dwAppID, lpData, lpdwDataSize );

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

  /* Fill in the fields - let them just use the ptrs */
  FIXME( dplay, "stub\n" );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_GetConnectionSettings
( LPDIRECTPLAYLOBBY2 iface,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyWImpl_GetConnectionSettings( (LPDIRECTPLAYLOBBY)This,
                                                  dwAppID, lpData, lpdwDataSize );
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
  FIXME( dplay, ":stub %p %08lx %08lx %p %p %p\n", This, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBY2A iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyAImpl_ReceiveLobbyMessage( (LPDIRECTPLAYLOBBYA)This, dwFlags, dwAppID,
                                                 lpdwMessageFlags, lpData, lpdwDataSize );
}


static HRESULT WINAPI IDirectPlayLobbyW_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
  FIXME( dplay, ":stub %p %08lx %08lx %p %p %p\n", This, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBY2 iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyW_ReceiveLobbyMessage( (LPDIRECTPLAYLOBBY)This, dwFlags, dwAppID,
                                                 lpdwMessageFlags, lpData, lpdwDataSize );
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
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_RunApplication
( LPDIRECTPLAYLOBBY2A iface,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyAImpl_RunApplication( (LPDIRECTPLAYLOBBYA)This, dwFlags,
                                           lpdwAppID, lpConn, hReceiveEvent );
}

static HRESULT WINAPI IDirectPlayLobbyW_RunApplication
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_RunApplication
( LPDIRECTPLAYLOBBY2 iface,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyW_RunApplication( (LPDIRECTPLAYLOBBY)This, dwFlags,
                                           lpdwAppID, lpConn, hReceiveEvent );
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
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_SendLobbyMessage
( LPDIRECTPLAYLOBBY2A iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyAImpl_SendLobbyMessage( (LPDIRECTPLAYLOBBYA)This, dwFlags, 
                                             dwAppID, lpData, dwDataSize ); 
}


static HRESULT WINAPI IDirectPlayLobbyW_SendLobbyMessage
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_SendLobbyMessage
( LPDIRECTPLAYLOBBY2 iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyW_SendLobbyMessage( (LPDIRECTPLAYLOBBY)This, dwFlags,
                                              dwAppID, lpData, dwDataSize );
}

/********************************************************************
 *
 * Modifies the DPLCONNECTION structure to contain all information
 * needed to start and connect an application.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyW_SetConnectionSettings
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
  TRACE( dplay, ": This=%p, dwFlags=%08lx, dwAppId=%08lx, lpConn=%p\n",
         This, dwFlags, dwAppID, lpConn );

  /* Paramater check */
  if( dwFlags || !This || !lpConn )
  {
    ERR( dplay, "invalid parameters.\n");
    return DPERR_INVALIDPARAMS;
  }

  /* See if there is a connection associated with this request.
   * dwAppID == 0 indicates that this request isn't associated with a connection.
   */
  if( dwAppID )
  {
     FIXME( dplay, ": Connection dwAppID=%08lx given. Not implemented yet.\n",
            dwAppID );

     /* Need to add a check for this application Id...*/
     return DPERR_NOTLOBBIED;
  }

  if(  lpConn->dwSize != sizeof(DPLCONNECTION) )
  {
    ERR( dplay, ": old/new DPLCONNECTION type? Size=%08lx vs. expected=%ul bytes\n", 
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
    ERR( dplay, "DPSESSIONDESC passed in? Size=%08lx vs. expected=%ul bytes\n",
         lpConn->lpSessionDesc->dwSize, sizeof( DPSESSIONDESC2 ) );
    return DPERR_INVALIDPARAMS;
  }

  /* Need to actually store the stuff here. Check if we've already allocated each field first. */
  This->dwConnFlags = lpConn->dwFlags;

  /* Copy LPDPSESSIONDESC2 struct - this is required */
  memcpy( &(This->sessionDesc), lpConn->lpSessionDesc, sizeof( *(lpConn->lpSessionDesc) ) );

  if( lpConn->lpSessionDesc->sess.lpszSessionName )
    This->sessionDesc.sess.lpszSessionName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->sess.lpszSessionName );
  else
    This->sessionDesc.sess.lpszSessionName = NULL;
 
  if( lpConn->lpSessionDesc->pass.lpszPassword )
    This->sessionDesc.pass.lpszPassword = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->pass.lpszPassword );
  else
    This->sessionDesc.pass.lpszPassword = NULL;

  /* I don't know what to use the reserved for ... */
  This->sessionDesc.dwReserved1 = This->sessionDesc.dwReserved2 = 0;

  /* Copy DPNAME struct - seems to be optional - check for existance first */
  if( lpConn->lpPlayerName )
  {
     memcpy( &(This->playerName), lpConn->lpPlayerName, sizeof( *lpConn->lpPlayerName ) ); 

     if( lpConn->lpPlayerName->psn.lpszShortName )
       This->playerName.psn.lpszShortName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->psn.lpszShortName ); 

     if( lpConn->lpPlayerName->pln.lpszLongName )
       This->playerName.pln.lpszLongName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->pln.lpszLongName );

  }

  memcpy( &(This->guidSP), &(lpConn->guidSP), sizeof( lpConn->guidSP ) );  
  
  This->lpAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->dwAddressSize ); 
  memcpy( This->lpAddress, lpConn->lpAddress, lpConn->dwAddressSize );

  This->dwAddressSize = lpConn->dwAddressSize;

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_SetConnectionSettings
( LPDIRECTPLAYLOBBY2 iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyW_SetConnectionSettings( (LPDIRECTPLAYLOBBY)This, 
                                                  dwFlags, dwAppID, lpConn );
}

static HRESULT WINAPI IDirectPlayLobbyAImpl_SetConnectionSettings
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  ICOM_THIS(IDirectPlayLobbyImpl,iface);
  FIXME( dplay, ": This=%p, dwFlags=%08lx, dwAppId=%08lx, lpConn=%p: stub\n",
         This, dwFlags, dwAppID, lpConn );
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_SetConnectionSettings
( LPDIRECTPLAYLOBBY2A iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyAImpl_SetConnectionSettings( (LPDIRECTPLAYLOBBYA)This,
                                                  dwFlags, dwAppID, lpConn );
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
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY2A iface,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyAImpl_SetLobbyMessageEvent( (LPDIRECTPLAYLOBBYA)This, dwFlags,
                                                 dwAppID, hReceiveEvent ); 
}

static HRESULT WINAPI IDirectPlayLobbyW_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2WImpl_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY2 iface,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  ICOM_THIS(IDirectPlayLobby2Impl,iface);
  return IDirectPlayLobbyW_SetLobbyMessageEvent( (LPDIRECTPLAYLOBBY)This, dwFlags,
                                                 dwAppID, hReceiveEvent ); 
}


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
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_CreateCompoundAddress
( LPDIRECTPLAYLOBBY2A iface,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}


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
  XCAST(AddRef)IDirectPlayLobby2AImpl_AddRef,
  XCAST(Release)IDirectPlayLobby2AImpl_Release,
  XCAST(Connect)IDirectPlayLobby2AImpl_Connect,
  XCAST(CreateAddress)IDirectPlayLobby2AImpl_CreateAddress,
  XCAST(EnumAddress)IDirectPlayLobby2AImpl_EnumAddress,
  XCAST(EnumAddressTypes)IDirectPlayLobby2AImpl_EnumAddressTypes,
  XCAST(EnumLocalApplications)IDirectPlayLobby2AImpl_EnumLocalApplications,
  XCAST(GetConnectionSettings)IDirectPlayLobby2AImpl_GetConnectionSettings,
  XCAST(ReceiveLobbyMessage)IDirectPlayLobby2AImpl_ReceiveLobbyMessage,
  XCAST(RunApplication)IDirectPlayLobby2AImpl_RunApplication,
  XCAST(SendLobbyMessage)IDirectPlayLobby2AImpl_SendLobbyMessage,
  XCAST(SetConnectionSettings)IDirectPlayLobby2AImpl_SetConnectionSettings,
  XCAST(SetLobbyMessageEvent)IDirectPlayLobby2AImpl_SetLobbyMessageEvent
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
  XCAST(AddRef)IDirectPlayLobby2WImpl_AddRef,
  XCAST(Release)IDirectPlayLobby2WImpl_Release,
  XCAST(Connect)IDirectPlayLobby2WImpl_Connect,
  XCAST(CreateAddress)IDirectPlayLobby2WImpl_CreateAddress, 
  XCAST(EnumAddress)IDirectPlayLobby2WImpl_EnumAddress,
  XCAST(EnumAddressTypes)IDirectPlayLobby2WImpl_EnumAddressTypes,
  XCAST(EnumLocalApplications)IDirectPlayLobby2WImpl_EnumLocalApplications,
  XCAST(GetConnectionSettings)IDirectPlayLobby2WImpl_GetConnectionSettings,
  XCAST(ReceiveLobbyMessage)IDirectPlayLobby2WImpl_ReceiveLobbyMessage,
  XCAST(RunApplication)IDirectPlayLobby2WImpl_RunApplication,
  XCAST(SendLobbyMessage)IDirectPlayLobby2WImpl_SendLobbyMessage,
  XCAST(SetConnectionSettings)IDirectPlayLobby2WImpl_SetConnectionSettings,
  XCAST(SetLobbyMessageEvent)IDirectPlayLobby2WImpl_SetLobbyMessageEvent
};
#undef XCAST


/* Direct Play Lobby 2 (ascii) Virtual Table for methods */
static ICOM_VTABLE(IDirectPlayLobby2) directPlayLobby2AVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  IDirectPlayLobby2AImpl_QueryInterface,
  IDirectPlayLobby2AImpl_AddRef,
  IDirectPlayLobby2AImpl_Release,
  IDirectPlayLobby2AImpl_Connect,
  IDirectPlayLobby2AImpl_CreateAddress,
  IDirectPlayLobby2AImpl_EnumAddress,
  IDirectPlayLobby2AImpl_EnumAddressTypes,
  IDirectPlayLobby2AImpl_EnumLocalApplications,
  IDirectPlayLobby2AImpl_GetConnectionSettings,
  IDirectPlayLobby2AImpl_ReceiveLobbyMessage,
  IDirectPlayLobby2AImpl_RunApplication,
  IDirectPlayLobby2AImpl_SendLobbyMessage,
  IDirectPlayLobby2AImpl_SetConnectionSettings,
  IDirectPlayLobby2AImpl_SetLobbyMessageEvent,
  IDirectPlayLobby2AImpl_CreateCompoundAddress 
};

/* Direct Play Lobby 2 (unicode) Virtual Table for methods */
static ICOM_VTABLE(IDirectPlayLobby2) directPlayLobby2WVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  IDirectPlayLobby2WImpl_QueryInterface,
  IDirectPlayLobby2WImpl_AddRef, 
  IDirectPlayLobby2WImpl_Release,
  IDirectPlayLobby2WImpl_Connect,
  IDirectPlayLobby2WImpl_CreateAddress,
  IDirectPlayLobby2WImpl_EnumAddress,
  IDirectPlayLobby2WImpl_EnumAddressTypes,
  IDirectPlayLobby2WImpl_EnumLocalApplications,
  IDirectPlayLobby2WImpl_GetConnectionSettings,
  IDirectPlayLobby2WImpl_ReceiveLobbyMessage,
  IDirectPlayLobby2WImpl_RunApplication,
  IDirectPlayLobby2WImpl_SendLobbyMessage,
  IDirectPlayLobby2WImpl_SetConnectionSettings,
  IDirectPlayLobby2WImpl_SetLobbyMessageEvent,
  IDirectPlayLobby2WImpl_CreateCompoundAddress
};

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
  IDirectPlayLobbyAImpl** ilplpDPL=(IDirectPlayLobbyAImpl**)lplpDPL;
  TRACE(dplay,"lpGUIDDSP=%p lplpDPL=%p lpUnk=%p lpData=%p dwDataSize=%08lx\n",
        lpGUIDDSP,ilplpDPL,lpUnk,lpData,dwDataSize);

  /* Parameter Check: lpGUIDSP, lpUnk & lpData must be NULL. dwDataSize must
   * equal 0. These fields are mostly for future expansion.
   */
  if ( lpGUIDDSP || lpUnk || lpData || dwDataSize )
  {
     *ilplpDPL = NULL;
     return DPERR_INVALIDPARAMS;
  }

  /* Yes...really we should be returning a lobby 1 object */
  *ilplpDPL = (IDirectPlayLobbyAImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                            sizeof( IDirectPlayLobbyAImpl ) );

  if( ! (*ilplpDPL) )
  {
     return DPERR_OUTOFMEMORY;
  }

  (*ilplpDPL)->lpvtbl = &directPlayLobbyAVT;
  (*ilplpDPL)->ref    = 1;

  /* All fields were nulled out by the allocation */

  return DP_OK;
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
  IDirectPlayLobbyImpl** ilplpDPL=(IDirectPlayLobbyImpl**)lplpDPL;
  TRACE(dplay,"lpGUIDDSP=%p lplpDPL=%p lpUnk=%p lpData=%p dwDataSize=%08lx\n",
        lpGUIDDSP,ilplpDPL,lpUnk,lpData,dwDataSize);

  /* Parameter Check: lpGUIDSP, lpUnk & lpData must be NULL. dwDataSize must 
   * equal 0. These fields are mostly for future expansion.
   */
  if ( lpGUIDDSP || lpUnk || lpData || dwDataSize )
  {
     *ilplpDPL = NULL;
     ERR( dplay, "Bad parameters!\n" );
     return DPERR_INVALIDPARAMS;
  }

  /* Yes...really we should bre returning a lobby 1 object */
  *ilplpDPL = (IDirectPlayLobbyImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           sizeof( IDirectPlayLobbyImpl ) );

  if( !*ilplpDPL)
  {
     return DPERR_OUTOFMEMORY;
  }

  (*ilplpDPL)->lpvtbl = &directPlayLobbyWVT;
  (*ilplpDPL)->ref    = 1;

  /* All fields were nulled out by the allocation */

  return DP_OK;

}

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

  HKEY hkResult; 
  LPCSTR searchSubKey    = "SOFTWARE\\Microsoft\\DirectPlay\\Service Providers";
  LPSTR guidDataSubKey   = "Guid";
  LPSTR majVerDataSubKey = "dwReserved1";
  DWORD dwIndex, sizeOfSubKeyName=50;
  char subKeyName[51]; 

  TRACE( dplay, ": lpEnumCallback=%p lpContext=%p\n", lpEnumCallback, lpContext );

  if( !lpEnumCallback || !*lpEnumCallback )
  {
     return DPERR_INVALIDPARAMS;
  }

  /* Need to loop over the service providers in the registry */
  if( RegOpenKeyExA( HKEY_LOCAL_MACHINE, searchSubKey,
                       0, KEY_ENUMERATE_SUB_KEYS, &hkResult ) != ERROR_SUCCESS )
  {
    /* Hmmm. Does this mean that there are no service providers? */ 
    ERR(dplay, ": no service providers?\n");
    return DP_OK; 
  }

  /* Traverse all the service providers we have available */
  for( dwIndex=0;
       RegEnumKeyA( hkResult, dwIndex, subKeyName, sizeOfSubKeyName ) !=
         ERROR_NO_MORE_ITEMS;
       ++dwIndex )
  {
    HKEY     hkServiceProvider;
    GUID     serviceProviderGUID;
    DWORD    returnTypeGUID, returnTypeReserved1, sizeOfReturnBuffer=50;
    char     returnBuffer[51];
    DWORD    majVersionNum /*, minVersionNum */;
    LPWSTR   lpWGUIDString; 

    TRACE( dplay, " this time through: %s\n", subKeyName );

    /* Get a handle for this particular service provider */
    if( RegOpenKeyExA( hkResult, subKeyName, 0, KEY_QUERY_VALUE,
                         &hkServiceProvider ) != ERROR_SUCCESS )
    {
      ERR( dplay, ": what the heck is going on?\n" );
      continue;
    }

    /* Get the GUID, Device major number and device minor number 
     * from the registry. 
     */
    if( RegQueryValueExA( hkServiceProvider, guidDataSubKey,
                            NULL, &returnTypeGUID, returnBuffer,
                            &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
      ERR( dplay, ": missing GUID registry data members\n" );
      continue; 
    }

    /* FIXME: Check return types to ensure we're interpreting data right */
    lpWGUIDString = HEAP_strdupAtoW( GetProcessHeap(), 0, returnBuffer );
    CLSIDFromString( (LPCOLESTR)lpWGUIDString, &serviceProviderGUID ); 
    HeapFree( GetProcessHeap(), 0, lpWGUIDString );

    sizeOfReturnBuffer = 50;
 
    if( RegQueryValueExA( hkServiceProvider, majVerDataSubKey,
                            NULL, &returnTypeReserved1, returnBuffer,
                            &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
      ERR( dplay, ": missing dwReserved1 registry data members\n") ;
      continue; 
    }
    /* FIXME: This couldn't possibly be right...*/
    majVersionNum = GET_DWORD( returnBuffer );

    /* The enumeration will return FALSE if we are not to continue */
    if( !lpEnumCallback( &serviceProviderGUID , subKeyName,
                         majVersionNum, (DWORD)0, lpContext ) )
    {
      WARN( dplay, "lpEnumCallback returning FALSE\n" );
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

  FIXME( dplay, ":stub\n");

  return DPERR_OUTOFMEMORY; 

}

/***************************************************************************
 *  DirectPlayCreate (DPLAYX.1) (DPLAY.1)
 *
 */
HRESULT WINAPI DirectPlayCreate
( LPGUID lpGUID, LPDIRECTPLAY2 *lplpDP, IUnknown *pUnk)
{

  TRACE(dplay,"lpGUID=%p lplpDP=%p pUnk=%p\n", lpGUID,lplpDP,pUnk);

  if( pUnk != NULL )
  {
    /* Hmmm...wonder what this means! */
    ERR(dplay, "What does a NULL here mean?\n" ); 
    return DPERR_OUTOFMEMORY;
  }

  if( IsEqualGUID( &IID_IDirectPlay2A, lpGUID ) )
  {
    IDirectPlay2AImpl** ilplpDP=(IDirectPlay2AImpl**)lplpDP;
    *ilplpDP = (IDirectPlay2AImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                         sizeof( **ilplpDP ) );

    if( !*ilplpDP )
    {
       return DPERR_OUTOFMEMORY;
    }

    (*ilplpDP)->lpvtbl = &directPlay2AVT;
    (*ilplpDP)->ref    = 1;
  
    return DP_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlay2, lpGUID ) )
  {
    IDirectPlay2Impl** ilplpDP=(IDirectPlay2Impl**)lplpDP;
    *ilplpDP = (IDirectPlay2Impl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        sizeof( **ilplpDP ) );

    if( !*ilplpDP )
    {
       return DPERR_OUTOFMEMORY;
    }

    (*ilplpDP)->lpvtbl = &directPlay2WVT;
    (*ilplpDP)->ref    = 1;

    return DP_OK;
  }

  /* Unknown interface type */
  return DPERR_NOINTERFACE;

}

/* Direct Play helper methods */

/* Get a new interface. To be used by QueryInterface. */ 
static HRESULT directPlay_QueryInterface 
         ( REFIID riid, LPVOID* ppvObj )
{

  if( IsEqualGUID( &IID_IDirectPlay2, riid ) )
  {
    IDirectPlay2Impl* lpDP = (IDirectPlay2Impl*)*ppvObj;

    lpDP = (IDirectPlay2Impl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      sizeof( *lpDP ) );

    if( !lpDP ) 
    {
       return DPERR_OUTOFMEMORY;
    }

    lpDP->lpvtbl = &directPlay2WVT;
    lpDP->ref    = 1;

    return S_OK;
  } 
  else if( IsEqualGUID( &IID_IDirectPlay2A, riid ) )
  {
    IDirectPlay2AImpl* lpDP = (IDirectPlay2AImpl*)*ppvObj;

    lpDP = (IDirectPlay2AImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      sizeof( *lpDP ) );

    if( !lpDP )
    {
       return DPERR_OUTOFMEMORY;
    }

    lpDP->lpvtbl = &directPlay2AVT;
    lpDP->ref    = 1;

    return S_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlay3, riid ) )
  {
    IDirectPlay3Impl* lpDP = (IDirectPlay3Impl*)*ppvObj;

    lpDP = (IDirectPlay3Impl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      sizeof( *lpDP ) );

    if( !lpDP )
    {
       return DPERR_OUTOFMEMORY;
    }

    lpDP->lpvtbl = &directPlay3WVT;
    lpDP->ref    = 1;

    return S_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlay3A, riid ) )
  {
    IDirectPlay3AImpl* lpDP = (IDirectPlay3AImpl*)*ppvObj;

    lpDP = (IDirectPlay3AImpl*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      sizeof( *lpDP ) );

    if( !lpDP )
    {
       return DPERR_OUTOFMEMORY;
    }

    lpDP->lpvtbl = &directPlay3AVT;
    lpDP->ref    = 1;

    return S_OK;

  }

  *ppvObj = NULL;
  return E_NOINTERFACE;
}


/* Direct Play methods */
static HRESULT WINAPI DirectPlay2W_QueryInterface
         ( LPDIRECTPLAY2 iface, REFIID riid, LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  TRACE( dplay, "(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay2, riid )
    )
  {
    IDirectPlay2_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }
  return directPlay_QueryInterface( riid, ppvObj );
}

static HRESULT WINAPI DirectPlay2A_QueryInterface
         ( LPDIRECTPLAY2A iface, REFIID riid, LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlay2Impl,iface);
  TRACE( dplay, "(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay2A, riid )
    )
  {
    IDirectPlay2_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlay_QueryInterface( riid, ppvObj );
}

static HRESULT WINAPI DirectPlay3WImpl_QueryInterface
         ( LPDIRECTPLAY3 iface, REFIID riid, LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  TRACE( dplay, "(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay3, riid )
    )
  {
    IDirectPlay3_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlay_QueryInterface( riid, ppvObj );
}

static HRESULT WINAPI DirectPlay3A_QueryInterface
         ( LPDIRECTPLAY3A iface, REFIID riid, LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  TRACE( dplay, "(%p)->(%p,%p)\n", This, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay3A, riid )
    )
  {
    IDirectPlay3_AddRef( iface );
    *ppvObj = This;
    return S_OK;
  }

  return directPlay_QueryInterface( riid, ppvObj );
}


/* Shared between all dplay types */
static ULONG WINAPI DirectPlay3WImpl_AddRef
         ( LPDIRECTPLAY3 iface )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  ++(This->ref);
  TRACE( dplay,"ref count now %lu\n", This->ref );
  return (This->ref);
}

static ULONG WINAPI DirectPlay3WImpl_Release
( LPDIRECTPLAY3 iface )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  TRACE( dplay, "ref count decremeneted from %lu\n", This->ref );

  This->ref--;

  /* Deallocate if this is the last reference to the object */
  if( !(This->ref) )
  {
    FIXME( dplay, "memory leak\n" );
    /* Implement memory deallocation */

    HeapFree( GetProcessHeap(), 0, This );

    return 0;
  }

  return This->ref;
}

static ULONG WINAPI DirectPlay3A_Release
( LPDIRECTPLAY3A iface )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  TRACE( dplay, "ref count decremeneted from %lu\n", This->ref );

  This->ref--;

  /* Deallocate if this is the last reference to the object */
  if( !(This->ref) )
  {
    FIXME( dplay, "memory leak\n" );
    /* Implement memory deallocation */

    HeapFree( GetProcessHeap(), 0, This );

    return 0;
  }

  return This->ref;
}

HRESULT WINAPI DirectPlay3A_AddPlayerToGroup
          ( LPDIRECTPLAY3A iface, DPID a, DPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_AddPlayerToGroup
          ( LPDIRECTPLAY3 iface, DPID a, DPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", This, a, b );
  return DP_OK;
}


HRESULT WINAPI DirectPlay3A_Close
          ( LPDIRECTPLAY3A iface )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(): stub", This );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_Close
          ( LPDIRECTPLAY3 iface )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(): stub", This );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_CreateGroup
          ( LPDIRECTPLAY3A iface, LPDPID a, LPDPNAME b, LPVOID c, DWORD d, DWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx,0x%08lx): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_CreateGroup
          ( LPDIRECTPLAY3 iface, LPDPID a, LPDPNAME b, LPVOID c, DWORD d, DWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx,0x%08lx): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_CreatePlayer
          ( LPDIRECTPLAY3A iface, LPDPID a, LPDPNAME b, HANDLE c, LPVOID d, DWORD e, DWORD f )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,%d,%p,0x%08lx,0x%08lx): stub", This, a, b, c, d, e, f );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_CreatePlayer
          ( LPDIRECTPLAY3 iface, LPDPID a, LPDPNAME b, HANDLE c, LPVOID d, DWORD e, DWORD f )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,%d,%p,0x%08lx,0x%08lx): stub", This, a, b, c, d, e, f );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_DeletePlayerFromGroup
          ( LPDIRECTPLAY3A iface, DPID a, DPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_DeletePlayerFromGroup
          ( LPDIRECTPLAY3 iface, DPID a, DPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_DestroyGroup
          ( LPDIRECTPLAY3A iface, DPID a )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx): stub", This, a );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_DestroyGroup
          ( LPDIRECTPLAY3 iface, DPID a )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx): stub", This, a );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_DestroyPlayer
          ( LPDIRECTPLAY3A iface, DPID a )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx): stub", This, a );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_DestroyPlayer
          ( LPDIRECTPLAY3 iface, DPID a )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx): stub", This, a );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumGroupPlayers
          ( LPDIRECTPLAY3A iface, DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c,
            LPVOID d, DWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_EnumGroupPlayers
          ( LPDIRECTPLAY3 iface, DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c,
            LPVOID d, DWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumGroups
          ( LPDIRECTPLAY3A iface, LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_EnumGroups
          ( LPDIRECTPLAY3 iface, LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumPlayers
          ( LPDIRECTPLAY3A iface, LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_EnumPlayers
          ( LPDIRECTPLAY3 iface, LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumSessions
          ( LPDIRECTPLAY3A iface, LPDPSESSIONDESC2 a, DWORD b, LPDPENUMSESSIONSCALLBACK2 c,
            LPVOID d, DWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx,%p,%p,0x%08lx): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_EnumSessions
          ( LPDIRECTPLAY3 iface, LPDPSESSIONDESC2 a, DWORD b, LPDPENUMSESSIONSCALLBACK2 c,
            LPVOID d, DWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx,%p,%p,0x%08lx): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetCaps
          ( LPDIRECTPLAY3A iface, LPDPCAPS a, DWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetCaps
          ( LPDIRECTPLAY3 iface, LPDPCAPS a, DWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetGroupData
          ( LPDIRECTPLAY3 iface, DPID a, LPVOID b, LPDWORD c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetGroupData
          ( LPDIRECTPLAY3 iface, DPID a, LPVOID b, LPDWORD c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetGroupName
          ( LPDIRECTPLAY3A iface, DPID a, LPVOID b, LPDWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetGroupName
          ( LPDIRECTPLAY3 iface, DPID a, LPVOID b, LPDWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetMessageCount
          ( LPDIRECTPLAY3A iface, DPID a, LPDWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetMessageCount
          ( LPDIRECTPLAY3 iface, DPID a, LPDWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerAddress
          ( LPDIRECTPLAY3A iface, DPID a, LPVOID b, LPDWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetPlayerAddress
          ( LPDIRECTPLAY3 iface, DPID a, LPVOID b, LPDWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerCaps
          ( LPDIRECTPLAY3A iface, DPID a, LPDPCAPS b, DWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetPlayerCaps
          ( LPDIRECTPLAY3 iface, DPID a, LPDPCAPS b, DWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerData
          ( LPDIRECTPLAY3A iface, DPID a, LPVOID b, LPDWORD c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetPlayerData
          ( LPDIRECTPLAY3 iface, DPID a, LPVOID b, LPDWORD c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerName
          ( LPDIRECTPLAY3 iface, DPID a, LPVOID b, LPDWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetPlayerName
          ( LPDIRECTPLAY3 iface, DPID a, LPVOID b, LPDWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetSessionDesc
          ( LPDIRECTPLAY3A iface, LPVOID a, LPDWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetSessionDesc
          ( LPDIRECTPLAY3 iface, LPVOID a, LPDWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_Initialize
          ( LPDIRECTPLAY3A iface, LPGUID a )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p): stub", This, a );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_Initialize
          ( LPDIRECTPLAY3 iface, LPGUID a )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p): stub", This, a );
  return DP_OK;
}


HRESULT WINAPI DirectPlay3A_Open
          ( LPDIRECTPLAY3A iface, LPDPSESSIONDESC2 a, DWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_Open
          ( LPDIRECTPLAY3 iface, LPDPSESSIONDESC2 a, DWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_Receive
          ( LPDIRECTPLAY3A iface, LPDPID a, LPDPID b, DWORD c, LPVOID d, LPDWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,0x%08lx,%p,%p): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_Receive
          ( LPDIRECTPLAY3 iface, LPDPID a, LPDPID b, DWORD c, LPVOID d, LPDWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,0x%08lx,%p,%p): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_Send
          ( LPDIRECTPLAY3A iface, DPID a, DPID b, DWORD c, LPVOID d, DWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,0x%08lx): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_Send
          ( LPDIRECTPLAY3 iface, DPID a, DPID b, DWORD c, LPVOID d, DWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,0x%08lx): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetGroupData
          ( LPDIRECTPLAY3A iface, DPID a, LPVOID b, DWORD c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_SetGroupData
          ( LPDIRECTPLAY3 iface, DPID a, LPVOID b, DWORD c, DWORD d )
{   
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetGroupName
          ( LPDIRECTPLAY3A iface, DPID a, LPDPNAME b, DWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_SetGroupName
          ( LPDIRECTPLAY3 iface, DPID a, LPDPNAME b, DWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetPlayerData
          ( LPDIRECTPLAY3A iface, DPID a, LPVOID b, DWORD c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_SetPlayerData
          ( LPDIRECTPLAY3 iface, DPID a, LPVOID b, DWORD c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetPlayerName
          ( LPDIRECTPLAY3A iface, DPID a, LPDPNAME b, DWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_SetPlayerName
          ( LPDIRECTPLAY3 iface, DPID a, LPDPNAME b, DWORD c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetSessionDesc
          ( LPDIRECTPLAY3A iface, LPDPSESSIONDESC2 a, DWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_SetSessionDesc
          ( LPDIRECTPLAY3 iface, LPDPSESSIONDESC2 a, DWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_AddGroupToGroup
          ( LPDIRECTPLAY3A iface, DPID a, DPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_AddGroupToGroup
          ( LPDIRECTPLAY3 iface, DPID a, DPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_CreateGroupInGroup
          ( LPDIRECTPLAY3A iface, DPID a, LPDPID b, LPDPNAME c, LPVOID d, DWORD e, DWORD f )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx,0x%08lx): stub", This, a, b, c, d, e, f );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_CreateGroupInGroup
          ( LPDIRECTPLAY3 iface, DPID a, LPDPID b, LPDPNAME c, LPVOID d, DWORD e, DWORD f )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx,0x%08lx): stub", This, a, b, c, d, e, f );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_DeleteGroupFromGroup
          ( LPDIRECTPLAY3A iface, DPID a, DPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_DeleteGroupFromGroup
          ( LPDIRECTPLAY3 iface, DPID a, DPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumConnections
          ( LPDIRECTPLAY3A iface, LPCGUID a, LPDPENUMCONNECTIONSCALLBACK b, LPVOID c, DWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_EnumConnections
          ( LPDIRECTPLAY3 iface, LPCGUID a, LPDPENUMCONNECTIONSCALLBACK b, LPVOID c, DWORD d )
{ 
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumGroupsInGroup
          ( LPDIRECTPLAY3A iface, DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c, LPVOID d, DWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_EnumGroupsInGroup
          ( LPDIRECTPLAY3 iface, DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c, LPVOID d, DWORD e )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub", This, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetGroupConnectionSettings
          ( LPDIRECTPLAY3A iface, DWORD a, DPID b, LPVOID c, LPDWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p,%p): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetGroupConnectionSettings
          ( LPDIRECTPLAY3 iface, DWORD a, DPID b, LPVOID c, LPDWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p,%p): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_InitializeConnection
          ( LPDIRECTPLAY3A iface, LPVOID a, DWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_InitializeConnection
          ( LPDIRECTPLAY3 iface, LPVOID a, DWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SecureOpen
          ( LPDIRECTPLAY3A iface, LPCDPSESSIONDESC2 a, DWORD b, LPCDPSECURITYDESC c, LPCDPCREDENTIALS d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx,%p,%p): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_SecureOpen
          ( LPDIRECTPLAY3 iface, LPCDPSESSIONDESC2 a, DWORD b, LPCDPSECURITYDESC c, LPCDPCREDENTIALS d )
{   
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(%p,0x%08lx,%p,%p): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SendChatMessage
          ( LPDIRECTPLAY3A iface, DPID a, DPID b, DWORD c, LPDPCHAT d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,0x%08lx,%p): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_SendChatMessage
          ( LPDIRECTPLAY3 iface, DPID a, DPID b, DWORD c, LPDPCHAT d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,0x%08lx,%p): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetGroupConnectionSettings
          ( LPDIRECTPLAY3A iface, DWORD a, DPID b, LPDPLCONNECTION c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_SetGroupConnectionSettings
          ( LPDIRECTPLAY3 iface, DWORD a, DPID b, LPDPLCONNECTION c )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p): stub", This, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_StartSession
          ( LPDIRECTPLAY3A iface, DWORD a, DPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_StartSession
          ( LPDIRECTPLAY3 iface, DWORD a, DPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", This, a, b );
  return DP_OK;
}
 
HRESULT WINAPI DirectPlay3A_GetGroupFlags
          ( LPDIRECTPLAY3A iface, DPID a, LPDWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetGroupFlags
          ( LPDIRECTPLAY3 iface, DPID a, LPDWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetGroupParent
          ( LPDIRECTPLAY3A iface, DPID a, LPDPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetGroupParent
          ( LPDIRECTPLAY3 iface, DPID a, LPDPID b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerAccount
          ( LPDIRECTPLAY3A iface, DPID a, DWORD b, LPVOID c, LPDWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p,%p): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetPlayerAccount
          ( LPDIRECTPLAY3 iface, DPID a, DWORD b, LPVOID c, LPDWORD d )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p,%p): stub", This, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerFlags
          ( LPDIRECTPLAY3A iface, DPID a, LPDWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", This, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3WImpl_GetPlayerFlags
          ( LPDIRECTPLAY3 iface, DPID a, LPDWORD b )
{
  ICOM_THIS(IDirectPlay3Impl,iface);
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", This, a, b );
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
  XCAST(AddRef)DirectPlay3WImpl_AddRef,
  XCAST(Release)DirectPlay3WImpl_Release,
  XCAST(AddPlayerToGroup)DirectPlay3WImpl_AddPlayerToGroup,
  XCAST(Close)DirectPlay3WImpl_Close,
  XCAST(CreateGroup)DirectPlay3WImpl_CreateGroup,
  XCAST(CreatePlayer)DirectPlay3WImpl_CreatePlayer,
  XCAST(DeletePlayerFromGroup)DirectPlay3WImpl_DeletePlayerFromGroup,
  XCAST(DestroyGroup)DirectPlay3WImpl_DestroyGroup,
  XCAST(DestroyPlayer)DirectPlay3WImpl_DestroyPlayer,
  XCAST(EnumGroupPlayers)DirectPlay3WImpl_EnumGroupPlayers,
  XCAST(EnumGroups)DirectPlay3WImpl_EnumGroups,
  XCAST(EnumPlayers)DirectPlay3WImpl_EnumPlayers,
  XCAST(EnumSessions)DirectPlay3WImpl_EnumSessions,
  XCAST(GetCaps)DirectPlay3WImpl_GetCaps,
  XCAST(GetGroupData)DirectPlay3WImpl_GetGroupData,
  XCAST(GetGroupName)DirectPlay3WImpl_GetGroupName,
  XCAST(GetMessageCount)DirectPlay3WImpl_GetMessageCount,
  XCAST(GetPlayerAddress)DirectPlay3WImpl_GetPlayerAddress,
  XCAST(GetPlayerCaps)DirectPlay3WImpl_GetPlayerCaps,
  XCAST(GetPlayerData)DirectPlay3WImpl_GetPlayerData,
  XCAST(GetPlayerName)DirectPlay3WImpl_GetPlayerName,
  XCAST(GetSessionDesc)DirectPlay3WImpl_GetSessionDesc,
  XCAST(Initialize)DirectPlay3WImpl_Initialize,
  XCAST(Open)DirectPlay3WImpl_Open,
  XCAST(Receive)DirectPlay3WImpl_Receive,
  XCAST(Send)DirectPlay3WImpl_Send,
  XCAST(SetGroupData)DirectPlay3WImpl_SetGroupData,
  XCAST(SetGroupName)DirectPlay3WImpl_SetGroupName,
  XCAST(SetPlayerData)DirectPlay3WImpl_SetPlayerData,
  XCAST(SetPlayerName)DirectPlay3WImpl_SetPlayerName,
  XCAST(SetSessionDesc)DirectPlay3WImpl_SetSessionDesc
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
  XCAST(AddRef)DirectPlay3WImpl_AddRef,
  XCAST(Release)DirectPlay3A_Release,
  XCAST(AddPlayerToGroup)DirectPlay3A_AddPlayerToGroup,
  XCAST(Close)DirectPlay3A_Close,
  XCAST(CreateGroup)DirectPlay3A_CreateGroup,
  XCAST(CreatePlayer)DirectPlay3A_CreatePlayer,
  XCAST(DeletePlayerFromGroup)DirectPlay3A_DeletePlayerFromGroup,
  XCAST(DestroyGroup)DirectPlay3A_DestroyGroup,
  XCAST(DestroyPlayer)DirectPlay3A_DestroyPlayer,
  XCAST(EnumGroupPlayers)DirectPlay3A_EnumGroupPlayers,
  XCAST(EnumGroups)DirectPlay3A_EnumGroups,
  XCAST(EnumPlayers)DirectPlay3A_EnumPlayers,
  XCAST(EnumSessions)DirectPlay3A_EnumSessions,
  XCAST(GetCaps)DirectPlay3A_GetCaps,
  XCAST(GetGroupData)DirectPlay3A_GetGroupData,
  XCAST(GetGroupName)DirectPlay3A_GetGroupName,
  XCAST(GetMessageCount)DirectPlay3A_GetMessageCount,
  XCAST(GetPlayerAddress)DirectPlay3A_GetPlayerAddress,
  XCAST(GetPlayerCaps)DirectPlay3A_GetPlayerCaps,
  XCAST(GetPlayerData)DirectPlay3A_GetPlayerData,
  XCAST(GetPlayerName)DirectPlay3A_GetPlayerName,
  XCAST(GetSessionDesc)DirectPlay3A_GetSessionDesc,
  XCAST(Initialize)DirectPlay3A_Initialize,
  XCAST(Open)DirectPlay3A_Open,
  XCAST(Receive)DirectPlay3A_Receive,
  XCAST(Send)DirectPlay3A_Send,
  XCAST(SetGroupData)DirectPlay3A_SetGroupData,
  XCAST(SetGroupName)DirectPlay3A_SetGroupName,
  XCAST(SetPlayerData)DirectPlay3A_SetPlayerData,
  XCAST(SetPlayerName)DirectPlay3A_SetPlayerName,
  XCAST(SetSessionDesc)DirectPlay3A_SetSessionDesc
};
#undef XCAST


static ICOM_VTABLE(IDirectPlay3) directPlay3AVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DirectPlay3A_QueryInterface,
  DirectPlay3WImpl_AddRef,
  DirectPlay3A_Release,
  DirectPlay3A_AddPlayerToGroup,
  DirectPlay3A_Close,
  DirectPlay3A_CreateGroup,
  DirectPlay3A_CreatePlayer,
  DirectPlay3A_DeletePlayerFromGroup,
  DirectPlay3A_DestroyGroup,
  DirectPlay3A_DestroyPlayer,
  DirectPlay3A_EnumGroupPlayers,
  DirectPlay3A_EnumGroups,
  DirectPlay3A_EnumPlayers,
  DirectPlay3A_EnumSessions,
  DirectPlay3A_GetCaps,
  DirectPlay3A_GetGroupData,
  DirectPlay3A_GetGroupName,
  DirectPlay3A_GetMessageCount,
  DirectPlay3A_GetPlayerAddress,
  DirectPlay3A_GetPlayerCaps,
  DirectPlay3A_GetPlayerData,
  DirectPlay3A_GetPlayerName,
  DirectPlay3A_GetSessionDesc,
  DirectPlay3A_Initialize,
  DirectPlay3A_Open,
  DirectPlay3A_Receive,
  DirectPlay3A_Send,
  DirectPlay3A_SetGroupData,
  DirectPlay3A_SetGroupName,
  DirectPlay3A_SetPlayerData,
  DirectPlay3A_SetPlayerName,
  DirectPlay3A_SetSessionDesc,

  DirectPlay3A_AddGroupToGroup,
  DirectPlay3A_CreateGroupInGroup,
  DirectPlay3A_DeleteGroupFromGroup,
  DirectPlay3A_EnumConnections,
  DirectPlay3A_EnumGroupsInGroup,
  DirectPlay3A_GetGroupConnectionSettings,
  DirectPlay3A_InitializeConnection,
  DirectPlay3A_SecureOpen,
  DirectPlay3A_SendChatMessage,
  DirectPlay3A_SetGroupConnectionSettings,
  DirectPlay3A_StartSession,
  DirectPlay3A_GetGroupFlags,
  DirectPlay3A_GetGroupParent,
  DirectPlay3A_GetPlayerAccount,
  DirectPlay3A_GetPlayerFlags
};

static ICOM_VTABLE(IDirectPlay3) directPlay3WVT = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DirectPlay3WImpl_QueryInterface,
  DirectPlay3WImpl_AddRef,
  DirectPlay3WImpl_Release,
  DirectPlay3WImpl_AddPlayerToGroup,
  DirectPlay3WImpl_Close,
  DirectPlay3WImpl_CreateGroup,
  DirectPlay3WImpl_CreatePlayer,
  DirectPlay3WImpl_DeletePlayerFromGroup,
  DirectPlay3WImpl_DestroyGroup,
  DirectPlay3WImpl_DestroyPlayer,
  DirectPlay3WImpl_EnumGroupPlayers,
  DirectPlay3WImpl_EnumGroups,
  DirectPlay3WImpl_EnumPlayers,
  DirectPlay3WImpl_EnumSessions,
  DirectPlay3WImpl_GetCaps,
  DirectPlay3WImpl_GetGroupData,
  DirectPlay3WImpl_GetGroupName,
  DirectPlay3WImpl_GetMessageCount,
  DirectPlay3WImpl_GetPlayerAddress,
  DirectPlay3WImpl_GetPlayerCaps,
  DirectPlay3WImpl_GetPlayerData,
  DirectPlay3WImpl_GetPlayerName,
  DirectPlay3WImpl_GetSessionDesc,
  DirectPlay3WImpl_Initialize,
  DirectPlay3WImpl_Open,
  DirectPlay3WImpl_Receive,
  DirectPlay3WImpl_Send,
  DirectPlay3WImpl_SetGroupData,
  DirectPlay3WImpl_SetGroupName,
  DirectPlay3WImpl_SetPlayerData,
  DirectPlay3WImpl_SetPlayerName,
  DirectPlay3WImpl_SetSessionDesc,

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
