/* Direct Play 3 and Direct Play Lobby 1 Implementation
 * Presently only the Lobby skeleton is implemented.                    
 *
 * Copyright 1998 - Peter Hunnisett
 *
 */
#include "windows.h"
#include "heap.h"
#include "wintypes.h"
#include "winerror.h"
#include "interfaces.h"
#include "mmsystem.h"
#include "dplay.h"
#include "debug.h"
#include "winnt.h"
#include "winreg.h"


static HRESULT WINAPI IDirectPlayLobby_QueryInterface
( LPDIRECTPLAYLOBBY2 this,
  REFIID riid,
  LPVOID* ppvObj )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static ULONG WINAPI IDirectPlayLobby_AddRef
( LPDIRECTPLAYLOBBY2 this )
{
  ++(this->ref);
  TRACE( dplay," ref count now %ld\n", this->ref );
  return (this->ref);
}

static ULONG WINAPI IDirectPlayLobby_Release
( LPDIRECTPLAYLOBBY2 this )
{
  this->ref--;

  TRACE( dplay, " ref count now %ld\n", this->ref );
  
  /* Deallocate if this is the last reference to the object */
  if( !(this->ref) )
  {
    HeapFree( GetProcessHeap(), 0, this ); 
    return 0;
  }

  return this->ref;
}

/********************************************************************
 * 
 * Connects an application to the session specified by the DPLCONNECTION
 * structure currently stored with the DirectPlayLobby object.
 *
 * Returns a IDirectPlay2 or IDirectPlay2A interface.
 *
 */
static HRESULT WINAPI IDirectPlayLobby_Connect
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  LPDIRECTPLAY2* lplpDP,
  IUnknown* pUnk)
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
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
static HRESULT WINAPI IDirectPlayLobby_CreateAddress
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
}

/********************************************************************
 *
 * Parses out chunks from the DirectPlay Address buffer by calling the
 * given callback function, with lpContext, for each of the chunks.
 *
 */
static HRESULT WINAPI IDirectPlayLobby_EnumAddress
( LPDIRECTPLAYLOBBY2 this,
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
static HRESULT WINAPI IDirectPlayLobby_EnumAddressTypes
( LPDIRECTPLAYLOBBY2 this,
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
static HRESULT WINAPI IDirectPlayLobby_EnumLocalApplications
( LPDIRECTPLAYLOBBY2 this,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK a,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME( dplay, ":stub\n");
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
static HRESULT WINAPI IDirectPlayLobby_GetConnectionSettings
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwAppID, 
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  LPDPLCONNECTION lpConnectionSettings;

  FIXME( dplay, ":semi stub %p %08lx %p %p \n", this, dwAppID, lpData, lpdwDataSize );

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

  /* Ok we assume that we've been given a buffer that is large enough for our needs */
  lpConnectionSettings = ( LPDPLCONNECTION ) lpData; 

  /* Fill in the fields */

  return DPERR_NOTLOBBIED;
}

/********************************************************************
 *
 * Retrieves the message sent between a lobby client and a DirectPlay 
 * application. All messages are queued until received.
 *
 */
static HRESULT WINAPI IDirectPlayLobby_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  FIXME( dplay, ":stub %p %08lx %08lx %p %p %p\n", this, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Starts an application and passes to it all the information to
 * connect to a session.
 *
 */
static HRESULT WINAPI IDirectPlayLobby_RunApplication
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE32 hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Sends a message between the application and the lobby client.
 * All messages are queued until received.
 *
 */
static HRESULT WINAPI IDirectPlayLobby_SendLobbyMessage
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Modifies the DPLCONNECTION structure to contain all information
 * needed to start and connect an application.
 *
 */
static HRESULT WINAPI IDirectPlayLobby_SetConnectionSettings
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  FIXME( dplay, ": this=%p, dwFlags=%08lx, dwAppId=%08lx, lpConn=%p: stub\n",
         this, dwFlags, dwAppID, lpConn );

  /* Paramater check */
  if( dwFlags || !this || !lpConn )
  {
    return DPERR_INVALIDPARAMS;
  }

  if( !( lpConn->dwSize == sizeof(DPLCONNECTION) ) )
  {
    /* Is this the right return code? */
    return DPERR_INVALIDPARAMS;
  }

  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Registers an event that will be set when a lobby message is received.
 *
 */
static HRESULT WINAPI IDirectPlayLobby_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY2 this,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE32 hReceiveEvent )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby_CreateCompoundAddress
( LPDIRECTPLAYLOBBY2 this,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;
}

/* Direct Play Lobby 2 Virtual Table for methods */
static struct tagLPDIRECTPLAYLOBBY2_VTABLE lobby2VT = {
  IDirectPlayLobby_QueryInterface,
  IDirectPlayLobby_AddRef, 
  IDirectPlayLobby_Release,
  IDirectPlayLobby_Connect,
  IDirectPlayLobby_CreateAddress,
  IDirectPlayLobby_EnumAddress,
  IDirectPlayLobby_EnumAddressTypes,
  IDirectPlayLobby_EnumLocalApplications,
  IDirectPlayLobby_GetConnectionSettings,
  IDirectPlayLobby_ReceiveLobbyMessage,
  IDirectPlayLobby_RunApplication,
  IDirectPlayLobby_SendLobbyMessage,
  IDirectPlayLobby_SetConnectionSettings,
  IDirectPlayLobby_SetLobbyMessageEvent,
  IDirectPlayLobby_CreateCompoundAddress
};

/***************************************************************************
 *  DirectPlayLobbyCreateA   (DPLAYX.4)
 *
 */
HRESULT WINAPI DirectPlayLobbyCreateA( LPGUID lpGUIDDSP,
                                       LPDIRECTPLAYLOBBY2A *lplpDPL,
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

  *lplpDPL = (LPDIRECTPLAYLOBBY2A)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                            sizeof( IDirectPlayLobby2A ) );

  if( ! (*lplpDPL) )
  {
     return DPERR_OUTOFMEMORY;
  }

  (*lplpDPL)->lpvtbl = &lobby2VT;
  (*lplpDPL)->ref    = 1;

  /* Still some stuff to do here */

  return DP_OK;
}

/***************************************************************************
 *  DirectPlayLobbyCreateW   (DPLAYX.5)
 *
 */
HRESULT WINAPI DirectPlayLobbyCreateW( LPGUID lpGUIDDSP, 
                                       LPDIRECTPLAYLOBBY2 *lplpDPL,
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

  *lplpDPL = (LPDIRECTPLAYLOBBY2)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           sizeof( IDirectPlayLobby2 ) );

  if( !*lplpDPL)
  {
     return DPERR_OUTOFMEMORY;
  }

  (*lplpDPL)->lpvtbl = &lobby2VT;
  (*lplpDPL)->ref    = 1;

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
( LPGUID lpGUID, LPDIRECTPLAY *lplpDP, IUnknown *pUnk)
{

  FIXME( dplay, ":stub\n");
  return DPERR_OUTOFMEMORY;

}
