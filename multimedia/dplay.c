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
#include "heap.h"
#include "debug.h"

struct IDirectPlayLobby {
    LPDIRECTPLAYLOBBY_VTABLE lpVtbl;
    ULONG                    ref;
    DWORD                    dwConnFlags;
    DPSESSIONDESC2           sessionDesc;
    DPNAME                   playerName;
    GUID                     guidSP;
    LPVOID                   lpAddress;
    DWORD                    dwAddressSize;
};

struct IDirectPlayLobby2 {
    LPDIRECTPLAYLOBBY2_VTABLE lpVtbl;
    ULONG                     ref;
    DWORD                     dwConnFlags;
    DPSESSIONDESC2            lpSessionDesc;
    DPNAME                    lpPlayerName;
    GUID                      guidSP;
    LPVOID                    lpAddress;
    DWORD                     dwAddressSize;
};


/* Forward declarations of virtual tables */
static DIRECTPLAYLOBBY_VTABLE  directPlayLobbyAVT;
static DIRECTPLAYLOBBY_VTABLE  directPlayLobbyWVT;
static DIRECTPLAYLOBBY2_VTABLE directPlayLobby2AVT;
static DIRECTPLAYLOBBY2_VTABLE directPlayLobby2WVT;


static DIRECTPLAY2_VTABLE directPlay2WVT;
static DIRECTPLAY2_VTABLE directPlay2AVT;
static DIRECTPLAY3_VTABLE directPlay3WVT;
static DIRECTPLAY3_VTABLE directPlay3AVT;




struct IDirectPlay2 {
  LPDIRECTPLAY2_VTABLE lpVtbl;
  ULONG                ref;
};

struct IDirectPlay3 {
  LPDIRECTPLAY3_VTABLE lpVtbl;
  ULONG                ref;
};

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
     LPDIRECTPLAYLOBBY lpDpL = (LPDIRECTPLAYLOBBY)(*ppvObj);

     lpDpL = (LPDIRECTPLAYLOBBY)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           sizeof( *lpDpL ) );

    if( !lpDpL )
    {
      return E_NOINTERFACE;
    }

    lpDpL->lpVtbl = &directPlayLobbyWVT;
    lpDpL->ref    = 1;

    return S_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobbyA, riid ) )
  {
     LPDIRECTPLAYLOBBYA lpDpL = (LPDIRECTPLAYLOBBYA)(*ppvObj);

     lpDpL = (LPDIRECTPLAYLOBBYA)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                            sizeof( *lpDpL ) );

    if( !lpDpL )
    {
      return E_NOINTERFACE;
    }

    lpDpL->lpVtbl = &directPlayLobbyAVT;
    lpDpL->ref    = 1;

    return S_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2, riid ) )
  {
     LPDIRECTPLAYLOBBY2 lpDpL = (LPDIRECTPLAYLOBBY2)(*ppvObj);

     lpDpL = (LPDIRECTPLAYLOBBY2)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                             sizeof( *lpDpL ) );

    if( !lpDpL )
    {
      return E_NOINTERFACE;
    }

    lpDpL->lpVtbl = &directPlayLobby2WVT;
    lpDpL->ref    = 1;

    return S_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2A, riid ) )
  {
     LPDIRECTPLAYLOBBY2A lpDpL = (LPDIRECTPLAYLOBBY2A)(*ppvObj);

     lpDpL = (LPDIRECTPLAYLOBBY2)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                             sizeof( *lpDpL ) );

    if( !lpDpL )
    {
      return E_NOINTERFACE;
    }

    lpDpL->lpVtbl = &directPlayLobby2AVT;
    lpDpL->ref    = 1;

    return S_OK;
  }

  /* Unknown interface */
  *ppvObj = NULL;
  return E_NOINTERFACE;
}
static HRESULT WINAPI IDirectPlayLobbyA_QueryInterface
( LPDIRECTPLAYLOBBYA this,
  REFIID riid,
  LPVOID* ppvObj )
{
  TRACE( dplay, "(%p)->(%p,%p)\n", this, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid )  ||
      IsEqualGUID( &IID_IDirectPlayLobbyA, riid )
    )
  {
    this->lpVtbl->fnAddRef( this );
    *ppvObj = this;
    return S_OK;
  }

  return directPlayLobby_QueryInterface( riid, ppvObj );

}

static HRESULT WINAPI IDirectPlayLobbyW_QueryInterface
( LPDIRECTPLAYLOBBY this,
  REFIID riid,
  LPVOID* ppvObj )
{
  TRACE( dplay, "(%p)->(%p,%p)\n", this, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid )  ||
      IsEqualGUID( &IID_IDirectPlayLobby, riid )
    )
  {
    this->lpVtbl->fnAddRef( this );
    *ppvObj = this;
    return S_OK;
  }

  return directPlayLobby_QueryInterface( riid, ppvObj );
}


static HRESULT WINAPI IDirectPlayLobby2A_QueryInterface
( LPDIRECTPLAYLOBBY2A this,
  REFIID riid,
  LPVOID* ppvObj )
{
  TRACE( dplay, "(%p)->(%p,%p)\n", this, riid, ppvObj );

  /* Compare riids. We know this object is a direct play lobby 2A object.
     If we are asking about the same type of interface we're fine.
   */
  if( IsEqualGUID( &IID_IUnknown, riid )  ||
      IsEqualGUID( &IID_IDirectPlayLobby2A, riid )
    )
  {
    this->lpVtbl->fnAddRef( this );
    *ppvObj = this;
    return S_OK;
  }
  return directPlayLobby_QueryInterface( riid, ppvObj ); 
};

static HRESULT WINAPI IDirectPlayLobby2W_QueryInterface
( LPDIRECTPLAYLOBBY2 this,
  REFIID riid,
  LPVOID* ppvObj )
{

  /* Compare riids. We know this object is a direct play lobby 2 object.
     If we are asking about the same type of interface we're fine.
   */
  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlayLobby2, riid ) 
    )
  {
    this->lpVtbl->fnAddRef( this );
    *ppvObj = this;
    return S_OK;
  }

  return directPlayLobby_QueryInterface( riid, ppvObj ); 

};

/* 
 * Simple procedure. Just increment the reference count to this
 * structure and return the new reference count.
 */
static ULONG WINAPI IDirectPlayLobby2A_AddRef
( LPDIRECTPLAYLOBBY2A this )
{
  ++(this->ref);
  TRACE( dplay,"ref count now %lu\n", this->ref );
  return (this->ref);
};

static ULONG WINAPI IDirectPlayLobby2W_AddRef
( LPDIRECTPLAYLOBBY2 this )
{
  return IDirectPlayLobby2A_AddRef( (LPDIRECTPLAYLOBBY2) this );
};


/*
 * Simple COM procedure. Decrease the reference count to this object.
 * If the object no longer has any reference counts, free up the associated
 * memory.
 */
static ULONG WINAPI IDirectPlayLobby2A_Release
( LPDIRECTPLAYLOBBY2A this )
{
  TRACE( dplay, "ref count decremeneted from %lu\n", this->ref );

  this->ref--;

  /* Deallocate if this is the last reference to the object */
  if( !(this->ref) )
  {
    FIXME( dplay, "memory leak\n" );
    /* Implement memory deallocation */

    HeapFree( GetProcessHeap(), 0, this );

    return 0;
  }

  return this->ref;
};

static ULONG WINAPI IDirectPlayLobby2W_Release
( LPDIRECTPLAYLOBBY2 this )
{
  return IDirectPlayLobby2A_Release( (LPDIRECTPLAYLOBBY2A) this );
};


/********************************************************************
 * 
 * Connects an application to the session specified by the DPLCONNECTION
 * structure currently stored with the DirectPlayLobby object.
 *
 * Returns a IDirectPlay interface.
 *
 */
static HRESULT WINAPI IDirectPlayLobby2A_Connect
( LPDIRECTPLAYLOBBY2A this,
  DWORD dwFlags,
  LPDIRECTPLAY2* lplpDP,
  IUnknown* pUnk)
{
  FIXME( dplay, ": dwFlags=%08lx %p %p stub\n", dwFlags, lplpDP, pUnk );
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2W_Connect
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  LPDIRECTPLAY2* lplpDP,
  IUnknown* pUnk)
{
  LPDIRECTPLAY2* directPlay2W;
  HRESULT        createRC;

  FIXME( dplay, "(%p)->(%08lx,%p,%p): stub\n", this, dwFlags, lplpDP, pUnk );

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
  if( this->lpSession->dwFlags == DPLCONNECTION_CREATESESSION )
  {
    DWORD threadIdSink;

    /* Spawn a thread to deal with all of this and to handle the incomming requests */
    threadIdSink = CreateThread( NULL, 0, &DPLobby_Spawn_Server,
                                (LPVOID)this->lpSession->lpConn->lpSessionDesc, 0, &threadIdSink );  

  }
  else if ( this->lpSession->dwFlags == DPLCONNECTION_JOINSESSION ) 
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

};

/********************************************************************
 *
 * Creates a DirectPlay Address, given a service provider-specific network
 * address. 
 * Returns an address contains the globally unique identifier
 * (GUID) of the service provider and data that the service provider can
 * interpret as a network address.
 *
 */
static HRESULT WINAPI IDirectPlayLobby2A_CreateAddress
( LPDIRECTPLAYLOBBY2A this,
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData, 
  DWORD dwDataSize,
  LPVOID lpAddress, 
  LPDWORD lpdwAddressSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2W_CreateAddress
( LPDIRECTPLAYLOBBY2 this,
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData,
  DWORD dwDataSize,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};


/********************************************************************
 *
 * Parses out chunks from the DirectPlay Address buffer by calling the
 * given callback function, with lpContext, for each of the chunks.
 *
 */
static HRESULT WINAPI IDirectPlayLobby2A_EnumAddress
( LPDIRECTPLAYLOBBY2A this,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2W_EnumAddress
( LPDIRECTPLAYLOBBY2 this,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

/********************************************************************
 *
 * Enumerates all the address types that a given service provider needs to
 * build the DirectPlay Address.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyA_EnumAddressTypes
( LPDIRECTPLAYLOBBYA this,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2A_EnumAddressTypes
( LPDIRECTPLAYLOBBY2A this,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP, 
  LPVOID lpContext,
  DWORD dwFlags )
{
  return IDirectPlayLobbyA_EnumAddressTypes( (LPDIRECTPLAYLOBBYA)this, lpEnumAddressTypeCallback,
                                             guidSP, lpContext, dwFlags );
};

static HRESULT WINAPI IDirectPlayLobby2W_EnumAddressTypes
( LPDIRECTPLAYLOBBY2 this,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

/********************************************************************
 *
 * Enumerates what applications are registered with DirectPlay by
 * invoking the callback function with lpContext.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyW_EnumLocalApplications
( LPDIRECTPLAYLOBBY this,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK a,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2W_EnumLocalApplications
( LPDIRECTPLAYLOBBY2 this,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK a,
  LPVOID lpContext,
  DWORD dwFlags )
{
  return IDirectPlayLobbyW_EnumLocalApplications( (LPDIRECTPLAYLOBBY)this, a,
                                                  lpContext, dwFlags ); 
};

static HRESULT WINAPI IDirectPlayLobbyA_EnumLocalApplications
( LPDIRECTPLAYLOBBYA this,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK a,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2A_EnumLocalApplications
( LPDIRECTPLAYLOBBY2A this,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK a,
  LPVOID lpContext,
  DWORD dwFlags )
{
  return IDirectPlayLobbyA_EnumLocalApplications( (LPDIRECTPLAYLOBBYA)this, a,
                                                  lpContext, dwFlags ); 
};


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
static HRESULT WINAPI IDirectPlayLobbyA_GetConnectionSettings
( LPDIRECTPLAYLOBBYA this,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  LPDPLCONNECTION lpDplConnection;

  FIXME( dplay, ": semi stub (%p)->(0x%08lx,%p,%p)\n", this, dwAppID, lpData, lpdwDataSize );
 
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
  lpDplConnection->dwFlags = this->dwConnFlags;

  /* Copy LPDPSESSIONDESC2 struct */
  lpDplConnection->lpSessionDesc = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( this->sessionDesc ) );
  memcpy( lpDplConnection->lpSessionDesc, &(this->sessionDesc), sizeof( this->sessionDesc ) );

  if( this->sessionDesc.sess.lpszSessionName )
  {
    lpDplConnection->lpSessionDesc->sess.lpszSessionName = 
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, this->sessionDesc.sess.lpszSessionName );
  }

  if( this->sessionDesc.pass.lpszPassword )
  {
    lpDplConnection->lpSessionDesc->pass.lpszPassword = 
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, this->sessionDesc.pass.lpszPassword );
  }
     
  /* I don't know what to use the reserved for. We'll set it to 0 just for fun */
  this->sessionDesc.dwReserved1 = this->sessionDesc.dwReserved2 = 0;

  /* Copy DPNAME struct - seems to be optional - check for existance first */
  lpDplConnection->lpPlayerName = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( this->playerName ) );
  memcpy( lpDplConnection->lpPlayerName, &(this->playerName), sizeof( this->playerName ) );

  if( this->playerName.psn.lpszShortName )
  {
    lpDplConnection->lpPlayerName->psn.lpszShortName =
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, this->playerName.psn.lpszShortName );  
  }

  if( this->playerName.pln.lpszLongName )
  {
    lpDplConnection->lpPlayerName->pln.lpszLongName =
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, this->playerName.pln.lpszLongName );
  }



  memcpy( &(lpDplConnection->guidSP), &(this->guidSP), sizeof( this->guidSP ) );

  lpDplConnection->lpAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, this->dwAddressSize );
  memcpy( lpDplConnection->lpAddress, this->lpAddress, this->dwAddressSize );

  lpDplConnection->dwAddressSize = this->dwAddressSize;

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby2A_GetConnectionSettings
( LPDIRECTPLAYLOBBY2A this,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  return IDirectPlayLobbyA_GetConnectionSettings( (LPDIRECTPLAYLOBBYA)this,
                                                  dwAppID, lpData, lpdwDataSize ); 
}

static HRESULT WINAPI IDirectPlayLobbyW_GetConnectionSettings
( LPDIRECTPLAYLOBBY this,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  FIXME( dplay, ":semi stub %p %08lx %p %p \n", this, dwAppID, lpData, lpdwDataSize );

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
};

static HRESULT WINAPI IDirectPlayLobby2W_GetConnectionSettings
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  return IDirectPlayLobbyW_GetConnectionSettings( (LPDIRECTPLAYLOBBY)this,
                                                  dwAppID, lpData, lpdwDataSize );
}

/********************************************************************
 *
 * Retrieves the message sent between a lobby client and a DirectPlay 
 * application. All messages are queued until received.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyA_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBYA this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  FIXME( dplay, ":stub %p %08lx %08lx %p %p %p\n", this, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2A_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBY2A this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  return IDirectPlayLobbyA_ReceiveLobbyMessage( (LPDIRECTPLAYLOBBYA)this, dwFlags, dwAppID,
                                                 lpdwMessageFlags, lpData, lpdwDataSize );
};


static HRESULT WINAPI IDirectPlayLobbyW_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBY this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  FIXME( dplay, ":stub %p %08lx %08lx %p %p %p\n", this, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2W_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  return IDirectPlayLobbyW_ReceiveLobbyMessage( (LPDIRECTPLAYLOBBY)this, dwFlags, dwAppID,
                                                 lpdwMessageFlags, lpData, lpdwDataSize );
};

/********************************************************************
 *
 * Starts an application and passes to it all the information to
 * connect to a session.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyA_RunApplication
( LPDIRECTPLAYLOBBYA this,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2A_RunApplication
( LPDIRECTPLAYLOBBY2A this,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  return IDirectPlayLobbyA_RunApplication( (LPDIRECTPLAYLOBBYA)this, dwFlags,
                                           lpdwAppID, lpConn, hReceiveEvent );
};

static HRESULT WINAPI IDirectPlayLobbyW_RunApplication
( LPDIRECTPLAYLOBBY this,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2W_RunApplication
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  return IDirectPlayLobbyW_RunApplication( (LPDIRECTPLAYLOBBY)this, dwFlags,
                                           lpdwAppID, lpConn, hReceiveEvent );
};


/********************************************************************
 *
 * Sends a message between the application and the lobby client.
 * All messages are queued until received.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyA_SendLobbyMessage
( LPDIRECTPLAYLOBBYA this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2A_SendLobbyMessage
( LPDIRECTPLAYLOBBY2A this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  return IDirectPlayLobbyA_SendLobbyMessage( (LPDIRECTPLAYLOBBYA)this, dwFlags, 
                                             dwAppID, lpData, dwDataSize ); 
};


static HRESULT WINAPI IDirectPlayLobbyW_SendLobbyMessage
( LPDIRECTPLAYLOBBY this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2W_SendLobbyMessage
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  return IDirectPlayLobbyW_SendLobbyMessage( (LPDIRECTPLAYLOBBY)this, dwFlags,
                                              dwAppID, lpData, dwDataSize );
};

/********************************************************************
 *
 * Modifies the DPLCONNECTION structure to contain all information
 * needed to start and connect an application.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyW_SetConnectionSettings
( LPDIRECTPLAYLOBBY this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  TRACE( dplay, ": this=%p, dwFlags=%08lx, dwAppId=%08lx, lpConn=%p\n",
         this, dwFlags, dwAppID, lpConn );

  /* Paramater check */
  if( dwFlags || !this || !lpConn )
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
  this->dwConnFlags = lpConn->dwFlags;

  /* Copy LPDPSESSIONDESC2 struct - this is required */
  memcpy( &(this->sessionDesc), lpConn->lpSessionDesc, sizeof( *(lpConn->lpSessionDesc) ) );

  if( lpConn->lpSessionDesc->sess.lpszSessionName )
    this->sessionDesc.sess.lpszSessionName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->sess.lpszSessionName );
  else
    this->sessionDesc.sess.lpszSessionName = NULL;
 
  if( lpConn->lpSessionDesc->pass.lpszPassword )
    this->sessionDesc.pass.lpszPassword = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->pass.lpszPassword );
  else
    this->sessionDesc.pass.lpszPassword = NULL;

  /* I don't know what to use the reserved for ... */
  this->sessionDesc.dwReserved1 = this->sessionDesc.dwReserved2 = 0;

  /* Copy DPNAME struct - seems to be optional - check for existance first */
  if( lpConn->lpPlayerName )
  {
     memcpy( &(this->playerName), lpConn->lpPlayerName, sizeof( *lpConn->lpPlayerName ) ); 

     if( lpConn->lpPlayerName->psn.lpszShortName )
       this->playerName.psn.lpszShortName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->psn.lpszShortName ); 

     if( lpConn->lpPlayerName->pln.lpszLongName )
       this->playerName.pln.lpszLongName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->pln.lpszLongName );

  }

  memcpy( &(this->guidSP), &(lpConn->guidSP), sizeof( lpConn->guidSP ) );  
  
  this->lpAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->dwAddressSize ); 
  memcpy( this->lpAddress, lpConn->lpAddress, lpConn->dwAddressSize );

  this->dwAddressSize = lpConn->dwAddressSize;

  return DP_OK;
};

static HRESULT WINAPI IDirectPlayLobby2W_SetConnectionSettings
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  return IDirectPlayLobbyW_SetConnectionSettings( (LPDIRECTPLAYLOBBY)this, 
                                                  dwFlags, dwAppID, lpConn );
}

static HRESULT WINAPI IDirectPlayLobbyA_SetConnectionSettings
( LPDIRECTPLAYLOBBYA this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  FIXME( dplay, ": this=%p, dwFlags=%08lx, dwAppId=%08lx, lpConn=%p: stub\n",
         this, dwFlags, dwAppID, lpConn );
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2A_SetConnectionSettings
( LPDIRECTPLAYLOBBY2A this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  return IDirectPlayLobbyA_SetConnectionSettings( (LPDIRECTPLAYLOBBYA)this,
                                                  dwFlags, dwAppID, lpConn );
};

/********************************************************************
 *
 * Registers an event that will be set when a lobby message is received.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyA_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBYA this,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2A_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY2A this,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  return IDirectPlayLobbyA_SetLobbyMessageEvent( (LPDIRECTPLAYLOBBYA)this, dwFlags,
                                                 dwAppID, hReceiveEvent ); 
};

static HRESULT WINAPI IDirectPlayLobbyW_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY this,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2W_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  return IDirectPlayLobbyW_SetLobbyMessageEvent( (LPDIRECTPLAYLOBBY)this, dwFlags,
                                                 dwAppID, hReceiveEvent ); 
};


/********************************************************************
 *
 * Registers an event that will be set when a lobby message is received.
 *
 */
static HRESULT WINAPI IDirectPlayLobby2W_CreateCompoundAddress
( LPDIRECTPLAYLOBBY2 this,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2A_CreateCompoundAddress
( LPDIRECTPLAYLOBBY2A this,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};


/* Direct Play Lobby 1 (ascii) Virtual Table for methods */
/* All lobby 1 methods are exactly the same except QueryInterface */
static struct tagLPDIRECTPLAYLOBBY_VTABLE directPlayLobbyAVT = {
  IDirectPlayLobbyA_QueryInterface,
  (void*)IDirectPlayLobby2A_AddRef,
  (void*)IDirectPlayLobby2A_Release,
  (void*)IDirectPlayLobby2A_Connect,
  (void*)IDirectPlayLobby2A_CreateAddress,
  (void*)IDirectPlayLobby2A_EnumAddress,
  (void*)IDirectPlayLobby2A_EnumAddressTypes,
  (void*)IDirectPlayLobby2A_EnumLocalApplications,
  (void*)IDirectPlayLobby2A_GetConnectionSettings,
  (void*)IDirectPlayLobby2A_ReceiveLobbyMessage,
  (void*)IDirectPlayLobby2A_RunApplication,
  (void*)IDirectPlayLobby2A_SendLobbyMessage,
  (void*)IDirectPlayLobby2A_SetConnectionSettings,
  (void*)IDirectPlayLobby2A_SetLobbyMessageEvent
};

/* Direct Play Lobby 1 (unicode) Virtual Table for methods */
static struct tagLPDIRECTPLAYLOBBY_VTABLE directPlayLobbyWVT = {
  IDirectPlayLobbyW_QueryInterface,
  (void*)IDirectPlayLobby2W_AddRef,
  (void*)IDirectPlayLobby2W_Release,
  (void*)IDirectPlayLobby2W_Connect,
  (void*)IDirectPlayLobby2W_CreateAddress, 
  (void*)IDirectPlayLobby2W_EnumAddress,
  (void*)IDirectPlayLobby2W_EnumAddressTypes,
  (void*)IDirectPlayLobby2W_EnumLocalApplications,
  (void*)IDirectPlayLobby2W_GetConnectionSettings,
  (void*)IDirectPlayLobby2W_ReceiveLobbyMessage,
  (void*)IDirectPlayLobby2W_RunApplication,
  (void*)IDirectPlayLobby2W_SendLobbyMessage,
  (void*)IDirectPlayLobby2W_SetConnectionSettings,
  (void*)IDirectPlayLobby2W_SetLobbyMessageEvent
};


/* Direct Play Lobby 2 (ascii) Virtual Table for methods */
static struct tagLPDIRECTPLAYLOBBY2_VTABLE directPlayLobby2AVT = {
  IDirectPlayLobby2A_QueryInterface,
  IDirectPlayLobby2A_AddRef,
  IDirectPlayLobby2A_Release,
  IDirectPlayLobby2A_Connect,
  IDirectPlayLobby2A_CreateAddress,
  IDirectPlayLobby2A_EnumAddress,
  IDirectPlayLobby2A_EnumAddressTypes,
  IDirectPlayLobby2A_EnumLocalApplications,
  IDirectPlayLobby2A_GetConnectionSettings,
  IDirectPlayLobby2A_ReceiveLobbyMessage,
  IDirectPlayLobby2A_RunApplication,
  IDirectPlayLobby2A_SendLobbyMessage,
  IDirectPlayLobby2A_SetConnectionSettings,
  IDirectPlayLobby2A_SetLobbyMessageEvent,
  IDirectPlayLobby2A_CreateCompoundAddress 
};

/* Direct Play Lobby 2 (unicode) Virtual Table for methods */
static struct tagLPDIRECTPLAYLOBBY2_VTABLE directPlayLobby2WVT = {
  IDirectPlayLobby2W_QueryInterface,
  IDirectPlayLobby2W_AddRef, 
  IDirectPlayLobby2W_Release,
  IDirectPlayLobby2W_Connect,
  IDirectPlayLobby2W_CreateAddress,
  IDirectPlayLobby2W_EnumAddress,
  IDirectPlayLobby2W_EnumAddressTypes,
  IDirectPlayLobby2W_EnumLocalApplications,
  IDirectPlayLobby2W_GetConnectionSettings,
  IDirectPlayLobby2W_ReceiveLobbyMessage,
  IDirectPlayLobby2W_RunApplication,
  IDirectPlayLobby2W_SendLobbyMessage,
  IDirectPlayLobby2W_SetConnectionSettings,
  IDirectPlayLobby2W_SetLobbyMessageEvent,
  IDirectPlayLobby2W_CreateCompoundAddress
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
  TRACE(dplay,"lpGUIDDSP=%p lplpDPL=%p lpUnk=%p lpData=%p dwDataSize=%08lx\n",
        lpGUIDDSP,lplpDPL,lpUnk,lpData,dwDataSize);

  /* Parameter Check: lpGUIDSP, lpUnk & lpData must be NULL. dwDataSize must
   * equal 0. These fields are mostly for future expansion.
   */
  if ( lpGUIDDSP || lpUnk || lpData || dwDataSize )
  {
     *lplpDPL = NULL;
     return DPERR_INVALIDPARAMS;
  }

  /* Yes...really we should be returning a lobby 1 object */
  *lplpDPL = (LPDIRECTPLAYLOBBYA)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                            sizeof( IDirectPlayLobbyA ) );

  if( ! (*lplpDPL) )
  {
     return DPERR_OUTOFMEMORY;
  }

  (*lplpDPL)->lpVtbl = &directPlayLobbyAVT;
  (*lplpDPL)->ref    = 1;

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
  TRACE(dplay,"lpGUIDDSP=%p lplpDPL=%p lpUnk=%p lpData=%p dwDataSize=%08lx\n",
        lpGUIDDSP,lplpDPL,lpUnk,lpData,dwDataSize);

  /* Parameter Check: lpGUIDSP, lpUnk & lpData must be NULL. dwDataSize must 
   * equal 0. These fields are mostly for future expansion.
   */
  if ( lpGUIDDSP || lpUnk || lpData || dwDataSize )
  {
     *lplpDPL = NULL;
     ERR( dplay, "Bad parameters!\n" );
     return DPERR_INVALIDPARAMS;
  }

  /* Yes...really we should bre returning a lobby 1 object */
  *lplpDPL = (LPDIRECTPLAYLOBBY)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           sizeof( IDirectPlayLobby ) );

  if( !*lplpDPL)
  {
     return DPERR_OUTOFMEMORY;
  }

  (*lplpDPL)->lpVtbl = &directPlayLobbyWVT;
  (*lplpDPL)->ref    = 1;

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

};

/***************************************************************************
 *  DirectPlayEnumerateW (DPLAYX.3)
 *
 */
HRESULT WINAPI DirectPlayEnumerateW( LPDPENUMDPCALLBACKW lpEnumCallback, LPVOID lpContext )
{

  FIXME( dplay, ":stub\n");

  return DPERR_OUTOFMEMORY; 

};

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
    *lplpDP = (LPDIRECTPLAY2A)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                         sizeof( **lplpDP ) );

    if( !*lplpDP )
    {
       return DPERR_OUTOFMEMORY;
    }

    (*lplpDP)->lpVtbl = &directPlay2AVT;
    (*lplpDP)->ref    = 1;
  
    return DP_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlay2, lpGUID ) )
  {
    *lplpDP = (LPDIRECTPLAY2)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        sizeof( **lplpDP ) );

    if( !*lplpDP )
    {
       return DPERR_OUTOFMEMORY;
    }

    (*lplpDP)->lpVtbl = &directPlay2WVT;
    (*lplpDP)->ref    = 1;

    return DP_OK;
  }

  /* Unknown interface type */
  return DPERR_NOINTERFACE;

};

/* Direct Play helper methods */

/* Get a new interface. To be used by QueryInterface. */ 
static HRESULT directPlay_QueryInterface 
         ( REFIID riid, LPVOID* ppvObj )
{

  if( IsEqualGUID( &IID_IDirectPlay2, riid ) )
  {
    LPDIRECTPLAY2 lpDP = (LPDIRECTPLAY2)*ppvObj;

    lpDP = (LPDIRECTPLAY2)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      sizeof( *lpDP ) );

    if( !lpDP ) 
    {
       return DPERR_OUTOFMEMORY;
    }

    lpDP->lpVtbl = &directPlay2WVT;
    lpDP->ref    = 1;

    return S_OK;
  } 
  else if( IsEqualGUID( &IID_IDirectPlay2A, riid ) )
  {
    LPDIRECTPLAY2A lpDP = (LPDIRECTPLAY2A)*ppvObj;

    lpDP = (LPDIRECTPLAY2A)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      sizeof( *lpDP ) );

    if( !lpDP )
    {
       return DPERR_OUTOFMEMORY;
    }

    lpDP->lpVtbl = &directPlay2AVT;
    lpDP->ref    = 1;

    return S_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlay3, riid ) )
  {
    LPDIRECTPLAY3 lpDP = (LPDIRECTPLAY3)*ppvObj;

    lpDP = (LPDIRECTPLAY3)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      sizeof( *lpDP ) );

    if( !lpDP )
    {
       return DPERR_OUTOFMEMORY;
    }

    lpDP->lpVtbl = &directPlay3WVT;
    lpDP->ref    = 1;

    return S_OK;
  }
  else if( IsEqualGUID( &IID_IDirectPlay3A, riid ) )
  {
    LPDIRECTPLAY3A lpDP = (LPDIRECTPLAY3A)*ppvObj;

    lpDP = (LPDIRECTPLAY3A)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      sizeof( *lpDP ) );

    if( !lpDP )
    {
       return DPERR_OUTOFMEMORY;
    }

    lpDP->lpVtbl = &directPlay3AVT;
    lpDP->ref    = 1;

    return S_OK;

  }

  *ppvObj = NULL;
  return E_NOINTERFACE;
}


/* Direct Play methods */
static HRESULT WINAPI DirectPlay2W_QueryInterface
         ( LPDIRECTPLAY2 this, REFIID riid, LPVOID* ppvObj )
{
  TRACE( dplay, "(%p)->(%p,%p)\n", this, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay2, riid )
    )
  {
    this->lpVtbl->fnAddRef( this );
    *ppvObj = this;
    return S_OK;
  }
  return directPlay_QueryInterface( riid, ppvObj );
}

static HRESULT WINAPI DirectPlay2A_QueryInterface
         ( LPDIRECTPLAY2A this, REFIID riid, LPVOID* ppvObj )
{
  TRACE( dplay, "(%p)->(%p,%p)\n", this, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay2A, riid )
    )
  {
    this->lpVtbl->fnAddRef( this );
    *ppvObj = this;
    return S_OK;
  }

  return directPlay_QueryInterface( riid, ppvObj );
}

static HRESULT WINAPI DirectPlay3W_QueryInterface
         ( LPDIRECTPLAY3 this, REFIID riid, LPVOID* ppvObj )
{
  TRACE( dplay, "(%p)->(%p,%p)\n", this, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay3, riid )
    )
  {
    this->lpVtbl->fnAddRef( this );
    *ppvObj = this;
    return S_OK;
  }

  return directPlay_QueryInterface( riid, ppvObj );
}

static HRESULT WINAPI DirectPlay3A_QueryInterface
         ( LPDIRECTPLAY3A this, REFIID riid, LPVOID* ppvObj )
{
  TRACE( dplay, "(%p)->(%p,%p)\n", this, riid, ppvObj );

  if( IsEqualGUID( &IID_IUnknown, riid ) ||
      IsEqualGUID( &IID_IDirectPlay3A, riid )
    )
  {
    this->lpVtbl->fnAddRef( this );
    *ppvObj = this;
    return S_OK;
  }

  return directPlay_QueryInterface( riid, ppvObj );
}


/* Shared between all dplay types */
static ULONG WINAPI DirectPlay3W_AddRef
         ( LPDIRECTPLAY3 this )
{
  ++(this->ref);
  TRACE( dplay,"ref count now %lu\n", this->ref );
  return (this->ref);
}

static ULONG WINAPI DirectPlay3W_Release
( LPDIRECTPLAY3 this )
{
  TRACE( dplay, "ref count decremeneted from %lu\n", this->ref );

  this->ref--;

  /* Deallocate if this is the last reference to the object */
  if( !(this->ref) )
  {
    FIXME( dplay, "memory leak\n" );
    /* Implement memory deallocation */

    HeapFree( GetProcessHeap(), 0, this );

    return 0;
  }

  return this->ref;
};

static ULONG WINAPI DirectPlay3A_Release
( LPDIRECTPLAY3A this )
{
  TRACE( dplay, "ref count decremeneted from %lu\n", this->ref );

  this->ref--;

  /* Deallocate if this is the last reference to the object */
  if( !(this->ref) )
  {
    FIXME( dplay, "memory leak\n" );
    /* Implement memory deallocation */

    HeapFree( GetProcessHeap(), 0, this );

    return 0;
  }

  return this->ref;
};

HRESULT WINAPI DirectPlay3A_AddPlayerToGroup
          ( LPDIRECTPLAY3A this, DPID a, DPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_AddPlayerToGroup
          ( LPDIRECTPLAY3 this, DPID a, DPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", this, a, b );
  return DP_OK;
}


HRESULT WINAPI DirectPlay3A_Close
          ( LPDIRECTPLAY3A this )
{
  FIXME( dplay,"(%p)->(): stub", this );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_Close
          ( LPDIRECTPLAY3 this )
{
  FIXME( dplay,"(%p)->(): stub", this );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_CreateGroup
          ( LPDIRECTPLAY3A this, LPDPID a, LPDPNAME b, LPVOID c, DWORD d, DWORD e )
{
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx,0x%08lx): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_CreateGroup
          ( LPDIRECTPLAY3 this, LPDPID a, LPDPNAME b, LPVOID c, DWORD d, DWORD e )
{
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx,0x%08lx): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_CreatePlayer
          ( LPDIRECTPLAY3A this, LPDPID a, LPDPNAME b, HANDLE c, LPVOID d, DWORD e, DWORD f )
{
  FIXME( dplay,"(%p)->(%p,%p,%d,%p,0x%08lx,0x%08lx): stub", this, a, b, c, d, e, f );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_CreatePlayer
          ( LPDIRECTPLAY3 this, LPDPID a, LPDPNAME b, HANDLE c, LPVOID d, DWORD e, DWORD f )
{
  FIXME( dplay,"(%p)->(%p,%p,%d,%p,0x%08lx,0x%08lx): stub", this, a, b, c, d, e, f );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_DeletePlayerFromGroup
          ( LPDIRECTPLAY3A this, DPID a, DPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_DeletePlayerFromGroup
          ( LPDIRECTPLAY3 this, DPID a, DPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_DestroyGroup
          ( LPDIRECTPLAY3A this, DPID a )
{
  FIXME( dplay,"(%p)->(0x%08lx): stub", this, a );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_DestroyGroup
          ( LPDIRECTPLAY3 this, DPID a )
{
  FIXME( dplay,"(%p)->(0x%08lx): stub", this, a );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_DestroyPlayer
          ( LPDIRECTPLAY3A this, DPID a )
{
  FIXME( dplay,"(%p)->(0x%08lx): stub", this, a );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_DestroyPlayer
          ( LPDIRECTPLAY3 this, DPID a )
{
  FIXME( dplay,"(%p)->(0x%08lx): stub", this, a );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumGroupPlayers
          ( LPDIRECTPLAY3A this, DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c,
            LPVOID d, DWORD e )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_EnumGroupPlayers
          ( LPDIRECTPLAY3 this, DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c,
            LPVOID d, DWORD e )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumGroups
          ( LPDIRECTPLAY3A this, LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d )
{
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_EnumGroups
          ( LPDIRECTPLAY3 this, LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d )
{
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumPlayers
          ( LPDIRECTPLAY3A this, LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d )
{
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_EnumPlayers
          ( LPDIRECTPLAY3 this, LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d )
{
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumSessions
          ( LPDIRECTPLAY3A this, LPDPSESSIONDESC2 a, DWORD b, LPDPENUMSESSIONSCALLBACK2 c,
            LPVOID d, DWORD e )
{
  FIXME( dplay,"(%p)->(%p,0x%08lx,%p,%p,0x%08lx): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_EnumSessions
          ( LPDIRECTPLAY3 this, LPDPSESSIONDESC2 a, DWORD b, LPDPENUMSESSIONSCALLBACK2 c,
            LPVOID d, DWORD e )
{
  FIXME( dplay,"(%p)->(%p,0x%08lx,%p,%p,0x%08lx): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetCaps
          ( LPDIRECTPLAY3A this, LPDPCAPS a, DWORD b )
{
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetCaps
          ( LPDIRECTPLAY3 this, LPDPCAPS a, DWORD b )
{
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetGroupData
          ( LPDIRECTPLAY3 this, DPID a, LPVOID b, LPDWORD c, DWORD d )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetGroupData
          ( LPDIRECTPLAY3 this, DPID a, LPVOID b, LPDWORD c, DWORD d )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetGroupName
          ( LPDIRECTPLAY3A this, DPID a, LPVOID b, LPDWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetGroupName
          ( LPDIRECTPLAY3 this, DPID a, LPVOID b, LPDWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetMessageCount
          ( LPDIRECTPLAY3A this, DPID a, LPDWORD b )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetMessageCount
          ( LPDIRECTPLAY3 this, DPID a, LPDWORD b )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerAddress
          ( LPDIRECTPLAY3A this, DPID a, LPVOID b, LPDWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetPlayerAddress
          ( LPDIRECTPLAY3 this, DPID a, LPVOID b, LPDWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerCaps
          ( LPDIRECTPLAY3A this, DPID a, LPDPCAPS b, DWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetPlayerCaps
          ( LPDIRECTPLAY3 this, DPID a, LPDPCAPS b, DWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerData
          ( LPDIRECTPLAY3A this, DPID a, LPVOID b, LPDWORD c, DWORD d )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetPlayerData
          ( LPDIRECTPLAY3 this, DPID a, LPVOID b, LPDWORD c, DWORD d )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerName
          ( LPDIRECTPLAY3 this, DPID a, LPVOID b, LPDWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetPlayerName
          ( LPDIRECTPLAY3 this, DPID a, LPVOID b, LPDWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetSessionDesc
          ( LPDIRECTPLAY3A this, LPVOID a, LPDWORD b )
{
  FIXME( dplay,"(%p)->(%p,%p): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetSessionDesc
          ( LPDIRECTPLAY3 this, LPVOID a, LPDWORD b )
{
  FIXME( dplay,"(%p)->(%p,%p): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_Initialize
          ( LPDIRECTPLAY3A this, LPGUID a )
{
  FIXME( dplay,"(%p)->(%p): stub", this, a );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_Initialize
          ( LPDIRECTPLAY3 this, LPGUID a )
{
  FIXME( dplay,"(%p)->(%p): stub", this, a );
  return DP_OK;
}


HRESULT WINAPI DirectPlay3A_Open
          ( LPDIRECTPLAY3A this, LPDPSESSIONDESC2 a, DWORD b )
{
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_Open
          ( LPDIRECTPLAY3 this, LPDPSESSIONDESC2 a, DWORD b )
{
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_Receive
          ( LPDIRECTPLAY3A this, LPDPID a, LPDPID b, DWORD c, LPVOID d, LPDWORD e )
{
  FIXME( dplay,"(%p)->(%p,%p,0x%08lx,%p,%p): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_Receive
          ( LPDIRECTPLAY3 this, LPDPID a, LPDPID b, DWORD c, LPVOID d, LPDWORD e )
{
  FIXME( dplay,"(%p)->(%p,%p,0x%08lx,%p,%p): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_Send
          ( LPDIRECTPLAY3A this, DPID a, DPID b, DWORD c, LPVOID d, DWORD e )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,0x%08lx): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_Send
          ( LPDIRECTPLAY3 this, DPID a, DPID b, DWORD c, LPVOID d, DWORD e )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,0x%08lx): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetGroupData
          ( LPDIRECTPLAY3A this, DPID a, LPVOID b, DWORD c, DWORD d )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_SetGroupData
          ( LPDIRECTPLAY3 this, DPID a, LPVOID b, DWORD c, DWORD d )
{   
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetGroupName
          ( LPDIRECTPLAY3A this, DPID a, LPDPNAME b, DWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_SetGroupName
          ( LPDIRECTPLAY3 this, DPID a, LPDPNAME b, DWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetPlayerData
          ( LPDIRECTPLAY3A this, DPID a, LPVOID b, DWORD c, DWORD d )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_SetPlayerData
          ( LPDIRECTPLAY3 this, DPID a, LPVOID b, DWORD c, DWORD d )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetPlayerName
          ( LPDIRECTPLAY3A this, DPID a, LPDPNAME b, DWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_SetPlayerName
          ( LPDIRECTPLAY3 this, DPID a, LPDPNAME b, DWORD c )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,0x%08lx): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetSessionDesc
          ( LPDIRECTPLAY3A this, LPDPSESSIONDESC2 a, DWORD b )
{
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_SetSessionDesc
          ( LPDIRECTPLAY3 this, LPDPSESSIONDESC2 a, DWORD b )
{
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_AddGroupToGroup
          ( LPDIRECTPLAY3A this, DPID a, DPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_AddGroupToGroup
          ( LPDIRECTPLAY3 this, DPID a, DPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_CreateGroupInGroup
          ( LPDIRECTPLAY3A this, DPID a, LPDPID b, LPDPNAME c, LPVOID d, DWORD e, DWORD f )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx,0x%08lx): stub", this, a, b, c, d, e, f );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_CreateGroupInGroup
          ( LPDIRECTPLAY3 this, DPID a, LPDPID b, LPDPNAME c, LPVOID d, DWORD e, DWORD f )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx,0x%08lx): stub", this, a, b, c, d, e, f );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_DeleteGroupFromGroup
          ( LPDIRECTPLAY3A this, DPID a, DPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_DeleteGroupFromGroup
          ( LPDIRECTPLAY3 this, DPID a, DPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumConnections
          ( LPDIRECTPLAY3A this, LPCGUID a, LPDPENUMCONNECTIONSCALLBACK b, LPVOID c, DWORD d )
{
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_EnumConnections
          ( LPDIRECTPLAY3 this, LPCGUID a, LPDPENUMCONNECTIONSCALLBACK b, LPVOID c, DWORD d )
{ 
  FIXME( dplay,"(%p)->(%p,%p,%p,0x%08lx): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_EnumGroupsInGroup
          ( LPDIRECTPLAY3A this, DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c, LPVOID d, DWORD e )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_EnumGroupsInGroup
          ( LPDIRECTPLAY3 this, DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c, LPVOID d, DWORD e )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p,%p,%p,0x%08lx): stub", this, a, b, c, d, e );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetGroupConnectionSettings
          ( LPDIRECTPLAY3A this, DWORD a, DPID b, LPVOID c, LPDWORD d )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p,%p): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetGroupConnectionSettings
          ( LPDIRECTPLAY3 this, DWORD a, DPID b, LPVOID c, LPDWORD d )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p,%p): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_InitializeConnection
          ( LPDIRECTPLAY3A this, LPVOID a, DWORD b )
{
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_InitializeConnection
          ( LPDIRECTPLAY3 this, LPVOID a, DWORD b )
{
  FIXME( dplay,"(%p)->(%p,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SecureOpen
          ( LPDIRECTPLAY3A this, LPCDPSESSIONDESC2 a, DWORD b, LPCDPSECURITYDESC c, LPCDPCREDENTIALS d )
{
  FIXME( dplay,"(%p)->(%p,0x%08lx,%p,%p): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_SecureOpen
          ( LPDIRECTPLAY3 this, LPCDPSESSIONDESC2 a, DWORD b, LPCDPSECURITYDESC c, LPCDPCREDENTIALS d )
{   
  FIXME( dplay,"(%p)->(%p,0x%08lx,%p,%p): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SendChatMessage
          ( LPDIRECTPLAY3A this, DPID a, DPID b, DWORD c, LPDPCHAT d )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,0x%08lx,%p): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_SendChatMessage
          ( LPDIRECTPLAY3 this, DPID a, DPID b, DWORD c, LPDPCHAT d )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,0x%08lx,%p): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_SetGroupConnectionSettings
          ( LPDIRECTPLAY3A this, DWORD a, DPID b, LPDPLCONNECTION c )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_SetGroupConnectionSettings
          ( LPDIRECTPLAY3 this, DWORD a, DPID b, LPDPLCONNECTION c )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p): stub", this, a, b, c );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_StartSession
          ( LPDIRECTPLAY3A this, DWORD a, DPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_StartSession
          ( LPDIRECTPLAY3 this, DWORD a, DPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx): stub", this, a, b );
  return DP_OK;
}
 
HRESULT WINAPI DirectPlay3A_GetGroupFlags
          ( LPDIRECTPLAY3A this, DPID a, LPDWORD b )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetGroupFlags
          ( LPDIRECTPLAY3 this, DPID a, LPDWORD b )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetGroupParent
          ( LPDIRECTPLAY3A this, DPID a, LPDPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetGroupParent
          ( LPDIRECTPLAY3 this, DPID a, LPDPID b )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerAccount
          ( LPDIRECTPLAY3A this, DPID a, DWORD b, LPVOID c, LPDWORD d )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p,%p): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetPlayerAccount
          ( LPDIRECTPLAY3 this, DPID a, DWORD b, LPVOID c, LPDWORD d )
{
  FIXME( dplay,"(%p)->(0x%08lx,0x%08lx,%p,%p): stub", this, a, b, c, d );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3A_GetPlayerFlags
          ( LPDIRECTPLAY3A this, DPID a, LPDWORD b )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", this, a, b );
  return DP_OK;
}

HRESULT WINAPI DirectPlay3W_GetPlayerFlags
          ( LPDIRECTPLAY3 this, DPID a, LPDWORD b )
{
  FIXME( dplay,"(%p)->(0x%08lx,%p): stub", this, a, b );
  return DP_OK;
}

static struct tagLPDIRECTPLAY2_VTABLE directPlay2WVT = {
  DirectPlay2W_QueryInterface,
  (void*)DirectPlay3W_AddRef,
  (void*)DirectPlay3W_Release,
  (void*)DirectPlay3W_AddPlayerToGroup,
  (void*)DirectPlay3W_Close,
  (void*)DirectPlay3W_CreateGroup,
  (void*)DirectPlay3W_CreatePlayer,
  (void*)DirectPlay3W_DeletePlayerFromGroup,
  (void*)DirectPlay3W_DestroyGroup,
  (void*)DirectPlay3W_DestroyPlayer,
  (void*)DirectPlay3W_EnumGroupPlayers,
  (void*)DirectPlay3W_EnumGroups,
  (void*)DirectPlay3W_EnumPlayers,
  (void*)DirectPlay3W_EnumSessions,
  (void*)DirectPlay3W_GetCaps,
  (void*)DirectPlay3W_GetGroupData,
  (void*)DirectPlay3W_GetGroupName,
  (void*)DirectPlay3W_GetMessageCount,
  (void*)DirectPlay3W_GetPlayerAddress,
  (void*)DirectPlay3W_GetPlayerCaps,
  (void*)DirectPlay3W_GetPlayerData,
  (void*)DirectPlay3W_GetPlayerName,
  (void*)DirectPlay3W_GetSessionDesc,
  (void*)DirectPlay3W_Initialize,
  (void*)DirectPlay3W_Open,
  (void*)DirectPlay3W_Receive,
  (void*)DirectPlay3W_Send,
  (void*)DirectPlay3W_SetGroupData,
  (void*)DirectPlay3W_SetGroupName,
  (void*)DirectPlay3W_SetPlayerData,
  (void*)DirectPlay3W_SetPlayerName,
  (void*)DirectPlay3W_SetSessionDesc
};

static struct tagLPDIRECTPLAY2_VTABLE directPlay2AVT = {
  DirectPlay2A_QueryInterface,
  (void*)DirectPlay3W_AddRef,
  (void*)DirectPlay3A_Release,
  (void*)DirectPlay3A_AddPlayerToGroup,
  (void*)DirectPlay3A_Close,
  (void*)DirectPlay3A_CreateGroup,
  (void*)DirectPlay3A_CreatePlayer,
  (void*)DirectPlay3A_DeletePlayerFromGroup,
  (void*)DirectPlay3A_DestroyGroup,
  (void*)DirectPlay3A_DestroyPlayer,
  (void*)DirectPlay3A_EnumGroupPlayers,
  (void*)DirectPlay3A_EnumGroups,
  (void*)DirectPlay3A_EnumPlayers,
  (void*)DirectPlay3A_EnumSessions,
  (void*)DirectPlay3A_GetCaps,
  (void*)DirectPlay3A_GetGroupData,
  (void*)DirectPlay3A_GetGroupName,
  (void*)DirectPlay3A_GetMessageCount,
  (void*)DirectPlay3A_GetPlayerAddress,
  (void*)DirectPlay3A_GetPlayerCaps,
  (void*)DirectPlay3A_GetPlayerData,
  (void*)DirectPlay3A_GetPlayerName,
  (void*)DirectPlay3A_GetSessionDesc,
  (void*)DirectPlay3A_Initialize,
  (void*)DirectPlay3A_Open,
  (void*)DirectPlay3A_Receive,
  (void*)DirectPlay3A_Send,
  (void*)DirectPlay3A_SetGroupData,
  (void*)DirectPlay3A_SetGroupName,
  (void*)DirectPlay3A_SetPlayerData,
  (void*)DirectPlay3A_SetPlayerName,
  (void*)DirectPlay3A_SetSessionDesc
};

static struct tagLPDIRECTPLAY3_VTABLE directPlay3AVT = {
  DirectPlay3A_QueryInterface,
  (void*)DirectPlay3W_AddRef,
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

static struct tagLPDIRECTPLAY3_VTABLE directPlay3WVT = {
  DirectPlay3W_QueryInterface,
  DirectPlay3W_AddRef,
  DirectPlay3W_Release,
  DirectPlay3W_AddPlayerToGroup,
  DirectPlay3W_Close,
  DirectPlay3W_CreateGroup,
  DirectPlay3W_CreatePlayer,
  DirectPlay3W_DeletePlayerFromGroup,
  DirectPlay3W_DestroyGroup,
  DirectPlay3W_DestroyPlayer,
  DirectPlay3W_EnumGroupPlayers,
  DirectPlay3W_EnumGroups,
  DirectPlay3W_EnumPlayers,
  DirectPlay3W_EnumSessions,
  DirectPlay3W_GetCaps,
  DirectPlay3W_GetGroupData,
  DirectPlay3W_GetGroupName,
  DirectPlay3W_GetMessageCount,
  DirectPlay3W_GetPlayerAddress,
  DirectPlay3W_GetPlayerCaps,
  DirectPlay3W_GetPlayerData,
  DirectPlay3W_GetPlayerName,
  DirectPlay3W_GetSessionDesc,
  DirectPlay3W_Initialize,
  DirectPlay3W_Open,
  DirectPlay3W_Receive,
  DirectPlay3W_Send,
  DirectPlay3W_SetGroupData,
  DirectPlay3W_SetGroupName,
  DirectPlay3W_SetPlayerData,
  DirectPlay3W_SetPlayerName,
  DirectPlay3W_SetSessionDesc,

  DirectPlay3W_AddGroupToGroup,
  DirectPlay3W_CreateGroupInGroup,
  DirectPlay3W_DeleteGroupFromGroup,
  DirectPlay3W_EnumConnections,
  DirectPlay3W_EnumGroupsInGroup,
  DirectPlay3W_GetGroupConnectionSettings,
  DirectPlay3W_InitializeConnection,
  DirectPlay3W_SecureOpen,
  DirectPlay3W_SendChatMessage,
  DirectPlay3W_SetGroupConnectionSettings,
  DirectPlay3W_StartSession,
  DirectPlay3W_GetGroupFlags,
  DirectPlay3W_GetGroupParent,
  DirectPlay3W_GetPlayerAccount,
  DirectPlay3W_GetPlayerFlags
};

