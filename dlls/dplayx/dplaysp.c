/* This contains the implementation of the interface Service
 * Providers require to communicate with Direct Play 
 *
 * Copyright 2000 Peter Hunnisett <hunnise@nortelnetworks.com>
 */

#include "heap.h"
#include "winerror.h"
#include "debugtools.h"

#include "dpinit.h"
#include "dplaysp.h"
#include "dplay_global.h"
#include "name_server.h"
#include "dplayx_messages.h"

#include "dplayx_global.h" /* FIXME: For global hack */

/* FIXME: Need to add interface locking inside procedures */

DEFAULT_DEBUG_CHANNEL(dplay)

/* Prototypes */
static BOOL DPSP_CreateIUnknown( LPVOID lpSP );
static BOOL DPSP_DestroyIUnknown( LPVOID lpSP );
static BOOL DPSP_CreateDirectPlaySP( LPVOID lpSP, IDirectPlay2Impl* dp );
static BOOL DPSP_DestroyDirectPlaySP( LPVOID lpSP );


/* Predefine the interface */
typedef struct IDirectPlaySPImpl IDirectPlaySPImpl;

typedef struct tagDirectPlaySPIUnknownData
{
  ULONG             ulObjRef;
  CRITICAL_SECTION  DPSP_lock;
} DirectPlaySPIUnknownData;

typedef struct tagDirectPlaySPData
{
  LPVOID lpSpRemoteData;
  DWORD  dwSpRemoteDataSize; /* Size of data pointed to by lpSpRemoteData */

  LPVOID lpSpLocalData;
  DWORD  dwSpLocalDataSize; /* Size of data pointed to by lpSpLocalData */

  IDirectPlay2Impl* dplay; /* FIXME: This should perhaps be iface not impl */

  LPVOID lpPlayerData; /* FIXME: Need to figure out how this actually behaves */
  DWORD dwPlayerDataSize;
} DirectPlaySPData;

#define DPSP_IMPL_FIELDS \
   ULONG ulInterfaceRef; \
   DirectPlaySPIUnknownData* unk; \
   DirectPlaySPData* sp;

struct IDirectPlaySPImpl
{
  ICOM_VFIELD(IDirectPlaySP);
  DPSP_IMPL_FIELDS
};

/* Forward declaration of virtual tables */
static ICOM_VTABLE(IDirectPlaySP) directPlaySPVT;



/* Create the SP interface */
extern
HRESULT DPSP_CreateInterface( REFIID riid, LPVOID* ppvObj, IDirectPlay2Impl* dp )
{
  TRACE( " for %s\n", debugstr_guid( riid ) );

  *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       sizeof( IDirectPlaySPImpl ) );

  if( *ppvObj == NULL )
  {
    return DPERR_OUTOFMEMORY;
  }
 
  if( IsEqualGUID( &IID_IDirectPlaySP, riid ) )
  {
    ICOM_THIS(IDirectPlaySPImpl,*ppvObj);
    ICOM_VTBL(This) = &directPlaySPVT;
  }
  else
  {
    /* Unsupported interface */
    HeapFree( GetProcessHeap(), 0, *ppvObj );
    *ppvObj = NULL;

    return E_NOINTERFACE;
  }

  /* Initialize it */
  if( DPSP_CreateIUnknown( *ppvObj ) &&
      DPSP_CreateDirectPlaySP( *ppvObj, dp )
    )
  {
    IDirectPlaySP_AddRef( (LPDIRECTPLAYSP)*ppvObj );
    return S_OK;
  }

  /* Initialize failed, destroy it */
  DPSP_DestroyDirectPlaySP( *ppvObj );
  DPSP_DestroyIUnknown( *ppvObj );

  HeapFree( GetProcessHeap(), 0, *ppvObj );
  *ppvObj = NULL;

  return DPERR_NOMEMORY;
}

static BOOL DPSP_CreateIUnknown( LPVOID lpSP )
{
  ICOM_THIS(IDirectPlaySPImpl,lpSP);

  This->unk = (DirectPlaySPIUnknownData*)HeapAlloc( GetProcessHeap(), 
                                                    HEAP_ZERO_MEMORY,
                                                    sizeof( *(This->unk) ) );

  if ( This->unk == NULL )
  {
    return FALSE;
  }

  InitializeCriticalSection( &This->unk->DPSP_lock );

  return TRUE;
}

static BOOL DPSP_DestroyIUnknown( LPVOID lpSP )
{
  ICOM_THIS(IDirectPlaySPImpl,lpSP);
 
  DeleteCriticalSection( &This->unk->DPSP_lock );
  HeapFree( GetProcessHeap(), 0, This->unk );

  return TRUE;
}


static BOOL DPSP_CreateDirectPlaySP( LPVOID lpSP, IDirectPlay2Impl* dp )
{
  ICOM_THIS(IDirectPlaySPImpl,lpSP);

  This->sp = (DirectPlaySPData*)HeapAlloc( GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           sizeof( *(This->sp) ) );

  if ( This->sp == NULL )
  {
    return FALSE;
  }

  This->sp->dplay = dp;

  /* Normally we should be keeping a reference, but since only the dplay
   * interface that created us can destroy us, we do not keep a reference
   * to it (ie we'd be stuck with always having one reference to the dplay
   * object, and hence us, around).
   * NOTE: The dp object does reference count us.
   */
  /* IDirectPlayX_AddRef( (LPDIRECTPLAY2)dp ); */

  return TRUE;
}

static BOOL DPSP_DestroyDirectPlaySP( LPVOID lpSP )
{
  ICOM_THIS(IDirectPlaySPImpl,lpSP);

  /* Normally we should be keeping a reference, but since only the dplay
   * interface that created us can destroy us, we do not keep a reference
   * to it (ie we'd be stuck with always having one reference to the dplay
   * object, and hence us, around).
   * NOTE: The dp object does reference count us.
   */
  /*IDirectPlayX_Release( (LPDIRECTPLAY2)This->sp->dplay ); */

  HeapFree( GetProcessHeap(), 0, This->sp->lpSpRemoteData );
  HeapFree( GetProcessHeap(), 0, This->sp->lpSpLocalData );

  /* FIXME: Need to delete player queue */

  HeapFree( GetProcessHeap(), 0, This->sp );
  return TRUE;
}

/* Interface implementation */

static HRESULT WINAPI DPSP_QueryInterface
( LPDIRECTPLAYSP iface,
  REFIID riid,
  LPVOID* ppvObj )
{
  ICOM_THIS(IDirectPlaySPImpl,iface);
  TRACE("(%p)->(%s,%p)\n", This, debugstr_guid( riid ), ppvObj );

  *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       sizeof( IDirectPlaySPImpl ) );

  if( *ppvObj == NULL )
  {
    return DPERR_OUTOFMEMORY;
  }

  CopyMemory( *ppvObj, iface, sizeof( IDirectPlaySPImpl )  );
  (*(IDirectPlaySPImpl**)ppvObj)->ulInterfaceRef = 0;

  if( IsEqualGUID( &IID_IDirectPlaySP, riid ) )
  {
    ICOM_THIS(IDirectPlaySPImpl,*ppvObj);
    ICOM_VTBL(This) = &directPlaySPVT;
  }
  else
  {
    /* Unsupported interface */
    HeapFree( GetProcessHeap(), 0, *ppvObj );
    *ppvObj = NULL;

    return E_NOINTERFACE;
  }
 
  IDirectPlaySP_AddRef( (LPDIRECTPLAYSP)*ppvObj );

  return S_OK;
} 

static ULONG WINAPI DPSP_AddRef
( LPDIRECTPLAYSP iface )
{
  ULONG ulInterfaceRefCount, ulObjRefCount;
  ICOM_THIS(IDirectPlaySPImpl,iface);

  ulObjRefCount       = InterlockedIncrement( &This->unk->ulObjRef );
  ulInterfaceRefCount = InterlockedIncrement( &This->ulInterfaceRef );

  TRACE( "ref count incremented to %lu:%lu for %p\n",
         ulInterfaceRefCount, ulObjRefCount, This );

  return ulObjRefCount;
}

static ULONG WINAPI DPSP_Release
( LPDIRECTPLAYSP iface )
{
  ULONG ulInterfaceRefCount, ulObjRefCount;
  ICOM_THIS(IDirectPlaySPImpl,iface);

  ulObjRefCount       = InterlockedDecrement( &This->unk->ulObjRef );
  ulInterfaceRefCount = InterlockedDecrement( &This->ulInterfaceRef );

  TRACE( "ref count decremented to %lu:%lu for %p\n",
         ulInterfaceRefCount, ulObjRefCount, This );

  /* Deallocate if this is the last reference to the object */
  if( ulObjRefCount == 0 )
  {
     DPSP_DestroyDirectPlaySP( This );
     DPSP_DestroyIUnknown( This );
  }

  if( ulInterfaceRefCount == 0 )
  {
    HeapFree( GetProcessHeap(), 0, This );
  }

  return ulInterfaceRefCount;
}

static HRESULT WINAPI IDirectPlaySPImpl_AddMRUEntry
( LPDIRECTPLAYSP iface,
  LPCWSTR lpSection,
  LPCWSTR lpKey,
  LPCVOID lpData, 
  DWORD   dwDataSize, 
  DWORD   dwMaxEntries
)
{
  ICOM_THIS(IDirectPlaySPImpl,iface);

  FIXME( "(%p)->(%p,%p%p,0x%08lx,0x%08lx): stub\n", 
         This, lpSection, lpKey, lpData, dwDataSize, dwMaxEntries );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_CreateAddress
( LPDIRECTPLAYSP iface,
  REFGUID guidSP, 
  REFGUID guidDataType, 
  LPCVOID lpData, 
  DWORD   dwDataSize, 
  LPVOID  lpAddress,
  LPDWORD lpdwAddressSize
)
{
  ICOM_THIS(IDirectPlaySPImpl,iface);

  FIXME( "(%p)->(%s,%s,%p,0x%08lx,%p,%p): stub\n", 
         This, debugstr_guid(guidSP), debugstr_guid(guidDataType),
         lpData, dwDataSize, lpAddress, lpdwAddressSize );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_EnumAddress
( LPDIRECTPLAYSP iface,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback, 
  LPCVOID lpAddress, 
  DWORD dwAddressSize, 
  LPVOID lpContext
)
{
  ICOM_THIS(IDirectPlaySPImpl,iface);

  TRACE( "(%p)->(%p,%p,0x%08lx,%p)\n", 
         This, lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );

  DPL_EnumAddress( lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_EnumMRUEntries
( LPDIRECTPLAYSP iface,
  LPCWSTR lpSection, 
  LPCWSTR lpKey, 
  LPENUMMRUCALLBACK lpEnumMRUCallback, 
  LPVOID lpContext
)
{
  ICOM_THIS(IDirectPlaySPImpl,iface);

  FIXME( "(%p)->(%p,%p,%p,%p,): stub\n", 
         This, lpSection, lpKey, lpEnumMRUCallback, lpContext );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_GetPlayerFlags
( LPDIRECTPLAYSP iface,
  DPID idPlayer, 
  LPDWORD lpdwPlayerFlags
)
{
  ICOM_THIS(IDirectPlaySPImpl,iface);

  FIXME( "(%p)->(0x%08lx,%p): stub\n", 
         This, idPlayer, lpdwPlayerFlags );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_GetSPPlayerData
( LPDIRECTPLAYSP iface,
  DPID idPlayer, 
  LPVOID* lplpData, 
  LPDWORD lpdwDataSize, 
  DWORD dwFlags
)
{
  ICOM_THIS(IDirectPlaySPImpl,iface);

  TRACE( "Called on process 0x%08lx\n", GetCurrentProcessId() );
  FIXME( "(%p)->(0x%08lx,%p,%p,0x%08lx): stub\n", 
         This, idPlayer, lplpData, lpdwDataSize, dwFlags );

  /* What to do in the case where there is nothing set yet? */

  *lplpData     = This->sp->lpPlayerData;
  *lpdwDataSize = This->sp->dwPlayerDataSize;

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_HandleMessage
( LPDIRECTPLAYSP iface,
  LPVOID lpMessageBody, 
  DWORD  dwMessageBodySize, 
  LPVOID lpMessageHeader
)
{
  LPDPMSG_SENDENVELOPE lpMsg = (LPDPMSG_SENDENVELOPE)lpMessageBody;
  HRESULT hr = DPERR_GENERIC;
  WORD wCommandId;
  WORD wVersion;

  ICOM_THIS(IDirectPlaySPImpl,iface);

  TRACE( "Called on process 0x%08lx\n", GetCurrentProcessId() );
  FIXME( "(%p)->(%p,0x%08lx,%p): mostly stub\n", 
         This, lpMessageBody, dwMessageBodySize, lpMessageHeader );

  wCommandId = lpMsg->wCommandId;
  wVersion   = lpMsg->wVersion;

  TRACE( "Incomming message has envelope of 0x%08lx, %u, %u\n", 
         lpMsg->dwMagic, wCommandId, wVersion );

  if( lpMsg->dwMagic != DPMSGMAGIC_DPLAYMSG )
  {
    ERR( "Unknown magic 0x%08lx!\n", lpMsg->dwMagic );
  }

  switch( lpMsg->wCommandId )
  {
    /* Name server needs to handle this request */
    case DPMSGCMD_ENUMSESSIONSREQUEST:
    {
      DPSP_REPLYDATA data;

      data.lpSPMessageHeader = lpMessageHeader;
      data.idNameServer      = 0;
      data.lpISP             = iface;
 
      NS_ReplyToEnumSessionsRequest( lpMessageBody, &data, This->sp->dplay );

      hr = (This->sp->dplay->dp2->spData.lpCB->Reply)( &data );

      if( FAILED(hr) )
      {
        ERR( "Reply failed 0x%08lx\n", hr );
      }

      break;
    }

    /* Name server needs to handle this request */
    case DPMSGCMD_ENUMSESSIONSREPLY:
    {
      NS_SetRemoteComputerAsNameServer( lpMessageHeader, 
                                        This->sp->dplay->dp2->spData.dwSPHeaderSize,
                                        (LPDPMSG_ENUMSESSIONSREPLY)lpMessageBody,
                                        This->sp->dplay->dp2->lpNameServerData );

      /* No reply expected */

      break;
    }

    case DPMSGCMD_GETNAMETABLE:
    case DPMSGCMD_GETNAMETABLEREPLY:
    case DPMSGCMD_NEWPLAYERIDREPLY:
    case DPMSGCMD_REQUESTNEWPLAYERID:
    {
      DPSP_REPLYDATA data;
      
      data.lpMessage     = NULL;
      data.dwMessageSize = 0;

      /* Pass this message to the dplay interface to handle */
      DP_HandleMessage( This->sp->dplay, lpMessageBody, dwMessageBodySize,
                        lpMessageHeader, wCommandId, wVersion, 
                        &data.lpMessage, &data.dwMessageSize );

      /* Do we want a reply? */
      if( data.lpMessage != NULL )
      {
        HRESULT hr;

        data.lpSPMessageHeader = lpMessageHeader;
        data.idNameServer      = 0;
        data.lpISP             = iface;

        hr = (This->sp->dplay->dp2->spData.lpCB->Reply)( &data );

        if( FAILED(hr) )
        {
          ERR( "Reply failed %s\n", DPLAYX_HresultToString(hr) );
        }
      }

      break;
    }

    default:
      FIXME( "Unknown Command of %u and size 0x%08lx\n", 
             lpMsg->wCommandId, dwMessageBodySize );
  }

#if 0
  HRESULT hr = DP_OK;
  HANDLE  hReceiveEvent = 0;
  /* FIXME: Aquire some sort of interface lock */
  /* FIXME: Need some sort of context for this callback. Need to determine
   *        how this is actually done with the SP
   */
  /* FIXME: Who needs to delete the message when done? */
  switch( lpMsg->dwType )
  {
    case DPSYS_CREATEPLAYERORGROUP:
    {
      LPDPMSG_CREATEPLAYERORGROUP msg = (LPDPMSG_CREATEPLAYERORGROUP)lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_CreatePlayer( This, lpMessageHeader, msg->dpId, 
                                 &msg->dpnName, 0, msg->lpData, 
                                 msg->dwDataSize, msg->dwFlags, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        /* Group in group situation? */
        if( msg->dpIdParent == DPID_NOPARENT_GROUP )
        {
          hr = DP_IF_CreateGroup( This, lpMessageHeader, msg->dpId, 
                                  &msg->dpnName, 0, msg->lpData, 
                                  msg->dwDataSize, msg->dwFlags, ... );
        }
        else /* Group in Group */
        {
          hr = DP_IF_CreateGroupInGroup( This, lpMessageHeader, msg->dpIdParent,
                                         &msg->dpnName, 0, msg->lpData, 
                                         msg->dwDataSize, msg->dwFlags, ... );
        }
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for DPSYS_CREATEPLAYERORGROUP\n" );
        return;
      }

      break;
    }

    case DPSYS_DESTROYPLAYERORGROUP:
    {
      LPDPMSG_DESTROYPLAYERORGROUP msg = (LPDPMSG_DESTROYPLAYERORGROUP)lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_DestroyPlayer( This, msg->dpId, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        hr = DP_IF_DestroyGroup( This, msg->dpId, ... );
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for DPSYS_DESTROYPLAYERORGROUP\n" );
        return;
      }

      break;
    }

    case DPSYS_ADDPLAYERTOGROUP:
    {
      LPDPMSG_ADDPLAYERTOGROUP msg = (LPDPMSG_ADDPLAYERTOGROUP)lpMsg;

      hr = DP_IF_AddPlayerToGroup( This, msg->dpIdGroup, msg->dpIdPlayer, ... );
      break;
    }

    case DPSYS_DELETEPLAYERFROMGROUP:
    {
      LPDPMSG_DELETEPLAYERFROMGROUP msg = (LPDPMSG_DELETEPLAYERFROMGROUP)lpMsg;

      hr = DP_IF_DeletePlayerFromGroup( This, msg->dpIdGroup, msg->dpIdPlayer,
                                        ... );

      break;
    }

    case DPSYS_SESSIONLOST:
    {
      LPDPMSG_SESSIONLOST msg = (LPDPMSG_SESSIONLOST)lpMsg;

      FIXME( "DPSYS_SESSIONLOST not handled\n" );

      break;
    }

    case DPSYS_HOST:
    {
      LPDPMSG_HOST msg = (LPDPMSG_HOST)lpMsg;

      FIXME( "DPSYS_HOST not handled\n" );

      break;
    }

    case DPSYS_SETPLAYERORGROUPDATA:
    {
      LPDPMSG_SETPLAYERORGROUPDATA msg = (LPDPMSG_SETPLAYERORGROUPDATA)lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_SetPlayerData( This, msg->dpId, msg->lpData, msg->dwDataSize,                                  DPSET_REMOTE, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        hr = DP_IF_SetGroupData( This, msg->dpId, msg->lpData, msg->dwDataSize,
                                 DPSET_REMOTE, ... );
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for LPDPMSG_SETPLAYERORGROUPDATA\n" );
        return;
      }

      break;
    }

    case DPSYS_SETPLAYERORGROUPNAME:
    {
      LPDPMSG_SETPLAYERORGROUPNAME msg = (LPDPMSG_SETPLAYERORGROUPNAME)lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_SetPlayerName( This, msg->dpId, msg->dpnName, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        hr = DP_IF_SetGroupName( This, msg->dpId, msg->dpnName, ... );
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for LPDPMSG_SETPLAYERORGROUPDATA\n" );
        return;
      }

      break;
    }

    case DPSYS_SETSESSIONDESC;
    {
      LPDPMSG_SETSESSIONDESC msg = (LPDPMSG_SETSESSIONDESC)lpMsg;

      hr = DP_IF_SetSessionDesc( This, &msg->dpDesc );

      break;
    }

    case DPSYS_ADDGROUPTOGROUP:
    {
      LPDPMSG_ADDGROUPTOGROUP msg = (LPDPMSG_ADDGROUPTOGROUP)lpMsg;

      hr = DP_IF_AddGroupToGroup( This, msg->dpIdParentGroup, msg->dpIdGroup,
                                  ... );

      break;
    }

    case DPSYS_DELETEGROUPFROMGROUP:
    {
      LPDPMSG_DELETEGROUPFROMGROUP msg = (LPDPMSG_DELETEGROUPFROMGROUP)lpMsg;

      hr = DP_IF_DeleteGroupFromGroup( This, msg->dpIdParentGroup,
                                       msg->dpIdGroup, ... );

      break;
    }

    case DPSYS_SECUREMESSAGE:
    {
      LPDPMSG_SECUREMESSAGE msg = (LPDPMSG_SECUREMESSAGE)lpMsg;

      FIXME( "DPSYS_SECUREMESSAGE not implemented\n" );

      break;
    }

    case DPSYS_STARTSESSION:
    {
      LPDPMSG_STARTSESSION msg = (LPDPMSG_STARTSESSION)lpMsg;

      FIXME( "DPSYS_STARTSESSION not implemented\n" );

      break;
    }

    case DPSYS_CHAT:
    {
      LPDPMSG_CHAT msg = (LPDPMSG_CHAT)lpMsg;

      FIXME( "DPSYS_CHAT not implemeneted\n" );

      break;
    }   

    case DPSYS_SETGROUPOWNER:
    {
      LPDPMSG_SETGROUPOWNER msg = (LPDPMSG_SETGROUPOWNER)lpMsg;

      FIXME( "DPSYS_SETGROUPOWNER not implemented\n" );

      break;
    }

    case DPSYS_SENDCOMPLETE:
    {
      LPDPMSG_SENDCOMPLETE msg = (LPDPMSG_SENDCOMPLETE)lpMsg;

      FIXME( "DPSYS_SENDCOMPLETE not implemented\n" );

      break;
    }

    default:
    {
      /* NOTE: This should be a user defined type. There is nothing that we
       *       need to do with it except queue it.
       */
      TRACE( "Received user message type(?) 0x%08lx through SP.\n",
              lpMsg->dwType );
      break;
    }
  }

  FIXME( "Queue message in the receive queue. Need some context data!\n" );

  if( FAILED(hr) )
  {
    ERR( "Unable to perform action for msg type 0x%08lx\n", lpMsg->dwType );
  }
  /* If a receieve event was registered for this player, invoke it */
  if( hReceiveEvent )
  {
    SetEvent( hReceiveEvent );
  }
#endif

  return hr;
}

static HRESULT WINAPI IDirectPlaySPImpl_SetSPPlayerData
( LPDIRECTPLAYSP iface,
  DPID idPlayer, 
  LPVOID lpData, 
  DWORD dwDataSize, 
  DWORD dwFlags
)
{
  ICOM_THIS(IDirectPlaySPImpl,iface);

  /* FIXME: I'm not sure if this stuff should be associated with the DPlay
   *        player lists. How else would this stuff get deleted? 
   */

  TRACE( "Called on process 0x%08lx\n", GetCurrentProcessId() );
  FIXME( "(%p)->(0x%08lx,%p,0x%08lx,0x%08lx): stub\n", 
         This, idPlayer, lpData, dwDataSize, dwFlags );

  This->sp->lpPlayerData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize ); 

  This->sp->dwPlayerDataSize = dwDataSize;
  CopyMemory( This->sp->lpPlayerData, lpData, dwDataSize );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_CreateCompoundAddress
( LPDIRECTPLAYSP iface,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements, 
  DWORD dwElementCount, 
  LPVOID lpAddress, 
  LPDWORD lpdwAddressSize
)
{
  ICOM_THIS(IDirectPlaySPImpl,iface);

  FIXME( "(%p)->(%p,0x%08lx,%p,%p): stub\n", 
         This, lpElements, dwElementCount, lpAddress, lpdwAddressSize );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_GetSPData
( LPDIRECTPLAYSP iface,
  LPVOID* lplpData, 
  LPDWORD lpdwDataSize, 
  DWORD dwFlags
)
{
  ICOM_THIS(IDirectPlaySPImpl,iface);

  TRACE( "Called on process 0x%08lx\n", GetCurrentProcessId() );
  TRACE( "(%p)->(%p,%p,0x%08lx)\n", 
         This, lplpData, lpdwDataSize, dwFlags );

#if 0
  /* This is what the documentation says... */
  if( dwFlags != 0 )
  {
    return DPERR_INVALIDPARAMS;
  }
#else
  /* ... but most service providers call this with 1 */
  /* Guess that this is using a DPSET_LOCAL or DPSET_REMOTE type of
   * thing?
   */
  if( dwFlags != 0 )
  {
    FIXME( "Undocumented dwFlags 0x%08lx used\n", dwFlags );
  }
#endif

  /* FIXME: What to do in the case where this isn't initialized yet? */

  /* Yes, we're supposed to return a pointer to the memory we have stored! */
  if( dwFlags == DPSET_REMOTE )
  {
    *lpdwDataSize = This->sp->dwSpRemoteDataSize;
    *lplpData     = This->sp->lpSpRemoteData;
  }
  else if( dwFlags == DPSET_LOCAL )
  {
    *lpdwDataSize = This->sp->dwSpLocalDataSize;
    *lplpData     = This->sp->lpSpLocalData;
  }

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_SetSPData
( LPDIRECTPLAYSP iface,
  LPVOID lpData, 
  DWORD dwDataSize, 
  DWORD dwFlags
)
{
  LPVOID lpSpData;

  ICOM_THIS(IDirectPlaySPImpl,iface);

  TRACE( "Called on process 0x%08lx\n", GetCurrentProcessId() );
  TRACE( "(%p)->(%p,0x%08lx,0x%08lx)\n", 
         This, lpData, dwDataSize, dwFlags );

#if 0
  /* This is what the documentation says... */
  if( dwFlags != 0 )
  {
    return DPERR_INVALIDPARAMS;
  }
#else
  /* ... but most service providers call this with 1 */
  /* Guess that this is using a DPSET_LOCAL or DPSET_REMOTE type of
   * thing?
   */
  if( dwFlags != 0 )
  {
    FIXME( "Undocumented dwFlags 0x%08lx used\n", dwFlags );
  }
#endif

  if( dwFlags == DPSET_REMOTE )
  {
    lpSpData = DPLAYX_PrivHeapAlloc( HEAP_ZERO_MEMORY, dwDataSize );
  }  
  else
  {
    lpSpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize ); 
  }

  CopyMemory( lpSpData, lpData, dwDataSize );

  /* If we have data already allocated, free it and replace it */
  if( dwFlags == DPSET_REMOTE )
  {
    /* FIXME: This doesn't strictly make sense as there is no means to share
     *        this shared data. Must be misinterpreting something...
     */
    if( This->sp->lpSpRemoteData )
    {
      DPLAYX_PrivHeapFree( This->sp->lpSpRemoteData );
    }

    /* NOTE: dwDataSize is also stored in the heap structure */
    This->sp->dwSpRemoteDataSize = dwDataSize;
    This->sp->lpSpRemoteData = lpSpData;
  }
  else if ( dwFlags == DPSET_LOCAL )
  {
    if( This->sp->lpSpLocalData )
    {
      HeapFree( GetProcessHeap(), 0, This->sp->lpSpLocalData );
    }

    This->sp->lpSpLocalData     = lpSpData;
    This->sp->dwSpLocalDataSize = dwDataSize;
  }

  return DP_OK;
}

static VOID WINAPI IDirectPlaySPImpl_SendComplete
( LPDIRECTPLAYSP iface,
  LPVOID unknownA, 
  DWORD unknownB
)
{
  ICOM_THIS(IDirectPlaySPImpl,iface);

  FIXME( "(%p)->(%p,0x%08lx): stub\n", 
         This, unknownA, unknownB );
}


static struct ICOM_VTABLE(IDirectPlaySP) directPlaySPVT =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE

  DPSP_QueryInterface,
  DPSP_AddRef,
  DPSP_Release,

  IDirectPlaySPImpl_AddMRUEntry,
  IDirectPlaySPImpl_CreateAddress,
  IDirectPlaySPImpl_EnumAddress,
  IDirectPlaySPImpl_EnumMRUEntries,
  IDirectPlaySPImpl_GetPlayerFlags,
  IDirectPlaySPImpl_GetSPPlayerData,
  IDirectPlaySPImpl_HandleMessage,
  IDirectPlaySPImpl_SetSPPlayerData,
  IDirectPlaySPImpl_CreateCompoundAddress,
  IDirectPlaySPImpl_GetSPData,
  IDirectPlaySPImpl_SetSPData,
  IDirectPlaySPImpl_SendComplete
};
