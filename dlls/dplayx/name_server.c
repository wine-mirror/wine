/* DPLAYX.DLL name server implementation
 *
 * Copyright 2000 - Peter Hunnisett
 *
 * <presently under construction - contact hunnise@nortelnetworks.com>
 *
 */
 
/* NOTE: Methods with the NS_ prefix are name server methods */

#include "winbase.h"
#include "debugtools.h"

#include "dplayx_global.h"
#include "name_server.h"

/* FIXME: Need to aquire the interface semaphore before didlling data */

DEFAULT_DEBUG_CHANNEL(dplay);

/* NS specific structures */
typedef struct tagNSCacheData
{
  struct tagNSCacheData* next;

  LPDPSESSIONDESC2 data;

} NSCacheData, *lpNSCacheData;

typedef struct tagNSCache
{
  lpNSCacheData present; /* keep track of what is to be looked at */
  lpNSCacheData first; 
} NSCache, *lpNSCache;

/* Local Prototypes */
static void NS_InvalidateSessionCache( lpNSCache lpCache );


/* Name Server functions 
 * --------------------- 
 */
void NS_SetLocalComputerAsNameServer( LPCDPSESSIONDESC2 lpsd )
{
  DPLAYX_SetLocalSession( lpsd );
}

/* This function is responsible for sending a request for all other known
   nameservers to send us what sessions they have registered locally
 */
void NS_SendSessionRequestBroadcast( LPVOID lpNSInfo )
{
  UINT index = 0;
  lpNSCache lpCache = (lpNSCache)lpNSInfo;
  LPDPSESSIONDESC2 lpTmp = NULL; 

  /* Invalidate the session cache for the interface */
  NS_InvalidateSessionCache( lpCache );
 
  /* Add the local known sessions to the cache */
  if( ( lpTmp = DPLAYX_CopyAndAllocateLocalSession( &index ) ) != NULL )
  {
    lpCache->first = (lpNSCacheData)HeapAlloc( GetProcessHeap(), 
                                               HEAP_ZERO_MEMORY,
                                               sizeof( *(lpCache->first) ) );
    lpCache->first->data = lpTmp;
    lpCache->first->next = NULL;
    lpCache->present = lpCache->first;

    while( ( lpTmp = DPLAYX_CopyAndAllocateLocalSession( &index ) ) != NULL )
    {
      lpCache->present->next = (lpNSCacheData)HeapAlloc( GetProcessHeap(), 
                                                         HEAP_ZERO_MEMORY,
                                                         sizeof( *(lpCache->present) ) ); 
      lpCache->present = lpCache->present->next;
      lpCache->present->data = lpTmp;
      lpCache->present->next = NULL;
    }

    lpCache->present = lpCache->first;
  }

  /* Send out requests for matching sessions to all other known computers */
  FIXME( ": no remote requests sent\n" );
  /* FIXME - how to handle responses to messages anyways? */
}

/* Render all data in a session cache invalid */
static void NS_InvalidateSessionCache( lpNSCache lpCache )
{
  if( lpCache == NULL )
  {
    ERR( ": invalidate non existant cache\n" );
    return;
  }

  /* Remove everything from the cache */
  while( lpCache->first )
  {
    lpCache->present = lpCache->first;
    lpCache->first = lpCache->first->next; 
    HeapFree( GetProcessHeap(), 0, lpCache->present );
  }

  /* NULL out the cache pointers */
  lpCache->present = NULL;
  lpCache->first   = NULL;
}

/* Create and initialize a session cache */
BOOL NS_InitializeSessionCache( LPVOID* lplpNSInfo )
{
  lpNSCache lpCache = (lpNSCache)HeapAlloc( GetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            sizeof( *lpCache ) );

  *lplpNSInfo = lpCache;

  if( lpCache == NULL )
  {
    return FALSE;
  }

  lpCache->first = lpCache->present = NULL;

  return TRUE;
}

/* Delete a session cache */
void NS_DeleteSessionCache( LPVOID lpNSInfo )
{
  NS_InvalidateSessionCache( (lpNSCache)lpNSInfo );
}

/* Reinitialize the present pointer for this cache */
void NS_ResetSessionEnumeration( LPVOID lpNSInfo )
{
 
  ((lpNSCache)lpNSInfo)->present = ((lpNSCache)lpNSInfo)->first;
}

LPDPSESSIONDESC2 NS_WalkSessions( LPVOID lpNSInfo )
{
  LPDPSESSIONDESC2 lpSessionDesc;
  lpNSCache lpCache = (lpNSCache)lpNSInfo;

  /* Test for end of the list */ 
  if( lpCache->present == NULL )
  {
    return NULL;
  }

  lpSessionDesc = lpCache->present->data;

  /* Advance tracking pointer */
  lpCache->present = lpCache->present->next;

  return lpSessionDesc;
}
