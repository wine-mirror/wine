/* DirectPlay & DirectPlayLobby messaging implementation
 *
 * Copyright 2000 - Peter Hunnisett
 *
 * <presently under construction - contact hunnise@nortelnetworks.com>
 *
 */

#include "winbase.h"
#include "debugtools.h"

#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#include "dplayx_messages.h"
#include "dplay_global.h"
#include "dplayx_global.h"

DEFAULT_DEBUG_CHANNEL(dplay)

typedef struct tagMSGTHREADINFO
{
  HANDLE hStart;
  HANDLE hDeath; 
  HANDLE hSettingRead;
  HANDLE hNotifyEvent; 
} MSGTHREADINFO, *LPMSGTHREADINFO;


static DWORD CALLBACK DPL_MSG_ThreadMain( LPVOID lpContext );

/* Create the message reception thread to allow the application to receive
 * asynchronous message reception
 */
DWORD CreateLobbyMessageReceptionThread( HANDLE hNotifyEvent, HANDLE hStart, 
                                         HANDLE hDeath, HANDLE hConnRead )
{
  DWORD           dwMsgThreadId;
  LPMSGTHREADINFO lpThreadInfo;

  lpThreadInfo = HeapAlloc( GetProcessHeap(), 0, sizeof( *lpThreadInfo ) );
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

  if( !CreateThread( NULL,                  /* Security attribs */
                     0,                     /* Stack */ 
                     DPL_MSG_ThreadMain,    /* Msg reception function */
                     lpThreadInfo,          /* Msg reception func parameter */
                     0,                     /* Flags */
                     &dwMsgThreadId         /* Updated with thread id */
                   )
    )
  {
    ERR( "Unable to create msg thread\n" );
    goto error;
  }

  /* FIXME: Should I be closing the handle to the thread or does that
            terminate the thread? */
 
  return dwMsgThreadId;

error:

  HeapFree( GetProcessHeap(), 0, lpThreadInfo );

  return 0;
}

static DWORD CALLBACK DPL_MSG_ThreadMain( LPVOID lpContext )
{
  LPMSGTHREADINFO lpThreadInfo = (LPMSGTHREADINFO)lpContext;
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

  TRACE( "App created && intialized starting main message reception loop\n" );

  for ( ;; )
  {
    MSG lobbyMsg;
#ifdef STRICT
    HANDLE hNullHandle = NULL;
#else
    HANDLE hNullHandle = 0;
#endif

    GetMessageW( &lobbyMsg, hNullHandle, 0, 0 );
  }

end_of_thread:
  TRACE( "Msg thread exiting!\n" );
  HeapFree( GetProcessHeap(), 0, lpThreadInfo );

  return 0;
}


HRESULT DP_MSG_SendRequestPlayerId( IDirectPlay2AImpl* This, DWORD dwFlags,
                                    LPDPID lpdpidAllocatedId )
{
  LPVOID                     lpMsg;
  LPDPMSG_REQUESTNEWPLAYERID lpMsgBody;
  DWORD                      dwMsgSize;
  DWORD                      dwWaitReturn;
  HRESULT                    hr = DP_OK;

  FIXME( "semi stub\n" );

  DebugBreak();

  dwMsgSize = This->dp2->spData.dwSPHeaderSize + sizeof( *lpMsgBody );

  lpMsg = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwMsgSize );

  lpMsgBody = (LPDPMSG_REQUESTNEWPLAYERID)( (BYTE*)lpMsg + 
                                             This->dp2->spData.dwSPHeaderSize );

  lpMsgBody->envelope.dwMagic    = DPMSGMAGIC_DPLAYMSG;
  lpMsgBody->envelope.wCommandId = DPMSGCMD_REQUESTNEWPLAYERID;
  lpMsgBody->envelope.wVersion   = DPMSGVER_DP6;

  lpMsgBody->dwFlags = dwFlags;

  /* FIXME: Need to have a more advanced queuing system as this needs to
   *        block on send until we get response. Otherwise we need to be
   *        able to ensure we can pick out the exact response. Of course,
   *        with something as non critical as this, would it matter? The
   *        id has been effectively reserved for this session...
   */
  { 
    DPSP_SENDDATA data;

    data.dwFlags        = DPSEND_GUARANTEED;
    data.idPlayerTo     = 0; /* Name server */
    data.idPlayerFrom   = 0; /* Sending from DP */
    data.lpMessage      = lpMsg;
    data.dwMessageSize  = dwMsgSize;
    data.bSystemMessage = TRUE; /* Allow reply to be sent */
    data.lpISP          = This->dp2->spData.lpISP;

    /* Setup for receipt */
    This->dp2->hMsgReceipt = CreateEventA( NULL, FALSE, FALSE, NULL );
 
    hr = (*This->dp2->spData.lpCB->Send)( &data );

    if( FAILED(hr) )
    {
      ERR( "Request for new playerID send failed: %s\n",
           DPLAYX_HresultToString( hr ) );
    }
  }

  dwWaitReturn = WaitForSingleObject( This->dp2->hMsgReceipt, 30000 );
  if( dwWaitReturn != WAIT_OBJECT_0 )
  {
    ERR( "Wait failed 0x%08lx\n", dwWaitReturn );
  }

  CloseHandle( This->dp2->hMsgReceipt );
  This->dp2->hMsgReceipt = 0;

  /* Need to examine the data and extract the new player id */
  /* I just hope that dplay doesn't return the whole new player! */

  return hr;
}


/* This function seems to cause a trap in the SP. It would seem unnecessary */
/* FIXME: Remove this method if not required */
HRESULT DP_MSG_OpenStream( IDirectPlay2AImpl* This )
{
  HRESULT       hr;
  DPSP_SENDDATA data;

  data.dwFlags        = DPSEND_OPENSTREAM;
  data.idPlayerTo     = 0; /* Name server */
  data.idPlayerFrom   = 0; /* From DP itself */
  data.lpMessage      = NULL;
  data.dwMessageSize  = This->dp2->spData.dwSPHeaderSize;
  data.bSystemMessage = FALSE; /* FIXME? */
  data.lpISP          = This->dp2->spData.lpISP;

  hr = (*This->dp2->spData.lpCB->Send)( &data );

  if( FAILED(hr) )
  {
      ERR( "Request for open stream send failed: %s\n",
           DPLAYX_HresultToString( hr ) );
  }

  /* FIXME: hack to give some time for channel to open */
  SleepEx( 1000 /* 1 sec */, FALSE );

  return hr;
}
