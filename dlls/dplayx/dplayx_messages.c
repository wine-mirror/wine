/* DirectPlay & DirectPlayLobby messaging implementation
 *
 * Copyright 2000,2001 - Peter Hunnisett
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 *  o Messaging interface required for both DirectPlay and DirectPlayLobby.
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#include "dplayx_messages.h"
#include "dplay_global.h"
#include "dplayx_global.h"
#include "name_server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

typedef struct tagMSGTHREADINFO
{
  HANDLE hStart;
  HANDLE hDeath;
  HANDLE hSettingRead;
  HANDLE hNotifyEvent;
} MSGTHREADINFO, *LPMSGTHREADINFO;

static DWORD CALLBACK DPL_MSG_ThreadMain( LPVOID lpContext );
static HRESULT DP_MSG_ExpectReply( IDirectPlayImpl *This, DPSP_SENDEXDATA *data, DWORD dwWaitTime,
        WORD *replyCommandIds, DWORD replyCommandIdCount, void **lplpReplyMsg,
        DWORD *lpdwMsgBodySize );


/* Create the message reception thread to allow the application to receive
 * asynchronous message reception
 */
DWORD CreateLobbyMessageReceptionThread( HANDLE hNotifyEvent, HANDLE hStart,
                                         HANDLE hDeath, HANDLE hConnRead )
{
  DWORD           dwMsgThreadId;
  LPMSGTHREADINFO lpThreadInfo;
  HANDLE          hThread;

  lpThreadInfo = malloc( sizeof( *lpThreadInfo ) );
  if( lpThreadInfo == NULL )
  {
    return 0;
  }

  /* The notify event may or may not exist. Depends if async comm or not */
  if( hNotifyEvent &&
      !DuplicateHandle( GetCurrentProcess(), hNotifyEvent,
                        GetCurrentProcess(), &lpThreadInfo->hNotifyEvent,
                        0, FALSE, DUPLICATE_SAME_ACCESS ) )
  {
    ERR( "Unable to duplicate event handle\n" );
    goto error;
  }

  /* These 3 handles don't need to be duplicated because we don't keep a
   * reference to them where they're created. They're created specifically
   * for the message thread
   */
  lpThreadInfo->hStart       = hStart;
  lpThreadInfo->hDeath       = hDeath;
  lpThreadInfo->hSettingRead = hConnRead;

  hThread = CreateThread( NULL,                  /* Security attribs */
                          0,                     /* Stack */
                          DPL_MSG_ThreadMain,    /* Msg reception function */
                          lpThreadInfo,          /* Msg reception func parameter */
                          0,                     /* Flags */
                          &dwMsgThreadId         /* Updated with thread id */
                        );
  if ( hThread == NULL )
  {
    ERR( "Unable to create msg thread\n" );
    goto error;
  }

  CloseHandle(hThread);

  return dwMsgThreadId;

error:

  free( lpThreadInfo );

  return 0;
}

static DWORD CALLBACK DPL_MSG_ThreadMain( LPVOID lpContext )
{
  LPMSGTHREADINFO lpThreadInfo = lpContext;
  DWORD dwWaitResult;

  TRACE( "Msg thread created. Waiting on app startup\n" );

  /* Wait to ensure that the lobby application is started w/ 1 min timeout */
  dwWaitResult = WaitForSingleObject( lpThreadInfo->hStart, 10000 /* 10 sec */ );
  if( dwWaitResult == WAIT_TIMEOUT )
  {
    FIXME( "Should signal app/wait creation failure (0x%08lx)\n", dwWaitResult );
    goto end_of_thread;
  }

  /* Close this handle as it's not needed anymore */
  CloseHandle( lpThreadInfo->hStart );
  lpThreadInfo->hStart = 0;

  /* Wait until the lobby knows what it is */
  dwWaitResult = WaitForSingleObject( lpThreadInfo->hSettingRead, INFINITE );
  if( dwWaitResult == WAIT_TIMEOUT )
  {
    ERR( "App Read connection setting timeout fail (0x%08lx)\n", dwWaitResult );
  }

  /* Close this handle as it's not needed anymore */
  CloseHandle( lpThreadInfo->hSettingRead );
  lpThreadInfo->hSettingRead = 0;

  TRACE( "App created && initialized starting main message reception loop\n" );

  for ( ;; )
  {
    MSG lobbyMsg;
    GetMessageW( &lobbyMsg, 0, 0, 0 );
  }

end_of_thread:
  TRACE( "Msg thread exiting!\n" );
  free( lpThreadInfo );

  return 0;
}

/* DP messaging stuff */
static HANDLE DP_MSG_BuildAndLinkReplyStruct( IDirectPlayImpl *This,
        DP_MSG_REPLY_STRUCT_LIST *lpReplyStructList, WORD *replyCommandIds,
        DWORD replyCommandIdCount )
{
  lpReplyStructList->replyExpected.hReceipt           = CreateEventW( NULL, FALSE, FALSE, NULL );
  lpReplyStructList->replyExpected.expectedReplies    = replyCommandIds;
  lpReplyStructList->replyExpected.expectedReplyCount = replyCommandIdCount;
  lpReplyStructList->replyExpected.lpReplyMsg         = NULL;
  lpReplyStructList->replyExpected.dwMsgBodySize      = 0;

  /* Insert into the message queue while locked */
  EnterCriticalSection( &This->lock );
    DPQ_INSERT( This->dp2->repliesExpected, lpReplyStructList, repliesExpected );
  LeaveCriticalSection( &This->lock );

  return lpReplyStructList->replyExpected.hReceipt;
}

static DP_MSG_REPLY_STRUCT_LIST *DP_MSG_FindAndUnlinkReplyStruct( IDirectPlayImpl *This,
                                                                  WORD commandId )
{
    DP_MSG_REPLY_STRUCT_LIST *reply;
    DWORD i;

    EnterCriticalSection( &This->lock );

    for( reply = DPQ_FIRST( This->dp2->repliesExpected ); reply;
         reply = DPQ_NEXT( reply->repliesExpected ) )
    {
        for( i = 0; i < reply->replyExpected.expectedReplyCount; ++i )
        {
            if( reply->replyExpected.expectedReplies[ i ] == commandId )
            {
                DPQ_REMOVE( This->dp2->repliesExpected, reply, repliesExpected );
                LeaveCriticalSection( &This->lock );
                return reply;
            }
        }
    }

    LeaveCriticalSection( &This->lock );

    return NULL;
}

static
void DP_MSG_UnlinkReplyStruct( IDirectPlayImpl *This, DP_MSG_REPLY_STRUCT_LIST *lpReplyStructList )
{
  EnterCriticalSection( &This->lock );
    DPQ_REMOVE( This->dp2->repliesExpected, lpReplyStructList, repliesExpected );
  LeaveCriticalSection( &This->lock );
}

static
void DP_MSG_CleanReplyStruct( LPDP_MSG_REPLY_STRUCT_LIST lpReplyStructList,
                              LPVOID* lplpReplyMsg, LPDWORD lpdwMsgBodySize )
{
  CloseHandle( lpReplyStructList->replyExpected.hReceipt );

  *lplpReplyMsg    = lpReplyStructList->replyExpected.lpReplyMsg;
  *lpdwMsgBodySize = lpReplyStructList->replyExpected.dwMsgBodySize;
}

DWORD DP_MSG_ComputeMessageSize( SGBUFFER *buffers, DWORD bufferCount )
{
  DWORD size = 0;
  DWORD i;
  for ( i = 0; i < bufferCount; ++i )
    size += buffers[ i ].len;
  return size;
}

HRESULT DP_MSG_SendRequestPlayerId( IDirectPlayImpl *This, DWORD dwFlags, DPID *lpdpidAllocatedId )
{
  LPVOID                     lpMsg;
  DPMSG_REQUESTNEWPLAYERID   msgBody;
  DWORD                      dwMsgSize;
  HRESULT                    hr = DP_OK;

  /* Compose dplay message envelope */
  msgBody.envelope.dwMagic    = DPMSGMAGIC_DPLAYMSG;
  msgBody.envelope.wCommandId = DPMSGCMD_REQUESTNEWPLAYERID;
  msgBody.envelope.wVersion   = DPMSGVER_DP6;

  /* Compose the body of the message */
  msgBody.dwFlags = dwFlags;

  /* Send the message */
  {
    WORD replyCommand = DPMSGCMD_NEWPLAYERIDREPLY;
    SGBUFFER buffers[ 2 ] = { 0 };
    DPSP_SENDEXDATA data = { 0 };

    buffers[ 0 ].len = This->dp2->spData.dwSPHeaderSize;
    buffers[ 1 ].len = sizeof( msgBody );
    buffers[ 1 ].pData = (UCHAR *) &msgBody;

    data.lpISP          = This->dp2->spData.lpISP;
    data.dwFlags        = DPSEND_GUARANTEED;
    data.idPlayerTo     = 0; /* Name server */
    data.idPlayerFrom   = 0; /* Sending from DP */
    data.lpSendBuffers  = buffers;
    data.cBuffers       = ARRAYSIZE( buffers );
    data.dwMessageSize  = DP_MSG_ComputeMessageSize( data.lpSendBuffers, data.cBuffers );
    data.bSystemMessage = TRUE; /* Allow reply to be sent */

    TRACE( "Asking for player id w/ dwFlags 0x%08lx\n",
           msgBody.dwFlags );

    hr = DP_MSG_ExpectReply( This, &data, DPMSG_DEFAULT_WAIT_TIME, &replyCommand, 1,
                             &lpMsg, &dwMsgSize );
  }

  /* Need to examine the data and extract the new player id */
  if( SUCCEEDED(hr) )
  {
    LPCDPMSG_NEWPLAYERIDREPLY lpcReply;

    lpcReply = lpMsg;

    *lpdpidAllocatedId = lpcReply->dpidNewPlayerId;

    TRACE( "Received reply for id = 0x%08lx\n", lpcReply->dpidNewPlayerId );

    /* FIXME: I think that the rest of the message has something to do
     *        with remote data for the player that perhaps I need to setup.
     *        However, with the information that is passed, all that it could
     *        be used for is a standardized initialization value, which I'm
     *        guessing we can do without. Unless the message content is the same
     *        for several different messages?
     */

    free( lpMsg );
  }

  return hr;
}

HRESULT DP_MSG_ForwardPlayerCreation( IDirectPlayImpl *This, DPID dpidServer )
{
  LPVOID                   lpMsg;
  DPMSG_FORWARDADDPLAYER   msgBody;
  DWORD                    dwMsgSize;
  DPLAYI_PACKEDPLAYER      playerInfo;
  void                    *spPlayerData;
  DWORD                    spPlayerDataSize;
  const WCHAR             *password = L"";
  HRESULT                  hr;

  hr = IDirectPlaySP_GetSPPlayerData( This->dp2->spData.lpISP, dpidServer,
                                      &spPlayerData, &spPlayerDataSize, DPSET_REMOTE );
  if( FAILED( hr ) )
    return hr;

  msgBody.envelope.dwMagic    = DPMSGMAGIC_DPLAYMSG;
  msgBody.envelope.wCommandId = DPMSGCMD_FORWARDADDPLAYER;
  msgBody.envelope.wVersion   = DPMSGVER_DP6;
  msgBody.toId                = 0;
  msgBody.playerId            = dpidServer;
  msgBody.groupId             = 0;
  msgBody.createOffset        = sizeof( msgBody );
  msgBody.passwordOffset      = sizeof( msgBody ) + sizeof( playerInfo ) + spPlayerDataSize;

  playerInfo.size             = sizeof( playerInfo ) + spPlayerDataSize;
  playerInfo.flags            = DPLAYI_PLAYER_SYSPLAYER | DPLAYI_PLAYER_PLAYERLOCAL;
  playerInfo.id               = dpidServer;
  playerInfo.shortNameLength  = 0;
  playerInfo.longNameLength   = 0;
  playerInfo.spDataLength     = spPlayerDataSize;
  playerInfo.playerDataLength = 0;
  playerInfo.playerCount      = 0;
  playerInfo.systemPlayerId   = dpidServer;
  playerInfo.fixedSize        = sizeof( playerInfo );
  playerInfo.version          = DPMSGVER_DP6;
  playerInfo.parentId         = 0;

  /* Send the message */
  {
    WORD replyCommands[] = { DPMSGCMD_GETNAMETABLEREPLY, DPMSGCMD_SUPERENUMPLAYERSREPLY };
    SGBUFFER buffers[ 6 ] = { 0 };
    DPSP_SENDEXDATA data = { 0 };

    buffers[ 0 ].len = This->dp2->spData.dwSPHeaderSize;
    buffers[ 1 ].len = sizeof( msgBody );
    buffers[ 1 ].pData = (UCHAR *) &msgBody;
    buffers[ 2 ].len = sizeof( playerInfo );
    buffers[ 2 ].pData = (UCHAR *) &playerInfo;
    buffers[ 3 ].len = spPlayerDataSize;
    buffers[ 3 ].pData = (UCHAR *) spPlayerData;
    buffers[ 4 ].len = (lstrlenW( password ) + 1) * sizeof( WCHAR );
    buffers[ 4 ].pData = (UCHAR *) password;
    buffers[ 5 ].len = sizeof( This->dp2->lpSessionDesc->dwReserved1 );
    buffers[ 5 ].pData = (UCHAR *) &This->dp2->lpSessionDesc->dwReserved1;

    data.lpISP          = This->dp2->spData.lpISP;
    data.dwFlags        = DPSEND_GUARANTEED;
    data.idPlayerTo     = 0; /* Name server */
    data.idPlayerFrom   = dpidServer; /* Sending from session server */
    data.lpSendBuffers  = buffers;
    data.cBuffers       = ARRAYSIZE( buffers );
    data.dwMessageSize  = DP_MSG_ComputeMessageSize( data.lpSendBuffers, data.cBuffers );
    data.bSystemMessage = TRUE; /* Allow reply to be sent */

    TRACE( "Sending forward player request with 0x%08lx\n", dpidServer );

    hr = DP_MSG_ExpectReply( This, &data,
                             DPMSG_WAIT_60_SECS,
                             replyCommands, ARRAYSIZE( replyCommands ),
                             &lpMsg, &dwMsgSize );
  }

  /* Need to examine the data and extract the new player id */
  if( lpMsg != NULL )
  {
    FIXME( "Name Table reply received: stub\n" );
  }

  return hr;
}

/* Queue up a structure indicating that we want a reply of type wReplyCommandId. DPlay does
 * not seem to offer any way of uniquely differentiating between replies of the same type
 * relative to the request sent. There is an implicit assumption that there will be no
 * ordering issues on sends and receives from the opposite machine. No wonder MS is not
 * a networking company.
 */
static HRESULT DP_MSG_ExpectReply( IDirectPlayImpl *This, DPSP_SENDEXDATA *lpData, DWORD dwWaitTime,
        WORD *replyCommandIds, DWORD replyCommandIdCount, void **lplpReplyMsg,
        DWORD *lpdwMsgBodySize )
{
  HRESULT                  hr;
  HANDLE                   hMsgReceipt;
  DP_MSG_REPLY_STRUCT_LIST replyStructList;
  DWORD                    dwWaitReturn;

  /* Setup for receipt */
  hMsgReceipt = DP_MSG_BuildAndLinkReplyStruct( This, &replyStructList,
                                                replyCommandIds, replyCommandIdCount );

  TRACE( "Sending msg and expecting reply within %lu ticks\n", dwWaitTime );
  hr = (*This->dp2->spData.lpCB->SendEx)( lpData );

  if( FAILED(hr) )
  {
    ERR( "Send failed: %s\n", DPLAYX_HresultToString( hr ) );
    DP_MSG_UnlinkReplyStruct( This, &replyStructList );
    DP_MSG_CleanReplyStruct( &replyStructList, lplpReplyMsg, lpdwMsgBodySize );
    return hr;
  }

  /* The reply message will trigger the hMsgReceipt event effectively switching
   * control back to this thread. See DP_MSG_ReplyReceived.
   */
  dwWaitReturn = WaitForSingleObject( hMsgReceipt, dwWaitTime );
  if( dwWaitReturn != WAIT_OBJECT_0 )
  {
    ERR( "Wait failed 0x%08lx\n", dwWaitReturn );
    DP_MSG_UnlinkReplyStruct( This, &replyStructList );
    DP_MSG_CleanReplyStruct( &replyStructList, lplpReplyMsg, lpdwMsgBodySize );
    return DPERR_TIMEOUT;
  }

  /* Clean Up */
  DP_MSG_CleanReplyStruct( &replyStructList, lplpReplyMsg, lpdwMsgBodySize );
  return DP_OK;
}

/* Determine if there is a matching request for this incoming message and then copy
 * all important data. It is quite silly to have to copy the message, but the documents
 * indicate that a copy is taken. Silly really.
 */
void DP_MSG_ReplyReceived( IDirectPlayImpl *This, WORD wCommandId, const void *lpcMsgBody,
        DWORD dwMsgBodySize )
{
  LPDP_MSG_REPLY_STRUCT_LIST lpReplyList;

#if 0
  if( wCommandId == DPMSGCMD_FORWARDADDPLAYER )
  {
    DebugBreak();
  }
#endif

  /* Find, and immediately remove (to avoid double triggering), the appropriate entry. Call locked to
   * avoid problems.
   */
  lpReplyList = DP_MSG_FindAndUnlinkReplyStruct( This, wCommandId );

  if( lpReplyList != NULL )
  {
    lpReplyList->replyExpected.dwMsgBodySize = dwMsgBodySize;
    lpReplyList->replyExpected.lpReplyMsg = malloc( dwMsgBodySize );
    CopyMemory( lpReplyList->replyExpected.lpReplyMsg,
                lpcMsgBody, dwMsgBodySize );

    /* Signal the thread which sent the message that it has a reply */
    SetEvent( lpReplyList->replyExpected.hReceipt );
  }
  else
  {
    ERR( "No receipt event set - only expecting in reply mode\n" );
    DebugBreak();
  }
}

void DP_MSG_ToSelf( IDirectPlayImpl *This, DPID dpidSelf )
{
  LPVOID                   lpMsg;
  DPMSG_SENDENVELOPE       msgBody;
  DWORD                    dwMsgSize;

  /* Compose dplay message envelope */
  msgBody.dwMagic    = DPMSGMAGIC_DPLAYMSG;
  msgBody.wCommandId = DPMSGCMD_JUSTENVELOPE;
  msgBody.wVersion   = DPMSGVER_DP6;

  /* Send the message to ourselves */
  {
    WORD replyCommand = DPMSGCMD_JUSTENVELOPE;
    SGBUFFER buffers[ 2 ] = { 0 };
    DPSP_SENDEXDATA data = { 0 };

    buffers[ 0 ].len = This->dp2->spData.dwSPHeaderSize;
    buffers[ 1 ].len = sizeof( msgBody );
    buffers[ 1 ].pData = (UCHAR *) &msgBody;

    data.lpISP          = This->dp2->spData.lpISP;
    data.dwFlags        = 0;
    data.idPlayerTo     = dpidSelf; /* Sending to session server */
    data.idPlayerFrom   = 0; /* Sending from session server */
    data.lpSendBuffers  = buffers;
    data.cBuffers       = ARRAYSIZE( buffers );
    data.dwMessageSize  = DP_MSG_ComputeMessageSize( data.lpSendBuffers, data.cBuffers );
    data.bSystemMessage = TRUE; /* Allow reply to be sent */

    DP_MSG_ExpectReply( This, &data,
                        DPMSG_WAIT_5_SECS,
                        &replyCommand, 1,
                        &lpMsg, &dwMsgSize );
  }
}

void DP_MSG_ErrorReceived( IDirectPlayImpl *This, WORD wCommandId, const void *lpMsgBody,
        DWORD dwMsgBodySize )
{
  LPCDPMSG_FORWARDADDPLAYERNACK lpcErrorMsg;

  lpcErrorMsg = lpMsgBody;

  ERR( "Received error message %u. Error is %s\n",
       wCommandId, DPLAYX_HresultToString( lpcErrorMsg->errorCode) );
  DebugBreak();
}
