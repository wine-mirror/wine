/* dplayx.dll global data implementation.
 *
 * Copyright 1999,2000 - Peter Hunnisett
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

#include "dplayx_global.h"
#include "dplayx_messages.h" /* For CreateMessageReceptionThread only */

DEFAULT_DEBUG_CHANNEL(dplay);

/* FIXME: Need to do all that fun other dll referencing type of stuff */

/* Static data for all processes */
static LPCSTR lpszDplayxSemaName = "WINE_DPLAYX_SM";
static HANDLE hDplayxSema;

static LPCSTR lpszDplayxFileMapping = "WINE_DPLAYX_FM";
static HANDLE hDplayxSharedMem;

static LPVOID lpSharedStaticData = NULL;


#define DPLAYX_AquireSemaphore()  TRACE( "Waiting for DPLAYX semaphore\n" ); \
                                  WaitForSingleObject( hDplayxSema, INFINITE );\
                                  TRACE( "Through wait\n" )

#define DPLAYX_ReleaseSemaphore() ReleaseSemaphore( hDplayxSema, 1, NULL ); \
                                  TRACE( "DPLAYX Semaphore released\n" ) /* FIXME: Is this correct? */


/* HACK for simple global data right now */ 
#define dwStaticSharedSize (128 * 1024) /* FIXME: Way too much */
#define dwDynamicSharedSize (128 * 1024) /* FIXME: Enough? */
#define dwTotalSharedSize  (dwStaticSharedSize + dwDynamicSharedSize)


/* FIXME: Is there no easier way? */

/* Pretend the entire dynamic area is carved up into 512 byte blocks.
 * Each block has 4 bytes which are 0 unless used */
#define dwBlockSize 512
#define dwMaxBlock  (dwDynamicSharedSize/dwBlockSize)
DWORD dwBlockOn  = 0;

typedef struct 
{
  DWORD used;
  DWORD data[dwBlockSize-sizeof(DWORD)];
} DPLAYX_MEM_SLICE;

DPLAYX_MEM_SLICE* lpMemArea;

void DPLAYX_PrivHeapFree( LPVOID addr );
void DPLAYX_PrivHeapFree( LPVOID addr )
{
  LPVOID lpAddrStart;
  DWORD dwBlockUsed;

  /* Handle getting passed a NULL */
  if( addr == NULL )
  {
    return;
  } 

  lpAddrStart = addr - sizeof(DWORD); /* Find block header */
  dwBlockUsed =  ((BYTE*)lpAddrStart - (BYTE*)lpMemArea)/dwBlockSize;
  
  lpMemArea[ dwBlockUsed ].used = 0;
}

LPVOID DPLAYX_PrivHeapAlloc( DWORD flags, DWORD size );
LPVOID DPLAYX_PrivHeapAlloc( DWORD flags, DWORD size )
{
  LPVOID lpvArea = NULL;
  UINT   uBlockUsed;

  if( size > (dwBlockSize - sizeof(DWORD)) )
  {
    FIXME( "Size exceeded. Request of 0x%08lx\n", size );
    size = dwBlockSize - sizeof(DWORD);
  } 

  /* Find blank area */
  uBlockUsed = 0; 
  while( ( lpMemArea[ uBlockUsed ].used != 0 ) && ( uBlockUsed <= dwMaxBlock ) ) { uBlockUsed++; } 

  if( uBlockUsed <= dwMaxBlock )
  {
    /* Set the area used */
    lpMemArea[ uBlockUsed ].used = 1;
    lpvArea = &(lpMemArea[ uBlockUsed ].data);
  }
  else
  {
    ERR( "No free block found\n" );
    return NULL;
  }
  
  if( flags & HEAP_ZERO_MEMORY )
  {
    ZeroMemory( lpvArea, size );
  }

  return lpvArea;
}

LPSTR DPLAYX_strdupA( DWORD flags, LPCSTR str );
LPSTR DPLAYX_strdupA( DWORD flags, LPCSTR str )
{
  LPSTR p = DPLAYX_PrivHeapAlloc( flags, strlen(str) + 1 );
  if(p) {
    strcpy( p, str );
  }
  return p;
}

LPWSTR DPLAYX_strdupW( DWORD flags, LPCWSTR str );
LPWSTR DPLAYX_strdupW( DWORD flags, LPCWSTR str )
{                   
  INT len = lstrlenW(str) + 1;
  LPWSTR p = DPLAYX_PrivHeapAlloc( flags, len * sizeof(WCHAR) );
  if(p) {
    lstrcpyW( p, str );
  }
  return p;
}


enum { numSupportedLobbies = 32, numSupportedSessions = 32 };
typedef struct tagDPLAYX_LOBBYDATA
{
  /* Points to lpConn + block of contiguous extra memory for dynamic parts
   * of the struct directly following 
   */
  LPDPLCONNECTION lpConn;

  /* Information for dplobby interfaces */
  DWORD           dwAppID;
  HANDLE          hReceiveEvent;
  DWORD           dwAppLaunchedFromID;

  /* Should this lobby app send messages to creator at important life
   * stages
   */
  BOOL bInformOnConnect;     /* FIXME: Not used yet */
  BOOL bInformOnSettingRead;
  BOOL bInformOnAppDeath;    /* FIXME: Not used yet */

  BOOL bWaitForConnectionSettings;

} DPLAYX_LOBBYDATA, *LPDPLAYX_LOBBYDATA;

static DPLAYX_LOBBYDATA* lobbyData = NULL;
/* static DPLAYX_LOBBYDATA lobbyData[ numSupportedLobbies ]; */

static DPSESSIONDESC2* sessionData = NULL; 
/* static DPSESSIONDESC2* sessionData[ numSupportedSessions ]; */

/* Function prototypes */
DWORD DPLAYX_SizeOfLobbyDataA( LPDPLCONNECTION lpDplData );
DWORD DPLAYX_SizeOfLobbyDataW( LPDPLCONNECTION lpDplData );
void DPLAYX_CopyConnStructA( LPDPLCONNECTION dest, LPDPLCONNECTION src );
void DPLAYX_CopyConnStructW( LPDPLCONNECTION dest, LPDPLCONNECTION src );
BOOL DPLAYX_IsAppIdLobbied( DWORD dwAppId, LPDPLAYX_LOBBYDATA* dplData );
void DPLAYX_InitializeLobbyDataEntry( LPDPLAYX_LOBBYDATA lpData );
BOOL DPLAYX_CopyIntoSessionDesc2A( LPDPSESSIONDESC2  lpSessionDest,
                                   LPCDPSESSIONDESC2 lpSessionSrc );



/*************************************************************************** 
 * Called to initialize the global data. This will only be used on the 
 * loading of the dll 
 ***************************************************************************/ 
BOOL DPLAYX_ConstructData(void)
{
  SECURITY_ATTRIBUTES s_attrib;
  BOOL                bInitializeSharedMemory = FALSE;
  LPVOID              lpDesiredMemoryMapStart = (LPVOID)0x50000000;

  TRACE( "DPLAYX dll loaded - construct called\n" );

  /* Create a semaphore to block access to DPLAYX global data structs */

  s_attrib.bInheritHandle       = TRUE;
  s_attrib.lpSecurityDescriptor = NULL;
  s_attrib.nLength              = sizeof(s_attrib);

  hDplayxSema = CreateSemaphoreA( &s_attrib, 1, 1, lpszDplayxSemaName ); 

  /* First instance creates the semaphore. Others just use it */
  if( GetLastError() == ERROR_SUCCESS )
  {
    TRACE( "Semaphore %u created\n", hDplayxSema );

    /* The semaphore creator will also build the shared memory */
    bInitializeSharedMemory = TRUE;
  }
  else if ( GetLastError() == ERROR_ALREADY_EXISTS )
  {
    TRACE( "Found semaphore handle %u\n", hDplayxSema );
  }
  else
  {
    ERR( ": semaphore error 0x%08lx\n", GetLastError() );
    return FALSE;
  }

  SetLastError( ERROR_SUCCESS );

  DPLAYX_AquireSemaphore();
 
  hDplayxSharedMem = CreateFileMappingA( INVALID_HANDLE_VALUE,
                                         &s_attrib,
                                         PAGE_READWRITE | SEC_COMMIT,
                                         0, 
                                         dwTotalSharedSize,
                                         lpszDplayxFileMapping ); 

  if( GetLastError() == ERROR_SUCCESS )
  {
    TRACE( "File mapped %u created\n", hDplayxSharedMem );
  }
  else if ( GetLastError() == ERROR_ALREADY_EXISTS )
  {
    TRACE( "Found FileMapping handle %u\n", hDplayxSharedMem );
  }
  else
  {
    ERR( ": unable to create shared memory 0x%08lx\n", GetLastError() );
    return FALSE;
  }

  lpSharedStaticData = MapViewOfFileEx( hDplayxSharedMem, 
                                        FILE_MAP_WRITE, 
                                        0, 0, 0, lpDesiredMemoryMapStart );

  if( lpSharedStaticData == NULL )
  {
    ERR( ": unable to map static data into process memory space: 0x%08lx\n", 
         GetLastError() );
    return FALSE;
  }
  else
  {
    if( lpDesiredMemoryMapStart == lpSharedStaticData )
    {
      TRACE( "File mapped to %p\n", lpSharedStaticData );
    }
    else
    {
      /* Presently the shared data structures use pointers. If the
       * files are no mapped into the same area, the pointers will no
       * longer make any sense :(
       * FIXME: In the future make the shared data structures have some
       *        sort of fixup to make them independent between data spaces.
       *        This will also require a rework of the session data stuff.
       */
      ERR( "File mapped to %p (not %p). Expect failure\n",
            lpSharedStaticData, lpDesiredMemoryMapStart );
    }
  }

  /* Dynamic area starts just after the static area */
  lpMemArea = (LPVOID)((BYTE*)lpSharedStaticData + dwStaticSharedSize);

  /* FIXME: Crude hack */
  lobbyData   = (DPLAYX_LOBBYDATA*)lpSharedStaticData;
  sessionData = (DPSESSIONDESC2*)((BYTE*)lpSharedStaticData + (dwStaticSharedSize/2));

  /* Initialize shared data segments. */
  if( bInitializeSharedMemory )
  {
    UINT i;

    TRACE( "Initializing shared memory\n" );

    /* Set all lobbies to be "empty" */ 
    for( i=0; i < numSupportedLobbies; i++ )
    {
      DPLAYX_InitializeLobbyDataEntry( &lobbyData[ i ] );
    }

    /* Set all sessions to be "empty" */
    for( i=0; i < numSupportedSessions; i++ )
    {
      sessionData[i].dwSize = 0;
    }

    /* Zero out the dynmaic area */
    ZeroMemory( lpMemArea, dwDynamicSharedSize );

    /* Just for fun sync the whole data area */
    FlushViewOfFile( lpSharedStaticData, dwTotalSharedSize );
  }
  
  DPLAYX_ReleaseSemaphore();

  return TRUE;
}

/*************************************************************************** 
 * Called to destroy all global data. This will only be used on the 
 * unloading of the dll 
 ***************************************************************************/ 
BOOL DPLAYX_DestructData(void)
{
  TRACE( "DPLAYX dll unloaded - destruct called\n" );

  /* Delete the semaphore */
  CloseHandle( hDplayxSema );
 
  /* Delete shared memory file mapping */
  UnmapViewOfFile( lpSharedStaticData );
  CloseHandle( hDplayxSharedMem );

  return FALSE;
}


void DPLAYX_InitializeLobbyDataEntry( LPDPLAYX_LOBBYDATA lpData )
{
  ZeroMemory( lpData, sizeof( *lpData ) );

  /* Set the handle to a better invalid value */
  lpData->hReceiveEvent = INVALID_HANDLE_VALUE;
}

/* NOTE: This must be called with the semaphore aquired. 
 * TRUE/FALSE with a pointer to it's data returned. Pointer data is
 * is only valid if TRUE is returned.
 */
BOOL DPLAYX_IsAppIdLobbied( DWORD dwAppID, LPDPLAYX_LOBBYDATA* lplpDplData )
{
  UINT i;

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
      TRACE( "Found 0x%08lx @ %u\n", dwAppID, i );
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
      TRACE( "Setting lobbyData[%u] for (0x%08lx,%u,0x%08lx)\n",
              i, dwAppID, hReceiveEvent, GetCurrentProcessId() );

      lobbyData[ i ].dwAppID              = dwAppID;
      lobbyData[ i ].hReceiveEvent        = hReceiveEvent;
      lobbyData[ i ].dwAppLaunchedFromID  = GetCurrentProcessId();

      lobbyData[ i ].bInformOnConnect     = TRUE;
      lobbyData[ i ].bInformOnSettingRead = TRUE;
      lobbyData[ i ].bInformOnAppDeath    = TRUE;

      DPLAYX_ReleaseSemaphore();
      return TRUE;
    }
  }

  ERR( "No empty lobbies\n" );

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
      /* FIXME: Should free up anything unused. Tisk tisk :0 */
      /* Mark this entry unused */
      TRACE( "Marking lobbyData[%u] unused\n", i );
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
  LPDWORD lpdwDataSize,
  LPBOOL  lpbSendHaveReadMessage )
{
  LPDPLAYX_LOBBYDATA lpDplData;
  DWORD              dwRequiredDataSize = 0;

  DPLAYX_AquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();

    TRACE( "Application 0x%08lx is not lobbied\n", dwAppID );
    return DPERR_NOTLOBBIED;
  }

  dwRequiredDataSize = DPLAYX_SizeOfLobbyDataA( lpDplData->lpConn );

  /* Do they want to know the required buffer size or is the provided buffer
   * big enough? 
   */
  if ( ( lpData == NULL ) ||
       ( *lpdwDataSize < dwRequiredDataSize )
     )
  {
    DPLAYX_ReleaseSemaphore();

    *lpdwDataSize = DPLAYX_SizeOfLobbyDataA( lpDplData->lpConn );

    return DPERR_BUFFERTOOSMALL;
  }

  /* They have gotten the information */
  *lpbSendHaveReadMessage = lpDplData->bInformOnSettingRead;
  lpDplData->bInformOnSettingRead = FALSE;

  DPLAYX_CopyConnStructA( (LPDPLCONNECTION)lpData, lpDplData->lpConn );

  DPLAYX_ReleaseSemaphore();

  return DP_OK;
}

/* Assumption: Enough contiguous space was allocated at dest */
void DPLAYX_CopyConnStructA( LPDPLCONNECTION dest, LPDPLCONNECTION src )
{
  BYTE*              lpStartOfFreeSpace;

  memcpy( dest, src, sizeof( DPLCONNECTION ) );

  lpStartOfFreeSpace = ((BYTE*)dest) + sizeof( DPLCONNECTION );

  /* Copy the LPDPSESSIONDESC2 structure if it exists */
  if( src->lpSessionDesc )
  {
    dest->lpSessionDesc = (LPDPSESSIONDESC2)lpStartOfFreeSpace;
    lpStartOfFreeSpace += sizeof( DPSESSIONDESC2 );
    memcpy( dest->lpSessionDesc, src->lpSessionDesc, sizeof( DPSESSIONDESC2 ) );

    /* Session names may or may not exist */
    if( src->lpSessionDesc->sess.lpszSessionNameA )
    {
      lstrcpyA( (LPSTR)lpStartOfFreeSpace, src->lpSessionDesc->sess.lpszSessionNameA );
      dest->lpSessionDesc->sess.lpszSessionNameA = (LPSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=  
        lstrlenA( (LPSTR)dest->lpSessionDesc->sess.lpszSessionName ) + 1;
    }

    if( src->lpSessionDesc->pass.lpszPasswordA )
    {
      lstrcpyA( (LPSTR)lpStartOfFreeSpace, src->lpSessionDesc->pass.lpszPasswordA );
      dest->lpSessionDesc->pass.lpszPasswordA = (LPSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace += 
        lstrlenA( (LPSTR)dest->lpSessionDesc->pass.lpszPasswordA ) + 1;
    }
  }

  /* DPNAME structure is optional */
  if( src->lpPlayerName )
  {
    dest->lpPlayerName = (LPDPNAME)lpStartOfFreeSpace;
    lpStartOfFreeSpace += sizeof( DPNAME );
    memcpy( dest->lpPlayerName, src->lpPlayerName, sizeof( DPNAME ) );

    if( src->lpPlayerName->psn.lpszShortNameA )
    {
      lstrcpyA( (LPSTR)lpStartOfFreeSpace, src->lpPlayerName->psn.lpszShortNameA );
      dest->lpPlayerName->psn.lpszShortNameA = (LPSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=  
        lstrlenA( (LPSTR)dest->lpPlayerName->psn.lpszShortNameA ) + 1;
    }

    if( src->lpPlayerName->pln.lpszLongNameA )
    {
      lstrcpyA( (LPSTR)lpStartOfFreeSpace, src->lpPlayerName->pln.lpszLongNameA );
      dest->lpPlayerName->pln.lpszLongNameA = (LPSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=  
        lstrlenA( (LPSTR)dest->lpPlayerName->pln.lpszLongName ) + 1 ;
    }

  }

  /* Copy address if it exists */
  if( src->lpAddress )
  {
    dest->lpAddress = (LPVOID)lpStartOfFreeSpace;
    memcpy( lpStartOfFreeSpace, src->lpAddress, src->dwAddressSize );
    /* No need to advance lpStartOfFreeSpace as there is no more "dynamic" data */
  }
}

HRESULT DPLAYX_GetConnectionSettingsW
( DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize,
  LPBOOL  lpbSendHaveReadMessage )
{
  LPDPLAYX_LOBBYDATA lpDplData;
  DWORD              dwRequiredDataSize = 0;

  DPLAYX_AquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return DPERR_NOTLOBBIED;
  }

  dwRequiredDataSize = DPLAYX_SizeOfLobbyDataW( lpDplData->lpConn );

  /* Do they want to know the required buffer size or is the provided buffer
   * big enough? 
   */
  if ( ( lpData == NULL ) ||
       ( *lpdwDataSize < dwRequiredDataSize )
     )
  {
    DPLAYX_ReleaseSemaphore();

    *lpdwDataSize = DPLAYX_SizeOfLobbyDataW( lpDplData->lpConn );

    return DPERR_BUFFERTOOSMALL;
  }

  /* They have gotten the information */
  *lpbSendHaveReadMessage = lpDplData->bInformOnSettingRead;
  lpDplData->bInformOnSettingRead = FALSE;

  DPLAYX_CopyConnStructW( (LPDPLCONNECTION)lpData, lpDplData->lpConn );

  DPLAYX_ReleaseSemaphore();

  return DP_OK;
}

/* Assumption: Enough contiguous space was allocated at dest */
void DPLAYX_CopyConnStructW( LPDPLCONNECTION dest, LPDPLCONNECTION src )
{
  BYTE*              lpStartOfFreeSpace;

  memcpy( dest, src, sizeof( DPLCONNECTION ) );

  lpStartOfFreeSpace = ( (BYTE*)dest) + sizeof( DPLCONNECTION );

  /* Copy the LPDPSESSIONDESC2 structure if it exists */
  if( src->lpSessionDesc )
  {
    dest->lpSessionDesc = (LPDPSESSIONDESC2)lpStartOfFreeSpace;
    lpStartOfFreeSpace += sizeof( DPSESSIONDESC2 );
    memcpy( dest->lpSessionDesc, src->lpSessionDesc, sizeof( DPSESSIONDESC2 ) ); 

    /* Session names may or may not exist */
    if( src->lpSessionDesc->sess.lpszSessionName )
    {
      lstrcpyW( (LPWSTR)lpStartOfFreeSpace, dest->lpSessionDesc->sess.lpszSessionName );
      src->lpSessionDesc->sess.lpszSessionName = (LPWSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=  sizeof(WCHAR) *
        ( lstrlenW( (LPWSTR)dest->lpSessionDesc->sess.lpszSessionName ) + 1 );
    }

    if( src->lpSessionDesc->pass.lpszPassword )
    {
      lstrcpyW( (LPWSTR)lpStartOfFreeSpace, src->lpSessionDesc->pass.lpszPassword );
      dest->lpSessionDesc->pass.lpszPassword = (LPWSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=  sizeof(WCHAR) *
        ( lstrlenW( (LPWSTR)dest->lpSessionDesc->pass.lpszPassword ) + 1 );
    }
  }

  /* DPNAME structure is optional */
  if( src->lpPlayerName )
  {
    dest->lpPlayerName = (LPDPNAME)lpStartOfFreeSpace;
    lpStartOfFreeSpace += sizeof( DPNAME );
    memcpy( dest->lpPlayerName, src->lpPlayerName, sizeof( DPNAME ) );
   
    if( src->lpPlayerName->psn.lpszShortName )
    {
      lstrcpyW( (LPWSTR)lpStartOfFreeSpace, src->lpPlayerName->psn.lpszShortName );
      dest->lpPlayerName->psn.lpszShortName = (LPWSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=  sizeof(WCHAR) *
        ( lstrlenW( (LPWSTR)dest->lpPlayerName->psn.lpszShortName ) + 1 );
    }

    if( src->lpPlayerName->pln.lpszLongName )
    {
      lstrcpyW( (LPWSTR)lpStartOfFreeSpace, src->lpPlayerName->pln.lpszLongName );
      dest->lpPlayerName->pln.lpszLongName = (LPWSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=  sizeof(WCHAR) *
        ( lstrlenW( (LPWSTR)dest->lpPlayerName->pln.lpszLongName ) + 1 );
    }

  }

  /* Copy address if it exists */
  if( src->lpAddress )
  {
    dest->lpAddress = (LPVOID)lpStartOfFreeSpace;
    memcpy( lpStartOfFreeSpace, src->lpAddress, src->dwAddressSize );  
    /* No need to advance lpStartOfFreeSpace as there is no more "dynamic" data */
  }

}

/* Store the structure into the shared data structre. Ensure that allocs for
 * variable length strings come from the shared data structure.
 * FIXME: We need to free information as well 
 */
HRESULT DPLAYX_SetConnectionSettingsA
( DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  LPDPLAYX_LOBBYDATA lpDplData;

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

  DPLAYX_AquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();

    return DPERR_NOTLOBBIED;
  }

  if(  (!lpConn->lpSessionDesc ) ||
       ( lpConn->lpSessionDesc->dwSize != sizeof( DPSESSIONDESC2 ) )
    )
  {
    DPLAYX_ReleaseSemaphore();

    ERR("DPSESSIONDESC passed in? Size=%lu vs. expected=%u bytes\n",
         lpConn->lpSessionDesc->dwSize, sizeof( DPSESSIONDESC2 ) );

    return DPERR_INVALIDPARAMS;
  }

  /* Free the existing memory */
  DPLAYX_PrivHeapFree( lpDplData->lpConn );

  lpDplData->lpConn = DPLAYX_PrivHeapAlloc( HEAP_ZERO_MEMORY, 
                                            DPLAYX_SizeOfLobbyDataA( lpConn ) );

  DPLAYX_CopyConnStructA( lpDplData->lpConn, lpConn );


  DPLAYX_ReleaseSemaphore();

  /* FIXME: Send a message - I think */

  return DP_OK;
}

/* Store the structure into the shared data structre. Ensure that allocs for
 * variable length strings come from the shared data structure.
 * FIXME: We need to free information as well 
 */
HRESULT DPLAYX_SetConnectionSettingsW
( DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  LPDPLAYX_LOBBYDATA lpDplData;

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

  DPLAYX_AquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();

    return DPERR_NOTLOBBIED;
  }

  /* Free the existing memory */
  DPLAYX_PrivHeapFree( lpDplData->lpConn );

  lpDplData->lpConn = DPLAYX_PrivHeapAlloc( HEAP_ZERO_MEMORY,
                                            DPLAYX_SizeOfLobbyDataW( lpConn ) );

  DPLAYX_CopyConnStructW( lpDplData->lpConn, lpConn );


  DPLAYX_ReleaseSemaphore();

  /* FIXME: Send a message - I think */

  return DP_OK;
}

DWORD DPLAYX_SizeOfLobbyDataA( LPDPLCONNECTION lpConn )
{
  DWORD dwTotalSize = sizeof( DPLCONNECTION );

  /* Just a safety check */
  if( lpConn == NULL )
  {
    ERR( "lpConn is NULL\n" );
    return 0;
  }

  if( lpConn->lpSessionDesc != NULL )
  {
    dwTotalSize += sizeof( DPSESSIONDESC2 );

    if( lpConn->lpSessionDesc->sess.lpszSessionNameA )
    {
      dwTotalSize += lstrlenA( lpConn->lpSessionDesc->sess.lpszSessionNameA ) + 1;
    }
 
    if( lpConn->lpSessionDesc->pass.lpszPasswordA )
    {
      dwTotalSize += lstrlenA( lpConn->lpSessionDesc->pass.lpszPasswordA ) + 1;
    }
  }

  if( lpConn->lpPlayerName != NULL )
  {
    dwTotalSize += sizeof( DPNAME );

    if( lpConn->lpPlayerName->psn.lpszShortNameA )
    {
      dwTotalSize += lstrlenA( lpConn->lpPlayerName->psn.lpszShortNameA ) + 1;
    }

    if( lpConn->lpPlayerName->pln.lpszLongNameA )
    {
      dwTotalSize += lstrlenA( lpConn->lpPlayerName->pln.lpszLongNameA ) + 1;
    }

  }

  dwTotalSize += lpConn->dwAddressSize;

  return dwTotalSize;
}

DWORD DPLAYX_SizeOfLobbyDataW( LPDPLCONNECTION lpConn )
{
  DWORD dwTotalSize = sizeof( DPLCONNECTION );

  /* Just a safety check */
  if( lpConn == NULL )
  {
    ERR( "lpConn is NULL\n" );
    return 0;
  }

  if( lpConn->lpSessionDesc != NULL )
  {
    dwTotalSize += sizeof( DPSESSIONDESC2 );

    if( lpConn->lpSessionDesc->sess.lpszSessionName )
    {
      dwTotalSize += sizeof( WCHAR ) *
        ( lstrlenW( lpConn->lpSessionDesc->sess.lpszSessionName ) + 1 );
    }

    if( lpConn->lpSessionDesc->pass.lpszPassword )
    {
      dwTotalSize += sizeof( WCHAR ) *
        ( lstrlenW( lpConn->lpSessionDesc->pass.lpszPassword ) + 1 );
    }
  }

  if( lpConn->lpPlayerName != NULL )
  {
    dwTotalSize += sizeof( DPNAME );

    if( lpConn->lpPlayerName->psn.lpszShortName )
    {
      dwTotalSize += sizeof( WCHAR ) *
        ( lstrlenW( lpConn->lpPlayerName->psn.lpszShortName ) + 1 );
    }

    if( lpConn->lpPlayerName->pln.lpszLongName )
    {
      dwTotalSize += sizeof( WCHAR ) *
        ( lstrlenW( lpConn->lpPlayerName->pln.lpszLongName ) + 1 );
    }

  }

  dwTotalSize += lpConn->dwAddressSize;

  return dwTotalSize;
}



LPDPSESSIONDESC2 DPLAYX_CopyAndAllocateSessionDesc2A( LPCDPSESSIONDESC2 lpSessionSrc )
{
   LPDPSESSIONDESC2 lpSessionDest = 
     (LPDPSESSIONDESC2)DPLAYX_PrivHeapAlloc( HEAP_ZERO_MEMORY, sizeof( *lpSessionSrc ) );
   DPLAYX_CopyIntoSessionDesc2A( lpSessionDest, lpSessionSrc );

   return lpSessionDest;
}

/* Copy an ANSI session desc structure to the given buffer */
BOOL DPLAYX_CopyIntoSessionDesc2A( LPDPSESSIONDESC2  lpSessionDest,
                                   LPCDPSESSIONDESC2 lpSessionSrc )
{
  memcpy( lpSessionDest, lpSessionSrc, sizeof( *lpSessionSrc ) );

  if( lpSessionSrc->sess.lpszSessionNameA )
  {
    lpSessionDest->sess.lpszSessionNameA =
      DPLAYX_strdupA( HEAP_ZERO_MEMORY, lpSessionSrc->sess.lpszSessionNameA );
  }
  if( lpSessionSrc->pass.lpszPasswordA )
  {
    lpSessionDest->pass.lpszPasswordA =
      DPLAYX_strdupA( HEAP_ZERO_MEMORY, lpSessionSrc->pass.lpszPasswordA );
  }

  return TRUE;
}

/* Start the index at 0. index will be updated to equal that which should
   be passed back into this function for the next element */
LPDPSESSIONDESC2 DPLAYX_CopyAndAllocateLocalSession( UINT* index )
{
  for( ; (*index) < numSupportedSessions; (*index)++ )
  {
    if( sessionData[(*index)].dwSize != 0 )
    {
      return DPLAYX_CopyAndAllocateSessionDesc2A( &sessionData[(*index)++] ); 
    }
  }
 
  /* No more sessions */
  return NULL;
}

/* Start the index at 0. index will be updated to equal that which should
   be passed back into this function for the next element */
BOOL DPLAYX_CopyLocalSession( UINT* index, LPDPSESSIONDESC2 lpsd )
{
  for( ; (*index) < numSupportedSessions; (*index)++ )
  {
    if( sessionData[(*index)].dwSize != 0 )
    {
      return DPLAYX_CopyIntoSessionDesc2A( lpsd, &sessionData[(*index)++] );
    }
  }

  /* No more sessions */
  return FALSE;
}

void DPLAYX_SetLocalSession( LPCDPSESSIONDESC2 lpsd )
{
  UINT i;

  /* FIXME: Is this an error if it exists already? */
 
  /* Crude/wrong implementation for now. Just always add to first empty spot */
  for( i=0; i < numSupportedSessions; i++ )
  {
    /* Is this one empty? */
    if( sessionData[i].dwSize == 0 )
    {
      DPLAYX_CopyIntoSessionDesc2A( &sessionData[i], lpsd );  
      break;
    }
  }

}

BOOL DPLAYX_WaitForConnectionSettings( BOOL bWait )
{
  LPDPLAYX_LOBBYDATA lpLobbyData;

  DPLAYX_AquireSemaphore();
  
  if( !DPLAYX_IsAppIdLobbied( 0, &lpLobbyData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return FALSE;
  }

  lpLobbyData->bWaitForConnectionSettings = bWait;

  DPLAYX_ReleaseSemaphore();

  return TRUE;
}

BOOL DPLAYX_AnyLobbiesWaitingForConnSettings(void)
{
  UINT i;
  BOOL bFound = FALSE;

  DPLAYX_AquireSemaphore();

  for( i=0; i < numSupportedLobbies; i++ )
  {
    if( ( lobbyData[ i ].dwAppID != 0 ) &&            /* lobby initialized */
        ( lobbyData[ i ].bWaitForConnectionSettings ) /* Waiting */
      )
    { 
      bFound = TRUE;
      break; 
    }
  }

  DPLAYX_ReleaseSemaphore();

  return bFound;
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

