/* Direct Play 3 and Direct Play Lobby 2 Implementation
 *
 * Copyright 1998 - Peter Hunnisett
 *
 * <presently under construction - contact hunnise@nortel.ca>
 *
 */
#include "interfaces.h"
#include "heap.h"
#include "winerror.h"
#include "debug.h"
#include "winnt.h"
#include "winreg.h"
#include "compobj.h"
#include "dplay.h"

#include "thread.h"
#include <sys/queue.h>

#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

struct IDirectPlayLobby {
    LPDIRECTPLAYLOBBY_VTABLE lpVtbl;
    ULONG                    ref;
    LPDPLCONNECTION          lpSession;
};

struct IDirectPlayLobby2 {
    LPDIRECTPLAYLOBBY2_VTABLE lpVtbl;
    ULONG                     ref;
    LPDPLCONNECTION           lpSession;
};


/* Forward declarations of virtual tables */
static DIRECTPLAYLOBBY_VTABLE  directPlayLobbyAVT;
static DIRECTPLAYLOBBY_VTABLE  directPlayLobbyWVT;
static DIRECTPLAYLOBBY2_VTABLE directPlayLobby2AVT;
static DIRECTPLAYLOBBY2_VTABLE directPlayLobby2WVT;

struct IDirectPlay2 {
  LPDIRECTPLAY2_VTABLE lpVtbl;
  ULONG                ref;
};

struct IDirectPlay3 {
  LPDIRECTPLAY3_VTABLE lpVtbl;
  ULONG                ref;
};


static DIRECTPLAY2_VTABLE directPlay2AVT;
static DIRECTPLAY2_VTABLE directPlay2WVT;
static DIRECTPLAY3_VTABLE directPlay3AVT;
static DIRECTPLAY3_VTABLE directPlay3WVT;

/* Routine to delete the entire DPLCONNECTION tree. Works for both unicode and ascii. */
void deleteDPConnection( LPDPLCONNECTION* ptrToDelete )
{
 
  /* This is most definitely wrong. We're not even keeping dwCurrentPlayers over this */
   LPDPLCONNECTION toDelete = *ptrToDelete;

   FIXME( dplay, "incomplete.\n" );

   if( !toDelete )
     return;
    
   /* Clear out DPSESSIONDESC2 */
   if( toDelete->lpSessionDesc )
   {
     if( toDelete->lpSessionDesc->sess.lpszSessionName )
       HeapFree( GetProcessHeap(), 0, toDelete->lpSessionDesc->sess.lpszSessionName );

     if( toDelete->lpSessionDesc->pass.lpszPassword ) 
       HeapFree( GetProcessHeap(), 0, toDelete->lpSessionDesc->pass.lpszPassword ); 

     if( toDelete->lpSessionDesc );  
       HeapFree( GetProcessHeap(), 0, toDelete->lpSessionDesc ); 
   }

   /* Clear out LPDPNAME */
   if( toDelete->lpPlayerName )
   { 
     if( toDelete->lpPlayerName->psn.lpszShortName )
       HeapFree( GetProcessHeap(), 0, toDelete->lpPlayerName->psn.lpszShortName );

     if( toDelete->lpPlayerName->pln.lpszLongName ) 
       HeapFree( GetProcessHeap(), 0, toDelete->lpPlayerName->pln.lpszLongName );

     if( toDelete->lpPlayerName ) 
       HeapFree( GetProcessHeap(), 0, toDelete->lpPlayerName );
   }

   /* Clear out lpAddress. TO DO...Once we actually copy it. */

   /* Clear out DPLCONNECTION */
   HeapFree( GetProcessHeap(), 0, toDelete );

   toDelete = NULL;
}


#if 0
/* Routine which copies and allocates all the store required for the DPLCONNECTION struct. */
void rebuildDPConnectionW( LPDPLCONNECTION dest, LPDPLCONNECTION src )
{
 
  /* Need to delete everything that already exists first */
  FIXME( dplay, "function is incomplete.\n" ); 

  if( !src )
  {
    /* Nothing to copy...hmmm...*/
    ERR( dplay, "nothing to copy\n" );
    return;
  }

  /* Copy DPLCONNECTION struct. If dest isn't NULL then we have a DPLCONNECTION
     struct but that's it
   */
  if( dest == NULL )
  {
    dest = HeapAlloc( GetProcessHeap(), 0, sizeof( *src ) );
  }
  memcpy( dest, src, sizeof( *src ) );

  /* Copy LPDPSESSIONDESC2 struct */
  if( src->lpSessionDesc )
  {
    dest->lpSessionDesc = HeapAlloc( GetProcessHeap(), 0,
                                     sizeof( *(src->lpSessionDesc) ) );

    memcpy( dest->lpSessionDesc, src->lpSessionDesc,
            sizeof( *(src->lpSessionDesc) ) ); 

    if( src->lpSessionDesc )
    { 
      /* Hmmm...do we have to assume the system heap? */ 
      dest->lpSessionDesc->sess.lpszSessionName = HEAP_strdupW( GetProcessHeap(), 0,
                                                                src->lpSessionDesc->sess.lpszSessionName );
    }

    if( src->lpSessionDesc->pass.lpszPassword )
    {
      dest->lpSessionDesc->pass.lpszPassword = HEAP_strdupW( GetProcessHeap(), 0,
                                                             src->lpSessionDesc->pass.lpszPassword );
    }
    dest->lpSessionDesc->dwReserved1 = src->lpSessionDesc->dwReserved2 = 0;
  }

  /* Copy DPNAME struct */	
  if( src->lpPlayerName )
  {
    dest->lpPlayerName = HeapAlloc( GetProcessHeap(), 0, sizeof( *(src->lpPlayerName) ) ); 
    memcpy( dest->lpPlayerName, src->lpPlayerName, sizeof( *(src->lpPlayerName) ) ); 

    if( src->lpPlayerName->psn.lpszShortName )
    { 
      dest->lpPlayerName->psn.lpszShortName = HEAP_strdupW( GetProcessHeap(), 0,
                                                            src->lpPlayerName->psn.lpszShortName );
    }
 
    if( src->lpPlayerName->pln.lpszLongName )
    {
      dest->lpPlayerName->pln.lpszLongName = HEAP_strdupW( GetProcessHeap(), 0,
                                                           src->lpPlayerName->pln.lpszLongName );
    }
  }

  /* Copy Address of Service Provider -TBD */
  if( src->lpAddress ) 
  {
    /* What do we do here? */ 
  } 

}

#endif

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
static HRESULT WINAPI IDirectPlayLobbyA_QueryInterface
( LPDIRECTPLAYLOBBYA this,
  REFIID riid,
  LPVOID* ppvObj )
{
  return DPERR_OUTOFMEMORY; 
}

static HRESULT WINAPI IDirectPlayLobbyW_QueryInterface
( LPDIRECTPLAYLOBBY this,
  REFIID riid,
  LPVOID* ppvObj )
{
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby2A_QueryInterface
( LPDIRECTPLAYLOBBY2A this,
  REFIID riid,
  LPVOID* ppvObj )
{
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
  /* They're requesting a unicode version of the interface */
  else if( IsEqualGUID( &IID_IDirectPlayLobby2, riid ) )
  {
     LPDIRECTPLAYLOBBY2 lpDpL = (LPDIRECTPLAYLOBBY2)(*ppvObj);

     lpDpL = (LPDIRECTPLAYLOBBY2)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                             sizeof( IDirectPlayLobby2 ) );

    if( !lpDpL )
    {
      return E_NOINTERFACE;
    }

    lpDpL->lpVtbl = &directPlayLobby2WVT;
    lpDpL->ref    = 1;

    return S_OK;
  }

  /* Unexpected interface request! */ 
  *ppvObj = NULL;
  return E_NOINTERFACE; 
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
  else if( IsEqualGUID( &IID_IDirectPlayLobby2A, riid ) )
  {
     LPDIRECTPLAYLOBBY2A lpDpL = (LPDIRECTPLAYLOBBY2A)(*ppvObj);

     lpDpL = (LPDIRECTPLAYLOBBY2A)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                             sizeof( IDirectPlayLobby2A ) );

    if( !lpDpL )
    {
      return E_NOINTERFACE;
    }

    lpDpL->lpVtbl = &directPlayLobby2AVT;
    lpDpL->ref    = 1;

    return S_OK;
  }

  /* Unexpected interface request! */
  *ppvObj = NULL;
  return E_NOINTERFACE;

};

/* 
 * Simple procedure. Just increment the reference count to this
 * structure and return the new reference count.
 */
static ULONG WINAPI IDirectPlayLobbyA_AddRef
( LPDIRECTPLAYLOBBYA this )
{
  ++(this->ref);
  TRACE( dplay,"ref count now %lu\n", this->ref );
  return (this->ref);
}
static ULONG WINAPI IDirectPlayLobbyW_AddRef
( LPDIRECTPLAYLOBBY this )
{
  return IDirectPlayLobbyA_AddRef( (LPDIRECTPLAYLOBBY) this );
}

static ULONG WINAPI IDirectPlayLobby2A_AddRef
( LPDIRECTPLAYLOBBY2A this )
{
  return IDirectPlayLobbyA_AddRef( (LPDIRECTPLAYLOBBY) this );
};

static ULONG WINAPI IDirectPlayLobby2W_AddRef
( LPDIRECTPLAYLOBBY2 this )
{
  return IDirectPlayLobbyA_AddRef( (LPDIRECTPLAYLOBBY) this );
};


/*
 * Simple COM procedure. Decrease the reference count to this object.
 * If the object no longer has any reference counts, free up the associated
 * memory.
 */
static ULONG WINAPI IDirectPlayLobbyA_Release
( LPDIRECTPLAYLOBBYA this )
{
  TRACE( dplay, "ref count decremeneted from %lu\n", this->ref );

  this->ref--;

  /* Deallocate if this is the last reference to the object */
  if( !(this->ref) )
  {
    deleteDPConnection( &(this->lpSession) );
    HeapFree( GetProcessHeap(), 0, this );
    return S_OK;
  }

  return this->ref;

}
static ULONG WINAPI IDirectPlayLobbyW_Release
( LPDIRECTPLAYLOBBY this )
{
  return IDirectPlayLobbyA_Release( (LPDIRECTPLAYLOBBYA) this );
}
static ULONG WINAPI IDirectPlayLobby2A_Release
( LPDIRECTPLAYLOBBY2A this )
{
  return IDirectPlayLobbyA_Release( (LPDIRECTPLAYLOBBYA) this );
};

static ULONG WINAPI IDirectPlayLobby2W_Release
( LPDIRECTPLAYLOBBY2 this )
{
  return IDirectPlayLobbyA_Release( (LPDIRECTPLAYLOBBYA) this );
};


/********************************************************************
 * 
 * Connects an application to the session specified by the DPLCONNECTION
 * structure currently stored with the DirectPlayLobby object.
 *
 * Returns a IDirectPlay interface.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyA_Connect
( LPDIRECTPLAYLOBBYA this,
  DWORD dwFlags,
  LPDIRECTPLAY* lplpDP,
  IUnknown* pUnk)
{
  FIXME( dplay, ": dwFlags=%08lx %p %p stub\n", dwFlags, lplpDP, pUnk );
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2A_Connect
( LPDIRECTPLAYLOBBY2A this,
  DWORD dwFlags,
  LPDIRECTPLAY* lplpDP,
  IUnknown* pUnk)
{
  return IDirectPlayLobbyA_Connect( (LPDIRECTPLAYLOBBYA)this, dwFlags, lplpDP, pUnk );
};

static HRESULT WINAPI IDirectPlayLobbyW_Connect
( LPDIRECTPLAYLOBBY this,
  DWORD dwFlags,
  LPDIRECTPLAY* lplpDP,
  IUnknown* pUnk)
{
  LPDIRECTPLAY2A directPlay2A;
  LPDIRECTPLAY2  directPlay2W;
  HRESULT        createRC;

  FIXME( dplay, ": dwFlags=%08lx %p %p stub\n", dwFlags, lplpDP, pUnk );

#if 0

  /* See dpbuild_4301.txt */
  /* Create the direct play 2 W interface */
  if( ( ( createRC = DirectPlayCreate( NULL, &directPlay2A, pUnk ) ) != DP_OK ) ||
      ( ( createRC = directPlay2A->lpVtbl->fnQueryInterface
           ( directPlay2A, IID_IDirectPlay2, &directPlay2W ) ) != DP_OK )
    )
  {
     ERR( dplay, "error creating Direct Play 2 (W) interface. Return Code = %d.\n", createRC );
     return createRC;
  }

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

static HRESULT WINAPI IDirectPlayLobby2W_Connect
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  LPDIRECTPLAY* lplpDP,
  IUnknown* pUnk)
{
  return IDirectPlayLobbyW_Connect( (LPDIRECTPLAYLOBBY)this, dwFlags, lplpDP, pUnk ); 
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
static HRESULT WINAPI IDirectPlayLobbyA_CreateAddress
( LPDIRECTPLAYLOBBY this,
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

static HRESULT WINAPI IDirectPlayLobby2A_CreateAddress
( LPDIRECTPLAYLOBBY2A this,
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData, 
  DWORD dwDataSize,
  LPVOID lpAddress, 
  LPDWORD lpdwAddressSize )
{
  return IDirectPlayLobbyA_CreateAddress( (LPDIRECTPLAYLOBBY)this, guidSP, guidDataType,
                                           lpData, dwDataSize, lpAddress, lpdwAddressSize ); 
};

static HRESULT WINAPI IDirectPlayLobbyW_CreateAddress
( LPDIRECTPLAYLOBBY this,
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
  return IDirectPlayLobbyW_CreateAddress( (LPDIRECTPLAYLOBBY)this, guidSP, guidDataType, 
                                          lpData, dwDataSize, lpAddress, lpdwAddressSize );
};


/********************************************************************
 *
 * Parses out chunks from the DirectPlay Address buffer by calling the
 * given callback function, with lpContext, for each of the chunks.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyA_EnumAddress
( LPDIRECTPLAYLOBBYA this,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2A_EnumAddress
( LPDIRECTPLAYLOBBY2A this,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback, 
  LPCVOID lpAddress,
  DWORD dwAddressSize, 
  LPVOID lpContext )
{
  return IDirectPlayLobbyA_EnumAddress( (LPDIRECTPLAYLOBBYA)this, lpEnumAddressCallback,
                                        lpAddress, dwAddressSize, lpContext );
};

static HRESULT WINAPI IDirectPlayLobbyW_EnumAddress
( LPDIRECTPLAYLOBBY this,
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
  return IDirectPlayLobbyW_EnumAddress( (LPDIRECTPLAYLOBBY)this, lpEnumAddressCallback,
                                        lpAddress, dwAddressSize, lpContext );
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

static HRESULT WINAPI IDirectPlayLobbyW_EnumAddressTypes
( LPDIRECTPLAYLOBBY this,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2W_EnumAddressTypes
( LPDIRECTPLAYLOBBY2 this,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP,
  LPVOID lpContext,
  DWORD dwFlags )
{
  return IDirectPlayLobbyW_EnumAddressTypes( (LPDIRECTPLAYLOBBY)this, lpEnumAddressTypeCallback,
                                             guidSP, lpContext, dwFlags );
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
  FIXME( dplay, ": semi stub %p %08lx %p %p \n", this, dwAppID, lpData, lpdwDataSize );
 
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
  if( ((LPDPLCONNECTION)lpData)->lpSessionDesc )
  {
    
  }
  memcpy( lpData, this->lpSession, sizeof( *(this->lpSession) ) );

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
  memcpy( lpData, this->lpSession, sizeof( *(this->lpSession) ) );

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
  HANDLE32 hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2A_RunApplication
( LPDIRECTPLAYLOBBY2A this,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE32 hReceiveEvent )
{
  return IDirectPlayLobbyA_RunApplication( (LPDIRECTPLAYLOBBYA)this, dwFlags,
                                           lpdwAppID, lpConn, hReceiveEvent );
};

static HRESULT WINAPI IDirectPlayLobbyW_RunApplication
( LPDIRECTPLAYLOBBY this,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE32 hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2W_RunApplication
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE32 hReceiveEvent )
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
  FIXME( dplay, ": this=%p, dwFlags=%08lx, dwAppId=%08lx, lpConn=%p: semi stub\n",
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

  /* Need to actually store the stuff here */

  return DPERR_OUTOFMEMORY;
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
  HANDLE32 hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2A_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY2A this,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE32 hReceiveEvent )
{
  return IDirectPlayLobbyA_SetLobbyMessageEvent( (LPDIRECTPLAYLOBBYA)this, dwFlags,
                                                 dwAppID, hReceiveEvent ); 
};

static HRESULT WINAPI IDirectPlayLobbyW_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY this,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE32 hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
};

static HRESULT WINAPI IDirectPlayLobby2W_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE32 hReceiveEvent )
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
static struct tagLPDIRECTPLAYLOBBY_VTABLE directPlayLobbyAVT = {
  IDirectPlayLobbyA_QueryInterface,
  IDirectPlayLobbyA_AddRef,
  IDirectPlayLobbyA_Release,
  IDirectPlayLobbyA_Connect,
  IDirectPlayLobbyA_CreateAddress,
  IDirectPlayLobbyA_EnumAddress,
  IDirectPlayLobbyA_EnumAddressTypes,
  IDirectPlayLobbyA_EnumLocalApplications,
  IDirectPlayLobbyA_GetConnectionSettings,
  IDirectPlayLobbyA_ReceiveLobbyMessage,
  IDirectPlayLobbyA_RunApplication,
  IDirectPlayLobbyA_SendLobbyMessage,
  IDirectPlayLobbyA_SetConnectionSettings,
  IDirectPlayLobbyA_SetLobbyMessageEvent
};

/* Direct Play Lobby 1 (unicode) Virtual Table for methods */
static struct tagLPDIRECTPLAYLOBBY_VTABLE directPlayLobbyWVT = {
  IDirectPlayLobbyW_QueryInterface,
  IDirectPlayLobbyW_AddRef,
  IDirectPlayLobbyW_Release,
  IDirectPlayLobbyW_Connect,
  IDirectPlayLobbyW_CreateAddress, 
  IDirectPlayLobbyW_EnumAddress,
  IDirectPlayLobbyW_EnumAddressTypes,
  IDirectPlayLobbyW_EnumLocalApplications,
  IDirectPlayLobbyW_GetConnectionSettings,
  IDirectPlayLobbyW_ReceiveLobbyMessage,
  IDirectPlayLobbyW_RunApplication,
  IDirectPlayLobbyW_SendLobbyMessage,
  IDirectPlayLobbyW_SetConnectionSettings,
  IDirectPlayLobbyW_SetLobbyMessageEvent
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

  /* Yes...really we should bre returning a lobby 1 object */
  *lplpDPL = (LPDIRECTPLAYLOBBYA)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                            sizeof( IDirectPlayLobbyA ) );

  if( ! (*lplpDPL) )
  {
     return DPERR_OUTOFMEMORY;
  }

  (*lplpDPL)->lpVtbl = &directPlayLobbyAVT;
  (*lplpDPL)->ref    = 1;

  (*lplpDPL)->lpSession = (LPDPLCONNECTION)HeapAlloc( GetProcessHeap(),
                                                      HEAP_ZERO_MEMORY,
                                                      sizeof( DPLCONNECTION ) );
  (*lplpDPL)->lpSession->dwSize = sizeof( DPLCONNECTION );

  (*lplpDPL)->lpSession->lpSessionDesc = (LPDPSESSIONDESC2)HeapAlloc(
          GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( DPSESSIONDESC2 ) );
  (*lplpDPL)->lpSession->lpSessionDesc->dwSize = sizeof( DPSESSIONDESC2 ); 

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


  (*lplpDPL)->lpSession = (LPDPLCONNECTION)HeapAlloc( GetProcessHeap(), 
                                                      HEAP_ZERO_MEMORY,
                                                      sizeof( DPLCONNECTION ) );
  (*lplpDPL)->lpSession->dwSize = sizeof( DPLCONNECTION );
  (*lplpDPL)->lpSession->lpSessionDesc = (LPDPSESSIONDESC2)HeapAlloc(
          GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( DPSESSIONDESC2 ) );
  (*lplpDPL)->lpSession->lpSessionDesc->dwSize = sizeof( DPSESSIONDESC2 ); 

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
  if( RegOpenKeyEx32A( HKEY_LOCAL_MACHINE, searchSubKey,
                       0, KEY_ENUMERATE_SUB_KEYS, &hkResult ) != ERROR_SUCCESS )
  {
    /* Hmmm. Does this mean that there are no service providers? */ 
    ERR(dplay, ": no service providers?\n");
    return DP_OK; 
  }

  /* Traverse all the service providers we have available */
  for( dwIndex=0;
       RegEnumKey32A( hkResult, dwIndex, subKeyName, sizeOfSubKeyName ) !=
         ERROR_NO_MORE_ITEMS;
       ++dwIndex )
  {
    HKEY     hkServiceProvider;
    GUID     serviceProviderGUID;
    DWORD    returnTypeGUID, returnTypeReserved1, sizeOfReturnBuffer=50;
    char     returnBuffer[51];
    DWORD    majVersionNum, minVersionNum;
    LPWSTR   lpWGUIDString; 

    TRACE( dplay, " this time through: %s\n", subKeyName );

    /* Get a handle for this particular service provider */
    if( RegOpenKeyEx32A( hkResult, subKeyName, 0, KEY_QUERY_VALUE,
                         &hkServiceProvider ) != ERROR_SUCCESS )
    {
      ERR( dplay, ": what the heck is going on?\n" );
      continue;
    }

    /* Get the GUID, Device major number and device minor number 
     * from the registry. 
     */
    if( RegQueryValueEx32A( hkServiceProvider, guidDataSubKey,
                            NULL, &returnTypeGUID, returnBuffer,
                            &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
      ERR( dplay, ": missing GUID registry data members\n" );
      continue; 
    }

    /* FIXME: Check return types to ensure we're interpreting data right */
    lpWGUIDString = HEAP_strdupAtoW( GetProcessHeap(), 0, returnBuffer );
    CLSIDFromString32( (LPCOLESTR32)lpWGUIDString, &serviceProviderGUID ); 
    HeapFree( GetProcessHeap(), 0, lpWGUIDString );

    sizeOfReturnBuffer = 50;
 
    if( RegQueryValueEx32A( hkServiceProvider, majVerDataSubKey,
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

  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;

  TRACE(dplay,"\n" );

  if( pUnk != NULL )
  {
    /* Hmmm...wonder what this means! */
    ERR(dplay, "What does a NULL here mean?\n" ); 
    return DPERR_OUTOFMEMORY;
  }

  *lplpDP = (LPDIRECTPLAY)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     sizeof( **lplpDP ) );

  if( !*lplpDP )
  {
     return DPERR_OUTOFMEMORY;
  }

  (*lplpDP)->lpVtbl = &directPlay2AVT;
  (*lplpDP)->ref    = 1;

  return DP_OK;

};

