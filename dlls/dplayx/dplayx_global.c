/* dplayx.dll global data implementation.
 *
 * Copyright 1999 - Peter Hunnisett
 *
 * <presently under construction - contact hunnise@nortelnetworks.com>
 *
 */

/* NOTE: Methods that begin with DPLAYX_ are used for dealing with 
 *       dplayx.dll data which is accessible from all processes.
 */ 

#include <stdio.h>
#include "debugtools.h"
#include "winbase.h"
#include "winerror.h"
#include "heap.h"  /* Really shouldn't be using those HEAP_strdupA interfaces should I? */

#include "dplayx_global.h"

DEFAULT_DEBUG_CHANNEL(dplay);

/* FIXME: Need to do all that fun other dll referencing type of stuff */
/* FIXME: Need to allocate a giant static area */

/* Static data for all processes */
static LPSTR lpszDplayxSemaName = "WINE_DPLAYX_SM";
static HANDLE hDplayxSema;



#define DPLAYX_AquireSemaphore()  TRACE( "Waiting for DPLAYX sema\n" ); \
                                  WaitForSingleObject( hDplayxSema, INFINITE ); TRACE( "Through wait\n" )
#define DPLAYX_ReleaseSemaphore() ReleaseSemaphore( hDplayxSema, 1, NULL ); \
                                  TRACE( "DPLAYX Sema released\n" ); /* FIXME: Is this correct? */


/* HACK for simple global data right now */ 
enum { numSupportedLobbies = 32 };
typedef struct tagDirectPlayLobbyData
{
  DWORD           dwConnFlags;
  DPSESSIONDESC2  sessionDesc;
  DPNAME          playerName;
  GUID            guidSP;
  LPVOID          lpAddress;
  DWORD           dwAddressSize;
  DWORD           dwAppID;
  HANDLE          hReceiveEvent;
  DWORD           dwAppLaunchedFromID;
} DirectPlayLobbyData, *lpDirectPlayLobbyData;

static DirectPlayLobbyData lobbyData[ numSupportedLobbies ];

/* Function prototypes */
BOOL  DPLAYX_IsAppIdLobbied( DWORD dwAppId, lpDirectPlayLobbyData* dplData );
void DPLAYX_InitializeLobbyDataEntry( lpDirectPlayLobbyData lpData );





/*************************************************************************** 
 * Called to initialize the global data. This will only be used on the 
 * loading of the dll 
 ***************************************************************************/ 
void DPLAYX_ConstructData(void)
{
  UINT i;

  TRACE( "DPLAYX dll loaded - construct called\n" );

  /* Create a semahopre to block access to DPLAYX global data structs 
     It starts unblocked, and allows up to 65000 users blocked on it. Seems reasonable
     for the time being */
  hDplayxSema = CreateSemaphoreA( NULL, 1, 65000, lpszDplayxSemaName ); 

  if( !hDplayxSema )
  {
    /* Really don't have any choice but to continue... */
    ERR( "Semaphore creation error 0x%08lx\n", GetLastError() );
  }

  /* Set all lobbies to be "empty" */ 
  for( i=0; i < numSupportedLobbies; i++ )
  {
    DPLAYX_InitializeLobbyDataEntry( &lobbyData[ i ] );
  }

}

/*************************************************************************** 
 * Called to destroy all global data. This will only be used on the 
 * unloading of the dll 
 ***************************************************************************/ 
void DPLAYX_DestructData(void)
{
  TRACE( "DPLAYX dll unloaded - destruct called\n" );

  /* delete the semaphore */
  CloseHandle( hDplayxSema );
}


void DPLAYX_InitializeLobbyDataEntry( lpDirectPlayLobbyData lpData )
{
  ZeroMemory( lpData, sizeof( *lpData ) );

  /* Set the handle to a better invalid value */
  lpData->hReceiveEvent = INVALID_HANDLE_VALUE;
}

/* NOTE: This must be called with the semaphore aquired. 
 * TRUE/FALSE with a pointer to it's data returned. Pointer data is
 * is only valid if TRUE is returned.
 */
BOOL  DPLAYX_IsAppIdLobbied( DWORD dwAppID, lpDirectPlayLobbyData* lplpDplData )
{
  INT i;

  *lplpDplData = NULL;

  if( dwAppID == 0 )
  {
    dwAppID = GetCurrentProcessId();
    TRACE( "Translated dwAppID == 0 into 0x%08lx\n", dwAppID );
  }

  for( i=0; i < numSupportedLobbies; i++ )
  {
    if( lobbyData[ i ].dwAppID == dwAppID )
    {
      /* This process is lobbied */ 
      *lplpDplData = &lobbyData[ i ];
      return TRUE;
    }
  }

  return FALSE;
}

/* Reserve a spot for the new appliction. TRUE means success and FALSE failure.  */
BOOL DPLAYX_CreateLobbyApplication( DWORD dwAppID, HANDLE hReceiveEvent )
{
  UINT i;

  /* 0 is the marker for unused application data slots */
  if( dwAppID == 0 )
  {
    return FALSE;
  } 

  DPLAYX_AquireSemaphore();

  /* Find an empty space in the list and insert the data */
  for( i=0; i < numSupportedLobbies; i++ )
  {
    if( lobbyData[ i ].dwAppID == 0 )
    {
      /* This process is now lobbied */
      lobbyData[ i ].dwAppID             = dwAppID;
      lobbyData[ i ].hReceiveEvent       = hReceiveEvent;
      lobbyData[ i ].dwAppLaunchedFromID = GetCurrentProcessId();

      DPLAYX_ReleaseSemaphore();
      return TRUE;
    }
  }

  DPLAYX_ReleaseSemaphore();
  return FALSE;
}

/* I'm not sure when I'm going to need this, but here it is */
BOOL DPLAYX_DestroyLobbyApplication( DWORD dwAppID ) 
{
  UINT i;

  DPLAYX_AquireSemaphore();

  /* Find an empty space in the list and insert the data */
  for( i=0; i < numSupportedLobbies; i++ )
  {
    if( lobbyData[ i ].dwAppID == dwAppID )
    {
      /* Mark this entry unused */
      DPLAYX_InitializeLobbyDataEntry( &lobbyData[ i ] );

      DPLAYX_ReleaseSemaphore();
      return TRUE;
    }
  }

  DPLAYX_ReleaseSemaphore();
  ERR( "Unable to find global entry for application\n" );
  return FALSE;
}

HRESULT DPLAYX_GetConnectionSettingsA
( DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  lpDirectPlayLobbyData lpDplData;
  LPDPLCONNECTION lpDplConnection;

  /* Verify buffer size */
  if ( ( lpData == NULL ) ||
       ( *lpdwDataSize < sizeof( DPLCONNECTION ) )
     )
  {
    *lpdwDataSize = sizeof( DPLCONNECTION );

    return DPERR_BUFFERTOOSMALL;
  }

  DPLAYX_AquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return DPERR_NOTLOBBIED;
  }

  /* Copy the information */
  lpDplConnection = (LPDPLCONNECTION)lpData;

  /* Copy everything we've got into here */
  /* Need to actually store the stuff here. Check if we've already allocated each field first. */
  lpDplConnection->dwSize = sizeof( DPLCONNECTION );
  lpDplConnection->dwFlags = lpDplData->dwConnFlags;

  /* Copy LPDPSESSIONDESC2 struct */
  lpDplConnection->lpSessionDesc = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( lpDplData->sessionDesc ) );
  memcpy( lpDplConnection->lpSessionDesc, &lpDplData->sessionDesc, sizeof( lpDplData->sessionDesc ) );

  if( lpDplData->sessionDesc.sess.lpszSessionNameA )
  {
    lpDplConnection->lpSessionDesc->sess.lpszSessionNameA =
      HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDplData->sessionDesc.sess.lpszSessionNameA );
  }

  if( lpDplData->sessionDesc.pass.lpszPasswordA )
  {
    lpDplConnection->lpSessionDesc->pass.lpszPasswordA =
      HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDplData->sessionDesc.pass.lpszPasswordA );
  }

  lpDplConnection->lpSessionDesc->dwReserved1 = lpDplData->sessionDesc.dwReserved1;
  lpDplConnection->lpSessionDesc->dwReserved2 = lpDplData->sessionDesc.dwReserved2;

  /* Copy DPNAME struct - seems to be optional - check for existance first */
  lpDplConnection->lpPlayerName = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( lpDplData->playerName ) );
  memcpy( lpDplConnection->lpPlayerName, &(lpDplData->playerName), sizeof( lpDplData->playerName ) );

  if( lpDplData->playerName.psn.lpszShortNameA )
  {
    lpDplConnection->lpPlayerName->psn.lpszShortNameA =
      HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDplData->playerName.psn.lpszShortNameA );
  }

  if( lpDplData->playerName.pln.lpszLongNameA )
  {
    lpDplConnection->lpPlayerName->pln.lpszLongNameA =
      HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDplData->playerName.pln.lpszLongNameA );
  }

  memcpy( &lpDplConnection->guidSP, &lpDplData->guidSP, sizeof( lpDplData->guidSP ) );

  lpDplConnection->lpAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDplData->dwAddressSize );
  memcpy( lpDplConnection->lpAddress, lpDplData->lpAddress, lpDplData->dwAddressSize );

  lpDplConnection->dwAddressSize = lpDplData->dwAddressSize;

  /* FIXME: Send a message - or only the first time? */

  DPLAYX_ReleaseSemaphore();

  return DP_OK;
}

HRESULT DPLAYX_GetConnectionSettingsW
( DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  lpDirectPlayLobbyData lpDplData;
  LPDPLCONNECTION lpDplConnection;

  /* Verify buffer size */
  if ( ( lpData == NULL ) ||
       ( *lpdwDataSize < sizeof( DPLCONNECTION ) )
     )
  {
    *lpdwDataSize = sizeof( DPLCONNECTION );

    return DPERR_BUFFERTOOSMALL;
  }

  DPLAYX_AquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return DPERR_NOTLOBBIED;
  }

  /* Copy the information */
  lpDplConnection = (LPDPLCONNECTION)lpData;

  /* Copy everything we've got into here */
  /* Need to actually store the stuff here. Check if we've already allocated each field first. */
  lpDplConnection->dwSize = sizeof( DPLCONNECTION );
  lpDplConnection->dwFlags = lpDplData->dwConnFlags;

  /* Copy LPDPSESSIONDESC2 struct */
  lpDplConnection->lpSessionDesc = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( lpDplData->sessionDesc ) );
  memcpy( lpDplConnection->lpSessionDesc, &lpDplData->sessionDesc, sizeof( lpDplData->sessionDesc ) );

  if( lpDplData->sessionDesc.sess.lpszSessionName )
  {
    lpDplConnection->lpSessionDesc->sess.lpszSessionName =
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDplData->sessionDesc.sess.lpszSessionName );
  }

  if( lpDplData->sessionDesc.pass.lpszPassword )
  {
    lpDplConnection->lpSessionDesc->pass.lpszPassword =
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDplData->sessionDesc.pass.lpszPassword );
  }

  lpDplConnection->lpSessionDesc->dwReserved1 = lpDplData->sessionDesc.dwReserved1;
  lpDplConnection->lpSessionDesc->dwReserved2 = lpDplData->sessionDesc.dwReserved2;

  /* Copy DPNAME struct - seems to be optional - check for existance first */
  lpDplConnection->lpPlayerName = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( lpDplData->playerName ) );
  memcpy( lpDplConnection->lpPlayerName, &(lpDplData->playerName), sizeof( lpDplData->playerName ) );

  if( lpDplData->playerName.psn.lpszShortName )
  {
    lpDplConnection->lpPlayerName->psn.lpszShortName =
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDplData->playerName.psn.lpszShortName );
  }

  if( lpDplData->playerName.pln.lpszLongName )
  {
    lpDplConnection->lpPlayerName->pln.lpszLongName =
      HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDplData->playerName.pln.lpszLongName );
  }

  memcpy( &lpDplConnection->guidSP, &lpDplData->guidSP, sizeof( lpDplData->guidSP ) );

  lpDplConnection->lpAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDplData->dwAddressSize );
  memcpy( lpDplConnection->lpAddress, lpDplData->lpAddress, lpDplData->dwAddressSize );

  lpDplConnection->dwAddressSize = lpDplData->dwAddressSize;

  /* FIXME: Send a message - or only the first time? */

  DPLAYX_ReleaseSemaphore();

  return DP_OK;
}


HRESULT DPLAYX_SetConnectionSettingsA
( DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  lpDirectPlayLobbyData lpDplData;

  /* Paramater check */
  if( dwFlags || !lpConn )
  {
    ERR("invalid parameters.\n");
    return DPERR_INVALIDPARAMS;
  }

  /* Store information */
  if(  lpConn->dwSize != sizeof(DPLCONNECTION) )
  {
    ERR(": old/new DPLCONNECTION type? Size=%08lx vs. expected=%ul bytes\n",
         lpConn->dwSize, sizeof( DPLCONNECTION ) );

    return DPERR_INVALIDPARAMS;
  }

  if( dwAppID == 0 )
  {
    dwAppID = GetCurrentProcessId();
  }

  DPLAYX_AquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    /* FIXME: Create a new entry for this dwAppID? */
    DPLAYX_ReleaseSemaphore();

    return DPERR_GENERIC;
  }

  /* Store information */
  if(  lpConn->dwSize != sizeof(DPLCONNECTION) )
  {
    DPLAYX_ReleaseSemaphore();

    ERR(": old/new DPLCONNECTION type? Size=%lu vs. expected=%u bytes\n",
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
    DPLAYX_ReleaseSemaphore();

    ERR("DPSESSIONDESC passed in? Size=%lu vs. expected=%u bytes\n",
         lpConn->lpSessionDesc->dwSize, sizeof( DPSESSIONDESC2 ) );

    return DPERR_INVALIDPARAMS;
  }

  /* FIXME: How do I store this stuff so that it can be read by all processes? Static area? What about strings? Ewww...large global shared */ 

  /* Need to actually store the stuff here. Check if we've already allocated each field first. */
  lpDplData->dwConnFlags = lpConn->dwFlags;

  /* Copy LPDPSESSIONDESC2 struct - this is required */
  memcpy( &lpDplData->sessionDesc, lpConn->lpSessionDesc, sizeof( *(lpConn->lpSessionDesc) ) );

  /* FIXME: Can this just be shorted? Does it handle the NULL case correctly? */
  if( lpConn->lpSessionDesc->sess.lpszSessionNameA )
    lpDplData->sessionDesc.sess.lpszSessionNameA = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->sess.lpszSessionNameA );
  else
    lpDplData->sessionDesc.sess.lpszSessionNameA = NULL;

  if( lpConn->lpSessionDesc->pass.lpszPasswordA )
    lpDplData->sessionDesc.pass.lpszPasswordA = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->pass.lpszPasswordA );
  else
    lpDplData->sessionDesc.pass.lpszPasswordA = NULL;

  lpDplData->sessionDesc.dwReserved1 = lpConn->lpSessionDesc->dwReserved1;
  lpDplData->sessionDesc.dwReserved2 = lpConn->lpSessionDesc->dwReserved2;

  /* Copy DPNAME struct - seems to be optional - check for existance first */
  if( lpConn->lpPlayerName )
  {
     memcpy( &lpDplData->playerName, lpConn->lpPlayerName, sizeof( *lpConn->lpPlayerName ) );

     if( lpConn->lpPlayerName->psn.lpszShortNameA )
       lpDplData->playerName.psn.lpszShortNameA = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->psn.lpszShortNameA );

     if( lpConn->lpPlayerName->pln.lpszLongNameA )
       lpDplData->playerName.pln.lpszLongNameA = HEAP_strdupA( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->pln.lpszLongNameA );

  }

  memcpy( &lpDplData->guidSP, &lpConn->guidSP, sizeof( lpConn->guidSP ) );

  lpDplData->lpAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->dwAddressSize );
  memcpy( lpDplData->lpAddress, lpConn->lpAddress, lpConn->dwAddressSize );

  lpDplData->dwAddressSize = lpConn->dwAddressSize;

  /* FIXME: Send a message - I think */

  DPLAYX_ReleaseSemaphore();

  return DP_OK;
}

HRESULT DPLAYX_SetConnectionSettingsW
( DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  lpDirectPlayLobbyData lpDplData;

  /* Paramater check */
  if( dwFlags || !lpConn )
  {
    ERR("invalid parameters.\n");
    return DPERR_INVALIDPARAMS;
  }

  /* Store information */
  if(  lpConn->dwSize != sizeof(DPLCONNECTION) )
  {
    ERR(": old/new DPLCONNECTION type? Size=%lu vs. expected=%u bytes\n",
         lpConn->dwSize, sizeof( DPLCONNECTION ) );

    return DPERR_INVALIDPARAMS;
  }

  if( dwAppID == 0 )
  {
    dwAppID = GetCurrentProcessId();
  }

  DPLAYX_AquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return DPERR_NOTLOBBIED;
  }

  /* Need to investigate the lpConn->lpSessionDesc to figure out
   * what type of session we need to join/create.
   */
  if(  (!lpConn->lpSessionDesc ) ||
       ( lpConn->lpSessionDesc->dwSize != sizeof( DPSESSIONDESC2 ) )
    )
  {
    DPLAYX_ReleaseSemaphore();

    ERR("DPSESSIONDESC passed in? Size=%lu vs. expected=%u bytes\n",
         lpConn->lpSessionDesc->dwSize, sizeof( DPSESSIONDESC2 ) );

    return DPERR_INVALIDPARAMS;
  }

  /* FIXME: How do I store this stuff so that it can be read by all processes? Static area? What about strings? Ewww...large global shared */ 

  /* Need to actually store the stuff here. Check if we've already allocated each field first. */
  lpDplData->dwConnFlags = lpConn->dwFlags;

  /* Copy LPDPSESSIONDESC2 struct - this is required */
  memcpy( &lpDplData->sessionDesc, lpConn->lpSessionDesc, sizeof( *(lpConn->lpSessionDesc) ) );

  /* FIXME: Can this just be shorted? Does it handle the NULL case correctly? */
  if( lpConn->lpSessionDesc->sess.lpszSessionName )
    lpDplData->sessionDesc.sess.lpszSessionName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->sess.lpszSessionName );
  else
    lpDplData->sessionDesc.sess.lpszSessionName = NULL;

  if( lpConn->lpSessionDesc->pass.lpszPassword )
    lpDplData->sessionDesc.pass.lpszPassword = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpSessionDesc->pass.lpszPassword );
  else
    lpDplData->sessionDesc.pass.lpszPassword = NULL;

  lpDplData->sessionDesc.dwReserved1 = lpConn->lpSessionDesc->dwReserved1;
  lpDplData->sessionDesc.dwReserved2 = lpConn->lpSessionDesc->dwReserved2;

  /* Copy DPNAME struct - seems to be optional - check for existance first */
  if( lpConn->lpPlayerName )
  {
     memcpy( &lpDplData->playerName, lpConn->lpPlayerName, sizeof( *lpConn->lpPlayerName ) );

     if( lpConn->lpPlayerName->psn.lpszShortName )
       lpDplData->playerName.psn.lpszShortName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->psn.lpszShortName );

     if( lpConn->lpPlayerName->pln.lpszLongName )
       lpDplData->playerName.pln.lpszLongName = HEAP_strdupW( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->lpPlayerName->pln.lpszLongName );

  }

  memcpy( &lpDplData->guidSP, &lpConn->guidSP, sizeof( lpConn->guidSP ) );

  lpDplData->lpAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, lpConn->dwAddressSize );
  memcpy( lpDplData->lpAddress, lpConn->lpAddress, lpConn->dwAddressSize );

  lpDplData->dwAddressSize = lpConn->dwAddressSize;

  /* FIXME: Send a message - I think */

  DPLAYX_ReleaseSemaphore();

  return DP_OK;
}

/* NOTE: This is potentially not thread safe. You are not guaranteed to end up 
         with the correct string printed in the case where the HRESULT is not
         known. You'll just get the last hr passed in printed. This can change
         over time if this method is used alot :) */
LPCSTR DPLAYX_HresultToString(HRESULT hr)
{
  static char szTempStr[12];

  switch (hr)
  {
    case DP_OK:  
      return "DP_OK";
    case DPERR_ALREADYINITIALIZED: 
      return "DPERR_ALREADYINITIALIZED";
    case DPERR_ACCESSDENIED: 
      return "DPERR_ACCESSDENIED";
    case DPERR_ACTIVEPLAYERS: 
      return "DPERR_ACTIVEPLAYERS";
    case DPERR_BUFFERTOOSMALL: 
      return "DPERR_BUFFERTOOSMALL";
    case DPERR_CANTADDPLAYER: 
      return "DPERR_CANTADDPLAYER";
    case DPERR_CANTCREATEGROUP: 
      return "DPERR_CANTCREATEGROUP";
    case DPERR_CANTCREATEPLAYER: 
      return "DPERR_CANTCREATEPLAYER";
    case DPERR_CANTCREATESESSION: 
      return "DPERR_CANTCREATESESSION";
    case DPERR_CAPSNOTAVAILABLEYET: 
      return "DPERR_CAPSNOTAVAILABLEYET";
    case DPERR_EXCEPTION: 
      return "DPERR_EXCEPTION";
    case DPERR_GENERIC: 
      return "DPERR_GENERIC";
    case DPERR_INVALIDFLAGS: 
      return "DPERR_INVALIDFLAGS";
    case DPERR_INVALIDOBJECT: 
      return "DPERR_INVALIDOBJECT";
    case DPERR_INVALIDPARAMS: 
      return "DPERR_INVALIDPARAMS";
    case DPERR_INVALIDPLAYER: 
      return "DPERR_INVALIDPLAYER";
    case DPERR_INVALIDGROUP: 
      return "DPERR_INVALIDGROUP";
    case DPERR_NOCAPS: 
      return "DPERR_NOCAPS";
    case DPERR_NOCONNECTION: 
      return "DPERR_NOCONNECTION";
    case DPERR_OUTOFMEMORY: 
      return "DPERR_OUTOFMEMORY";
    case DPERR_NOMESSAGES: 
      return "DPERR_NOMESSAGES";
    case DPERR_NONAMESERVERFOUND: 
      return "DPERR_NONAMESERVERFOUND";
    case DPERR_NOPLAYERS: 
      return "DPERR_NOPLAYERS";
    case DPERR_NOSESSIONS: 
      return "DPERR_NOSESSIONS";
/* This one isn't defined yet in WINE sources. I don't know the value
    case DPERR_PENDING: 
      return "DPERR_PENDING";
*/
    case DPERR_SENDTOOBIG: 
      return "DPERR_SENDTOOBIG";
    case DPERR_TIMEOUT: 
      return "DPERR_TIMEOUT";
    case DPERR_UNAVAILABLE: 
      return "DPERR_UNAVAILABLE";
    case DPERR_UNSUPPORTED: 
      return "DPERR_UNSUPPORTED";
    case DPERR_BUSY: 
      return "DPERR_BUSY";
    case DPERR_USERCANCEL: 
      return "DPERR_USERCANCEL";
    case DPERR_NOINTERFACE: 
      return "DPERR_NOINTERFACE";
    case DPERR_CANNOTCREATESERVER: 
      return "DPERR_CANNOTCREATESERVER";
    case DPERR_PLAYERLOST: 
      return "DPERR_PLAYERLOST";
    case DPERR_SESSIONLOST: 
      return "DPERR_SESSIONLOST";
    case DPERR_UNINITIALIZED: 
      return "DPERR_UNINITIALIZED";
    case DPERR_NONEWPLAYERS: 
      return "DPERR_NONEWPLAYERS";
    case DPERR_INVALIDPASSWORD: 
      return "DPERR_INVALIDPASSWORD";
    case DPERR_CONNECTING: 
      return "DPERR_CONNECTING";
    case DPERR_CONNECTIONLOST: 
      return "DPERR_CONNECTIONLOST";
    case DPERR_UNKNOWNMESSAGE:
      return "DPERR_UNKNOWNMESSAGE";
    case DPERR_CANCELFAILED: 
      return "DPERR_CANCELFAILED";
    case DPERR_INVALIDPRIORITY: 
      return "DPERR_INVALIDPRIORITY";
    case DPERR_NOTHANDLED: 
      return "DPERR_NOTHANDLED";
    case DPERR_CANCELLED: 
      return "DPERR_CANCELLED";
    case DPERR_ABORTED: 
      return "DPERR_ABORTED";
    case DPERR_BUFFERTOOLARGE: 
      return "DPERR_BUFFERTOOLARGE";
    case DPERR_CANTCREATEPROCESS: 
      return "DPERR_CANTCREATEPROCESS";
    case DPERR_APPNOTSTARTED: 
      return "DPERR_APPNOTSTARTED";
    case DPERR_INVALIDINTERFACE: 
      return "DPERR_INVALIDINTERFACE";
    case DPERR_NOSERVICEPROVIDER: 
      return "DPERR_NOSERVICEPROVIDER";
    case DPERR_UNKNOWNAPPLICATION: 
      return "DPERR_UNKNOWNAPPLICATION";
    case DPERR_NOTLOBBIED: 
      return "DPERR_NOTLOBBIED";
    case DPERR_SERVICEPROVIDERLOADED: 
      return "DPERR_SERVICEPROVIDERLOADED";
    case DPERR_ALREADYREGISTERED: 
      return "DPERR_ALREADYREGISTERED";
    case DPERR_NOTREGISTERED: 
      return "DPERR_NOTREGISTERED";
    case DPERR_AUTHENTICATIONFAILED: 
      return "DPERR_AUTHENTICATIONFAILED";
    case DPERR_CANTLOADSSPI: 
      return "DPERR_CANTLOADSSPI";
    case DPERR_ENCRYPTIONFAILED: 
      return "DPERR_ENCRYPTIONFAILED";
    case DPERR_SIGNFAILED: 
      return "DPERR_SIGNFAILED";
    case DPERR_CANTLOADSECURITYPACKAGE: 
      return "DPERR_CANTLOADSECURITYPACKAGE";
    case DPERR_ENCRYPTIONNOTSUPPORTED: 
      return "DPERR_ENCRYPTIONNOTSUPPORTED";
    case DPERR_CANTLOADCAPI: 
      return "DPERR_CANTLOADCAPI";
    case DPERR_NOTLOGGEDIN: 
      return "DPERR_NOTLOGGEDIN";
    case DPERR_LOGONDENIED: 
      return "DPERR_LOGONDENIED";
    default:
      /* For errors not in the list, return HRESULT as a string
         This part is not thread safe */
      WARN( "Unknown error 0x%08lx\n", hr );
      sprintf( szTempStr, "0x%08lx", hr );
      return szTempStr;
  }
}

