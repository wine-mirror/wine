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

#include "debugtools.h"
#include "winbase.h"
#include "winerror.h"
#include "heap.h"  /* Really shouldn't be using those HEAP_strdupA interfaces should I? */

#include "dplayx_global.h"

DEFAULT_DEBUG_CHANNEL(dplay)

/* FIXME: Need to do all that fun other dll referencing type of stuff */
/* FIXME: Need to allocate a giant static area - or a global data type thing for Get/Set */

/* Static data for all processes */
static LPSTR lpszDplayxSemaName = "DPLAYX_SM";
static HANDLE hDplayxSema;



#define DPLAYX_AquireSemaphore()  0L /* WaitForSingleObject( hDplayxSema, INFINITE? ) */
#define DPLAYX_ReleaseSemaphore() 0L /* ReleaseSemaphore( hDplayxSema, ..., ...  ) */


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
} DirectPlayLobbyData, *lpDirectPlayLobbyData;

static DirectPlayLobbyData lobbyData[ numSupportedLobbies ];

/* Function prototypes */
BOOL DPLAYX_AppIdLobbied( DWORD dwAppId, lpDirectPlayLobbyData* dplData );




/*************************************************************************** 
 * Called to initialize the global data. This will only be used on the 
 * loading of the dll 
 ***************************************************************************/ 
void DPLAYX_ConstructData(void)
{
  UINT i;

  TRACE( "DPLAYX dll loaded - construct called\n" );

  /* FIXME: This needs to be allocated shared */
  hDplayxSema = CreateSemaphoreA( NULL, 0, 1, lpszDplayxSemaName ); 

  if( !hDplayxSema )
  {
    /* Really don't have any choice but to continue... */
    ERR( "Semaphore creation error 0x%08lx\n", GetLastError() );
  }

  /* Set all lobbies to be "empty" */ 
  for( i=0; i < numSupportedLobbies; i++ )
  {
    lobbyData[ i ].dwAppID = 0;
  }

}

/*************************************************************************** 
 * Called to destroy all global data. This will only be used on the 
 * unloading of the dll 
 ***************************************************************************/ 
void DPLAYX_DestructData(void)
{
  /* Hmmm...what to call to delete the semaphore? Auto delete? */
  TRACE( "DPLAYX dll unloaded - destruct called\n" );
}


/* NOTE: This must be called with the semaphore aquired. 
 * TRUE/FALSE with a pointer to it's data returned
 */
BOOL DPLAYX_AppIdLobbied( DWORD dwAppID, lpDirectPlayLobbyData* lplpDplData )
{
  INT i;

  if( dwAppID == 0 )
  {
    dwAppID = GetCurrentProcessId();
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

#if !defined( WORKING_PROCESS_SUSPEND )
/* These two functions should not exist. We would normally create a process initially
   suspended when we RunApplication. This gives us time to actually store some data
   before the new process might try to read it. However, process suspension doesn't
   work yet and I'm too stupid to get it going. So, we'll just hack in fine fashion */
DWORD DPLAYX_AquireSemaphoreHack( void )
{
  return DPLAYX_AquireSemaphore();
}

DWORD DPLAYX_ReleaseSemaphoreHack( void )
{
  return DPLAYX_ReleaseSemaphore();
}

#endif




/* Reserve a spot for the new appliction. TRUE means success and FALSE failure.  */
BOOL DPLAYX_CreateLobbyApplication( DWORD dwAppID, HANDLE hReceiveEvent )
{
  INT  i;

  DPLAYX_AquireSemaphore();

  for( i=0; i < numSupportedLobbies; i++ )
  {
    if( lobbyData[ i ].dwAppID == 0 )
    {
      /* This process is now lobbied */
      lobbyData[ i ].dwAppID       = dwAppID;
      lobbyData[ i ].hReceiveEvent = hReceiveEvent;

      DPLAYX_ReleaseSemaphore();
      return TRUE;
    }
  }

  DPLAYX_ReleaseSemaphore();
  return FALSE;
}

HRESULT DPLAYX_GetConnectionSettingsA
( DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  lpDirectPlayLobbyData lpDplData;
  LPDPLCONNECTION lpDplConnection;

  DPLAYX_AquireSemaphore();

  if ( !DPLAYX_AppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return DPERR_NOTLOBBIED;
  }

  /* Verify buffer size */
  if ( ( lpData == NULL ) ||
       ( *lpdwDataSize < sizeof( DPLCONNECTION ) )
     )
  {
    DPLAYX_ReleaseSemaphore();

    *lpdwDataSize = sizeof( DPLCONNECTION );

    return DPERR_BUFFERTOOSMALL;
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

  /* Send a message - or only the first time? */

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

  DPLAYX_AquireSemaphore();

  if ( !DPLAYX_AppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return DPERR_NOTLOBBIED;
  }

  /* Verify buffer size */
  if ( ( lpData == NULL ) ||
       ( *lpdwDataSize < sizeof( DPLCONNECTION ) )
     )
  {
    DPLAYX_ReleaseSemaphore();

    *lpdwDataSize = sizeof( DPLCONNECTION );

    return DPERR_BUFFERTOOSMALL;
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

  /* Send a message - or only the first time? */

  DPLAYX_ReleaseSemaphore();

  return DP_OK;
}


HRESULT DPLAYX_SetConnectionSettingsA
( DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  lpDirectPlayLobbyData lpDplData;

  DPLAYX_AquireSemaphore();

  if ( !DPLAYX_AppIdLobbied( dwAppID, &lpDplData ) )
  {
    /* FIXME: Create a new entry for this dwAppID? */
    DPLAYX_ReleaseSemaphore();

    return DPERR_GENERIC;
  }

  /* Paramater check */
  if( dwFlags || !lpConn )
  {
    DPLAYX_ReleaseSemaphore();

    ERR("invalid parameters.\n");
    return DPERR_INVALIDPARAMS;
  }

  /* Store information */
  if(  lpConn->dwSize != sizeof(DPLCONNECTION) )
  {
    DPLAYX_ReleaseSemaphore();

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
    DPLAYX_ReleaseSemaphore();

    ERR("DPSESSIONDESC passed in? Size=%08lx vs. expected=%ul bytes\n",
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

  /* Send a message - I think */

  DPLAYX_ReleaseSemaphore();

  return DP_OK;
}

HRESULT DPLAYX_SetConnectionSettingsW
( DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  lpDirectPlayLobbyData lpDplData;

  DPLAYX_AquireSemaphore();

  if ( !DPLAYX_AppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return DPERR_NOTLOBBIED;
  }

  /* Paramater check */
  if( dwFlags || !lpConn )
  {
    DPLAYX_ReleaseSemaphore();
    ERR("invalid parameters.\n");
    return DPERR_INVALIDPARAMS;
  }

  /* Store information */
  if(  lpConn->dwSize != sizeof(DPLCONNECTION) )
  {
    DPLAYX_ReleaseSemaphore();

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
    DPLAYX_ReleaseSemaphore();

    ERR("DPSESSIONDESC passed in? Size=%08lx vs. expected=%ul bytes\n",
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

  /* Send a message - I think */

  DPLAYX_ReleaseSemaphore();

  return DP_OK;
}
